#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <thread>
#include <mutex>
#include <vector>

using namespace std;

mutex write_mutex;

bool should_skip(const string& page) {
    return page.find("<redirect title=") != string::npos ||
           page.find("(disambiguation)") != string::npos;
}

string extract_tag(const string& source, const string& tag) {
    size_t start = source.find("<" + tag);
    if (start == string::npos) return "";
    start = source.find(">", start);
    if (start == string::npos) return "";
    start++;
    size_t end = source.find("</" + tag + ">", start);
    if (end == string::npos) return "";
    return source.substr(start, end - start);
}

string clean_text(string result) {
    static const vector<pair<regex, string>> patterns = {
        {regex(R"(^=+.*?$)", regex_constants::multiline), ""},
        {regex(R"(^[A-Z]*TITLE:.*?$)", regex_constants::multiline), ""},
        {regex(R"(^[A-Z]*TEXT:)", regex_constants::multiline), ""},
        {regex(R"(^[A-Z]*RECT\s+\[\[.*?\]\])", regex_constants::multiline), ""},
        {regex(R"(&lt;)"), "<"},
        {regex(R"(&gt;)"), ">"},
        {regex(R"(&amp;)"), "&"},
        {regex(R"(&nbsp;)"), " "},
        {regex(R"(&quot;)"), "\""},
        {regex(R"(&#91;)"), "["},
        {regex(R"(&#93;)"), "]"},
        {regex(R"(<[^>]+>)"), ""},
        {regex(R"(\{\{[^{}]*\}\})"), ""},
        {regex(R"(\[\[(File|Tập_tin):[^\[\]]*\]\])", regex_constants::icase), ""},
        {regex(R"(https?:\/\/vi\.wikipedia\.org\/wiki\/T%E1%BA%ADp_tin:[^\s\|]+(\|[^\s\|]*)*)", regex_constants::icase), ""},
        {regex(R"(\[\[[^\[\]]*\|([^\[\]]+)\]\])"), "$1"},
        {regex(R"(\[\[([^\[\]]+)\]\])"), "$1"},
        {regex(R"(\[https?:\/\/[^\s\]]+\s*([^\]]*)\])"), "$1"},
        {regex(R"(https?:\/\/\S+|\bwww\.\S+)"), ""},
        {regex(R"([\[\]\{\}<>="`])"), ""},
        {regex(R"(^\s*[\|\!].*?$)", regex_constants::multiline), ""},
        {regex(R"(\|\s*colspan\s*=\s*\d+\s*\|)"), ""},
        {regex(R"(\!\s*rowspan\s*=\s*\d+\s*\|)"), ""},
        {regex(R"(!\s*&nbsp;)"), ""},
        {regex(R"(IPAblink|IPAplink|IPA|sub|ref|templatestyles|wikitable)", regex_constants::icase), ""},
        {regex(R"(\|\s*[a-zA-Z_ \-]+\s*=\s*[^|\n]+)"), ""},
        {regex(R"(<!--.*?-->)"), ""},
        {regex(R"(&lt;/?div[^&]*&gt;)"), ""},
        {regex(R"('{2,})"), ""},
        {regex(R"(=+)", regex_constants::icase), ""},
        {regex(R"(<br\s*/?>)", regex_constants::icase), ""},
        {regex(R"(\b\S+\.(svg|jpg|jpeg|png|gif|pdf)\b)", regex_constants::icase), ""},
        {regex(R"(\s+)", regex_constants::ECMAScript), " "}
    };

    for (int i = 0; i < 5; ++i)
        result = regex_replace(result, regex(R"(\{\{[^{}]*\}\})"), "");

    for (const auto& [pattern, replacement] : patterns)
        result = regex_replace(result, pattern, replacement);

    if (!result.empty() && result.front() == ' ') result.erase(0, 1);
    if (!result.empty() && result.back() == ' ') result.pop_back();

    return result;
}

void process_article(const string& page, const string& output_file) {
    string title = extract_tag(page, "title");
    string raw_text = extract_tag(page, "text");
    if (title.empty() && raw_text.empty()) return;

    string cleaned = clean_text(raw_text);
    lock_guard<mutex> lock(write_mutex);
    ofstream out(output_file, ios::app | ios::binary);
    if (out) {
        out << "====================\nTITLE: " << title << "\nTEXT:\n" << cleaned << "\n\n";
    }
}

void process_file(const string& input_file, const string& output_file) {
    ifstream in(input_file);
    if (!in) {
        cerr << "Cannot open input file.\n";
        return;
    }

    string line, article;
    bool inside_page = false;
    vector<thread> threads;

    while (getline(in, line)) {
        if (line.find("<page>") != string::npos) {
            inside_page = true;
            article = line + "\n";
        } else if (line.find("</page>") != string::npos) {
            article += line + "\n";
            threads.emplace_back(process_article, article, output_file);
            inside_page = false;
            if (threads.size() >= thread::hardware_concurrency()) {
                for (auto& t : threads) t.join();
                threads.clear();
            }
        } else if (inside_page) {
            article += line + "\n";
        }
    }

    for (auto& t : threads) t.join();
}

int main() {
    string input_file = "input.xml";
    string output_file = "cleaned_output.txt";
    process_file(input_file, output_file);
    return 0;
}
