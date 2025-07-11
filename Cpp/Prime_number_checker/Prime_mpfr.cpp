#include <iostream>
#include <gmp.h>
#include <mpfr.h>

using namespace std;

bool isPrime(mpz_t number) {
    return (mpz_cmp_ui(number, 2) < 0); // number < 2

    return (!mpz_even_p(number)) ; // !even

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

    mpz_clear(number);
    return 0;
}


/*
Command to run:
g++ -std=c++23 -I/mingw64/include -L/mingw64/lib Prime.cpp -o Prime -lmpfr -lgmp && Prime.exe

*/