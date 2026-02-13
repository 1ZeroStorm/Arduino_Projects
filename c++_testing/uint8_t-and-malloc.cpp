#include <iostream>
#include <stdint.h>


int main(){
    uint8_t imageBuffer[] = {255, 0, 0,   0, 255, 0,   0, 0, 255};
    std::cout << "Image buffer size: " << sizeof(imageBuffer) << std::endl;

    return 0;
}
