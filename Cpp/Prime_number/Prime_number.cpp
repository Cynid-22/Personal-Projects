#include <iostream>
#include <bits/stdc++.h>
#include <chrono>
#include <cmath>
#include <iomanip>

using namespace std;

auto start = chrono::high_resolution_clock::now();

void EndTime()
{
    auto end = chrono::high_resolution_clock::now();
    double time_taken = (chrono::duration_cast<chrono::nanoseconds>(end - start).count())*1e-6;
    cout << "Time taken : " << time_taken << " ms" << endl;
}

bool CheckPrime (int a)
{
    int CheckNum = sqrt(a);
    for (int i=2; i<=CheckNum; ++i)
        if (a % i == 0)
            return false;
    return true;
}

int main()
{

    int counter = 1;
    int stop = 1'000'000;
    int precision = 1000;
    double progress = stop/precision;
    for (int i=3; i<=stop; i+=2) {
        if (CheckPrime(i))
            counter++;
        if (i%int(progress) == 1)
            cout << fixed << setprecision(1) << (i/progress)/(precision/100) << "%\n";
    }
    cout << "\n*** " << counter << " Prime numbers below " << stop << "\n\n";

    EndTime();
    
    return 0;
}

/*
Output:
1,000,000
78,498

10,000,000
664,579

100,000,000
5,761,455

1,000,000,000
50,847,534

*/