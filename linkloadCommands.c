#include "20162004.h" // my header

/* Global Variables */
int PROGADDR = 0; // Program Address pointer
int EXECADDR = 0; // address of instruction to be executed
int PROGLEN = 0;

/* External System Table */
struct esNode* ESTAB[ESHASHSIZE]; // External Symbol Table
struct esNode** sortedESTAB; // for printing out the ESTAB
int sestabSize = 0; // how much space allocated sortedESTAB
int estabCount = 0; // how many entries in ESTAB

/* registers */
int A = 0; int X = 0;
int L = 0; int B = 0;
int S = 0; int T = 0;
int PC = 0; int SW = 0;

int llSetProgaddr(char* input){
    int value = 0;

    // get hexadecimal value from input string
    value = hexToInt(input);
    // if input string is a hexadecimal value we store it in PROGADDR
    if(value >= 0) PROGADDR = value;
    else return value;

    return 1;
}

int llLoadProgram(char **args, int n){
    int status = 1;
    int CSADDR = PROGADDR; // initialize CSADDR
    FILE* fp;
    int i;

    if(estabCount){ // if true, need to reset
        resetESTAB();
    }

    // loop to check all filenames
    for(i = 1; i < n; i++){
        status = findFile(args[i]);
        if(!status){ // file not found
            printf("Error: Can't find file \"%s\"\n", args[i]);
            return 1;
        }
        if((cmpExtension(args[i], ".obj"))){
            printf("Error: File not an object file. (%s)\n", args[i]);
            return 1;
        }
    }

    /* loop to run pass 1 on all given files */
    for(i = 1; i < n; i++){
        fp = fopen(args[i], "r");
        if(!fp){
            printf("Error: something went wrong opening file \"%s\"\n", args[i]);
            return 1;
        }

        /* run pass 1 on file*/
        CSADDR = llFirstPass(fp, CSADDR);

        /* print error messages */
        if(CSADDR < 0){
            llPrintError(CSADDR, args[i]);
            return 1;
        }
        fclose(fp);
    }

    /* set the address pointers */
    CSADDR = PROGADDR;
    EXECADDR = PROGADDR;

    /* loop to run pass 2 on all given files */
    for(i = 1; i < n; i++){
        fp = fopen(args[i], "r");
        if(!fp){
            printf("Error: something went wrong opening file \"%s\"\n", args[i]);
            return 1;
        }

        /* run pass 2 on file */
        CSADDR = llSecondPass(fp, CSADDR);

        /* print error messages */
        if(CSADDR < 0){
            llPrintError(CSADDR, args[i]);
            return 1;
        }
        fclose(fp);
    }

    printESTAB();
    return status;
}

int llFirstPass(FILE* fp, int CSADDR){
    int status = 0;
    int CSLTH = 0;
    char buffer[MAXBUF] = "";
    char progname[7] = "";
    char adrString[7] = "";
    int address;
    int i, j, k;
    struct esNode* esPtr;

    status = readline(buffer, fp); // read first line
    if(!status) return -2; // empty file
    if(buffer[0] != 'H') return -3; // first row is not head record

    /* main loop of pass 1 */
    do{

        /* read head record */
        for(i = 1, j = 0; i < 7; i++, j++){
            if(buffer[i] != ' ') progname[j] = buffer[i];
            else break;
        }
        progname[j] = '\0';
        CSLTH = newHexToInt(buffer+13, ADDRLEN);

        /* search ESTAB for control section name */
        esPtr = esSearch(progname, hashcode(progname, ESHASHSIZE));
        if(esPtr){
            printf("Error: Duplicate control section name. (%s)\n", progname);
            return -1;
        }
        /* insert into ESTAB */
        llExtSymTabInsert(progname, CSADDR, CSLTH, 'c');

        /* read until END record */
        while(status == 1 && (status = readline(buffer, fp))){
            if(buffer[0] == 'E') break;
            if(buffer[0] == 'D'){
                /* for each symbol in the record */
                k = 1;
                while(k < 73 && buffer[k] != '\0'){
                    /* get symbol name */
                    for(i = k, j = 0; i < k+ADDRLEN; i++, j++){
                        if(buffer[i] != ' ') progname[j] = buffer[i];
                        else break;
                    }
                    progname[j] = '\0';
                    k += ADDRLEN; // increment reading position
                    /* read address */
                    for(i = k, j = 0; i < k+ADDRLEN; i++, j++){
                        adrString[j] = buffer[i];
                    }
                    adrString[j] = '\0';
                    address = newHexToInt(adrString, ADDRLEN);
                    k += ADDRLEN; // increment reading position

                    /* search ESTAB for symbol name */
                    esPtr = esSearch(progname, hashcode(progname, ESHASHSIZE));
                    if(esPtr){
                        printf("Error: Duplicate control section name. (%s)\n", progname);
                        return -1;
                    }
                    /* insert symbol into ESTAB */
                    llExtSymTabInsert(progname, CSADDR+address, 0, 's');
                }
            }
        }
        CSADDR += CSLTH;

        /* find next head record */
        if(status == 1 && buffer[0] == 'E'){
            while(status && buffer[0] != 'H'){
                status = readline(buffer, fp);
            }
        }
    } while(status);
    return CSADDR;
}

int llSecondPass(FILE* fp, int CSADDR){
    int status = 1;
    char buffer[MAXBUF] = "";
    char progname[7];
    int CSLTH = 0;
    int currADDR = 0; // address we are currently working at
    int currLEN = 0; // length of current text record
    int i, j, k; // index offset variables
    struct esNode** refRecord = (struct esNode**) calloc(estabCount, sizeof(struct esNode*));
    char refName[7]; // reference symbol name
    int refNum; // used for reference records
    /* unsigned char bytes[4]; // used to split addresses into their individual bits */
    int instructionADDR;
    char hbFlag = 'f'; // half-byte flag

    /* read all sections in current file */
    do{
        status = readline(buffer, fp); // read first line
        if(!status) return -2; // empty file
        if(buffer[0] != 'H') return -3; // first row is not head record

        /* read head record */
        for(i = 1, j = 0; i < 7; i++, j++){
            if(buffer[i] != ' ') progname[j] = buffer[i];
            else break;
        }
        progname[j] = '\0';
        refRecord[1] = esSearch(progname, hashcode(progname, ESHASHSIZE));
        CSLTH = newHexToInt(buffer+13, ADDRLEN); // read the length of program section

        /* main loop for second pass */
        while(status == 1 && (status = readline(buffer, fp))){
            if(buffer[0] == 'E') break;

            if(buffer[0] == 'T'){ // text record
                k = ADDRLEN+3; // offset where first instruction starts
                currADDR = newHexToInt(buffer+1, ADDRLEN); // read address for first instruction
                currLEN = newHexToInt(buffer+ADDRLEN+1, 2); // read length of record

                /* load text record */
                while(k - ADDRLEN - 3 < 2 * currLEN){ // 2 * currLEN because 2 char per byte
                    /* object code is in half-bytes,
                       thus 2 entries from the buffer
                       are inserted per memory location
                    */
                    MEMORY[CSADDR+currADDR] = newHexToInt(&buffer[k], 2);
                    currADDR++;
                    k += 2;
                }
            }
            else if(buffer[0] == 'M'){ // modification record
                hbFlag = 'f'; // set half-byte flag to false
                refNum = newHexToInt(buffer+10, 2); // grab the reference number for the reference record
                if(refRecord[refNum]){
                    currADDR = newHexToInt(buffer+1, ADDRLEN); // get address to change
                    currLEN = newHexToInt(buffer+ADDRLEN+1, 2); // length of address in half-bytes
                    if(currLEN % 2){
                        currLEN += 1; // if uneven number add 1 to amount of bytes to grab
                        hbFlag = 't'; // set half-byte flag to true
                    }
                    currLEN = currLEN / 2; // find how many bytes to change
                    instructionADDR = 0;
                    /* gather the bytes that are to be changed */
                    for(i = 0; i < currLEN; i++){
                        if(i == 0 && hbFlag == 't') // only grab half of the byte
                            instructionADDR += MEMORY[CSADDR+currADDR] & 0x0F;
                        else
                            instructionADDR += MEMORY[CSADDR+currADDR+i];
                        if(i + 1 < currLEN)
                            instructionADDR <<= 8; // shift 1 byte left
                    }
                    if(buffer[9] == '+') instructionADDR += refRecord[refNum]->address;
                    else if(buffer[9] == '-') instructionADDR -= refRecord[refNum]->address;
                    else{
                        printf("Error: Incorrect sign '%c' in modification record.\n", buffer[9]);
                        return -1;
                    }
                    /* split address back into separate bytes and place them in memory */
                    for(i = 0; i < currLEN; i++){
                        if(i == 0 && hbFlag == 't'){ // first byte to insert should only be a half-byte
                            MEMORY[CSADDR+currADDR] = MEMORY[CSADDR+currADDR] & 0xF0; // remove lower half
                            /* add only lower half-byte by first shifting the bits
                               the needed amount to the right and doing bitwise AND to
                               only store the intended byte
                            */
                            MEMORY[CSADDR+currADDR] += (instructionADDR >> 8 * (currLEN - 1)) & 0x0F;
                        }
                        else{ // add the full byte to memory
                            MEMORY[CSADDR+currADDR+i] = (instructionADDR >> 8 * (currLEN - i - 1)) & 0xFF;
                        }
                    }
                }
                else{
                    printf("Error: Reference Number %02X not found.\n", refNum);
                    return -1;
                }
            }
            else if(buffer[0] == 'R'){ // Reference Record
                /* create array of symbols listed */
                k = 1;
                /* while not end of line and not exceeding entries in ESTAB */
                while(buffer[k] != '\0' && (k-1)/8 < estabCount){ // 8 = 2 chars for index + 6 for symbol
                    /* get the index number */
                    refNum = newHexToInt(buffer+k, 2);
                    k += 2;
                    /* copy symbol name from buffer */
                    for(i = 0; i < 6; i++){
                        refName[i] = buffer[k+i];
                        if(refName[i] == ' ') break;
                    }
                    refName[i] = '\0';
                    k += 6;
                    /* search for symbol in ESTAB */
                    refRecord[refNum] = esSearch(refName, hashcode(refName, ESHASHSIZE));

                    if(!refRecord[refNum]){ // reference not found
                        printf("Error: The reference %s was not found.\n", refName);
                        return -1;
                    }
                }
            }
        }
        if(buffer[0] == 'E'){ // check if there is an address in End Record
            if(buffer[1]){
                EXECADDR = newHexToInt(buffer+1, 6);
            }
        }

        CSADDR += CSLTH;
        /* find next head record */
        if(status == 1 && buffer[0] == 'E'){
            while(status && buffer[0] != 'H'){
                status = readline(buffer, fp);
            }
        }
    }while(status);


    free(refRecord);
    return CSADDR;
}

struct esNode* esSearch(char *key, int hashcode){
    struct esNode* ptr = ESTAB[hashcode];

    /* searches for a symbol in the hash table */
    /* returns pointer to symbol if found, else NULL */
    while(ptr){
        if(!(strcmp(ptr->SYMBOL, key))){
            return ptr;
        }
        if(!(ptr = ptr->next)) break;
    }
    return NULL;
}

void llExtSymTabInsert(char* symbol, int CSADDR, int length, char flag){
    /* this function only makes insertions to the hash table
       and does not check for duplicate entries. */

    int hash = hashcode(symbol, ESHASHSIZE);
    struct esNode* esPtr = ESTAB[hash];
    struct esNode* newNode = (struct esNode*) calloc(1, sizeof(struct esNode));

    /* initialize new node */
    strcpy(newNode->SYMBOL, symbol);
    newNode->flag = flag;
    newNode->address = CSADDR;
    newNode->length = length;
    newNode->next = NULL;

    /* if collision in hash table add to end of linked list */
    if(esPtr){
        newNode->last = esPtr->last;
        if(esPtr->next == NULL) esPtr->next = newNode; // if 2nd entry
        else esPtr->last->next = newNode; // if more than 2 entries
        esPtr->last = newNode;
    }
    else{
        newNode->last = NULL;
        ESTAB[hash] = newNode;
    }

    /* add to sortedESTAB */
    if(!sestabSize){ // first entry
        sortedESTAB = (struct esNode**) calloc(10, sizeof(struct esNode*));
        sestabSize = 10;
    }
    if(estabCount == sestabSize){ // if we need to resize sortedESTAB
        sortedESTAB = (struct esNode**) realloc(sortedESTAB, 2*sestabSize*sizeof(struct esNode*));
        sestabSize *= 2;
    }
    sortedESTAB[estabCount] = newNode;
    estabCount++;
}

void printESTAB(void){
    /* function to print External Symbol Table */
    int i;
    int totLen = 0;
    struct esNode* ptr;
    printf("Control\t\tSymbol\n");
    printf("section\t\tname\t\tAddress\t\tLength\n");
    printf("------------------------------------------------------\n");
    if(estabCount){ // if ESTAB is not empty
        for(i = 0; i < estabCount; i++){
            ptr = sortedESTAB[i];
            if(ptr->flag == 'c'){
                printf("%-6s\t\t\t\t%04X\t\t%04X\n",ptr->SYMBOL, ptr->address, ptr->length);
                totLen += ptr->length;
            }
            else
                printf("\t\t%-6s\t\t%04X\n",ptr->SYMBOL, ptr->address);
        }
    }
    printf("------------------------------------------------------\n");
    printf("\t\t\t\ttotal length %04X\n", totLen);
    PROGLEN = totLen;
}

void resetESTAB(void){
    /* this function frees the memory of ESTAB data structures */
    int i;
    for(i = 0; i < estabCount; i++){
        free(sortedESTAB[i]);
    }
    free(sortedESTAB);
    estabCount = 0;
    sestabSize = 0;
}

void llPrintError(int n, char* msg){
    switch(n){
    case -2: printf("Error: File empty. (%s)\n", msg); break;
    case -3: printf("Error: First row not initialized as head record. (%s)\n", msg); break;
    }
}

int llFindOpcodeFormat(unsigned char hex){
    /* this function looks at the OPTAB using the internal
       representation of the opcode.
       If opcode is found, function returns its format,
       if not found, returns 0
    */
    int i;
    struct opcodeNode* opPtr;

    for(i = 0; i < HASHSIZE; i++){
        opPtr = HASHTABLE[i];
        while(opPtr){
            if(opPtr->hex == hex) return opPtr->format[0] - '0';
            opPtr = opPtr->next;
        }
    }

    return 0;
}

int llRun(void){
    unsigned char opcode;
    unsigned char ni;
    int format;
    int instruction;
    int targetADDR; // Target Address
    int r1, r2; // register 1 & 2 for instructions using registers

    /* initialize Program Counter */
    PC = EXECADDR;

    while(1){
        if(PC > 0xFFFFF) break; // out of memory range
        if(PC > PROGADDR + PROGLEN) break; // reached end of program

        /* get object code and ni bits*/
        opcode = MEMORY[PC] & 0xFC; // ignores the 2 LSB
        ni = MEMORY[PC] & 0x03; // ignores all but 2 LSB

        /* find opcode format */
        format = llFindOpcodeFormat(opcode);
        if(!format){ // invalid opcode
            printf("Error: invalid opcode %02X\n", opcode);
            return -1;
        }

        /* get full instruction */
        instruction = MEMORY[PC];
        switch(format){
        case 2:
            instruction <<= 8;
            instruction += MEMORY[PC+1];
            break;
        case 3:
            instruction <<= 8;
            instruction += MEMORY[PC+1];
            instruction <<= 8;
            instruction += MEMORY[PC+2];
            if(!(MEMORY[PC+1] & 0x10)) break; // if the e bit is 0 we break else it is format 4
        case 4:
            instruction <<= 8;
            instruction += MEMORY[PC+3];
            format = 4;
            break;
        }

        /* program counter should be advanced after reading an instruction */
        PC += format;

        /* get target address if format 3 or 4 */
        if(format >= 3){
            targetADDR = llTargetAddress(instruction, format) + PROGADDR;
        }

        /* printf("Instruction: %08X\n", instruction); */
        /* printf("Format: %d\n", format); */
        /* printf("TA: %06X\n", targetADDR); */
        /* printf("A: %06X, PC: %06X\n", A, PC); */
        /* printf("B: %06X, X: %06X\n", B, X); */
        /* printf("L: %06X, SW: %06X\n", L, SW); */

        /* execute instruction */
        switch(opcode){

            /* Format 2 */
        case 0x90: // ADDR
            break;
        case 0xB4: // CLEAR
            r1 = instruction & 0xF0;
            r1 >>= 4;

            llSetRegVal(r1, 0);
            break;
        case 0xA0: // COMPR
            r1 = instruction & 0xF0;
            r1 >>= 4;
            r2 = instruction & 0xF;

            if(r1 < r2) SW = -1;
            else if(r1 > r2) SW = 1;
            else  SW = 0;
            break;
        case 0x9C: // DIVR
            break;
        case 0x98: // MULR
            break;
        case 0xAC: // RMO
            break;
        case 0xA4: // SHIFTL
            break;
        case 0x94: // SUBR
            break;
        case 0xB0: // SVC
            break;
        case 0xB8: // TIXR
            X += 1;
            r1 = instruction & 0xF0;
            r1 >>= 4;

            if(X < r1) SW = -1;
            else if(X > r1) SW = 1;
            else  SW = 0;
            break;

            /* Format 3/4 */
        case 0x18: // ADD
            break;
        case 0x40: // AND
            break;
        case 0x28: // COMP
            if(A > targetADDR) SW = 1;
            else if(A < targetADDR) SW = -1;
            else SW = 0;
            break;
        case 0x24: // DIV
            break;
        case 0x3C: // J
            PC = targetADDR;
            break;
        case 0x30: // JEQ
            if(SW == 0) PC = targetADDR;
            break;
        case 0x34: // JGT
            if(SW > 0) PC = targetADDR;
            break;
        case 0x38: // JLT
            if(SW < 0) PC = targetADDR;
            break;
        case 0x48: // JSUB
            L = PC;
            PC = targetADDR;
            break;
        case 0x00: // LDA
            A = llInterpretTA(targetADDR, ni, format);
            break;
        case 0x68: // LDB
            B = llInterpretTA(targetADDR, ni, format);
            break;
        case 0x50: // LDCH
            A = A & 0xFFFF00; // clear last byte
            A += llInterpretTA(targetADDR, ni, format) & 0x0FF; // only load last byte
            break;
        case 0x08: // LDL
            L = llInterpretTA(targetADDR, ni, format);
            break;
        case 0x6C: // LDS
            S = llInterpretTA(targetADDR, ni, format);
            break;
        case 0x74: // LDT
            T = llInterpretTA(targetADDR, ni, format);
            break;
        case 0x04: // LDX
            X = llInterpretTA(targetADDR, ni, format);
            break;
        case 0xD0: // LPS
            break;
        case 0x20: // MUL
            break;
        case 0x44: // OR
            break;
        case 0x4C: // RSUB
            PC = L;
            break;
        case 0x0C: // STA
            llLoadToAddress(targetADDR, A, 6);
            break;
        case 0x78: // STB
            llLoadToAddress(targetADDR, B, 6);
            break;
        case 0x54: // STCH
            llLoadToAddress(targetADDR, A & 0x0FF, 6);
            break;
        case 0x14: // STL
            llLoadToAddress(targetADDR, L, 6);
            break;
        case 0x7C: // STS
            llLoadToAddress(targetADDR, S, 6);
            break;
        case 0xE8: // STSW
            llLoadToAddress(targetADDR, SW, 6);
            break;
        case 0x84: // STT
            llLoadToAddress(targetADDR, T, 6);
            break;
        case 0x10: // STX
            llLoadToAddress(targetADDR, X, 6);
            break;
        case 0x1C: // SUB
            break;
        case 0x2C: // TIX
            break;
        }
    }

    return 1;
}

int llTargetAddress(int instruction, int format){
    int targetADDR = 0;
    int x, b, p;
    x = b = p = 0;

    /* find xbp bits and target address */
    switch(format){
    case 3:
        x = instruction & 0x8000;
        b = instruction & 0x4000;
        p = instruction & 0x2000;
        targetADDR = instruction & 0xFFF;
        break;
    case 4:
        x = instruction & 0x800000;
        b = instruction & 0x400000;
        p = instruction & 0x200000;
        targetADDR = instruction & 0xFFFFF;
        break;
    }
    if(x){
        targetADDR += X;
    }
    if(b){
        targetADDR += B;
    }
    else if(p){
        targetADDR += PC;
    }

    return targetADDR;
}

int llFetchFromMemory(int loc, int n){
    /* this function fetches n half-bytes from memory */
    int address = 0;
    int i = 0;

    if(n % 2){ // odd amount of half-bytes
        address = MEMORY[loc] & 0x0F; // take only half-byte
        i = 1;
    }

    /* get remaining half-bytes */
    for(; i * 2 < n; i++){
        address <<= 8;
        address += MEMORY[loc+i];
    }

    return address;
}

void llSetRegVal(int reg, int value){
    /* this function assigns a value to register */

    switch(reg){
    case 0: // A
        A = value;
        break;
    case 1: // X
        X = value;
        break;
    case 2: // L
        L = value;
        break;
    case 3: // B
        B = value;
        break;
    case 4: // S
        S = value;
        break;
    case 5: // T
        T = value;
        break;
    case 8: // PC
        PC = value;
        break;
    case 9: // SW
        SW = value;
        break;
    }
}

int llInterpretTA(int targetADDR, unsigned char ni, int format){
    int value = 0;
    switch(ni){
    case 0x01: // immediate mode
        value = targetADDR;
        break;
    case 0x02: // indirect mode
        value = llFetchFromMemory(targetADDR, format * 2);
        value = llFetchFromMemory(value, format * 2);
        break;
    case 0x03: // simple mode
        value = llFetchFromMemory(targetADDR, format * 2);
        break;
    }
    return value;
}

void llLoadToAddress(int address, int value, int n){
    /* insert value into MEMORY[address..adress+(n/2)] , n is in half-bytes */
    int i = 0;
    int k = 0;
    int bytes = n / 2;

    if(n % 2){ // if uneven amount of half-bytes
        MEMORY[address] = MEMORY[address] & 0xF0; // clear lower half-byte
        MEMORY[address] += (value >> ((n/2) * 8)) & 0x0F; // add lower half-byte
        i = 1;
    }

    for(; i < bytes; i++){
        MEMORY[address+i] += (value >> (8 * (bytes-k-1))) & 0xFF; // add one byte to memory
        k++;
    }
}
