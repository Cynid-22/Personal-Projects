#include <iostream>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <algorithm>
#include <thread>
#include <mutex>
#include <unicode/unistr.h>
#include <unicode/uchar.h>
#include <unicode/utf8.h>
#include <iomanip>

using namespace std;
using namespace icu;

static const size_t MAX_THREADS = thread::hardware_concurrency() ? max(int(thread::hardware_concurrency()) - 2, 1) : 1;

static const unordered_set<UChar32> vietnamese_chars_simp = {
    0x0061, 0x0062, 0x0063, 0x0064, 0x0111, 0x0065, 0x00EA, 0x0067, 0x0068,
    0x0069, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, 0x00F4, 0x01A1, 0x0070,
    0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x01B0, 0x0076, 0x0078, 0x0079, 0x0103,
    0x0066, 0x006A, 0x0077, 0x007A
};

static const unordered_set<UChar32> vietnamese_chars_comp = {
    // A/a group
    0x0041, 0x0061,       // A, a
    0x00C0, 0x00E0,       // À, à
    0x00C1, 0x00E1,       // Á, á
    0x00C2, 0x00E2,       // Â, â
    0x1EA6, 0x1EA7,       // Ầ, ầ
    0x1EA4, 0x1EA5,       // Ấ, ấ
    0x1EAA, 0x1EAB,       // Ẫ, ẫ
    0x1EA8, 0x1EA9,       // Ẩ, ẩ
    0x1EAC, 0x1EAD,       // Ậ, ậ
    0x0102, 0x0103,       // Ă, ă
    0x1EAE, 0x1EAF,       // Ắ, ắ
    0x1EB0, 0x1EB1,       // Ằ, ằ
    0x1EB2, 0x1EB3,       // Ẳ, ẳ
    0x1EB4, 0x1EB5,       // Ẵ, ẵ
    0x1EB6, 0x1EB7,       // Ặ, ặ
    0x1EA0, 0x1EA1,       // Ạ, ạ
    0x00C3, 0x00E3,       // Ã, ã

    // E/e group
    0x0045, 0x0065,       // E, e
    0x00C8, 0x00E8,       // È, è
    0x00C9, 0x00E9,       // É, é
    0x00CA, 0x00EA,       // Ê, ê
    0x1EC0, 0x1EC1,       // Ề, ề
    0x1EBE, 0x1EBF,       // Ế, ế
    0x1EC2, 0x1EC3,       // Ể, ể
    0x1EC4, 0x1EC5,       // Ễ, ễ
    0x1EC6, 0x1EC7,       // Ệ, ệ
    0x1EB8, 0x1EB9,       // Ẹ, ẹ
    0x1EBA, 0x1EBB,       // Ẻ, ẻ
    0x1EBC, 0x1EBD,       // Ẽ, ẽ

    // I/i group
    0x0049, 0x0069,       // I, i
    0x00CC, 0x00EC,       // Ì, ì
    0x00CD, 0x00ED,       // Í, í
    0x0128, 0x0129,       // Ĩ, ĩ
    0x1EC8, 0x1EC9,       // Ỉ, ỉ
    0x1ECA, 0x1ECB,       // Ị, ị

    // O/o group
    0x004F, 0x006F,       // O, o
    0x00D2, 0x00F2,       // Ò, ò
    0x00D3, 0x00F3,       // Ó, ó
    0x00D4, 0x00F4,       // Ô, ô
    0x1ED0, 0x1ED1,       // Ố, ố
    0x1ED2, 0x1ED3,       // Ồ, ồ
    0x1ED4, 0x1ED5,       // Ổ, ổ
    0x1ED6, 0x1ED7,       // Ỗ, ỗ
    0x1ED8, 0x1ED9,       // Ộ, ộ
    0x01A0, 0x01A1,       // Ơ, ơ
    0x1EDA, 0x1EDB,       // Ớ, ớ
    0x1EDC, 0x1EDD,       // Ờ, ờ
    0x1EDE, 0x1EDF,       // Ở, ở
    0x1EE0, 0x1EE1,       // Ỡ, ỡ
    0x1EE2, 0x1EE3,       // Ợ, ợ
    0x1ECC, 0x1ECD,       // Ọ, ọ
    0x1ECE, 0x1ECF,       // Ỏ, ỏ
    0x00D5, 0x00F5,       // Õ, õ

    // U/u group
    0x0055, 0x0075,       // U, u
    0x00D9, 0x00F9,       // Ù, ù
    0x00DA, 0x00FA,       // Ú, ú
    0x0168, 0x0169,       // Ũ, ũ
    0x1EE4, 0x1EE5,       // Ụ, ụ
    0x1EE6, 0x1EE7,       // Ủ, ủ
    0x01AF, 0x01B0,       // Ư, ư
    0x1EE8, 0x1EE9,       // Ứ, ứ
    0x1EEA, 0x1EEB,       // Ừ, ừ
    0x1EEC, 0x1EED,       // Ử, ử
    0x1EEE, 0x1EEF,       // Ữ, ữ
    0x1EF0, 0x1EF1,       // Ự, ự

    // Y/y group
    0x0059, 0x0079,       // Y, y
    0x00DD, 0x00FD,       // Ý, ý
    0x1EF2, 0x1EF3,       // Ỳ, ỳ
    0x1EF4, 0x1EF5,       // Ỵ, ỵ
    0x1EF6, 0x1EF7,       // Ỷ, ỷ
    0x1EF8, 0x1EF9,       // Ỹ, ỹ

    // Đ/đ
    0x0110, 0x0111,       // Đ, đ

    // Extra consonants
    0x0062, 0x0063, 0x0064, 0x0066, 0x0067, 0x0068, 0x006A,
    0x006B, 0x006C, 0x006D, 0x006E, 0x0070, 0x0071, 0x0072,
    0x0073, 0x0074, 0x0076, 0x0077, 0x0078, 0x007A
};

static const unordered_map<UChar32, UChar32> complex_to_simple = {
    // a
    {0x00E1, 0x0061}, // á
    {0x00E0, 0x0061}, // à
    {0x00E3, 0x0061}, // ã
    {0x1EA3, 0x0061}, // ả
    {0x1EA1, 0x0061}, // ạ

    // â
    {0x1EA5, 0x00E2}, // ấ
    {0x1EA7, 0x00E2}, // ầ
    {0x1EAB, 0x00E2}, // ẫ
    {0x1EA9, 0x00E2}, // ẩ
    {0x1EAD, 0x00E2}, // ậ

    // ă
    {0x1EAF, 0x0103}, // ắ
    {0x1EB1, 0x0103}, // ằ
    {0x1EB5, 0x0103}, // ẵ
    {0x1EB3, 0x0103}, // ẳ
    {0x1EB7, 0x0103}, // ặ

    // e
    {0x00E9, 0x0065}, // é
    {0x00E8, 0x0065}, // è
    {0x1EBB, 0x0065}, // ẻ
    {0x1EBD, 0x0065}, // ẽ
    {0x1EB9, 0x0065}, // ẹ

    // ê
    {0x1EBF, 0x00EA}, // ế
    {0x1EC1, 0x00EA}, // ề
    {0x1EC5, 0x00EA}, // ễ
    {0x1EC3, 0x00EA}, // ể
    {0x1EC7, 0x00EA}, // ệ

    // i
    {0x00ED, 0x0069}, // í
    {0x00EC, 0x0069}, // ì
    {0x1EC9, 0x0069}, // ỉ
    {0x0129, 0x0069}, // ĩ
    {0x1ECB, 0x0069}, // ị

    // o
    {0x00F3, 0x006F}, // ó
    {0x00F2, 0x006F}, // ò
    {0x1ECF, 0x006F}, // ỏ
    {0x00F5, 0x006F}, // õ
    {0x1ECD, 0x006F}, // ọ

    // ô
    {0x1ED1, 0x00F4}, // ố
    {0x1ED3, 0x00F4}, // ồ
    {0x1ED7, 0x00F4}, // ỗ
    {0x1ED5, 0x00F4}, // ổ
    {0x1ED9, 0x00F4}, // ộ

    // ơ
    {0x1EDB, 0x01A1}, // ớ
    {0x1EDD, 0x01A1}, // ờ
    {0x1EE1, 0x01A1}, // ỡ
    {0x1EDF, 0x01A1}, // ở
    {0x1EE3, 0x01A1}, // ợ

    // u
    {0x00FA, 0x0075}, // ú
    {0x00F9, 0x0075}, // ù
    {0x1EE7, 0x0075}, // ủ
    {0x0169, 0x0075}, // ũ
    {0x1EE5, 0x0075}, // ụ

    // ư
    {0x1EE9, 0x01B0}, // ứ
    {0x1EEB, 0x01B0}, // ừ
    {0x1EEF, 0x01B0}, // ữ
    {0x1EED, 0x01B0}, // ử
    {0x1EF1, 0x01B0}, // ự

    // y
    {0x00FD, 0x0079}, // ý
    {0x1EF3, 0x0079}, // ỳ
    {0x1EF7, 0x0079}, // ỹ
    {0x1EF9, 0x0079}, // ỷ
    {0x1EF5, 0x0079}, // ỵ

    // đ
    {0x0110, 0x0111}  // Đ → đ
};


void process_lines(const vector<string>& lines, size_t start, size_t end,
                   unordered_map<UChar32, int>& local_count, bool use_simple) {
    for (size_t idx = start; idx < end; ++idx) {
        const string& line = lines[idx];
        int32_t length = static_cast<int32_t>(line.length());
        int32_t i = 0;
        UChar32 c;

        while (i < length) {
            U8_NEXT(line.c_str(), i, length, c);
            if (c < 0) continue;
            c = u_tolower(c);

            if (c == U'z' || c == U'w' || c == U'j') continue;

            if (use_simple) {
                auto it = complex_to_simple.find(c);
                if (it != complex_to_simple.end()) c = it->second;
                if (vietnamese_chars_simp.count(c)) local_count[c]++;
            } else {
                if (vietnamese_chars_comp.count(c)) local_count[c]++;
            }
        }
    }
}

int main() {
    const bool use_simple = true;
    vector<string> lines;
    ifstream file("C:/Users/nguye/OneDrive/Desktop/viwiki-20250620-pages-articles-multistream/truyenkieu.txt", ios::in | ios::binary);
    if (!file.is_open()) {
        cerr << "Failed to open input file." << endl;
        return 1;
    }

    string line;
    size_t line_count = 0;
    while (getline(file, line)) {
        lines.push_back(move(line));
        line_count++;
        if (line_count % 1000000 == 0) {
            cout << "Read " << line_count << " lines..." << endl;
        }
    }
    file.close();

    cout << "Loaded " << lines.size() << " lines. Processing on " << MAX_THREADS << " threads..." << endl;

    vector<unordered_map<UChar32, int>> thread_counts(MAX_THREADS);
    vector<thread> threads;
    size_t chunk_size = (lines.size() + MAX_THREADS - 1) / MAX_THREADS;

    for (size_t t = 0; t < MAX_THREADS; ++t) {
        size_t start = t * chunk_size;
        size_t end = min(start + chunk_size, lines.size());
        threads.emplace_back(process_lines, cref(lines), start, end, ref(thread_counts[t]), use_simple);
    }

    for (auto& th : threads) th.join();

    unordered_map<UChar32, int> merged_count;
    for (const auto& local_map : thread_counts) {
        for (const auto& [c, count] : local_map) {
            merged_count[c] += count;
        }
    }

    cout << "Finished processing. Sorting and writing output..." << endl;

    vector<pair<UChar32, int>> sorted_counts(merged_count.begin(), merged_count.end());
    sort(sorted_counts.begin(), sorted_counts.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    ofstream output_file("C:/Users/nguye/OneDrive/Desktop/Git-temp/Personal-Projects/Cpp/Word-frequency/char_freq.txt", ios::out | ios::binary);
    if (!output_file.is_open()) {
        cerr << "Failed to open output file." << endl;
        return 1;
    }

    int total_count = 0;
    for (const auto& [_, count] : sorted_counts) {
        total_count += count;
    }


    for (const auto& [c, count] : sorted_counts) {
        UnicodeString ustr(c);
        string utf8;
        ustr.toUTF8String(utf8);
        double percentage = 100.0 * count / total_count;
        output_file << utf8 << ": " << setw(10) << count << " (" << fixed << setprecision(4) << percentage << "%)\n";
    }

    output_file.close();
    cout << "Done writing output to file." << endl;
    return 0;
}
