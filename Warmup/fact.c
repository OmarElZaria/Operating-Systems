#include "common.h"
#include <stdio.h>
#include <stdbool.h>

int factorial (int n);
bool is_integer(double d);
bool is_positive(double d);

int main(int argc, char **argv)
{
    double d = atof(argv[1]);
    
    if(!(!is_integer(d) || !is_positive(d))){
        printf("%d\n", factorial((int)d));
    }
}

int factorial (int n){
    if(n >= 1)
        return n*factorial(n-1);
    else
        return 1;
}

bool is_integer(double d){
    if(d == (int)d)
        return true;
    else
        printf("Huh?\n");
        
    return false;
}

bool is_positive(double d){
    if(d <= 0){
        printf("Huh?\n");
        return false;
    }else if(d > 12){
        printf("Overflow\n");
        return false;
    }
    
    return true;
}
