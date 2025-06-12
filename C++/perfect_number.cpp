#include <iostream>
#include <cmath>

using namespace std;

int SumDivisor (int a)
{
    double CheckNum = sqrt(a);
    int Sum = 1;
    if (CheckNum == int(CheckNum))
        Sum -= CheckNum;

    for (int i=2; i<=CheckNum; ++i) {
        if (a % i == 0)
            Sum = Sum + i + a/i;
    }
    return Sum;
}

int main()
{
    for (int i=2; i>0; i+=2) {
        if (SumDivisor(i) == i)
            cout << "\n\n" << i << "     ";
        if (i % 1000000 == 0)
            cout << i << " ";
    }
    return 0;
}