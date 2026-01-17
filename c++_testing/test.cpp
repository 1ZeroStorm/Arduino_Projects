#include <iostream>

using createstring = std::string;
using std::cout;
using std::cin;


int main(){
    int age;
    cout << "enter age";
    cin >> age;

    cout << "enter full name: ";
    createstring fullname;
    std::getline(cin >> std::ws, fullname);
    
    cout << "your full name is " << fullname;

    return 0;
}
