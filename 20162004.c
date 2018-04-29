#include "20162004.h" // my header

// global variables
unsigned char MEMORY[MEGA] = "";
struct opcodeNode* HASHTABLE[HASHSIZE]; // Operation Code Table
struct symbolNode* SYMTAB[SYMHASHSIZE]; // Symbol Table
int SYMTABCOUNT = 0; // keeps count of entries added to SYMTAB

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
    initializeASM(0);
    resetESTAB();

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
            status = funcType(args[1]);
        }
        else if(n == 2 && !(strcmp(args[0], "assemble"))){
            status = asmAssemble(args[1]);
        }
        else if((n == 2 || n == 1) && !(strcmp(args[0], "progaddr"))){
            if(n == 2) status = llSetProgaddr(args[1]);
            if(n == 1) status = llSetProgaddr("0");
        }
        else if(n >= 2 && !(strcmp(args[0], "loader"))){
            status = llLoadProgram(args, n);
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
            else if(!(strcmp(args[0], "symbol"))) status = asmPrintSymTab();
            else if(!(strcmp(args[0], "run"))) status = llRun();
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
