#include <iostream>
using namespace std;

int main(){
    
    
    int unsorted[] = {10,1,9,2,8,3,7,4,6,5}; 
    int size = sizeof(unsorted) / sizeof(int);
    int temp;
    int current; // 10
    int currentIndex; // 0
    int newnumber;
    int saveafter;
    
    
    // look for unsorted
    for (int j = 0;  j < size-1; j++){
        cout << "comparing " << unsorted[j] << " with "<<unsorted[j+1] << endl;
        if (unsorted[j] > unsorted[j+1]){
            current = unsorted[j];
            currentIndex = j;
            break;
        }
    }
    unsorted[currentIndex] = unsorted[currentIndex+1];  // {1,1,9,2,..}
    unsorted[currentIndex+1] = current; // {1,10,9,2,..}
    currentIndex = currentIndex+1; // 0 -> 1

    if (current > unsorted[currentIndex + 1]){
        unsorted[currentIndex] = unsorted[currentIndex+1];  // {1,9,9,2,..}
        unsorted[currentIndex+1] = current; // {1,9,10,2,..}
        currentIndex = currentIndex+1; // 1 -> 2
    } 

    return 0;
}