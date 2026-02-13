#include <iostream>

int main() {
    std::string name = "Nicho";

    auto lamb = [name](std::string last_name){
        return name + " " + last_name;
    };

    std::cout << lamb("Smith") << std::endl; // Output: "Nicho Smith"

    return 0;
}