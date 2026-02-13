#include <iostream>
using namespace std;

int main(){
    
    const int size = 5;
    int data[size] = {1,2,3,4,5};
    int reversedData[size];
    
    for (int i = 0 ; i< size ; i++){
        int revInd = 4 - i;
        reversedData[revInd] = *(data + i);
    }

    for (int j = 0; j < size ; j ++){
        cout << reversedData[j] << endl;
    }

    return 0;
}
