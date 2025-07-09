#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

int main() {
    ifstream infile("unigram_freq.csv");
    ofstream outfile("filtered_words.csv");

    if (!infile.is_open() || !outfile.is_open()) {
        cerr << "Failed to open file." << endl;
        return 1;
    }

    string line;
    bool header_skipped = false;

    while (getline(infile, line)) {
        if (!header_skipped) {
            header_skipped = true;
        }

        stringstream ss(line);
        string word, count;

        if (getline(ss, word, ',') && getline(ss, count)) {
            if (word.length() == 5) {
                outfile << word << "," << count << "\n";
            }
        }
    }

    infile.close();
    outfile.close();

    cout << "Filtering complete. Output written to filtered_words.csv" << endl;
    return 0;
}