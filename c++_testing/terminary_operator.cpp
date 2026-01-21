#include <iostream>
#include <cmath>
using namespace std;

int main(){
    
    int numb;
    cout << "insert your number: ";
    cin >> numb;

    numb % 2 == 0 ? cout << "even" : cout << "odd";

    return 0;
}