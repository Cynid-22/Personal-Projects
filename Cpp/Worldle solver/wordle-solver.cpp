#include <iostream>
#include <cctype>
#include <fstream>
#include <string>
#include <vector>
#include <random>
#include <algorithm>

using namespace std;

string getRandomWord(const vector<string>& words) {
    if (words.empty()) return "No match";
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(0, words.size() - 1);
    return words[dist(gen)];
}

void inputFile(vector<string>& allowedWordsList) {
    ifstream file("valid-wordle-words.txt");
    string line;
    while (getline(file, line)) {
        if (!line.empty())
            allowedWordsList.push_back(line);
    }
    file.close();
}

string CalculateWord(string wordsArr[6], char colorArr[6][5], int currentPlace,
                     vector<string>& allowedWordsList, vector<char>& disallowedLetterList) {

    vector<string> greenFiltered;
    string currentGuess = wordsArr[currentPlace - 1];
    int correctPlace[5] = {0, 0, 0, 0, 0};

    for (int i = 0; i < 5; ++i) {
        if (colorArr[currentPlace - 1][i] == 'G') {
            correctPlace[i] = 2;
        }
        else if (colorArr[currentPlace - 1][i] == 'Y') {
            correctPlace[i] = 1;
        }
    }

    // Filter green letters
    for (const string& word : allowedWordsList) {
        bool fail = false;
        for (int i = 0; i < 5; ++i) {
            if (correctPlace[i] == 2 && word[i] != currentGuess[i]) {
                fail = true;
                break;
            }
        }
        if (!fail)
            greenFiltered.push_back(word);
    }

    // Filter yellow letters
    vector<string> yellowFiltered;
    for (const string& word : greenFiltered) {
        bool fail = false;
        for (int i = 0; i < 5; ++i) {
            char guessChar = currentGuess[i];

            if (correctPlace[i] == 1) {
                if (word[i] == guessChar) {
                    fail = true;
                    break;
                }

                bool foundElsewhere = false;
                for (int j = 0; j < 5; ++j) {
                    if (j == i) continue;
                    if (correctPlace[j] == 2) continue;
                    if (word[j] == guessChar) {
                        foundElsewhere = true;
                        break;
                    }
                }

                if (!foundElsewhere) {
                    fail = true;
                    break;
                }
            }
        }
        if (!fail)
            yellowFiltered.push_back(word);
    }

    // filter black letters
    vector<string> blackFiltered;
    for (const string& word : yellowFiltered) {
        bool fail = false;
        for (int i = 0; i < 5 && fail == false; ++i)
            for(const char& letter : disallowedLetterList)
                if (word[i] == letter) {
                    fail = true;
                    break;
                }

        if (!fail)
            blackFiltered.push_back(word);
    }
    allowedWordsList = blackFiltered;

    return blackFiltered.empty() ? "No match" : getRandomWord(blackFiltered);
}

int main() {
    string words[6], strTemp;
    char color[6][5], chTemp;
    vector<string> allowedWords;
    vector<char> disallowedLetters;
    int count = 0;

    cout << "Do you want to automatically choose the first word? y/n: ";
    cin >> chTemp;

    if (chTemp == 'y' || chTemp == 'Y') {
        words[0] = "salet";
        cout << "\nThe first word is: salet\n";
    } else {
        cout << "Input word: ";
        cin >> strTemp;
        for (char& ch : strTemp)
            ch = tolower(ch);
        words[0] = strTemp;
    }

    inputFile(allowedWords);

    while (count < 6) {
        bool regenerate = false;

        cout << "Enter the result (with space between each letter, e.g., G G Y B B; R to retry):\n>";
        for (int i = 0; i < 5; i++) {
            cin >> chTemp;
            chTemp = toupper(chTemp);
            if (chTemp == 'R') {
                regenerate = true;
                break;
            }
            if (chTemp != 'G' && chTemp != 'Y' && chTemp != 'B') {
                cout << "Invalid input. Please enter G, Y, B, or R.\n";
                --i; // repeat this input
                continue;
            }
            color[count][i] = chTemp;
        }

        if (!regenerate) {
            vector<char> greenYellowLetters;

        for (int i = 0; i < 5; ++i) {
            char c = words[count][i];
            if (color[count][i] == 'G' || color[count][i] == 'Y') {
                if (find(greenYellowLetters.begin(), greenYellowLetters.end(), c) == greenYellowLetters.end())
                    greenYellowLetters.push_back(c);
            }
        }

        // Only add Black letters that are NOT marked green or yellow anywhere in this guess
        for (int i = 0; i < 5; ++i) {
            if (color[count][i] == 'B') {
                char c = words[count][i];
                if (find(greenYellowLetters.begin(), greenYellowLetters.end(), c) == greenYellowLetters.end() &&
                    find(disallowedLetters.begin(), disallowedLetters.end(), c) == disallowedLetters.end()) {
                    disallowedLetters.push_back(c);
                }
            }
        }

            bool allGreen = true;
            for (int i = 0; i < 5; i++) {
                if (color[count][i] != 'G') {
                    allGreen = false;
                    break;
                }
            }
            if (allGreen) {
                cout << "Finished! Word guessed in " << count + 1 << " tries.\n";
                return 0;
            }
            count++;
        }

        if (count >= 6)
            break;

        string nextWord;
        do nextWord = CalculateWord(words, color, count, allowedWords, disallowedLetters);
        while (nextWord == words[count - 1] && allowedWords.size() > 1);

        words[count] = nextWord;
        cout << "\nTry this word: " << words[count] << "\n";
    }

    cout << "\nOut of attempts!\n";
    return 0;
}
