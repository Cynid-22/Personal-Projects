#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <thread>
#include <mutex>
#include <vector>
#include <codecvt>
#include <locale>
#include <filesystem>
#include <sstream>
#include <queue>
#include <condition_variable>

using namespace std;

static const size_t MAX_THREADS = thread::hardware_concurrency() ? max(int(thread::hardware_concurrency())-2, 1) : 1;

mutex write_mutex;
ofstream global_out;
bool done_reading = false;
queue<string> work_queue;
condition_variable cv;

void worker_thread();

struct RegexRule {
    const regex pattern;
    const string replace;
};
static const vector<RegexRule> CLEAN_RULES = {
    { regex(R"(/[\w\-]+)"), "" },  // slash-prefixed words
    { regex(R"(&[a-zA-Z]+)"), "" },  // entities
    { regex(R"(\[\[(File|Tập_tin):[^\[\]]*\]\])", regex_constants::icase), "" },
    { regex(R"(https?:\/\/vi\.wikipedia\.org\/wiki\/T%E1%BA%ADp_tin:[^\s\|]+(\|[^\s\|]*)*)", regex_constants::icase), "" },
    { regex(R"(\[\[[^\[\]]*\|([^\[\]]+)\]\])"), " " },
    { regex(R"(\[\[([^\[\]]+)\]\])"), " " },
    { regex(R"(\[https?:\/\/[^\s\]]+\s*([^\]]*)\])"), "$1" },
    { regex(R"(https?:\/\/\S+|\bwww\.\S+)"), "" },
    { regex(R"([\[\]\{\}<>=])"), "" },
    { regex(R"(^\s*[\|\!].*?$)", regex_constants::multiline), "" }, //////////////
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
    tokens.reserve(32);
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
    output.reserve(text.size());
    int depth = 0;
    const char* ptr = text.data();
    const char* end = ptr + text.size();
    while (ptr < end) {
        if (*ptr != '{' && *ptr != '}') {
            output.push_back(*ptr++);
            continue;
        }
        if (ptr + 1 < end && ptr[0]=='{' && ptr[1]=='{') {
            depth++; ptr += 2;
        }
        else if (ptr + 1 < end && ptr[0]=='}' && ptr[1]=='}' && depth>0) {
            depth--; ptr += 2;
        }
        else {
            if (depth==0) output.push_back(*ptr);
            ptr++;
        }
    }
    return output;
}

string clean_text(const string& text) {
    string result = remove_templates(text);

    for (auto& rule : CLEAN_RULES)
        result = regex_replace(result, rule.pattern, rule.replace);

    if (!result.empty() && result.front()==' ') result.erase(0,1);
    if (!result.empty() && result.back() ==' ') result.pop_back();
    return result;
}

void process_article(const string& page) {
    if (should_skip(page)) return;

    auto title    = extract_tag(page, "title");
    auto raw_text = extract_tag(page, "text");
    if (title.empty() || raw_text.empty()) return;

    // filter out namespace pages
    static const vector<string> BAD_PREFIX = {
        "Wikipedia:", "MediaWiki:", "Trợ giúp:", "Bản mẫu:", "Tập tin:", "Cổng thông tin:"
    };

    for (auto& pre : BAD_PREFIX)
        if (title.rfind(pre,0)==0) return;

    // extract visible text
    string visible;
    for (auto& tk : tokenize(raw_text))
        if (tk.type=="TEXT") visible += tk.value;

    // clean up
    visible = clean_text(visible);

    // buffer output
    ostringstream oss;
    oss << "====================\n";
    oss << "TITLE: " << title << "\n";
    oss << "TEXT:\n" << visible << "\n\n";

    // write under lock
    {
        lock_guard<mutex> lk(write_mutex);
        global_out << oss.str();
    }
}

void worker_thread() {
    while (true) {
        string article;
        {
            unique_lock<mutex> lk(write_mutex);
            cv.wait(lk, [] { return !work_queue.empty() || done_reading; });
            if (work_queue.empty() && done_reading) break;
            article = move(work_queue.front());
            work_queue.pop();
        }

        try {
            process_article(article);
        } catch (const exception& e) {
            lock_guard<mutex> lock(write_mutex);
            cerr << "\n[ERROR] Exception in worker: " << e.what() << endl;
        } catch (...) {
            lock_guard<mutex> lock(write_mutex);
            cerr << "\n[ERROR] Unknown exception in worker.\n";
        }
    }
}

void process_file(const string& in_file, const string& out_file) {
    ifstream in(in_file);
    if (!in) throw runtime_error("Cannot open input file");
    global_out.open(out_file, ios::app | ios::binary);
    if (!global_out) throw runtime_error("Cannot open output file");

    // launch workers
    vector<thread> workers;
    for (size_t i = 0; i < MAX_THREADS; ++i)
        workers.emplace_back(worker_thread);

    // read & dispatch pages
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
            inside  = true;
            article = line + "\n";
        }
        else if (line.find("</page>") != string::npos) {
            article += line + "\n";
            {
                lock_guard<mutex> lk(write_mutex);
                work_queue.push(article);
            }
            cv.notify_one();
            inside = false;
        }
        else if (inside) {
            article += line + "\n";
        }
    }

    cout << "\rProgress: 100%\n";

    // signal workers to finish
    {
        lock_guard<mutex> lk(write_mutex);
        done_reading = true;
    }
    cv.notify_all();

    for (auto& t : workers) t.join();
}


int main() {
    const string input_file = "C:/Users/nguye/OneDrive/Desktop/viwiki-20250620-pages-articles-multistream/viwiki-20250620-pages-articles-multistream.xml";
    const string output_file = "C:/Users/nguye/OneDrive/Desktop/viwiki-20250620-pages-articles-multistream/all_text_cpp2.txt";
    cout << "Starting processing with " << MAX_THREADS << " threads...\n";
    process_file(input_file, output_file);
    cout << "Done.\n";
    return 0;
}
