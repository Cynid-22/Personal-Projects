#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <thread>
#include <mutex>
#include <vector>
#include <codecvt>
#include <locale>
#include <queue>
#include <condition_variable>
#include <filesystem> 

using namespace std;

const size_t MAX_THREADS = thread::hardware_concurrency() ? thread::hardware_concurrency() - 2 : 2;
mutex write_mutex;
mutex queue_mutex;
condition_variable cv;
queue<string> work_queue;
bool done_reading = false;


bool should_skip(const string& page) {
    return page.find("<redirect title=") != string::npos ||
           page.find("(disambiguation)") != string::npos;
}

// Extract content inside <tag>...</tag>
string extract_tag(const string& source, const string& tag) {
    size_t start = source.find("<" + tag);
    if (start == string::npos) return "";

    start = source.find(">", start);
    if (start == string::npos) return "";

    start += 1;
    size_t end = source.find("</" + tag + ">", start);
    if (end == string::npos) return "";

    return source.substr(start, end - start);
}

// Token structure for HTML tokenizer
struct Token {
    string type;
    string value;
};

// Tokenize HTML into TAG and TEXT parts
vector<Token> tokenize(const string& html) {
    vector<Token> tokens;
    size_t pos = 0;
    while (pos < html.size()) {
        if (html[pos] == '<') {
            size_t endPos = html.find('>', pos);
            if (endPos != string::npos) {
                tokens.push_back({"TAG", html.substr(pos, endPos - pos + 1)});
                pos = endPos + 1;
            } else {
                break;
            }
        } else {
            size_t endPos = html.find('<', pos);
            if (endPos == string::npos) endPos = html.size();
            tokens.push_back({"TEXT", html.substr(pos, endPos - pos)});
            pos = endPos;
        }
    }
    return tokens;
}

string remove_templates(const string& text) {
    string output;
    output.reserve(text.size());

    int depth = 0;
    const char* ptr = text.data();
    const char* end = ptr + text.size();

    while (ptr < end) {
        if (*ptr != '{' && *ptr != '}') {
            output.push_back(*ptr++);
            continue;
        }

        if (ptr + 1 < end && ptr[0] == '{' && ptr[1] == '{') {
            depth++;
            ptr += 2;
        }
        else if (ptr + 1 < end && ptr[0] == '}' && ptr[1] == '}' && depth > 0) {
            depth--;
            ptr += 2;
        }
        else {
            if (depth == 0)
                output.push_back(*ptr);
            ptr++;
        }
    }

    return output;
}

string clean_text(const string& text) {
    string result = text;

    result = regex_replace(result, regex(R"(/[\w\-]+)"), "");  // Remove slash-prefixed words like /div, /noninclude
    result = regex_replace(result, regex(R"(&[a-zA-Z]+)"), ""); // Remove entities like &div, &noninclude, etc.

    // Remove deeply nested templates like {{...}} with non-regex parser
    result = remove_templates(result);
    // for (int i = 0; i < 5; ++i)
    //     result = regex_replace(result, regex(R"(\{\{[^{}]*\}\})"), "");

    // Remove full media/file links like [[File:...]] or [[Tập_tin:...]]
    result = regex_replace(result, regex(R"(\[\[(File|Tập_tin):[^\[\]]*\]\])", regex_constants::icase), "");
    result = regex_replace(result, regex(R"(https?:\/\/vi\.wikipedia\.org\/wiki\/T%E1%BA%ADp_tin:[^\s\|]+(\|[^\s\|]*)*)", regex_constants::icase), "");

    // Remove wikilinks [[A|B]], and [[A]]
    result = regex_replace(result, regex(R"(\[\[[^\[\]]*\|([^\[\]]+)\]\])"), " ");
    result = regex_replace(result, regex(R"(\[\[([^\[\]]+)\]\])"), " ");

    // Remove external link markup [http://... text] → text, and plain URLs
    result = regex_replace(result, regex(R"(\[https?:\/\/[^\s\]]+\s*([^\]]*)\])"), "$1");
    result = regex_replace(result, regex(R"(https?:\/\/\S+|\bwww\.\S+)"), "");

    // Remove leftover symbols and brackets
    result = regex_replace(result, regex(R"([\[\]\{\}<>=])"), "");

    // Remove table formatting lines starting with | or !
    result = regex_replace(result, regex(R"(^\s*[\|\!].*?$)", regex_constants::multiline), "");
    result = regex_replace(result, regex(R"(\|\s*colspan\s*=\s*\d+\s*\|)"), "");
    result = regex_replace(result, regex(R"(\!\s*rowspan\s*=\s*\d+\s*\|)"), "");
    result = regex_replace(result, regex(R"(!\s*&nbsp;)"), "");

    // Remove template/meta keywords like IPA, ref, etc.
    result = regex_replace(result, regex(R"(IPAblink|IPAplink|IPA|sub|ref|templatestyles|wikitable|div|noinclude)", regex_constants::icase), "");

    // Remove things like "| id = value" and metadata
    result = regex_replace(result, regex(R"(\|\s*[a-zA-Z_ \-]+=\s*[^|\n]+)"), "");

    // Remove HTML comments <!-- ... -->
    result = regex_replace(result, regex(R"(<!--[\s\S]*?-->)"), "");

    // Remove multiple apostrophes '' or ''' used for bold/italic
    result = regex_replace(result, regex(R"('{2,})"), "");

    // Remove sequences of =
    result = regex_replace(result, regex(R"(=+)"), "");

    // Remove file extensions (.jpg, .png, .svg, ...)
    result = regex_replace(result, regex(R"(\b\S+\.(svg|jpg|jpeg|png|gif|pdf|html|css|com|co|us|vn)\b)", regex_constants::icase), "");

    // Normalize whitespace (collapse to single space)
    result = regex_replace(result, regex(R"(\s+)"), " ");

    // Trim leading and trailing spaces
    if (!result.empty() && result.front() == ' ') result.erase(0, 1);
    if (!result.empty() && result.back() == ' ') result.pop_back();

    return result;
}

void process_article(const string& page, const string& output_file) {
    try {
        string title = extract_tag(page, "title");
        string raw_text = extract_tag(page, "text");

        if (title.empty() && raw_text.empty()) {
            cerr << "Skipped: Empty title and text\n";
            return;
        }

        if (title.rfind("Wikipedia:", 0) == 0)
            return;

        if (title.rfind("MediaWiki:", 0) == 0)
            return;

        if (title.rfind("Trợ giúp:", 0) == 0)
            return;

        if (title.rfind("Bản mẫu:", 0) == 0)
            return;

        if (title.rfind("Tập tin:", 0) == 0)
            return;

        if (title.rfind("Cổng thông tin:", 0) == 0)
            return;

        // Extract visible text using tokenizer
        string visible_text;
        for (const auto& token : tokenize(raw_text)) {
            if (token.type == "TEXT") {
                visible_text += token.value;
            }
        }

        // Clean it
        visible_text = clean_text(visible_text);

        lock_guard<mutex> lock(write_mutex);
        ofstream out(output_file, ios::app | ios::binary);
        if (!out) {
            cerr << "ERROR: Cannot open file.\n";
            return;
        }

        out << "====================\n";
        out << "TITLE: " << title << "\n";
        out << "TEXT:\n" << visible_text << "\n\n";
    }
    catch (const regex_error& re) {
    cerr << "Regex error in page: " << extract_tag(page, "title") << " → " << re.what() << endl;
    }

    catch (const exception& e) {
        cerr << "Exception in page: " << extract_tag(page, "title") << " → " << e.what() << endl;
    }

    catch (...) {
        cerr << "Unknown error occurred in process_article" << endl;
    }
}

void worker(const string& output_file) {
    while (true) {
        string article;
        {
            unique_lock<mutex> lock(queue_mutex);
            cv.wait(lock, [] { return !work_queue.empty() || done_reading; });

            if (work_queue.empty() && done_reading) break;

            article = move(work_queue.front());
            work_queue.pop();
        }
        process_article(article, output_file);
    }
}

void process_file(const string& input_file, const string& output_file) {
    ifstream in(input_file);
    if (!in) {
        cerr << "Cannot open input file.\n";
        return;
    }

    uint64_t total_bytes = filesystem::file_size(input_file);  // Get total file size
    uint64_t bytes_read = 0;
    int last_percent = -1;

    string line;
    string article;
    bool inside_page = false;
    vector<thread> threads;

    while (getline(in, line)) {
        bytes_read += line.size() + 1; // account for newline
        int percent = static_cast<int>((bytes_read * 100) / total_bytes);

        if (percent != last_percent) {
            cout << "\rProgress: " << percent << "%" << flush;
            last_percent = percent;
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

    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    cout << "\nDone reading file." << endl;
}

int main() {
    
    const string input_file = "C:/Users/nguye/OneDrive/Desktop/viwiki-20250620-pages-articles-multistream/viwiki-20250620-pages-articles-multistream.xml";
    const string output_file = "C:/Users/nguye/OneDrive/Desktop/viwiki-20250620-pages-articles-multistream/all_text_cpp2.txt";

    cout << "Starting processing..." << endl;
    process_file(input_file, output_file);
    cout << "Finished processing" << endl;
    return 0;
}
