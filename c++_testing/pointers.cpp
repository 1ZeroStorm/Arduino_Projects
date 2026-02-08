#include <iostream>
using namespace std;

int main(){
    string bro = "hello";
    string pizza[5] = {"p1", "p2", "p3", "p4", "p5"};
    string *name = &bro; // stores the address
    // cout << name; // going to print the addree
    // cout << *name; // getting the value (dereferencing)

    string *add = pizza;
    cout << *add;

    return 0;
}
