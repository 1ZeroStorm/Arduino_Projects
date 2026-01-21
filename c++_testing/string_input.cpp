#include <iostream>
#include <cmath>


int main(){
    
    std::string name;
    std::cout << "enter name: ";
    std::getline(std::cin, name);

    // len function of cpp
    if (name.length() > 10){
        std::cout << "cant exceed 12 chars"; 
    }
    else {
        std::cout << "accepted!";
    }
    std::cout << "\n";
    // check if string is empty
    if (name.empty()){
        std::cout << "answer the question!"; 
    }
    else{
        std::cout << "question answered";
    }

    // name.clear() -- clearning a string to be ("")
    // name.append("-hello34") -- adding chars to string
    // name.at(0) -- returns the 0th character
    // name.insert(0, "@") -- replaces the 0th character with "@" and push other charactersto the right
    // name.find(' ') -- finding the character, returns the index
    // name.erase(0, 3) -- erase the 0th to 3rd letters 
    return 0;
}