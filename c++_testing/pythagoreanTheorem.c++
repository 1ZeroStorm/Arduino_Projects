#include <iostream>
#include <cmath>
using namespace std;

int main(){
    double a;
    double b;
    double c;

    cout << "get first number: ";
    cin >> c;

    cout << "get second number: ";
    cin >> b;

    a = sqrt(pow(b, 2) + pow(c, 2));
    cout << a;
    
    
    // a = sqrt(c*c + b*b);
    // cout << a;
    return 0;
}