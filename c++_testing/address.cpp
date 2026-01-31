#include <iostream>

void switch_string(std::string &X, std::string &Y);

int main(){
    std::string X = "SSSSSSSSSSSSSSSSSSSSSSS";
    std::string Y = "halo";
    
    std::cout << "before" << std::endl;
    std::cout << X << std::endl;
    std::cout << Y << std::endl;

    switch_string(X, Y);

    std::cout << "after" << std::endl;
    std::cout << X << std::endl;
    std::cout << Y << std::endl ;

    return 0;
}

void switch_string(std::string &X, std::string &Y){
    std::string temp = X;
    X = Y;
    Y = temp; 
}