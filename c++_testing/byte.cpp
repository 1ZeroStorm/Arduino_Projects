#include <iostream>
using namespace std;


int main(){
    
    
    double num = 9;
    char grade = 'A';
    string foodlist[] = {"apple", "orange", "banana"};
    string a_food = "mangoooooooooooooooooooo";
    
    cout << sizeof(num) << "\n"; // 8 byte
    cout << sizeof(foodlist) << "\n"; // 3 elements * 24 byte 
    cout << sizeof(grade) << "\n"; // 1 char = 1 byte
    cout << sizeof(a_food) << "\n"; // string with any length has 24 bytes

    return 0;
}