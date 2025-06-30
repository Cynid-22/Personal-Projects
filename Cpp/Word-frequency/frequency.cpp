#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <unicode/unistr.h>
#include <unicode/ustream.h>
#include <unicode/uchar.h>
#include <unicode/utf8.h>

using namespace std;
using namespace icu;

bool is_valid_vietnamese_char(UChar32 c) {
    // Exclude punctuation, symbols, spaces, digits
    return u_isalpha(c);
}

int main() {
    unordered_map<UChar32, int> char_count;

    ifstream file("C:/Users/nguye/OneDrive/Desktop/viwiki-20250620-pages-articles-multistream/all_text.txt", ios::in | ios::binary);
    if (!file.is_open()) {
        cerr << "Failed to open input file." << endl;
        return 1;
    }

    string line;
    while (getline(file, line)) {
        int32_t length = static_cast<int32_t>(line.length());
        int32_t i = 0;
        UChar32 c;

        while (i < length) {
            U8_NEXT(line.c_str(), i, length, c);
            if (c >= 0 && is_valid_vietnamese_char(c)) {
                char_count[c]++;
            }
        }
    }

    file.close();

    ofstream output_file("char_freq.txt", ios::out | ios::binary);
    if (!output_file.is_open()) {
        cerr << "Failed to open output file." << endl;
        return 1;
    }

    for (const auto& [c, count] : char_count) {
        UnicodeString ustr(c);
        string utf8;
        ustr.toUTF8String(utf8);
        output_file << utf8 << ": " << count << "\n";
    }

    output_file.close();
    cout << "Character frequency written to char_frequencies.txt\n";

    return 0;
}
