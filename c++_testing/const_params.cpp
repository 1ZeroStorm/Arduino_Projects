#include <iostream>

void printing(const std::string &name, const int &age);

int main(){
    std::string name = "bro";
    int age = 21;


    printing(name, age);

    age = 44;
    name = "nnnnn";

    std::cout << "\n";
    std::cout << name << std::endl;
    std::cout << age << std::endl;

    return 0;
}

void printing(const std::string &name, const int &age){
    std::cout << name << std::endl;
    std::cout << age << std::endl;
}