#include <iostream>
#include <cmath>
using namespace std;


double Area(double length);

int main(){

    
    cout << Area(14);
    return 0;
}

double Area(double len){
    return len * len;
}