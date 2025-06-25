#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>

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
        __float128 average = static_cast<__float128>(totalLength) / wordCount;
        cout << "Average word length: " << fixed << setprecision(20) << average << endl;
    }

    return 0;
}

/*
Average word length: 5.92279
*/