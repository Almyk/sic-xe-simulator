#include "20162004.h"

/**** global variables ****/

struct textRecordNode TRHEAD;

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

/* Registers */
unsigned int rB = 0;

/* ---------------------------- */

int asmAssemble(char* filename){
    int status = 0;

    status = findFile(filename);
    // file found
    if(status){
        status = cmpExtension(filename, ".asm"); // confirms extension
        if(!status){
            initializeASM(1); // initializes variables and data structures
            status = asmFirstPass(filename);
            if(status > 0){ // first pass was succesful
                status = asmSecondPass(filename);
                if(!status){ // an error occurred
                    return 1;
                }
            }
            else{ // first pass failed
                /* go here */
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
    if(imIndex){ // if there exist records in intermediate record
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
        SYMTABCOUNT = 0;
    }
    if(mode){
        /* allocate memory for ten intermediate records */
        intermediateRecord = (struct intermediateRecordNode**)\
            calloc(10, sizeof(struct intermediateRecordNode*)); /*line too long*/
        /* initialize global variables related to program */
        imIndex = 10; STARTADR = 0; ENDADR = 0; imCount = 0; PLENGTH = 0;

        /* insert registers to SYMTAB */
        asmSymTabInsert("A", 0, 0);
        asmSymTabInsert("X", 1, 0);
        asmSymTabInsert("L", 2, 0);
        asmSymTabInsert("B", 3, 0);
        asmSymTabInsert("S", 4, 0);
        asmSymTabInsert("T", 5, 0);
        asmSymTabInsert("F", 6, 0);
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
            if(symbolPtr->next) symbolPtr = symbolPtr->next;
            else break;
        }
        /* if it reached here, we didn't find label
           i.e. add label to the end of list*/
        newSymbol = (struct symbolNode*) malloc(sizeof(struct symbolNode));
        newSymbol->last = SYMTAB[hash]->last;
        newSymbol->next = NULL;
        SYMTAB[hash]->last = newSymbol;
        symbolPtr->next = newSymbol;
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
    SYMTABCOUNT++;
    return 1;
}

int asmAddIMRecord(int LOCCTR, int* imCount, char* label, char* opcode, char* operand, char* buffer, char flag){
    struct intermediateRecordNode* newRecord;

    /* if not enough indexes allocated to store new record */
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
    /* initializes the values in the intermediate record */
    intermediateRecord[*imCount] = newRecord;
    (*imCount)++;
    strcpy(newRecord->label, label);
    strcpy(newRecord->opcode, opcode);
    strcpy(newRecord->operand, operand);
    strcpy(newRecord->buffer, buffer);
    newRecord->linenumber = (*imCount)*5;
    newRecord->loc = LOCCTR;
    newRecord->n = 1;
    newRecord->i = 1;
    newRecord->x = 0;
    newRecord->b = 0;
    newRecord->p = 0;
    newRecord->e = 0;
    newRecord->flag = '\0';
    if(flag == '+') newRecord->e = 1;
    if(flag == 'c') newRecord->flag = 'c';

    return 1;
}

int asmOperandLength(char* operand){
    /* this function computes how many bytes are needed
     * to be reserved for the label */
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
                if(newHexToInt(buffer, j) >= 0){ // means that it is valid hexadecimal
                    result = j; // length of copied buffer
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

int asmCheckSymbol(char* input, int len){
    /*
      returns 1 if input is a symbol and there is no prefix
      returns 2 if input is a symbol with @ prefix
      returns 3 if input is a symbol with # prefix
      returns 0 if not a symbol
      returns a negative value in case of error
    */
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
            return 0;
        }
        /* if not a letter */
        else if(!(input[i] >= 'A' && input[i] <= 'Z')){
            /* if not the first index (prefixes can only be in first index) */
            if(i) result = 0;
            /* if first character is @, input has to be a symbol */
            else if(input[i] == '@'){
                isSymbol = 1;
                prefix = 1;
            }
            else if(input[i] == '#'){
                prefix = 2;
            }
            else result = 0;
        }
    }
    /* if input should have been a symbol but format is wrong */
    if(isSymbol && !result){
        /* print error */
        return -2;
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
        if(!result) return 1;
    }
    if(prefix == 1) return 2;
    if(prefix == 2) return 3;
    else return 1;
}

int asmIsSymbol(char* input){
    /*
      returns 1 if word1 is a symbol
      returns 2 if word2 is a symbol
      returns 3 if both words are symbols
      returns 0 if no symbols
      returns negative if error
    */
    int len = strlen(input);
    char tmpBuf[TOKLEN];
    char *word1, *word2;
    int w1Result, w2Result;
    int w1Len, w2Len;
    int result = 0;

    if(len > 0){
        strcpy(tmpBuf, input);
        word1 = strtok(tmpBuf, ", ");
        word2 = strtok(NULL, ", ");
        w1Len = strlen(word1);
        w2Len = strlen(word2);
    }
    else return 0; // empty string

    if(w1Len) w1Result = asmCheckSymbol(word1, w1Len);
    if(w2Len) w2Result = asmCheckSymbol(word2, w2Len);
    if(w1Result > 0) result = 1;
    if(w2Result > 0) result += 2;

    /* if there was an error */
    if(w1Result < 0 || w2Result < 0){
        result = -2;
    }
    return result;
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
    if(isComment) return -1;
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
            /* else if(opcode[0] == '@'){ */
            /*     flag = '@'; */
            /*     i = 1; */
            /* } */
            opcodePtr = opSearch(opcode+i, hashcode(opcode+i, HASHSIZE));
            if(opcodePtr){
                LOCCTR += (opcodePtr->format[0])-'0';
                if(flag == '+') LOCCTR += 1;
            }
            /* if not in object code list,
             compare the input with following assembler-directives*/
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
            else if(!(strcmp(opcode+i, "NOBASE"))){
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
        status = asmAddIMRecord(loc, &imCount, label, opcode+i, operand, buffer, flag);
        loc = LOCCTR;
        flag = '\0';

        /* read new line */
        status = readline(buffer, fp);
        if(!status) break;
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

    fclose(fp);
    return status;
} // end of asmFirstPass

int asmSecondPass(char* filename){
    int status = 1;
    int i;
    char* operand;
    char tmpOperand[TOKLEN];
    char *op1, *op2;
    int op1Len, op2Len;
    struct opcodeNode* opcodePtr;
    FILE *lstFPtr;
    FILE *objFPtr;
    char* lstFilename;
    char* objFilename;
    int len;
    int index = 0;
    int prefix = 0; // used as index offset in case of a @, # prefix
    struct intermediateRecordNode* imrPtr = NULL;
    struct textRecordNode* trPtr = NULL;
    struct textRecordNode* currentTR = NULL;
    struct textRecordNode* newTR = NULL;
    int trIndex = 0; // text record index
    unsigned int trStart = 0; // to keep track of where text record start
    unsigned int trLength = 0; // length of text record
    unsigned char bytes[4];
    unsigned int split;
    unsigned char temp;
    struct symbolNode* symbolPtr = NULL;
    unsigned int operAdr = 0;
    int notasymbol = 0;


    /* read first input line */
    if(!(strcmp(intermediateRecord[index]->opcode, "START"))){
        /* since listing file is created from the intermediate record
           after all of the code is parsed, we only need to advance index */
        imrPtr = intermediateRecord[index];
        /* read next line */
        index++;
    }
    /* store Header to later record to object file */
    sprintf(TRHEAD.record, "H%-6s%06X%06X\n", (imrPtr ? imrPtr->label : ""), STARTADR, PLENGTH);


    /* initialize first Text record */
    newTR = TRALLOC();
    newTR->next = NULL;
    TRHEAD.next = newTR;
    newTR->record[0] = 'T';
    currentTR = newTR;
    trIndex = 9; // initialize text record index
    trStart = STARTADR;
    sprintf(currentTR->record+1, "%06X", trStart);

    /* main loop of the Second Pass algorithm */
    while(strcmp(intermediateRecord[index]->opcode, "END") && index < imCount-1){
        /* go here */
        prefix = 0;
        imrPtr = intermediateRecord[index];
        /* if not a comment */
        if(imrPtr->flag != 'c'){
            /* search OPTAB for OPCODE */
            opcodePtr = opSearch(imrPtr->opcode, hashcode(imrPtr->opcode, HASHSIZE));
            /* if the opcode is found */
            if(opcodePtr){
                /* make a copy of the operand so we can use strtok() on the string */
                operand = imrPtr->operand;
                if((op1Len = strlen(operand)) > 0){ // not empty string
                    strcpy(tmpOperand, operand);
                    if(removeCommas(tmpOperand) < 0){
                        /* print error */
                        printf("Error on line %d: Too many commas in operand.\n", index+1);
                        return 0;
                    }
                    /* tokenize the operand */
                    op1 = strtok(tmpOperand, " ");
                    op1Len = strlen(op1);
                    op2 = strtok(NULL, " ");
                }
                /* if there is a symbol in OPERAND field */
                if((op1Len && (status = asmCheckSymbol(op1, op1Len))) > 0){ // verify symbol validity
                    if(status == 2){ // indirect addressing
                        /* imrPtr->flag = '@'; */
                        imrPtr->n = 1;
                        imrPtr->i = 0;
                        prefix = 1;
                    }
                    else if(status == 3){ // immediate addressing
                        /* imrPtr->flag = '#'; */
                        imrPtr->n = 0;
                        imrPtr->i = 1;
                        prefix = 1;
                    }
                    /* find symbol in symbol table */
                    symbolPtr = symSearch(op1+prefix, hashcode(op1+prefix, SYMHASHSIZE));
                    if(symbolPtr){
                        /* store symbol value as operand address */
                        operAdr = symbolPtr->loc;
                        if(opcodePtr->format[0] == '3' && op2){ // if there is an index offset(indexed addressing)
                            if((op2Len = strlen(op2)) == 1){
                                if(op2[0] == 'X'){
                                    imrPtr->x = 1;
                                }
                                else{
                                    printf("Error on line %d: incorrect register used for indexed addressing.\n", index+1);
                                    return 0;
                                }
                            }
                            else{
                                printf("Error on line %d: incorrect syntax for indexed addressing.\n", index+1);
                                return 0;
                            }
                        }
                        else if(opcodePtr->format[0] == '2'){ // register to register
                            if(!asmIsRegister(symbolPtr)){ // not a register
                                printf("Error on line %d: symbol is not a register.\n", index+1);
                                return 0;
                            }
                            operAdr <<= 4;
                            if(op2){
                                symbolPtr = symSearch(op2, hashcode(op2, SYMHASHSIZE));
                                if(symbolPtr && asmIsRegister(symbolPtr)){
                                    operAdr += symbolPtr->loc;
                                }
                                else{ // second symbol in operand is not a register
                                    printf("Error on line %d: second symbol in operand is not a register.\n", index+1);
                                    return 0;
                                }
                            }
                        }
                    }
                    else{
                        /* undefined symbol */
                        /* print error message */
                        printf("Error on line %d: undefined symbol used as operand.\n", index+1);
                        printf("Operand: %s\n", imrPtr->operand+prefix);
                        return 0;
                    }
                } // if symbol found
                else if(!status){ // a number in operand
                    if(imrPtr->operand[0] == '#'){ // immediate addressing
                        imrPtr->n = 0;
                        imrPtr->i = 1;
                        prefix = 1;
                    }
                    status = isNumber((imrPtr->operand)+prefix);
                    if(status >= 0){ // it is a number
                        operAdr = stringToInt((imrPtr->operand)+prefix);
                    }
                    else{ // not a number
                        /* print error message */
                        printf("Error on line %d: something wrong with the operand.\n", index+1);
                        printf("Operand: %s\n", imrPtr->operand+prefix);
                        return 0;
                    }
                }
                else if(status < 0){ // there was an error in syntax
                    /* print error message */
                    printf("Error on line %d: something wrong with the operand.\n", index+1);
                    printf("Operand: %s\n", imrPtr->operand+prefix);
                    return 0;
                }
                /* not a symbol */
                else{
                    /* store 0 as operand address */
                    operAdr = 0;
                    notasymbol = 1;
                }
                /* assemble the object code instruction */
                /* go here */
                status = asmCreateObjectCode(operAdr, imrPtr, opcodePtr, intermediateRecord[index+1]->loc, notasymbol);
                if(status <= 0){ // there was some error
                    return 1;
                }
                notasymbol = 0; // reset notasymbol
            } // end of if OPCODE found
            /* go here */
            /* need to add more exceptions for assembler-directives */
            else if(!strcmp(imrPtr->opcode, "BASE")){
                NOBASE = 0;
                symbolPtr = symSearch(imrPtr->operand, hashcode(imrPtr->operand, SYMHASHSIZE));
                if(!symbolPtr){
                    /* error finding symbol */
                    printf("Error on line %d: missing operand.\n", index+1);
                    return 0;
                }
                rB = symbolPtr->loc;
            }
            else if(!strcmp(imrPtr->opcode, "NOBASE")) NOBASE = 1;
            else if(!strcmp(imrPtr->opcode, "BYTE")){
                asmByteObjectCodeCreator(imrPtr);
            }
            /* store objectcode into text record */
            if(imrPtr->objectCode){
                /* go here */
                /* need to make this into its own function */
                trLength = intermediateRecord[index+1]->loc - imrPtr->loc;
                trLength *= 2; // for every byte there are 2 characters
                if(((trLength + trIndex) > TRMAXLEN) || (imrPtr->loc - trStart) > 0xff){ // initialize new text record row
                    /* write last information on current text record */
                    currentTR->record[trIndex] = '\0';
                    temp = currentTR->record[9];
                    sprintf(currentTR->record+7, "%02X", (trIndex-8)/2);
                    currentTR->record[9] = temp;

                    /* create new text record row */
                    newTR = TRALLOC();
                    newTR->record[0] = 'T';
                    currentTR->next = newTR;
                    currentTR = newTR;
                    trStart = imrPtr->loc;
                    /* write start address (loc) */
                    sprintf(currentTR->record+1, "%06X", trStart);
                    trIndex = 9;
                }
                /* find values of individual bytes by splitting them up */
                split = imrPtr->objectCode;
                bytes[0] = (split >> 24) & 0xff; // move x bytes to the right,
                bytes[1] = (split >> 16) & 0xff; // use logical AND to,
                bytes[2] = (split >> 8) & 0xff;  // only store the bits that,
                bytes[3] = split & 0xff;         // we are interested in.
                int ok = 0;
                /* write text record one byte at a time */
                for(i = 0; i < 4; i++){
                    if(bytes[i] || ok){
                        ok = 1;
                        sprintf((currentTR->record)+trIndex, "%02X", bytes[i]);
                        trIndex += 2;
                    }
                }

                /* sprintf((currentTR->record)+trIndex, "%0X", (unsigned int)imrPtr->objectCode); */
                /* trIndex += trLength; */
            }

            //printf("Objectcode: %02X\n", (unsigned int)imrPtr->objectCode);

        } // (if not a comment)
        index++;
    } // while not END
    /* finish last text record */
    currentTR->record[trIndex] = '\0';
    temp = currentTR->record[9];
    sprintf(currentTR->record+7, "%02X", (trIndex-8)/2);
    currentTR->record[9] = temp;
    /* add end record */
    newTR = TRALLOC();
    sprintf(newTR->record, "%c%06X",'E', STARTADR);
    currentTR->next = newTR;
    newTR->next = NULL;


    /***** WRITE TO FILE *****/
    /*
     * Allocate memory for the filenames:
     * filename.lst ; filename.obj
     * and copy name from filename but change extension
     */
    len = strlen(filename);
    lstFilename = calloc(len+1, sizeof(char));
    objFilename = calloc(len+1, sizeof(char));
    strcpy(lstFilename, filename);
    strcpy(objFilename, filename);
    strcpy(lstFilename+len-3, "lst");
    strcpy(objFilename+len-3, "obj");

    /* open list and object files */
    lstFPtr = fopen(lstFilename, "w");
    objFPtr = fopen(objFilename, "w");

    /*~~ write to listing file ~~*/
    for(i = 0; i < imCount; i++){
        imrPtr = intermediateRecord[i];
        if(i == imCount - 1){
            fprintf(lstFPtr, "%3d\t\t%-6s\t%-6s\t%s\n", imrPtr->linenumber, imrPtr->label, imrPtr->opcode, imrPtr->operand);
            break;
        }
        if(imrPtr->flag == 'c'){ // if it is a comment
            fprintf(lstFPtr, "%3d\t\t%s\n", imrPtr->linenumber, imrPtr->buffer);
        }
        else if(imrPtr->x){ // if printed with indexed addressing
            fprintf(lstFPtr, "%3d\t%04X\t%-6s\t%-6s\t%s\t%02X\n", imrPtr->linenumber, imrPtr->loc, imrPtr->label, imrPtr->opcode, imrPtr->operand, (unsigned int)imrPtr->objectCode);
        }
        else if(imrPtr->objectCode == 0){ // no objectcode to print
            if(intermediateRecord[i+1]->loc - imrPtr->loc)
                fprintf(lstFPtr, "%3d\t%04X\t%-6s\t%-6s\t%s\n", imrPtr->linenumber, imrPtr->loc, imrPtr->label, imrPtr->opcode, imrPtr->operand);
            else // no LOCCTR to print
                fprintf(lstFPtr, "%3d\t\t%-6s\t%-6s\t%s\n", imrPtr->linenumber, imrPtr->label, imrPtr->opcode, imrPtr->operand);
        }
        else{ // normal print
            fprintf(lstFPtr, "%3d\t%04X\t%-6s\t%-6s\t%s\t\t%02X\n", imrPtr->linenumber, imrPtr->loc, imrPtr->label, imrPtr->opcode, imrPtr->operand, (unsigned int)imrPtr->objectCode);
        }
    }

    /*~~ write object file ~~*/
    imrPtr = intermediateRecord[0];
    if(!(strcmp(imrPtr->opcode, "START"))){
        fprintf(objFPtr, "H%-6s%06X%06X\n", (imrPtr ? imrPtr->label : ""), STARTADR, PLENGTH);
    }
    else{
        fprintf(objFPtr, "H%-6s%06X%06X\n", "", STARTADR, PLENGTH);
    }
    trPtr = TRHEAD.next;
    while(trPtr){
        /* fprintf(objFPtr, "%s", trPtr->record); */
        for(i = 0; i < TRMAXLEN && trPtr->record[i] != '\0'; i++){
            fprintf(objFPtr, "%c", trPtr->record[i]);
        }
        trPtr = trPtr->next;
        fputc('\n', objFPtr);
    }

    printf("output file : [%s], [%s]\n",lstFilename, objFilename);
    /* close files and free memory */
    free(lstFilename);
    free(objFilename);
    trPtr = TRHEAD.next;
    while(trPtr){
        currentTR = trPtr;
        trPtr = trPtr->next;
        free(currentTR);
    }
    fclose(lstFPtr);
    fclose(objFPtr);
    return 1;
}

int asmCreateObjectCode(unsigned int operAdr, IMRNODE* imrPtr, struct opcodeNode* opcodePtr, unsigned int LOCCTR, int notasymbol){
    int format = opcodePtr->format[0]-'0'; // reads format
    long long int objectcode = 0;
    int tempInt = 0;
    int tempInt2 = 0;
    int TA = operAdr;

    if(format == 1){
        objectcode = (unsigned int)opcodePtr->hex;
    }
    else if(format == 2){
        objectcode = (unsigned int)opcodePtr->hex;
        objectcode <<= 8; // move value 1 byte to the left
        objectcode += operAdr;
    }
    else if(format == 3){

        /* if it is not immediate addressing and it is a symbol*/
        if(!(imrPtr->n == 0 && imrPtr->i == 1) && !notasymbol){ //!notasymbol is double negative, meaning it is a symbol
            if(imrPtr->e == 0){ // if format 3
                /* calculate the Target Address, i.e. use pc or base relative */
                tempInt = (int)operAdr - LOCCTR; // pc relative
                if(tempInt < -2048 || tempInt > 2047){ // out of bounds for pc relative
                    if(!NOBASE){ // possible to do base relative address
                        tempInt2 = (int)operAdr - rB;
                        if(tempInt2 < 0 || tempInt2 > 4095){ // out of bounds for base relative
                            /* addressing mode needs to be format 4 but is not set to 4 */
                            printf("Error on line %d: impossible to use pc- or base-relative addressing.\n", (imrPtr->linenumber));
                            printf("\tNeed to use extended formatting mode.\n");
                            return 0;
                        }
                        else{
                            imrPtr->b = 1;
                            imrPtr->p = 0;
                            TA = tempInt2;
                        }
                    }
                    else{
                        printf("Error on line %d: impossible to use pc relative addressing and BASE is not set.\n", (imrPtr->linenumber));
                        return 0;
                    }
                }
                else{
                    imrPtr->p = 1;
                    imrPtr->b = 0;
                    TA = tempInt;
                }
            }
        }

        /* create the object code */
        objectcode = (int)opcodePtr->hex;
        tempInt = imrPtr->n;
        tempInt <<= 1;
        objectcode += tempInt;
        objectcode += imrPtr->i;
        objectcode <<= 1;
        objectcode += imrPtr->x;
        objectcode <<= 1;
        objectcode += imrPtr->b;
        objectcode <<= 1;
        objectcode += imrPtr->p;
        objectcode <<= 1;
        objectcode += imrPtr->e;
        if(imrPtr->e == 1){ // format 4
            objectcode <<= 20;
            /* clearing all but 20 bits */
            TA |= 0xFFF00000;
            TA ^= 0xFFF00000;
        }
        else{ // format 3
            objectcode <<= 12;
            /* clearing all but 12 bits */
            TA |= 0xFFFFF000;
            TA ^= 0xFFFFF000;
        }
        objectcode += TA;
    }
    imrPtr->objectCode = objectcode;
    return 1;
}

struct symbolNode* symSearch(char *key, int hashcode){
    struct symbolNode* ptr = SYMTAB[hashcode];

    /* searches for a symbol in the hash table */
    /* returns pointer to symbol if found, else NULL */
    while(ptr){
        if(!(strcmp(ptr->label, key))){
            return ptr;
        }
        if(!(ptr = ptr->next)) break;
    }
    return NULL;
}

int asmIsRegister(struct symbolNode* symbolPtr){
    int result = 0;
    int len = strlen(symbolPtr->label);
    if(len == 1){
        switch(symbolPtr->label[0]){
        case 'A':
        case 'X':
        case 'L':
        case 'B':
        case 'S':
        case 'T':
        case 'F': result = 1; break;
        }
    }

    return result;
}

void asmByteObjectCodeCreator(struct intermediateRecordNode* imrPtr){
    int i, j;
    char buf[TOKLEN] = "";
    int TA = 0;

    /* if hexadecimal */
    if(imrPtr->operand[0] == 'X'){
        j = 2;
        for(i = 0; imrPtr->operand[j] != '\''; i++, j++){
            buf[i] = imrPtr->operand[j];
        }
        buf[i] = '\0';
        TA = newHexToInt(buf, i);
    }
    /* if ascii */
    else if(imrPtr->operand[0] == 'C'){
        j = 2;
        for(i = 0; imrPtr->operand[j] != '\''; i++, j++){
            buf[i] = imrPtr->operand[j];
        }
        buf[i] = '\0';
        for(j = 0; j < i; j++){
            TA += buf[j];
            if((j+1) < i) TA <<= 8; // if not last char, move 1 byte to left
        }
    }
    imrPtr->objectCode = TA;
}

int asmPrintSymTab(void){
    int status = 1;
    int i, j;
    struct symbolNode* symPtr;
    struct symbolNode **symList;
    if(SYMTABCOUNT){
        symList = (struct symbolNode**)malloc(sizeof(struct symbolNode*)*SYMTABCOUNT);
    }
    else{// In case of empty symbol table
        printf("Nothing to print, symbol table is empty.\n");
        return 1;
    }
    j = 0;
    /* fill list of symbols */
    for(i = 0; i < SYMHASHSIZE; i++){
        symPtr = SYMTAB[i];
        while(symPtr){
            symList[j] = symPtr;
            j++;
            symPtr = symPtr->next;
        }
    }
    /* sort list of symbols */
    for(i = 1; i < SYMTABCOUNT; i++){ // insertion sort
        symPtr = symList[i] ;
        j = i - 1;
        while(j >= 0 && (strcmp(symPtr->label, symList[j]->label)) > 0){
            symList[j+1] = symList[j];
            j--;
        }
        symList[j+1] = symPtr;
    }

    /* print list of symbols */
    for(i = 0; i < SYMTABCOUNT; i++){
        printf("\t%6s\t%04X\n", symList[i]->label, symList[i]->loc);
    }

    /* free memory */
    free(symList);

    return status;
}
