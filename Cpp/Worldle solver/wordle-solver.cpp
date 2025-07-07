#include <iostream>
#include <cctype>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

string CalculateWord(string wordsArr[6], char colorArr[6][5], int currentPlace, vector<string>& allowedWordsList)
{
    vector<string> remainingWords;
    int correctPlace[] = {0, 0, 0, 0, 0}, iter = 0;

    for (int i = 0; i < 5; ++i) {
        if (colorArr[currentPlace - 1][i] == 'G') {
            correctPlace[i] = 2; // green = 2, yellow = 1, gray/black = 0
        }
        if (colorArr[currentPlace - 1][i] == 'Y') {
            correctPlace[i] = 1;
        }
    }
    
    // Find correct (green) letter positions
    for (const string& word : allowedWordsList) {
        bool fail = false;
        for (int i = 0; i < 5; ++i) {
            if (correctPlace[i] == 2 && word[i] != wordsArr[currentPlace - 1][i]) {
                fail = true;
                break;
            }
        }
        if (!fail)
            remainingWords.push_back(word);
    }

    allowedWordsList = remainingWords;

    for (const string& word : allowedWordsList) {
        bool fail = false;
        for (int i = 0; i < 5; ++i) {
            if (correctPlace[i] == 1) {
                
            }
        }
        if (!fail)
            remainingWords.push_back(word);
    }
    
}

void inputFile(vector<string>& allowedWordsList)
{
    ifstream file("valid-wordle-words.txt");
    string line;
    while (getline(file, line)) {
        if (!line.empty())
            allowedWordsList.push_back(line);
    }
    file.close();
}

int main()
{
    string words[6], strTemp;
    char color[6][5], chTemp;
    vector<string> allowedWords;
    int count = 0;

    cout << "Do you want to automatically choose the first word? y/n: ";
    cin >> chTemp;

    if (chTemp == 'y' || chTemp == 'Y') {
        words[0] = "salet";
        cout << "\nThe first word is: salet\n";
    }
    else {
        cout << "Input word: ";
        cin >> strTemp;
        for (char &ch : strTemp)
            ch = tolower(ch);  // Wordle uses lowercase
        words[0] = strTemp;
    }

    inputFile(allowedWords);

    while (count < 6) {
        cout << "Enter the result like so: G G Y B B\n> ";
        for (int i = 0; i < 5; i++) {
            cin >> chTemp;
            color[count][i] = toupper(chTemp);
        }

        // Check if all letters are green
        if (color[count][0] == 'G' && color[count][1] == 'G' &&
            color[count][2] == 'G' && color[count][3] == 'G' &&
            color[count][4] == 'G') {
            cout << "Finished.\n";
            return 0;
        }

        count++;

        if (count >= 6)
            break;

        words[count] = CalculateWord(words, color, count, allowedWords);
        cout << "\nTry this word: " << words[count] << "\n";
    }

    cout << "Out of attempts!\n";
    return 0;
}
