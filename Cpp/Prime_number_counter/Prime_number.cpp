#include <iostream>
#include <bitset>
#include <vector>
#include <chrono>
#include <algorithm>

using namespace std;

auto start = chrono::high_resolution_clock::now();

void EndTime() {
    cout << "Time taken : " << chrono::duration_cast<chrono::nanoseconds>(chrono::high_resolution_clock::now() - start).count() * 1e-6 << " ms\n";
}

uint_fast64_t CountPrimes(const uint_fast64_t stop) {
    if (stop <= 10'000'000) {
        bitset<10'000'001> is_prime;
        is_prime.set();
        is_prime[0] = is_prime[1] = 0;

        for (uint_fast64_t i = 2; i * i <= stop; ++i)
            if (is_prime[i])
                for (uint_fast64_t j = i * i; j <= stop; j += i)
                    is_prime[j] = 0;

        uint_fast64_t total = 0;
        for (uint_fast64_t i = 2; i <= stop; ++i)
            total += is_prime[i];
            
        return total;

    } else {
        vector<bool> is_prime(stop + 1, true);
        is_prime[0] = is_prime[1] = false;

        for (uint_fast64_t i = 2; i * i <= stop; ++i)
            if (is_prime[i])
                for (uint_fast64_t j = i * i; j <= stop; j += i)
                    is_prime[j] = false;

        return count(is_prime.begin(), is_prime.end(), true);
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    const uint_fast64_t stop = 1'000'000, prime_count = CountPrimes(stop);

    cout << "\n*** " << prime_count << " Prime numbers below " << stop << "\n\n";
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
