#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <thread>
#include <mutex>
#include <vector>
#include <filesystem>
#include <sstream>
#include <queue>
#include <condition_variable>

using namespace std;

static const size_t MAX_THREADS = thread::hardware_concurrency() ? max(int(thread::hardware_concurrency()) - 2, 1) : 1;

mutex write_mutex;
mutex queue_mutex;
condition_variable cv;
queue<string> work_queue;
ofstream global_out;
bool done_reading = false;

struct RegexRule {
    const regex pattern;
    const string replace;
};
static const vector<RegexRule> CLEAN_RULES = {
    { regex(R"(<sha1>[^<]*</sha1>)"), "" },
    { regex(R"(/[\w\-]+/)"), "" },
    { regex(R"(\\[a-zA-Z]+)"), "" }, // remove LaTeX commands like \frac, \left, \sigma
    { regex(R"(/([^/\n]*[\t ][^/\n]*)/)"), "$1" },
    { regex(R"(&[a-zA-Z]+)"), "" },
    { regex(R"(\[\[(File|Tập_tin):[^\[\]]*\]\])", regex_constants::icase), "" },
    { regex(R"(https?:\/\/vi\.wikipedia\.org\/wiki\/T%E1%BA%ADp_tin:[^\s\|]+(\|[^\s\|]*)*)", regex_constants::icase), "" },
    { regex(R"(\[\[[^\[\]]*\|([^\[\]]+)\]\])"), " " },
    { regex(R"(\[\[([^\[\]]+)\]\])"), " " },
    { regex(R"(\[https?:\/\/[^\s\]]+\s*([^\]]*)\])"), "$1" },
    { regex(R"(https?:\/\/\S+|\bwww\.\S+)"), "" },
    { regex(R"([\[\]\{\}<>=])"), "" },
    { regex(R"(^\s*[\|\!].*?$)", regex_constants::multiline), "" },
    { regex(R"(\|\s*colspan\s*=\s*\d+\s*\|)"), "" },
    { regex(R"(\!\s*rowspan\s*=\s*\d+\s*\|)"), "" },
    { regex(R"(!\s*&nbsp;)"), "" },
    { regex(R"(IPAblink|IPAplink|IPA|sub|ref|templatestyles|wikitable|div|noinclude)", regex_constants::icase), "" },
    { regex(R"(\|\s*[a-zA-Z_ \-]+=\s*[^|\n]+)"), "" },
    { regex(R"(<!--[\s\S]*?-->)"), "" },
    { regex(R"('{2,})"), "" },
    { regex(R"(=+)"), "" },
    { regex(R"(\b\S+\.(svg|jpg|jpeg|png|gif|pdf|html|css|com|co|us|vn)\b)", regex_constants::icase), "" },
    { regex(R"(\s+)"), " " }
};

bool should_skip(const string& page) {
    return page.find("<redirect title=") != string::npos ||
           page.find("(disambiguation)") != string::npos;
}

string extract_tag(const string& source, const string& tag) {
    auto start = source.find("<" + tag);
    if (start == string::npos) return "";
    start = source.find('>', start);
    if (start == string::npos) return "";
    ++start;
    auto end = source.find("</" + tag + ">", start);
    if (end == string::npos) return "";
    return source.substr(start, end - start);
}

struct Token { string type, value; };

vector<Token> tokenize(const string& html) {
    vector<Token> tokens;
    size_t pos = 0, sz = html.size();
    while (pos < sz) {
        if (html[pos] == '<') {
            auto endPos = html.find('>', pos);
            if (endPos == string::npos) break;
            tokens.push_back({"TAG", html.substr(pos, endPos - pos + 1)});
            pos = endPos + 1;
        } else {
            auto endPos = html.find('<', pos);
            if (endPos == string::npos) endPos = sz;
            tokens.push_back({"TEXT", html.substr(pos, endPos - pos)});
            pos = endPos;
        }
    }
    return tokens;
}

string remove_templates(const string& text) {
    string output;
    size_t i = 0, n = text.size();
    int brace_depth = 0;

    while (i < n) {
        if (i + 1 < n && text[i] == '{' && text[i + 1] == '{') {
            brace_depth++;
            i += 2;
            continue;
        }

        if (i + 1 < n && text[i] == '}' && text[i + 1] == '}') {
            if (brace_depth > 0) brace_depth--;
            i += 2;
            continue;
        }

        if (brace_depth == 0)
            output += text[i];

        i++;
    }

    return output;
}

string clean_text(const string& text) {
    try {
        string result = remove_templates(text);

        if (!result.empty() && result.front() == ' ') result.erase(0, 1);
        if (!result.empty() && result.back() == ' ') result.pop_back();
        return result;
    } catch (const exception& e) {
        cerr << "[clean_text error] " << e.what() << endl;
        return "";
    }
}

void process_article(const string& page) {
    if (should_skip(page)) return;
    auto title = extract_tag(page, "title");
    auto raw_text = extract_tag(page, "text");
    if (title.empty() || raw_text.empty()) return;
    
    static const vector<string> BAD_PREFIX = {
        "Wikipedia:", "MediaWiki:", "Trợ giúp:", "Bản mẫu:", "Tập tin:", "Cổng thông tin:"
    };
    for (auto& pre : BAD_PREFIX)
    if (title.rfind(pre, 0) == 0) return;
    
    string visible;
    for (auto& tk : tokenize(raw_text))
    if (tk.type == "TEXT") visible += tk.value;
    
    visible = clean_text(visible);
    
    for (auto& rule : CLEAN_RULES)
            visible = regex_replace(visible, rule.pattern, rule.replace);

    ostringstream oss;
    oss << "====================\n";
    oss << title << "\n";
    oss << visible << "\n";

    lock_guard<mutex> lk(write_mutex);
    global_out << oss.str();
}

void worker_thread() {
    try {
        while (true) {
            string article;
            {
                unique_lock<mutex> lk(queue_mutex);
                cv.wait(lk, [] { return !work_queue.empty() || done_reading; });

                if (work_queue.empty() && done_reading) break;

                article = move(work_queue.front());
                work_queue.pop();
            }

            try {
                process_article(article);
            } catch (const exception& e) {
                lock_guard<mutex> lock(write_mutex);
                cerr << "Worker error " << e.what() << endl;
            } catch (...) {
                lock_guard<mutex> lock(write_mutex);
                cerr << "Unknown exception\n";
            }
        }
    } catch (...) {
        lock_guard<mutex> lock(write_mutex);
        cerr << "Uncaught exception in worker thread\n";
    }
}

void process_file(const string& in_file, const string& out_file) {
    ifstream in(in_file);
    if (!in) throw runtime_error("Cannot open input file");
    global_out.open(out_file, ios::app | ios::binary);
    if (!global_out) throw runtime_error("Cannot open output file");

    vector<thread> workers;
    for (size_t i = 0; i < MAX_THREADS; ++i)
        workers.emplace_back(worker_thread);

    string line, article;
    bool inside = false;
    uint64_t total_bytes = filesystem::file_size(in_file);
    uint64_t bytes_read = 0;
    int last_percent = -1;

    while (getline(in, line)) {
        if (line.find("</mediawiki>") != string::npos)
            break;

        bytes_read += line.size() + 1;
        int percent = static_cast<int>((min(bytes_read, total_bytes) * 100) / total_bytes);
        if (percent != last_percent) {
            cout << "\rProgress: " << percent << "% " << flush;
            last_percent = percent;
        }

        if (line.find("<page>") != string::npos) {
            inside = true;
            article = line + "\n";
        } else if (line.find("</page>") != string::npos) {
            article += line + "\n";
            {
                lock_guard<mutex> lk(queue_mutex);
                work_queue.push(article);
            }
            cv.notify_one();
            inside = false;
        } else if (inside) {
            article += line + "\n";
        }
    }

    cout << "\rProgress: 100%\n";
    {
        lock_guard<mutex> lk(queue_mutex);
        done_reading = true;
    }
    cv.notify_all();

    for (auto& t : workers)
        if (t.joinable()) t.join();
}

int main() {
    const string input_file = "C:/Users/nguye/OneDrive/Desktop/viwiki-20250620-pages-articles-multistream/viwiki-20250620-pages-articles-multistream.xml";
    const string output_file = "C:/Users/nguye/OneDrive/Desktop/viwiki-20250620-pages-articles-multistream/all_text_cpp2.txt";
    cout << "Starting processing with " << MAX_THREADS << " threads...\n";
    process_file(input_file, output_file);
    cout << "Done.\n";
    return 0;
}
