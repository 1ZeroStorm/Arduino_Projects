#include <iostream>
using namespace std;

void type_selection(char charas[]){
    for
}

int main(){
    
    string cars[] = {"lambo", "ferrari", "porche"};
    char types[] = {'A', 'B', 'C'};

    // using for loop 
    for (int i = 0; i < sizeof(cars) / sizeof(string); i++){
        cout << cars[i] << "\n";
    }
    cout << "\n";

    // alternative (foreach loop, just like for i in list)
    for (char type : types){
        cout << type << "\n";
    }

    //passing to a function
    type_selection(types);

    return 0;
}