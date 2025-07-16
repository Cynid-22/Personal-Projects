#include <iostream>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <cmath>

using cu64 = const uint_fast64_t;
using u64  = uint_fast64_t;

using namespace std;

auto start = chrono::high_resolution_clock::now();

void EndTime()
{
    auto end = chrono::high_resolution_clock::now();
    double time_taken = (chrono::duration_cast<chrono::nanoseconds>(end - start).count())*1e-6;
    cout << "\n\nTime taken : " << time_taken << " ms" << endl;
}

bool isPrime(cu64 number)
{
    start = chrono::high_resolution_clock::now();
    if (number%2 == 0 || number < 2)
        return false;

    cu64 checkNum = sqrtl(number);
    for (u64 i = 3; i < checkNum; ++i)
        if (number % i == 0) {
            cout << i << " * " << number/i << " = " << number << "\n";
            return false;
        }

    return true;
}

int main()
{
    u64 number;
    cout << "What number do you want to check? ";
    cin >> number;

    cout << "Calculating...\n";
    if (isPrime(number))
        cout << number << " is prime.";
    else
        cout << number << " is NOT prime.";

    
    EndTime();
    return 0;
}

/*
Max number:
18446744073709551615

Biggest Prime:
18446744073709551557
*/