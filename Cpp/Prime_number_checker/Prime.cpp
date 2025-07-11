#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <cmath>

using cu64 = const uint_fast64_t;
using u64  = uint_fast64_t;

using namespace std;

bool isPrime(cu64 number)
{
    if (number%2 == 0 || number < 2)
        return false;

    cu64 checkNum = sqrtl(number);
    for (u64 i = 3; i < checkNum; ++i)
        if (number % i == 0)
            return false;

    return true;
}

int main()
{
    u64 number;
    cout << "What number do you want to check? ";
    cin >> number;

    cout << number << " is ";
    if (isPrime(number))
        cout << "prime.";
    else
        cout << "NOT prime.";
    return 0;
}

