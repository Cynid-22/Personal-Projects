#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <random>
#include <cctype>
#include <algorithm>

using namespace std;

// Random word picker
string getRandomWord(const vector<string>& words) {
    if (words.empty()) return "No match";
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(0, words.size() - 1);
    return words[dist(gen)];
}

// Load word list from file
void inputFile(vector<string>& allowedWordsList) {
    ifstream file("valid-wordle-words.txt");
    string line;
    while (getline(file, line)) {
        if (!line.empty())
            allowedWordsList.push_back(line);
    }
    file.close();
}

// Word filter based on Wordle feedback
string CalculateWord(string wordsArr[6], char colorArr[6][5], int currentPlace,
                     vector<string>& allowedWordsList, vector<char>& disallowedLetterList,
                     const vector<string>& usedWords) {
    
    vector<string> greenFiltered;
    string currentGuess = wordsArr[currentPlace - 1];
    int correctPlace[5] = {0, 0, 0, 0, 0}; // 2 = green, 1 = yellow, 0 = black

    // Assign color codes
    for (int i = 0; i < 5; ++i) {
        if (colorArr[currentPlace - 1][i] == 'G')
            correctPlace[i] = 2;
        else if (colorArr[currentPlace - 1][i] == 'Y')
            correctPlace[i] = 1;
    }

    // Filter green positions
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
                    if (j == i || correctPlace[j] == 2) continue;
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

    // Filter black letters (but not if they are yellow/green elsewhere)
    vector<string> blackFiltered;
    for (const string& word : yellowFiltered) {
        bool fail = false;
        for (int i = 0; i < 5 && !fail; ++i) {
            for (const char& letter : disallowedLetterList) {
                if (word[i] == letter) {
                    fail = true;
                    break;
                }
            }
        }
        if (!fail)
            blackFiltered.push_back(word);
    }

    // Remove used words
    vector<string> finalCandidates;
    for (const string& word : blackFiltered) {
        if (find(usedWords.begin(), usedWords.end(), word) == usedWords.end())
            finalCandidates.push_back(word);
    }

    allowedWordsList = blackFiltered;
    return finalCandidates.empty() ? "No match" : getRandomWord(finalCandidates);
}

int main() {
    string words[6], strTemp;
    char color[6][5], chTemp;
    vector<string> allowedWords;
    vector<char> disallowedLetters;
    vector<string> usedWords;
    int count = 0, regenCount = 0;

    cout << "Do you want to automatically choose the first word? y/n: ";
    cin >> chTemp;

    if (chTemp == 'y' || chTemp == 'Y') {
        words[0] = "salet"; // kudos to 3Blue1Brown for this word
        cout << "\nThe first word is: salet\n";
    } else {
        cout << "Input word: ";
        cin >> strTemp;
        for (char& ch : strTemp)
            ch = tolower(ch);
        words[0] = strTemp;
    }

    inputFile(allowedWords);
    usedWords.push_back(words[0]);

    while (count < 6) {
        bool regenerate = false;

        cout << "Enter the result (e.g., G G Y B B or R to regenerate):\n> ";
        for (int i = 0; i < 5; i++) {
            cin >> chTemp;
            chTemp = toupper(chTemp);
            if (chTemp == 'R') {
                regenerate = true;
                regenCount++;
                break;
            }
            if (chTemp != 'G' && chTemp != 'Y' && chTemp != 'B') {
                cout << "Invalid input. Enter G, Y, B, or R.\n";
                --i;
                continue;
            }
            color[count][i] = chTemp;
        }

        if (!regenerate) {
            // Collect green/yellow letters this round
            vector<char> greenYellowLetters;
            for (int i = 0; i < 5; ++i) {
                char c = words[count][i];
                if (color[count][i] == 'G' || color[count][i] == 'Y') {
                    if (find(greenYellowLetters.begin(), greenYellowLetters.end(), c) == greenYellowLetters.end())
                        greenYellowLetters.push_back(c);
                }
            }

            // Add disallowed black letters not seen as green/yellow
            for (int i = 0; i < 5; ++i) {
                if (color[count][i] == 'B') {
                    char c = words[count][i];
                    if (find(greenYellowLetters.begin(), greenYellowLetters.end(), c) == greenYellowLetters.end() &&
                        find(disallowedLetters.begin(), disallowedLetters.end(), c) == disallowedLetters.end()) {
                        disallowedLetters.push_back(c);
                    }
                }
            }

            // Check for win
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

        // Generate a new word that hasn't been used
        string nextWord;
        do {
            nextWord = CalculateWord(words, color, count, allowedWords, disallowedLetters, usedWords);
        } while (nextWord == words[count - 1] && allowedWords.size() > 1);

        words[count] = nextWord;
        usedWords.push_back(nextWord);
        cout << "\nTry this word: " << words[count] << " | available words: " << allowedWords.size() - regenCount << "\n";
    }

    cout << "\nOut of attempts!\n";
    return 0;
}
