#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <cmath>
using namespace std;

int main()
{
    const long long END = 1'000'000'000 ;
    string line;
    long long counter[10] = {0};
    ifstream myfile("pi-billion.txt");

    myfile >> line;
    
    for (int i = 0; i <= END; ++i) {
        counter[int(line[i])-48]++;
    }

    for (int i = 0; i < 10; ++i) {
        cout << i << ": " << setw(log10(END/10)+1) << counter[i];
        cout << setprecision(log10(END)-1) << " - " << abs((counter[i]-(double(END)/10.0)))/(double(END)/10.0)*100 << "%" << endl;
    }

    return 0;
}

/*
Output:

0:  99993942 - 0.006058%
1:  99997334 - 0.002666%
2: 100002410 - 0.00241%
3:  99986912 - 0.013088%
4: 100011958 - 0.011958%
5:  99998885 - 0.001115%
6: 100010387 - 0.010387%
7:  99996061 - 0.003939%
8: 100001839 - 0.001839%
9: 100000272 - 0.000272%
*/