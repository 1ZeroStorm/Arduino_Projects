#include <iostream>
#include <cmath>
#include <ctime>
using namespace std;

int main(){
    
    srand(time(NULL));

    for (int i = 0; i <= 20; i++){
        int num = (rand()%20);
        cout << num << "\n";
    }
    
    return 0;
}