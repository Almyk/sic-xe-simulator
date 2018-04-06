#include "20162004.h"

/**** global variables ****/

/* variables regarding the intermediate record */
struct intermediateRecordNode** intermediateRecord;
int imIndex = 0; // how many entries there is memory allocated for
int imCount = 0; // how many entries there are in intermediateRecord

/* variables regarding program's memory addressing */
int STARTADR = 0; // start address of program
int ENDADR = 0; // end address of program
int PLENGTH = 0; // length of program

/*
  NOBASE is for BASE-Relative addressing
  if NOBASE == 1, we can't use base-relative addressing
  if NOBASE == 0, it is possible to use base-relative addressing
*/
int NOBASE = 1;


/* ---------------------------- */

int asmAssemble(char* filename){
    int status = 0;

    status = findFile(filename);
    // file found
    if(status){
        status = cmpExtension(filename, ".asm");
        if(!status){
            initializeASM(1);
            status = asmFirstPass(filename);
            if(status > 0){ // first pass was succesful
            }
        }
        // incorrect extension
        else{
            /* status = -8; */
            /* move error message so that command can be added to history correctly */
            status = 1;
            printf("Error: file is not a '.asm' file.\n");
        }
    }
    // file not found
    else{
        /* status = -7; */
        /* move error message so that command can be added to history correctly */
        status = 1;
        printf("Error: file not found.\n");
    }

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
    }
    if(mode){
        /* allocate memory for ten intermediate records */
        intermediateRecord = (struct intermediateRecordNode**)\
            calloc(10, sizeof(struct intermediateRecordNode*)); /*line too long*/
        /* initialize global variables related to program */
        imIndex = 10; STARTADR = 0; ENDADR = 0; imCount = 0; PLENGTH = 0;
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
                printf("Error: On line %d a duplicate of symbol %s was found.\n", lineNum, label);
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
} // end of asmSecondPass

int asmAddIMRecord(int LOCCTR, int* imCount, char* label, char* opcode, char* operand, char* buffer, char flag){
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
    strcpy(newRecord->buffer, buffer);
    newRecord->linenumber = *imCount;
    newRecord->loc = LOCCTR;
    newRecord->flag = flag;

    return 1;
}

int asmOperandLength(char* operand){
    /* this function computes how many bytes are needed
     * to be preserved for the label */
    int result = 0;
    int denominator = 0;
    int i = 2, j = 0;
    int len = strlen(operand);
    char buffer[TOKLEN] = "";

    if(len > 3 && len < TOKLEN){
        if(operand[1] == '\'' && operand[len-1] == '\''){
            if(operand[0] == 'X'){ // if hexadecimal
                denominator = 2;
                while(i < len-1) buffer[j++] = operand[i++];
                if(hexToInt(buffer) >= 0){ // means that it is valid hexadecimal
                    result = j; // length of copied buffed
                }
                else{ // something wrong with inputted hexadecimal
                    printf("Error: not a valid hexadecimal number.\n");
                    return -2;
                }
            }
            else if(operand[0] == 'C'){ // if character
                denominator = 1;
                result = len - 3;
            }
            else{ // error in syntax
                printf("Error: When using opcode BYTE, the operand must start with either X or C.\n");
                return -2;
            }
            // calculating required bytes
            if(result % denominator) result++;
            result = result / denominator;
        }
        else{ // error in syntax
            printf("Error: When using opcode BYTE, the string or hexadecimal must be encapsuled with quotes('').\n");
            return -2;
        }
    }
    else{ // invalid operand length
    }
    return result;
}

int asmIsSymbol(char* input){
    /*
      returns 0 if input is a symbol and there is no prefix
      returns 1 if input is a symbol with prefix
      list of prefixes: #, @
    */
    int len = strlen(input);
    int i;
    int result = 1;
    int isSymbol = 0;
    int prefix = 0;
    for(i = 0; i < len && result; i++){
        /*
          probably need to think about more cases
          of how a symbol can appear in the operand
          current approach is rather naive...
        */


        /* if input contains a number */
        if(input[i] >= '0' && input[i] <= '9'){
            result = 0;
        }
        /* if not a letter */
        else if(!(input[i] >= 'A' && input[i] <= 'Z')){
            /* if not the first index (prefixes can only be in first index) */
            if(i) result = 0;
            /* if first character is @, input has to be a symbol */
            else if(input[i] == '@') isSymbol = 1;
            else if(input[i] != '#') result = 0;
            if(result) prefix = 1; // raises prefix flag
        }
    }
    /* if input should have been a symbol but format is wrong */
    if(isSymbol && !result){
        /* go here */
        /* print error */
    }
    /* if input is a symbol */
    else if(result){
        /* check if it is a register */
        if(len == 1){
            switch(input[0]){
            case 'A':
            case 'X':
            case 'L':
            case 'B':
            case 'S':
            case 'T':
            case 'F': result = 0; break;
            }
        }
        /* if it is a register */
        if(!result) return -2;
    }
    if(prefix) return 1;
    else return 0;
}

int asmParseLine(char* buffer, char* label, char* opcode, char* operand){
    int labelLen, opcodeLen, operandLen;
    int index = 0;
    int result = 1;
    int isComment = 0;

    labelLen = getToken(buffer, label, 0, &index); // label field
    if(labelLen < 0){
        isComment = 1;
    }
    opcodeLen = getToken(buffer, opcode, 0, &index); // opcode field
    if(opcodeLen < 0){
        isComment = 1;
    }
    operandLen = getToken(buffer, operand, 0, &index); // operand field
    /* if not at end of line */
    if(!isComment && buffer[index] != '\0'){
        /* there is an index offset etc after first operand */
        if(buffer[index] == ','){
        }
        /* there is some character but not a comma */
        else{
            /* check if last character of operand was a comma */
            if(operand[operandLen-1] == ','){
                operand[operandLen] = ' ';
                operandLen = getToken(buffer, operand, operandLen+1, &index);
            }
            else{
                /* there is some error in the assembly code */
                result = 0;
            }
        }
    } // end of not at end of line
    return result;
}

int asmFirstPass(char* filename){
    /*
      this function performs the first pass through the source file.
      It stores the labels and their respective addresses.
      The function also keeps an intermediate record that will be used
      for the second pass through the file.
    */
    char buffer[MAXBUF] = "";
    char label[TOKLEN], opcode[TOKLEN], operand[TOKLEN];
    int i = 0; // used for opcode offset when there is a flag such as: + or @
    FILE* fp = fopen(filename, "r");
    int status = 1;
    int LOCCTR = 0;
    int loc = 0;
    char flag = '\0';
    struct opcodeNode* opcodePtr;

    // reads first line from file
    readline(buffer, fp);
    status = asmParseLine(buffer, label, opcode, operand);
    if(status == 0){
        printf("Error on line %d: There is something wrong with the assembly syntax.\n", imCount);
        printf("Buffer: %s\n", buffer);
        return -2;
    }
    else if(status < 0){ // line is a comment
        flag = 'c';
    }

    if(!(strcmp(opcode, "START"))){
        /* if START, initialize STARTADR and LOCCTR and add to record */
        STARTADR = stringToInt(operand);
        LOCCTR = STARTADR;
        status = asmAddIMRecord(LOCCTR, &imCount, label, opcode, operand, buffer, flag);
        if(status != 1) return status;
        if(strlen(label) > 0){
            /* if label exists we add it to the symbol table */
            status = asmSymTabInsert(label, LOCCTR, imCount);
            if(status != 1) return status;
        }
        /* read new line */
        readline(buffer, fp);
        status = asmParseLine(buffer, label, opcode, operand);
        if(status == 0){
            printf("Error on line %d: There is something wrong with the assembly syntax.\n", imCount);
            printf("Buffer: %s\n", buffer);
            return -2;
        }
        else if(status < 0){ // line is a comment
            flag = 'c';
        }
    }
    else{
        LOCCTR = 0;
        STARTADR = 0;
    }
    /* First Pass's Main loop */
    while(strcmp(opcode, "END")){
        i = 0;
        /* if not a comment */
        if(strcmp(label, ".") && flag != 'c'){
            if(strlen(label) > 0){
                /* if label exists we add it to the symbol table */
                status = asmSymTabInsert(label, LOCCTR, imCount);
                if(status != 1) return status; /* symbol already exists */
            }
            /* search HASHTABLE for opcode */
            if(opcode[0] == '+'){
                flag = '+';
                i = 1;
            }
            else if(opcode[0] == '@'){
                flag = '@';
                i = 1;
            }
            opcodePtr = opSearch(opcode+i, hashcode(opcode+i, HASHSIZE));
            if(opcodePtr){
                LOCCTR += (opcodePtr->format[0])-'0';
                if(flag == '+') LOCCTR += 1;
            }
            else if(!(strcmp(opcode+i, "WORD"))){
                LOCCTR += 3;
            }
            else if(!(strcmp(opcode+i, "RESW"))){
                LOCCTR += 3 * stringToInt(operand);
            }
            else if(!(strcmp(opcode+i, "RESB"))){
                LOCCTR += stringToInt(operand);
            }
            else if(!(strcmp(opcode+i, "BYTE"))){
                /* "find length of constant in bytes" */
                status = asmOperandLength(operand);
                if(status < 0){ // error occurred
                    printf("Error occured on line: %d\n", imCount);
                    return status;
                }
                LOCCTR += status;
            }
            else if(!(strcmp(opcode+i, "BASE"))){
                // this is used together with base relative opcodes
                /* go here */
            }
            else{
                printf("Error on line %d: opcode \"%s\" is not a valid opcode.\n", imCount, opcode);
                printf("buffer: %s\n", buffer);
                return -2;
            }
        }
        /* write line to IM Record */
        /* use loc instead of LOCCTR when writing to IM record */
        status = asmAddIMRecord(loc, &imCount, label, opcode, operand, buffer, flag);
        loc = LOCCTR;
        flag = '\0';

        /* read new line */
        readline(buffer, fp);
        status = asmParseLine(buffer, label, opcode, operand);
        if(status == 0){
            printf("Error on line %d: There is something wrong with the assembly syntax.\n", imCount);
            printf("Buffer: %s\n", buffer);
            return -2;
        }
        else if(status < 0){ // line is a comment
            flag = 'c';
        }
    }
    /* write last line to IM Record */
    status = asmAddIMRecord(LOCCTR, &imCount, label, opcode, operand, buffer, flag);
    /* store end address and program length in the global variables */
    ENDADR = LOCCTR;
    PLENGTH = LOCCTR - STARTADR;

    int k = 0;
    for(;k < imCount; k++) printf("LOCCTR %05X: %s\t%s\t%s\n", intermediateRecord[k]->loc, intermediateRecord[k]->label, intermediateRecord[k]->opcode, intermediateRecord[k]->operand);

    fclose(fp);
    return status;
} // end of asmFirstPass

int asmSecondPass(char* filename){
    char* opcode;
    char* operand;
    struct opcodeNode* opcodePtr;
    FILE *lstFPtr;
    FILE *objFPtr;
    char* lstFilename;
    char* objFilename;
    int len = strlen(filename);

    /*
     * Allocate memory for the filenames:
     * filename.lst ; filename.obj
     * and copy name from filename but change extension
    */
    lstFilename = calloc(len+1, sizeof(char));
    objFilename = calloc(len+1, sizeof(char));
    strcpy(lstFilename, filename);
    strcpy(objFilename, filename);
    strcat(lstFilename+len-3, "lst");
    strcat(objFilename+len-3, "obj");

    /* open list and object files */
    lstFPtr = fopen(lstFilename, "w");
    objFPtr = fopen(objFilename, "w");

    /* read first input line */
    if(!(strcmp(intermediateRecord[imCount]->opcode, "START"))){
        /* write listing line */
        /* read next line */
    }
    /* write Header record to object program */
    /* initialize first Text record */

    /* main loop of the Second Pass algorithm */
    while(strcmp(intermediateRecord[imCount]->opcode, "END")){
        /* if not a comment */
        if(intermediateRecord[imCount]->label[0] != '.'){
            /* search OPTAB for OPCODE */
            opcode = intermediateRecord[imCount]->opcode;
            opcodePtr = opSearch(opcode, hashcode(opcode, HASHSIZE));
            /* if the opcode is found */
            if(opcodePtr){
                operand = intermediateRecord[imCount]->operand;
                /* if there is a symbol in OPERAND field */
                if(asmIsSymbol(operand)){
                    /* go here */
                }
            }
        }
    }
    return 1;
}
