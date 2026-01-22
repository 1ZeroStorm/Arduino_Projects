#include <iostream>
#include <cmath>
using namespace std;

int main(){
    
    int row;
    
    cout << "insert amount of rows";
    cin >> row;

    for (int i = 0; i <= row-1; i++){
        for (int j = 0; j <= row-1-i; j++){
            cout << "*";
        }
        cout << "\n";
    }


    return 0;
}