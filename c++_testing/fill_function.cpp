#include <iostream>

int main(){
    const int SIZE = 99;
    std::string foods[SIZE];

    fill(foods, foods + (SIZE/3), "pizza");
    fill(foods + (SIZE/3), foods + (2*SIZE/3), "hamburger");
    fill(foods + (2*SIZE/3), foods + SIZE, "hot dogs");

    for (std::string food : foods){
        std::cout << food << std::endl;
    }

    return 0;
}