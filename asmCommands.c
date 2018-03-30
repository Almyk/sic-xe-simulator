#include "20162004.h"

// global variable
struct intermediateRecordNode** intermediateRecord;
int imIndex = 0; // keeps track of how many entries are allocated

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
    /* mode 1: initialize the record
       mode 0: delete the record (free the memory) */
    /* if there exist any records when initializing
       the memory for assembler they are first deleted! */
    int i = 0;
    struct symbolNode* symPtr;
    struct symbolNode* symDelete;
    if(imIndex){
        while(i < imIndex && intermediateRecord[i]){
            /* printf("inter: %s\n", intermediateRecord[i]->opcode); */
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
                    SYMTAB[i] = NULL;
                }
            }
        }
        printf("\n");
    }
    if(mode){
        /* allocate memory for ten intermediate records */
        intermediateRecord = (struct intermediateRecordNode**)\
            calloc(10, sizeof(struct intermediateRecordNode*)); /*line too long*/
        imIndex = 10;
    }
}

int asmSymTabInsert(char* label, int loc, int lineNum){
    struct symbolNode* newSymbol;
    struct symbolNode* symbolPtr;
    int hash;

    hash = hashcode(label, SYMHASHSIZE);
    if(SYMTAB[hash]){
        /* check for matching label
           if found, raise flag and error */
        symbolPtr = SYMTAB[hash];
        while(symbolPtr){
            if(!(strcmp(symbolPtr->label, label))){
                /* raise flag, print and return */
                /* print current line and what the error is */
                printf("Error: On line %d a duplicate of %s was found.\n", lineNum, label);
                printf("\tAborting the assemble.\n");
                return -2;
            }
            symbolPtr = symbolPtr->next;
        }
        /* if it reached here, we didn't find label
           i.e. add label to the end of list*/
        newSymbol = (struct symbolNode*) malloc(sizeof(struct symbolNode));
        newSymbol->last = SYMTAB[hash]->last;
        newSymbol->next = NULL;
        SYMTAB[hash]->last = newSymbol;
    }
    else{
        /* empty entry in Symbol Table
           allocate memory and store new Symbol*/
        newSymbol = (struct symbolNode*) malloc(sizeof(struct symbolNode));
        SYMTAB[hash] = newSymbol;
        newSymbol->next = NULL;
        newSymbol->last = NULL;
    }
    strcpy(newSymbol->label, label);
    newSymbol->loc = loc;
    return 1;
}

int asmAddIMRecord(int LOCCTR, int* imCount, char* label, char* opcode, char* operand){
    struct intermediateRecordNode* newRecord;

    /* need to add a check to see if imCount > imIndex */
    if(!(imCount < imIndex)){
        intermediateRecord = realloc();
    }

    newRecord = malloc(sizeof(struct intermediateRecordNode));
    if(!newRecord){
        printf("Error when allocating memory for a new IM Record.\n");
        return -2;
    }
    intermediateRecord[*imCount] = newRecord;
    (*imCount)++;
    strcpy(newRecord->label, label);
    strcpy(newRecord->opcode, opcode);
    strcpy(newRecord->operand, operand);
    newRecord->linenumber = *imCount;
    newRecord->loc = LOCCTR;

    return 1;
}

int asmFirstPass(char* filename){
    char buffer[MAXBUF] = "";
    char label[20], opcode[20], operand[20];
    int labelLen, opcodeLen, operandLen;
    int i = 0, index = 0;
    int imCount = 0; // this is basically the line counter
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
        /* maybe make this into its own function? */
        /* created it as below function. */
        /* still needs polishing */
        status = asmAddIMRecord(LOCCTR, &imCount, label, opcode, operand);
        if(labelLen){
            /* if label exists we add it to the symbol table */
            status = asmSymTabInsert(label, LOCCTR, imCount);
            if(status != 1){
                return status;
            }
        }
    }

    fclose(fp);
    return status;
}

int asmSecondPass(char* filename){
    return 1;
}
