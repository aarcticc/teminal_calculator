// stdio.h is called in defs.h
#include <stdbool.h>
#include "defs.h"

int main(){
    int beginning;
    double number[NUMBER_LENGTH][NUMBER_AMOUNT]; 
    char* input;

    // calc logic
    printf("What do you want to calculate: \n");
    while (input < NUMBER_AMOUNT || input < NUMBER_LENGTH){
        scanf(" %c", input);
    }

    return 0;
}
