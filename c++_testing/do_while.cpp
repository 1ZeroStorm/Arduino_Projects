#include <iostream>

int main(){
    
    // do a code once (without condition) and repeating it if passes 'while' rule
    
    std::string pass = "hello-world";
    std::string ip;
    do{
        std::cout << "enter correct pass: ";
        std::getline(std::cin, ip);
    } while(ip != pass);

    std::cout << "access granted!";

    return 0;
}