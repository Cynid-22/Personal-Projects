#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <thread>
#include <mutex>
#include <vector>
#include <codecvt>
#include <locale>

using namespace std;

mutex write_mutex;

bool should_skip(const string& page) {
    return page.find("<redirect title=") != string::npos ||
           page.find("(disambiguation)") != string::npos;
}

string extract_tag(const string& source, const string& tag) {
    // Match <tag ...> or <tag> and </tag>
    size_t start = source.find("<" + tag);
    if (start == string::npos) return "";

    start = source.find(">", start);
    if (start == string::npos) return "";

    start += 1; // skip '>'

    size_t end = source.find("</" + tag + ">", start);
    if (end == string::npos) return "";

    return source.substr(start, end - start);
}

string clean_text(const string& text) {
    string result = text;

    // 1. Remove TITLE, TEXT, REDIRECT labels
    result = regex_replace(result, regex(R"(^=+.*?$)", regex_constants::multiline), "");
    result = regex_replace(result, regex(R"(^[A-Z]*TITLE:.*?$)", regex_constants::multiline), "");
    result = regex_replace(result, regex(R"(^[A-Z]*TEXT:)", regex_constants::multiline), "");
    result = regex_replace(result, regex(R"(^[A-Z]*RECT\s+\[\[.*?\]\])", regex_constants::multiline), "");

    // 2. Replace HTML entities
    result = regex_replace(result, regex(R"(&lt;)"), "<");
    result = regex_replace(result, regex(R"(&gt;)"), ">");
    result = regex_replace(result, regex(R"(&amp;)"), "&");
    result = regex_replace(result, regex(R"(&nbsp;)"), " ");
    result = regex_replace(result, regex(R"(&quot;)"), "\"");
    result = regex_replace(result, regex(R"(&#91;)"), "[");  // sometimes encoded
    result = regex_replace(result, regex(R"(&#93;)"), "]");

    // 3. Remove full HTML tags
    result = regex_replace(result, regex(R"(<[^>]+>)"), "");

    // 4. Remove templates like {{...}} deeply
    for (int i = 0; i < 5; ++i)
        result = regex_replace(result, regex(R"(\{\{[^{}]*\}\})"), "");

    // 5. Remove file/media/image links like [[File:...|...]] or plain "Tập_tin:" URLs
    result = regex_replace(result, regex(R"(\[\[(File|Tập_tin):[^\[\]]*\]\])", regex_constants::icase), "");
    result = regex_replace(result, regex(R"(https?:\/\/vi\.wikipedia\.org\/wiki\/T%E1%BA%ADp_tin:[^\s\|]+(\|[^\s\|]*)*)", regex_constants::icase), "");

    // 6. Remove wikilinks but preserve label
    result = regex_replace(result, regex(R"(\[\[[^\[\]]*\|([^\[\]]+)\]\])"), "$1");
    result = regex_replace(result, regex(R"(\[\[([^\[\]]+)\]\])"), "$1");

    // 7. Remove [http... caption] style links
    result = regex_replace(result, regex(R"(\[https?:\/\/[^\s\]]+\s*([^\]]*)\])"), "$1");
    result = regex_replace(result, regex(R"(https?:\/\/\S+|\bwww\.\S+)"), "");

    // 8. Remove brackets and misc. symbols
    result = regex_replace(result, regex(R"([\[\]\{\}<>=])"), "");

    // 9. Remove table formatting and row markup
    result = regex_replace(result, regex(R"(^\s*[\|\!].*?$)", regex_constants::multiline), "");
    result = regex_replace(result, regex(R"(\|\s*colspan\s*=\s*\d+\s*\|)"), "");
    result = regex_replace(result, regex(R"(\!\s*rowspan\s*=\s*\d+\s*\|)"), "");
    result = regex_replace(result, regex(R"(!\s*&nbsp;)"), "");

    // 10. Remove specific Wikipedia keywords
    result = regex_replace(result, regex(R"(IPAblink|IPAplink|IPA|sub|ref|templatestyles|wikitable)", regex_constants::icase), "");

    // 11. Remove metadata keys like "| id = something"
    result = regex_replace(result, regex(R"(\|\s*[a-zA-Z_ \-]+=\s*[^|\n]+)"), "");

    // 12. Remove HTML comments
    result = regex_replace(result, regex(R"(<!--[\s\S]*?-->)"), "");

    // 13. Remove encoded div tags
    result = regex_replace(result, regex(R"(&lt;/?div[^&]*&gt;)"), "");

    // 14. Remove '' and =
    result = regex_replace(result, regex(R"('{2,})"), "");
    result = regex_replace(result, regex(R"(=+)"), "");

    // 15. Remove <br>, <br />, etc.
    result = regex_replace(result, regex(R"(<br\s*/?>)", regex_constants::icase), "");

    // 16. Remove file extensions like .svg, .jpg, .png, .pdf (in text or filenames)
    result = regex_replace(result, regex(R"(\b\S+\.(svg|jpg|jpeg|png|gif|pdf)\b)", regex_constants::icase), "");

    // 17. Normalize whitespace
    result = regex_replace(result, regex(R"(\s+)"), " ");

    // 18. Final trim
    if (!result.empty() && result.front() == ' ') result.erase(0, 1);
    if (!result.empty() && result.back() == ' ') result.pop_back();

    return result;
}

void process_article(const string& page, const string& output_file) {
    try {
        string title = extract_tag(page, "title");
        string raw_text = extract_tag(page, "text");

        if (title.empty() && raw_text.empty()) {
            cerr << "⚠️  Skipped: Empty title and text\n";
            return;
        }

        ofstream out(output_file, ios::app | ios::binary);
        if (!out) {
            cerr << "❌ ERROR: Cannot open file.\n";
            return;
        }

        out << "====================\n";
        out << "TITLE: " << title << "\n";
        out << "TEXT:\n" << raw_text << "\n\n";
    }
    catch (const exception& e) {
        cerr << "‼️  Exception caught in process_article: " << e.what() << endl;
    }
    catch (...) {
        cerr << "‼️  Unknown error occurred in process_article" << endl;
    }
}

void process_file(const string& input_file, const string& output_file) {
    ifstream in(input_file);
    if (!in) {
        cerr << "Cannot open input file.\n";
        return;
    }

    string line;
    string article;
    bool inside_page = false;
    vector<thread> threads;
    size_t line_count = 0;

    while (getline(in, line)) {
        line_count++;
        if (line_count % 100000 == 0) {
            cout << "Read " << line_count << " lines..." << endl;
        }

        if (line.find("<page>") != string::npos) {
            inside_page = true;
            article = line + "\n";
        } else if (line.find("</page>") != string::npos) {
            article += line + "\n";
            threads.emplace_back(process_article, article, output_file);
            inside_page = false;

            if (threads.size() >= 20) {
                for (auto& t : threads) t.join();
                threads.clear();
            }
        } else if (inside_page) {
            article += line + "\n";
        }
    }

    for (auto& t : threads) t.join(); // join remaining
}

int main() {
    string input_file = "C:/Users/nguye/OneDrive/Desktop/viwiki-20250620-pages-articles-multistream/viwiki-20250620-pages-articles-multistream.xml";
    string output_file = "C:/Users/nguye/OneDrive/Desktop/viwiki-20250620-pages-articles-multistream/all_text.txt";

    cout << "Starting processing..." << endl;
    process_file(input_file, output_file);
    cout << "Finished processing" << endl;
    return 0;
}
