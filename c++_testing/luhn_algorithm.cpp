#include <iostream>

void everysecondDigits(const long long &number, std::string &steptwoarr, std::string &stepthreearr);


int main(){
    long long number;
    std::string steptwostring;
    std::string stepthreearr;

    std::cout << "enter your number: "; 
    std::cin >> number;

    std::cout << "your number: " <<number << std::endl;

    everysecondDigits(number, steptwostring, stepthreearr);

    std::cout << steptwostring << "\n";
    std::cout << stepthreearr;
    

    //std::string name = "hello world";
    //std::cout << "\n" << name.size();

    return 0;
}

void everysecondDigits(const long long &number, std::string &steptwoarr, std::string &stepthreearr){
    std::string s = std::to_string(number);
    int size = s.size();

    for (int i = 0; i < size-1; i++){
        if (i % 2 != 0){
            steptwoarr.push_back(s[i]);
            std::string double_number = std::to_string(s[i]*2);
            int double_number_size = double_number.size();

            for (int j = 0; j < double_number_size - 1; j++){
                stepthreearr.push_back(double_number[j]); // does not output as expected
            }
        } 
    }



}