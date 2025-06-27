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

    // 1. Remove horizontal bars and variations of TITLE and TEXT headers
    result = regex_replace(result, regex(R"(^=+.*?$)", regex_constants::multiline), "");
    result = regex_replace(result, regex(R"(^[A-Z]*TITLE:.*?$)", regex_constants::multiline), "");
    result = regex_replace(result, regex(R"(^[A-Z]*TEXT:)", regex_constants::multiline), "");

    // Also remove generic redirect lines like 'IRECT [[Page]]', 'RECT [[California]]'
    result = regex_replace(result, regex(R"(^[A-Z]*RECT\s+\[\[.*?\]\])", regex_constants::multiline), "");

    // 2. Remove HTML entities
    result = regex_replace(result, regex(R"(&lt;)"), "<");
    result = regex_replace(result, regex(R"(&gt;)"), ">");
    result = regex_replace(result, regex(R"(&amp;)"), "&");
    result = regex_replace(result, regex(R"(&nbsp;)"), " ");

    // 3. Remove all HTML tags
    result = regex_replace(result, regex(R"(<[^>]+>)"), "");

    // 4. Remove all {{...}} templates (up to 5 levels deep)
    for (int i = 0; i < 5; ++i)
        result = regex_replace(result, regex(R"(\{\{[^{}]*\}\})"), "");

    // 5. Remove all [[...|...]] and [[...]] but keep visible text
    result = regex_replace(result, regex(R"(\[\[[^\[\]]*\|([^\[\]]+)\]\])"), "$1");
    result = regex_replace(result, regex(R"(\[\[([^\[\]]+)\]\])"), "$1");

    // 6. Remove raw and bracketed URLs
    result = regex_replace(result, regex(R"(\[https?:\/\/[^\s\]]+\s*([^\]]*)\])"), "$1"); // keep optional label
    result = regex_replace(result, regex(R"(https?:\/\/\S+|\bwww\.\S+)"), ""); // remove plain URLs

    // 7. Remove all bracket/angle/curly symbols
    result = regex_replace(result, regex(R"([\[\]\{\}<>=])"), "");

    // 8. Remove wiki table formatting and markup lines
    result = regex_replace(result, regex(R"(^\s*[\|\!].*?$)", regex_constants::multiline), "");
    result = regex_replace(result, regex(R"(\|\s*colspan\s*=\s*\d+\s*\|)"), "");
    result = regex_replace(result, regex(R"(\!\s*rowspan\s*=\s*\d+\s*\|)"), "");
    result = regex_replace(result, regex(R"(!\s*&nbsp;)"), "");

    // 9. Remove Wikipedia markup tokens
    result = regex_replace(result, regex(R"(IPAblink|IPAplink|IPA|sub|ref|templatestyles|wikitable)", regex_constants::icase), "");

    // 10. Remove metadata keys like "| id = something", "| icon modifier = something"
    result = regex_replace(result, regex(R"(\|\s*[a-zA-Z_ \-]+=\s*[^|\n]+)"), "");

    // 11. Remove HTML comments
    result = regex_replace(result, regex(R"(<!--[\s\S]*?-->)"), "");

    // 12. Remove encoded div tags
    result = regex_replace(result, regex(R"(&lt;/?div[^&]*&gt;)"), "");

    // 13. Remove '' (bold/italic) and equal signs (headers)
    result = regex_replace(result, regex(R"('{2,})"), "");
    result = regex_replace(result, regex(R"(=+)"), "");

    // 14. Remove HTML line breaks
    result = regex_replace(result, regex(R"(<br\s*/?>)", regex_constants::icase), "");

    // 15. Normalize whitespace
    result = regex_replace(result, regex(R"(\s+)"), " ");

    // 16. Final trim
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
    cout << "Finished processing." << endl;
    return 0;
}
