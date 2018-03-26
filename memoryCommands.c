#include "20162004.h"

/* This file holds all the function for the different memory commands:
 * memDump
 * memEdit
 * memFill
 * memReset
 */

int memDump(char** args, int n){
    static int memPos = 0;
    int startPos, endPos;
    int i, count;
    char buffer[17] = ""; // holds the string to be printed at the end of line
    int begIn = 0; // this variable keeps track on how many indentations we need

    // one argument given, i.e du or dump
    if(n == 1){
        startPos = memPos;
        begIn = (startPos % 16); // this calculates how many indentations are needed at the beginning of the first line
        endPos = startPos + 16 * 10;
    }
    // if start address is given
    else if(n == 2){
        startPos = hexToInt(args[1]);
        if(startPos < 0) return startPos; // if startPos < 0 it means there was an error
        begIn = (startPos % 16);
        endPos = startPos + 16 * 10;
    }
    // start and end address given
    else if(n == 3){
        startPos = hexToInt(args[1]);
        if(startPos < 0) return startPos; // if startPos < 0 it means there was an error
        begIn = (startPos % 16);
        endPos = hexToInt(args[2]);
        if(endPos < 0) return endPos; // same as with startPos
        if(endPos < startPos) return -3;
        endPos++;
    }
    // too many arguments
    else return -1;

    // loop for printing the memory dump
    for(i = startPos; i < endPos && i < MEGA;){
        printf("%05X ", i - (i % 16));
        for(count = 0; count < 16 && i < MEGA; count++){
            if(begIn){ // begIn = beginning indentation
                // prints 3 spaces and adds '.' to buffer
                printf("   ");
                buffer[count] = '.';
                begIn--;
            }
            else{
                printf("%02X ", MEMORY[i]);
                // if 20 <= x <= 7E it is added to buffer
                if(MEMORY[i] >= (char)0x20 && MEMORY[i] <= (char)0x7E){
                    buffer[count] = MEMORY[i];
                }
                else{
                    buffer[count] = '.';
                }
                i++;
                if(i == endPos){
                    while(++count < 16){
                        // prints 3 spaces and adds '.' to buffer
                        printf("   ");
                        buffer[count] = '.';
                    }
                }

            }
        }
        // adds terminating null to buffer and then prints buffer as a string
        buffer[count] = '\0';
        printf("; %s\n", buffer);
    }
    memPos = i;
    if(!(memPos < MEGA)) memPos = 0; // if end of memory is reached, reset position
    return 1;
}

int memEdit(char** args, int n){
    int address; 
    int value;
    
    // invalid amount of arguments
    if(n != 3) return -1;

    // convert the given values and check their validity
    if((address = hexToInt(args[1])) < 0){
        return address;
    }
    if((value = hexToInt(args[2])) < 0){
        return value;
    }
    if(value > 0xFF) return -6;
    
    // store in memory
    MEMORY[address] = (char)value;
    
    return 1;
}

int memFill(char** args, int n){
    int startPos, endPos;
    int value;

    // invalid amount of arguments
    if(n != 4) return -1;

    // convert the given values and check their validity
    if((startPos = hexToInt(args[1])) < 0) return startPos;
    if((endPos = hexToInt(args[2])) < 0) return endPos;
    if((value = hexToInt(args[3])) < 0) return value;
    if(value > 0xFF) return -6;
    if(endPos < startPos) return -3;

    // use memset to fill the memory in the selected range
    memset(MEMORY+startPos, (char)value, endPos-startPos+1);
    return 1;
}

int memReset(void){
    // memset resets all of memory to 0
    memset(MEMORY, '\0', MEGA);
    return 1;
}
