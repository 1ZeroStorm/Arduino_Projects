#include <iostream>
#include <cmath>


std::string getSpaces(int number){
    int digits;
    std::string spaces = "     ";
    
    if (number == 0){
        digits = 1;
    }
    else
    {
        digits = (int)std::log10(abs(number)) + 1;
    }

    for (int j = 0; j < digits; j++){
        spaces.replace(1, 1, "");
    }
    
    return spaces;
}

int main(){
    
    
    const int row_size = 10;
    const int col_size = 10;

    int my_array[row_size][col_size];

    for (int row_i = 0; row_i < row_size; row_i++){    
        for (int col_i = 0; col_i < col_size; col_i++){
            my_array[row_i][col_i] = row_i * col_i;
        }  
    }
    
    for (int row_i = 0; row_i < row_size; row_i++){    
        for (int col_i = 0; col_i < col_size; col_i++){
            
            std::string spaces = getSpaces(my_array[row_i][col_i]);
            std::cout << my_array[row_i][col_i] << spaces;
        }
        std::cout << "\n";
    }
    
    return 0;
}