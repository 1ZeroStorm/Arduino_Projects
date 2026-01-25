#include <iostream>
using namespace std;

// a function in which an array parameter is passed
void type_selection(char charas[]){
    for (int i=0; i< sizeof(charas) / sizeof(char); i++){
        switch (charas[i])
        {
        case 'A':
            cout << "happy birthday";
            break;
        case 'B':
            cout << "Merry Chirstmas";
            break;
        case 'C':
            cout << "kys (keep yourself save)";
            break;
        default:
            break;
        }
        cout << "\n";
    }
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

    cout << "\n";
    //passing to a function
    type_selection(types);

    return 0;
}