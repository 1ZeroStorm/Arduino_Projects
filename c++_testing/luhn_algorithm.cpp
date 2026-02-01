#include <iostream>
#include <typeinfo> 
#include <string>

int first_algo_retSum(const long long &number, std::string &steptwoarr, std::string &stepthreearr);
int second_algo_retSum(const long long &number, std::string &stepfourarr);

int main(){
    long long number = 371449635398431;
    std::string steptwostring;
    std::string stepthreearr;
    std::string stepfourarr;

    //std::cout << "enter your number: "; 
    //std::cin >> number;

    std::cout << "your number: " <<number << std::endl;

    int sum = first_algo_retSum(number, steptwostring, stepthreearr);

    std::cout << steptwostring << "\n";
    std::cout << stepthreearr << "\n";
    std::cout << sum << "\n";

    int sum_2 = second_algo_retSum(number);
    std::cout << stepthreearr << "\n";

    //std::string name = "hello world";
    //std::cout << "\n" << name.size();

    return 0;
}

int first_algo_retSum(const long long &number, std::string &steptwoarr, std::string &stepthreearr){
    std::string s = std::to_string(number);
    int size = s.size();

    for (int i = 0; i < size-1; i++){
        if (i % 2 != 0){
            steptwoarr.push_back(s[i]);
        } 
    }
    int steptwoarr_size = steptwoarr.size();
    for (int j = 0; j < steptwoarr_size-1; j++){
        int number = steptwoarr[j] - '0';
        int db = number * 2;
        stepthreearr += std::to_string(db);
    }

    int sum = 0;
    int stepthreearr_size = stepthreearr.size();
    for (int k = 0; k < stepthreearr_size; k++){
        int number = stepthreearr[k] - '0';
        sum += number;
    }
    return sum;

}

int second_algo_retSum(const long long &number, std::string &stepfourarr){
    std::string string_num = std::to_string(number);
    int size = string_num.size();

    int sum = 0;
    for (int j = 0; j < size; j++){
        if (j % 2 == 0){
            stepfourarr.push_back(string_num[j]);
        }
    }
    return sum;

}