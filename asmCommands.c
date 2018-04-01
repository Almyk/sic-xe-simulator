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
    if(!((*imCount) < imIndex)){
        /* reallocates memory to extend the imRecord */
        imIndex *= 2;
        intermediateRecord = realloc(intermediateRecord, sizeof(struct intermediateRecordNode) * imIndex);
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
    struct opcodeNode* opcodePtr;

    // reads first line from file
    readline(buffer, fp);
    labelLen = getToken(buffer, label, &index);
    opcodeLen = getToken(buffer, opcode, &index);
    operandLen = getToken(buffer, operand, &index);
    printf("%s\t%s\t%s\n", label, opcode, operand);

    if(!(strcmp(opcode, "START"))){
        /* if START, initialize STARTADR and LOCCTR and add to record */
        STARTADR = stringToInt(operand);
        LOCCTR = STARTADR;
        status = asmAddIMRecord(LOCCTR, &imCount, label, opcode, operand);
        if(status != 1) return status;
        if(labelLen){
            /* if label exists we add it to the symbol table */
            status = asmSymTabInsert(label, LOCCTR, imCount);
            if(status != 1) return status;
        }
        /* read new line */
        readline(buffer, fp);
        labelLen = getToken(buffer, label, &index);
        opcodeLen = getToken(buffer, opcode, &index);
        operandLen = getToken(buffer, operand, &index);
    }
    else{
        LOCCTR = 0;
    }
    /* First Pass's Main loop */
    while(strcmp(opcode, "END")){
        /* if not a comment */
        if(strcmp(label, ".")){
            if(labelLen){
                /* if label exists we add it to the symbol table */
                status = asmSymTabInsert(label, LOCCTR, imCount);
                if(status != 1) return status; /* symbol already exists */
            }
            /* search HASHTABLE for opcode */
            opcodePtr = opSearch(opcode, hashcode(opcode, HASHSIZE));
            if(opcodePtr){
            }
            else if(!(strcmp(opcode, "WORD"))){
                LOCCTR += 3;
            }
            else if(!(strcmp(opcode, "RESW"))){
                LOCCTR += 3 * stringToInt(operand);
            }
            else if(!(strcmp(opcode, "RESB"))){
                LOCCTR += stringToInt(operand);
            }
            else if(!(strcmp(opcode, "BYTE"))){
                /* "find length of constant in bytes" */
                /* seems like I will need to create a function
                   that looks for the structure: C'BLABLA'
                   and throws an error if it doesn't follow this structure.
                */
            }
            else{
                printf("Error on line %d: opcode \"%s\" is not a valid opcode.\n", imCount, opcode);
                return -2;
            }
        }
        /* write line to IM Record */
        /* read new line */
    }
    /* write last line to IM Record */
    /* save (LOCCTR - STARTADR) as program length */

    fclose(fp);
    return status;
}

int asmSecondPass(char* filename){
    return 1;
}
