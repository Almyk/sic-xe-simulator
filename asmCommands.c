#include "20162004.h"

// global variable
struct intermediateRecordNode** intermediateRecord;
int imIndex = 0;

int asmAssemble(char* filename){
    int status = 0;

    status = findFile(filename);
    // file found
    if(status){
        status = cmpExtension(filename, ".asm");
        if(!status){
            initializeASM(1);
            status = asmFirstPass(filename);
        }
        // incorrect extension
        else status = -8;
    }
    // file not found
    else status = -7;

    return status;
}

void initializeASM(int mode){
    int i = 0;
    struct symbolNode* symPtr;
    struct symbolNode* symDelete;
    if(imIndex){
        while(i < imIndex && intermediateRecord[i]){
            free(intermediateRecord[i]);
            i++;
        }
        free(intermediateRecord);
        imIndex = 0;
        for(i = 0; i < SYMHASHSIZE; i++){
            if(SYMTAB[i]){
                symPtr = SYMTAB[i];
                while(symPtr){
                    symDelete = symPtr;
                    symPtr = symPtr->next;
                    free(symDelete);
                }
            }
        }
    }
    if(mode){
        /* allocate memory for five intermediate records */
        intermediateRecord = (struct intermediateRecordNode**)\
            calloc(5, sizeof(struct intermediateRecord*)); /*line too long*/
        imIndex = 5;
    }
}

int asmFirstPass(char* filename){
    char buffer[MAXBUF] = "";
    char label[20], opcode[20], operand[20];
    int labelLen, opcodeLen, operandLen;
    int i = 0, index = 0;
    int count = 0;
    FILE* fp = fopen(filename, "r");
    int status = 1;
    int LOCCTR = 0;
    int STARTADR = 0;

    // reads first line from file
    readline(buffer, fp);
    labelLen = getToken(buffer, label, &index);
    skipSpaces(buffer, &index);
    opcodeLen = getToken(buffer, opcode, &index);
    skipSpaces(buffer, &index);
    operandLen = getToken(buffer, operand, &index);
    printf("%s\t%s\t%s\n", label, opcode, operand);

    if(!(strcmp(opcode, "START"))){
        STARTADR = stringToInt(operand);
        LOCCTR = STARTADR;
    }

    fclose(fp);
    return status;
}

int asmSecondPass(char* filename){
    return 1;
}
