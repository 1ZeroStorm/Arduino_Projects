#include <iostream>
#include <cmath>
using namespace std;

int main(){
    
    int numb;
    cout << "insert your number: ";
    cin >> numb;

    //numb % 2 == 0 ? cout << "even" : cout << "odd";

    cout << (numb %2 == 0 ? "even" : "Odd");

    return 0;
}