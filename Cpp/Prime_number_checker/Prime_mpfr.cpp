#include <iostream>
#include <gmp.h>
#include <mpfr.h>
#include <chrono>

using namespace std;

auto start = chrono::high_resolution_clock::now();

void EndTime()
{
    auto end = chrono::high_resolution_clock::now();
    double time_taken = (chrono::duration_cast<chrono::nanoseconds>(end - start).count())*1e-6;
    cout << "\n\nTime taken : " << time_taken << " ms" << endl;
}

bool isPrime(mpz_t number) {
    start = chrono::high_resolution_clock::now();

    if (mpz_cmp_ui(number, 2) < 0) return false; // number < 2

    if (mpz_even_p(number)) return mpz_cmp_ui(number, 2) == 0; // true if 2, false if even && != 2

    mpz_t checkNum, i, rem;
    mpz_inits(checkNum, i, rem, nullptr);

    mpz_sqrt(checkNum, number); // checkNum = sqrt(number)
    
    mpz_set_ui(i, 3); // i = 3
    while (mpz_cmp(i, checkNum) <= 0) { // while i <= checkNum
        mpz_mod(rem, number, i);
        if (mpz_cmp_ui(rem, 0) == 0) {
            mpz_clears(checkNum, i, rem, nullptr);
            return false;
        }
        mpz_add_ui(i, i, 2); // i += 2
    }

    mpz_clears(checkNum, i, rem, nullptr);
    return true;
}

int main() {
    mpz_t number;
    mpz_init(number);

    cout << "What number do you want to check? ";
    string input;
    cin >> input;
    mpz_set_str(number, input.c_str(), 10); // base 10 input

    cout << input << " is ";
    if (isPrime(number))
        cout << "prime.";
    else
        cout << "NOT prime.";
    cout << endl;

    EndTime();

    mpz_clear(number);
    return 0;
}


/*

Test prime:
18446744073709551557

Test compostite:
18446744073709551615

Command to run:
g++ -std=c++23 -I/mingw64/include -L/mingw64/lib Prime_mpfr.cpp -o Prime_mpfr -lmpfr -lgmp && Prime_mpfr.exe

*/