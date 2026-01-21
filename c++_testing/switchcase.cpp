#include <iostream>
using namespace std;

int main(){
    char eq;
    double a;
    double b;

    cout << "----------------------------calculator----------------------------\n";
    cout << "insert equation(+-*/): ";
    cin >> eq;
    

    cout << "insert the first number: ";
    cin >> a;

    cout << "insert the second number: ";
    cin >> b;

    cout << "result: ";

    switch (eq)
    {
    case '+':
        cout << (a + b);
        break;
    
    case '-':
        cout << (a - b);
        break;
    case '/':
        cout << (a / b);
        break;
    case '*':
        cout << (a * b);
        break;
    default:
        break;
    }
    return 0;
}