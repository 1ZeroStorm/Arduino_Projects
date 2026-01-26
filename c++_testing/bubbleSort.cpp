#include <iostream>
using namespace std;

int main(){
    
    
    int unsorted[] = {10,1,9,2,8,3,7,4,6,5}; // {1,10,9,2,}
    int size = sizeof(unsorted) / sizeof(int);
    int temp;
    int current;
    int newnumber;
    int saveafter;
    
    // look for unsorted
    for (int j = 0;  j < size-1; j++){
        cout << "comparing " << unsorted[j] << " with "<<unsorted[j+1] << endl;
        if (unsorted[j] < unsorted[j+1]){
            current = unsorted[j];
            saveafter = unsorted[j+1];
            break;
        }
    }

    for (int i = 0; i < )



    

    return 0;
}