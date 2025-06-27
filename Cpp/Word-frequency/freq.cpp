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
    string open_tag = "<" + tag + ">";
    string close_tag = "</" + tag + ">";
    size_t start = source.find(open_tag);
    size_t end = source.find(close_tag);
    if (start == string::npos || end == string::npos) return "";
    start += open_tag.length();
    return source.substr(start, end - start);
}

string clean_text(const string& text) {
    string result = text;

    regex brackets(R"(\[\[.*?\]\])");
    result = regex_replace(result, brackets, "");

    regex templates(R"(\{\{.*?\}\})");
    result = regex_replace(result, templates, "");

    regex quotes(R"('{2,})");
    result = regex_replace(result, quotes, "");

    regex tags(R"(<[^>]+>)");
    result = regex_replace(result, tags, "");

    regex whitespace(R"(\s+)");
    result = regex_replace(result, whitespace, " ");

    return result;
}

void process_article(string page, const string& output_file) {
    if (should_skip(page)) return;

    string title = extract_tag(page, "title");
    if (title.find(':') != string::npos) return;

    string raw_text = extract_tag(page, "text");
    string clean = clean_text(raw_text);

    if (clean.empty()) return;

    lock_guard<mutex> guard(write_mutex);
    ofstream out(output_file, ios::app);
    if (out) {
        out << clean << "\n\n";
        cout << "Saved: " << title << endl;
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
    string input_file = "C:/Users/nguye/OneDrive/Desktop/viwiki-20250620-pages-articles-multistream.xml/viwiki-20250620-pages-articles-multistream.xml";
    string output_file = "C:/Users/nguye/OneDrive/Desktop/viwiki-20250620-pages-articles-multistream.xml/all_text.txt";

    cout << "Starting processing..." << endl;
    process_file(input_file, output_file);
    cout << "Finished processing." << endl;
    return 0;
}
