#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <random>
#include <cctype>
#include <algorithm>
#include <unordered_map>

using namespace std;

string getWordRandom(const vector<string>& words) {
    if (words.empty()) return "No match";
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(0, words.size() - 1);
    return words[dist(gen)];
}

string getWord(const vector<string>& words, const unordered_map<string, uint64_t>& freqMap) {
    if (words.empty()) return "No match";

    string bestWord = words[0];
    uint64_t maxFreq = freqMap.count(bestWord) ? freqMap.at(bestWord) : 0;

    for (const string& word : words) {
        uint64_t freq = freqMap.count(word) ? freqMap.at(word) : 0;
        if (freq > maxFreq) {
            maxFreq = freq;
            bestWord = word;
        }
    }

    return bestWord;
}

unordered_map<string, uint64_t> loadFrequencies(const string& filename) {
    unordered_map<string, uint64_t> freqMap;
    ifstream file(filename);
    string line;

    while (getline(file, line)) {
        size_t commaPos = line.find(',');
        if (commaPos != string::npos) {
            string word = line.substr(0, commaPos);
            uint64_t freq = stoull(line.substr(commaPos + 1));
            freqMap[word] = freq;
        }
    }

    return freqMap;
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
                     vector<string>& allowedWordsList, vector<char>& disallowedLetterList,
                     const vector<string>& usedWords,  const unordered_map<string, uint64_t>& freqMap) {
    
    vector<string> greenFiltered;
    string currentGuess = wordsArr[currentPlace - 1];
    int correctPlace[5] = {0, 0, 0, 0, 0}; // 2 = green, 1 = yellow, 0 = black

    for (int i = 0; i < 5; ++i) {
        if (colorArr[currentPlace - 1][i] == 'G')
            correctPlace[i] = 2;
        else if (colorArr[currentPlace - 1][i] == 'Y')
            correctPlace[i] = 1;
    }

    // Filter green
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

    // Filter yellow
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

    // Filter black (except if they’re also yellow/green)
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

    // Remove words already used
    vector<string> finalCandidates;
    for (const string& word : blackFiltered) {
        if (find(usedWords.begin(), usedWords.end(), word) == usedWords.end())
            finalCandidates.push_back(word);
    }

    allowedWordsList = blackFiltered;

    if (finalCandidates.empty())
        return "No match";

    string chosen = getWord(finalCandidates, freqMap);

    allowedWordsList.erase(
        remove(allowedWordsList.begin(), allowedWordsList.end(), chosen),
        allowedWordsList.end()
    );

    return chosen;
}


int main() {
    restart:

    string words[6] = {}, strTemp;
    char color[6][5] = {}, chTemp;
    vector<char> disallowedLetters = {};
    vector<string> usedWords = {};
    int count = 0, regenCount = 0;
    
    vector<string> allowedWords;
    unordered_map<string, uint64_t> freqMap = loadFrequencies("filtered_words.csv");
    inputFile(allowedWords);


    cout << "Do you want to automatically choose the first word? y/n/r(random): ";
    cin >> chTemp;

    if (chTemp == 'y' || chTemp == 'Y') {
        words[0] = "salet"; // kudos to 3Blue1Brown for this word
        cout << "\nThe first word is: salet\n";
    }
    else if (chTemp == 'r' || chTemp == 'R'){
        words[0] = getWordRandom(allowedWords);
        cout << "\nThe first word is: " << words[0] << endl;
    } else {
        cout << "Input word: ";
        cin >> strTemp;
        for (char& ch : strTemp)
            ch = tolower(ch);
        words[0] = strTemp;
    }

    usedWords.push_back(words[0]);

    while (count < 6) {
        showResult:

        bool regenerate = false;

        cout << "Enter the result (e.g., G G Y B B or R-regenerate, S-show remaining words):\n> ";
        for (int i = 0; i < 5; i++) {
            cin >> chTemp;
            chTemp = toupper(chTemp);
            if (chTemp == 'R') {
                regenerate = true;
                regenCount++;
                break;
            }
            if (chTemp == 'S') {
                int iter = 0;
                for (const string& word : allowedWords) {
                    cout << word << "  ";
                    iter++;
                    if (iter % 6 == 0)
                        cout << "\n";
                }
                cout << "\n";
                goto showResult;
            }
            if (chTemp != 'G' && chTemp != 'Y' && chTemp != 'B' && chTemp != 'S') {
                cout << "Invalid input. Enter G, Y, B, R or S.\n";
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
                cout << "\n\nDo you want to continue playing? y/n: ";
                cin >> chTemp;
                chTemp = toupper(chTemp);
                if (chTemp == 'Y')
                    goto restart;
            }

            count++;
        }

        if (count >= 6)
            break;

        string nextWord;
        do {
            nextWord = CalculateWord(words, color, count, allowedWords, disallowedLetters, usedWords, freqMap);
        } while (nextWord == words[count - 1] && allowedWords.size() > 1);

        words[count] = nextWord;
        usedWords.push_back(nextWord);
        cout << "\nTry this word: " << words[count] << " | available words: " << allowedWords.size() << "\n";
    }

    cout << "There are " << allowedWords.size() << " words left: \n";
    int iter = 0;
    for (const string& word : allowedWords) {
        cout << word << "  ";
        iter++;
        if (iter % 5 == 0)
            cout << "\n";
    }

    cout << "\nOut of attempts!\n";

    cout << "\n\nDo you want to continue playing? y/n: ";
    cin >> chTemp;
    chTemp = toupper(chTemp);
    if (chTemp == 'Y')
        goto restart;
    
    return 0;
}

/*
salet
mochi
furzy
*/