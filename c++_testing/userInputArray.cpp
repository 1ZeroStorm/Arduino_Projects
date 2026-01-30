#include <iostream>

int main(){

    const int SIZE = 10;
    std::string foods[SIZE];
    int current_index = 0;
    std::string temp;
    
    while (current_index < SIZE){
        std::cout << "enter fav food or 'q' to quit: ";
        std::getline(std::cin, temp);
        if (temp == "q"){
            break;
        }
        foods[current_index] = temp;
        current_index++;
    }

    std::cout << "the food you like:\n";
    for (int i = 0; !foods[i].empty() && i < SIZE; i ++ ){
        std::cout << foods[i] << std::endl;
    }

    return 0;
}