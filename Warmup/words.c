#include "common.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    char* word;
    int i;
    
    for(i = 1; i < argc; i++){
        word = strtok(argv[i], " ");
        
        while(word != NULL){
            printf("%s\n", word);
            
            word = strtok(NULL, " ");
        }
    }
    
    return 0;
}
