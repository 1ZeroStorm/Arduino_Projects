#include <iostream>
using namespace std;

int main(){
    
    int *pointer;
    int x = 41;

    pointer = &x;
    if (pointer == nullptr){
        cout << "null, points to nothing";

    }else{
        cout << "points to somthing :" << *pointer;
    }

    return 0;
}
