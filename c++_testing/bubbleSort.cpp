#include <iostream>
using namespace std;

void resort(int unsorted[], int currentIndex, int current){
    unsorted[currentIndex] = unsorted[currentIndex+1];  // {1,1,9,2,..}
    unsorted[currentIndex+1] = current; // {1,10,9,2,..}
    currentIndex = currentIndex+1; // 0 -> 1

    if (current > unsorted[currentIndex + 1]){
        
    }
    else{
        
    }
}

int main(){
    
    
    int unsorted[] = {10,1,9,2,8,3,7,4,6,5}; 
    int size = sizeof(unsorted) / sizeof(int);
    int temp;
    int current; // 10
    int currentIndex; // 0
    int newnumber;
    int saveafter;
    bool completed = false;
    

    while (completed == false)
    
    {
        // look for unsorted
        for (int j = 0;  j < size-1; j++){
            cout << "comparing " << unsorted[j] << " with "<<unsorted[j+1] << " index: "<< j << endl;
            if (unsorted[j] > unsorted[j+1]){
                current = unsorted[j]; // unsorted[0] = 10 = current
                currentIndex = j; // 0
                break;
            }
            if (j == size-2){
                // sorting completed
                completed = true;
            }
        }

        while (current > unsorted[currentIndex + 1]){
            unsorted[currentIndex] = unsorted[currentIndex+1];  // {1,1,9,2,..}
            unsorted[currentIndex+1] = current; // {1,10,9,2,..}
            currentIndex = currentIndex+1; // 0 -> 1
        }
    }
    
    
    cout << "\nResult: ";
    for (int i = 0; i < size; i++){
        cout << unsorted[i] << ", ";
    }

    return 0;
}