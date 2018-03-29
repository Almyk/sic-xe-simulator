#include "20162004.h"

int asmAssemble(char* filename){
    int status = 0;

    status = findFile(filename);
    // file found
    if(status){
        status = cmpExtension(filename, ".asm");
        if(!status){
        }
        // incorrect extension
        else status = -8;
    }
    // not found
    else status = -7;

    return status;
}
