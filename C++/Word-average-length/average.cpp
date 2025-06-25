#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <mpfr.h>

using namespace std;

int main() {
    string word;
    int totalLength = 0;
    long double wordCount = 0;

    ifstream file("AgileWords.txt");

    while (getline(file, word))
        if (!word.empty()) {
            totalLength += word.length();
            wordCount++;
        }

    file.close();

    if (wordCount == 0) {
        cout << "No words found in file." << endl;
    } else {
        // Initialize MPFR numbers with 256-bit precision
        mpfr_t totalLenMPFR, wordCountMPFR, average;
        mpfr_init2(totalLenMPFR, 256);
        mpfr_init2(wordCountMPFR, 256);
        mpfr_init2(average, 256);

        // Set values
        mpfr_set_si(totalLenMPFR, totalLength, MPFR_RNDN);   // from int
        mpfr_set_si(wordCountMPFR, wordCount, MPFR_RNDN);

        // average = totalLen / wordCount
        mpfr_div(average, totalLenMPFR, wordCountMPFR, MPFR_RNDN);

        // Print result with 50 digits
        mpfr_printf("Average word length: %.50Rf\n", average);

        // Clean up
        mpfr_clear(totalLenMPFR);
        mpfr_clear(wordCountMPFR);
        mpfr_clear(average);

        // long double average = static_cast<long double>(totalLength) / wordCount;
        // cout << "Average word length: " << fixed << setprecision(50) << average << endl;

        // double average = static_cast<double>(totalLength) / wordCount;
        // cout << "Average word length: " << fixed << setprecision(50) << average << endl;
    }

    return 0;
}

/*
Average word length: 5.92279431924110325163129900751220047156878872621593
*/