#include <iostream>
#include <cstdint>
#include <mpfr.h>


using namespace std;

int main()
{
    const uint32_t precision = 512; // Set the precision to 512 bits
    mpfr_set_default_prec(precision); // Set the default precision for all MPFR operations
    mpfr_t s, t;
    const long double a = 5, b = 7; 

    mpfr_init2(t, precision); //initialize t with the specified precision
    mpfr_set_d(t, a, MPFR_RNDN); // Set t to 10.0 with rounding mode MPFR_RNDN - round to nearest

    mpfr_init2(s, precision);
    mpfr_set_d(s, b, MPFR_RNDN);

    mpfr_div(t, t, s, MPFR_RNDN); // Divide t by s and store the result in t with rounding mode MPFR_RNDN - t = t / s

    cout << t << "/" << s << " = ";
    mpfr_out_str(stdout, 10, 0, t, MPFR_RNDN); // Output t in base 10 with no digits after the decimal point
    putchar('\n'); 

    mpfr_clear(s);
    mpfr_clear(t);
    mpfr_free_cache();

    return 0;
}

/*
Output:

*/