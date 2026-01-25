#include <iostream>
using namespace std;

int findingElements(char ele[], char lookingfor);

int main(){
    
    char elements[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I'};
    char lookingFor;
    int resultIndex;
    
    cout << "type an element to be looked for: ";
    cin >> lookingFor;

    resultIndex = findingElements(elements, lookingFor);
    if (resultIndex != -1){
        cout << "alphabet: "<< lookingFor << " is at index " << resultIndex;
    }else{
        cout << "not found within the array";

    }
    return 0;
}

int findingElements(char ele[], char lookingfor){

    for (int i = 0; i < sizeof(ele) / sizeof(char); i++){
        if (ele[i] == lookingfor) {
            return i;
        }
    }
    return -1;
    //loopingArr(indexlist);
}