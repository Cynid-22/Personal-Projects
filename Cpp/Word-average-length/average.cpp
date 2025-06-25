#include <iostream>
#include <cstdint>
#include <fstream>
#include <string>
#include <iomanip>
#include <mpfr.h>

using namespace std;

int main() {
    string word;
    int totalLength = 0;
    long double wordCount = 0;
    const uint32_t precision = 1024;

    ifstream file("AgileWords.txt");

    while (getline(file, word))
        if (!word.empty()) {
            totalLength += word.length();
            wordCount++;
        }

    file.close();

    if (wordCount == 0) {
        cout << "No words found in file." << endl;
    } else {
        mpfr_t totalLenMPFR, wordCountMPFR, average;
        mpfr_init2(totalLenMPFR, precision);
        mpfr_init2(wordCountMPFR, precision);
        mpfr_init2(average, precision);

        mpfr_set_si(totalLenMPFR, totalLength, MPFR_RNDN);
        mpfr_set_si(wordCountMPFR, wordCount, MPFR_RNDN);

        mpfr_div(average, totalLenMPFR, wordCountMPFR, MPFR_RNDN);

        // Print result with 50 digits
        mpfr_printf("Average word length: %.1000Rf\n", average);

        mpfr_clear(totalLenMPFR);
        mpfr_clear(wordCountMPFR);
        mpfr_clear(average);

        // long double average = static_cast<long double>(totalLength) / wordCount;
        // cout << "Average word length: " << fixed << setprecision(50) << average << endl;

        // double average = static_cast<double>(totalLength) / wordCount;
        // cout << "Average word length: " << fixed << setprecision(50) << average << endl;
    }

    return 0;
}

/*
Average word length: 5.9227943192411032516312990075122004715687887262159346383725393430937105883643143060810440313648078083018040247847781981685584251795799747765531611558918681800734769973131545758622580468278773921149311838569940231397707956352470252782804189285518451499698415309535559576684761748094533092065580961780994681142775414892637521180803665579490848341972470680014105247367426617250396823216674582617663036740202414802302882448860678776840570267551637146619791568050337045932562376207070112020256219043481898970889113743914966871448487871614755548083216867262855927385029042715927736320326503278069981545302916224720728957331436291341622191629246864647900618438244048044209663873048581570196855056727764389953558176669772981241957014643531439403788744140451815358746929632844261594723717661989181277353362221212392229051486642663010580333906065835131946509101053088896367337911349331989205471835102329203882075733633799566258846672656994236518372397857514924875839005650891851628731343370852513441349174172501
*/