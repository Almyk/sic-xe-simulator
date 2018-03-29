#include "20162004.h" // my header

// global variables
unsigned char MEMORY[MEGA] = "";
struct opcodeNode* HASHTABLE[HASHSIZE];

int main(void){
    char command[MAXBUF] = ""; // input buffer
    char **args; // will point to input arguments
    int status = 0; // keeps track of correct/incorrect arguments from input

    if(!(status = opInsert())) printf("failed to read opcode.txt");

    // main loop
    do{
        printf("sicsim> ");
        getInput(command);
        args = parseInput(command, &status);
        if(args) status = runCommand(args, status);
        // only adds to history if the command is correct
        if(status == 1){
            addHistory(command);
        }
        if(args) freeArguments(args);
    }while(status);
    freeHistory();
    freeHashtable();

    return 1;
}

int runCommand(char **args, int n){
    int status = -2;
    // n is amount of arguments in args
    if(n > 0){
        if(!(strcmp(args[0], "dump")) || !(strcmp(args[0], "du"))){
            status = memDump(args, n);
        }
        else if(!(strcmp(args[0], "e")) || !(strcmp(args[0], "edit"))){
            status = memEdit(args, n);
        }
        else if(!(strcmp(args[0], "f")) || !(strcmp(args[0], "fill"))){
            status = memFill(args, n);
        }
        else if(n == 2 && !(strcmp(args[0], "opcode"))){
            status = opMnem(args);
        }
        else if(n == 2 && !(strcmp(args[0], "type"))){
            status = funcType(args);
        }
        // commands that should only take 1 argument
        else if(args[1] == NULL){
            if(!(strcmp(args[0], "help")) || !(strcmp(args[0], "h"))) 
                status = funcHelp();
            else if(!(strcmp(args[0], "dir")) || !(strcmp(args[0], "d"))) 
                status = funcDir();
            else if(!(strcmp(args[0], "quit")) || !(strcmp(args[0], "q"))) 
                status = funcQuit();
            else if(!(strcmp(args[0], "history")) || !(strcmp(args[0], "hi"))){
                status = funcHistory(args[0]);
            }
            else if(!(strcmp(args[0], "opcodelist"))) status = opPrintOpcodelist();
            else if(!(strcmp(args[0], "reset"))) status = memReset();
            else status = -1;
        }
        // if no match
        else status = -1;
    }

    if(n < 0){
        printError(n);
        status = n;
    }
    else if(status < 0) printError(status);
    return status;
}
