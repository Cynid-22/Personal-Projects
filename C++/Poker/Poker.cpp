#include <iostream>
#include <cstdlib>
#include <unordered_map>

using namespace std;

unordered_map<string, int> SettingUp(int& numberOfPlayers, int& defaultMoney) 
{
    unordered_map<string, int> moneyPerPerson;
    string tempName;
    
    cout << "Amount of players: ";
    cin >> numberOfPlayers;

    cout << "Default money: ";
    cin >> defaultMoney;

    for (int i = 0; i < numberOfPlayers; ++i) {
        cout << "Input player " << i+1 << " name: ";
        cin >> tempName;
        moneyPerPerson[tempName] = defaultMoney;
    }
    
    return moneyPerPerson;
}

int main()
{
    srand(time(0));
    int numPlayers = 3, defaultCash = 1000;
    auto players = SettingUp(numPlayers, defaultCash);
    string cards [52] = {"2-C", "3-C", "4-C", "5-C", "6-C", "7-C", "8-C", "9-C", "10-C", "J-C", "Q-C", "K-C", "A-C",  
                         "2-S", "3-S", "4-S", "5-S", "6-S", "7-S", "8-S", "9-S", "10-S", "J-S", "Q-S", "K-S", "A-S",  
                         "2-H", "3-H", "4-H", "5-H", "6-H", "7-H", "8-H", "9-H", "10-H", "J-H", "Q-H", "K-H", "A-H",  
                         "2-D", "3-D", "4-D", "5-D", "6-D", "7-D", "8-D", "9-D", "10-D", "J-D", "Q-D", "K-D", "A-D"};


    
    for (const auto& [name, money] : players) {
        cout << name << " has $" << money << endl;
    }

    return 0;
}

/*
Output:

*/