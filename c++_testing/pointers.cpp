#include <iostream>
using namespace std;

int main(){
    string bro = "hello";
    string pizza[5] = {"p1", "p2", "p3", "p4", "p5"};
    string *name = &bro;

    string *add = pizza;
    cout << *add;

    return 0;
}
