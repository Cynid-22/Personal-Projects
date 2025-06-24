#include <iostream>
#include <fstream>
#include <string>

using namespace std;

int main() {
    string word;
    int totalLength = 0;
    int wordCount = 0;

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
        double average = double(totalLength) / wordCount;
        cout << "Average word length: " << average << endl;
    }

    return 0;
}

/*
Average word length: 5.92279
*/