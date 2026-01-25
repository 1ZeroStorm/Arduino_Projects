#include <iostream>
using namespace std;

int main(){
    
    
    int unsorted[] = {10,1,9,2,8,3,7,4,6,5};
    int size = sizeof(unsorted) / sizeof(int);
    int temp;
    int current = unsorted[0];
    int newnumber;

    


    for (int j = 0;  j < size; j++){
        if (unsorted[j] > unsorted[j+1]){
            current = unsorted[j];
            unsorted[j] = -1;
            newnumber = unsorted[j+1];
            unsorted[j] = newnumber;
            unsorted[j+1] = current;
        }
        
    }

    return 0;
}