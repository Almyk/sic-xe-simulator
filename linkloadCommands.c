#include "20162004.h" // my header

/* Global Variables */
int PROGADDR = 0; // Program Address pointer
int EXECADDR = 0; // address of instruction to be executed

struct esNode* ESTAB[ESHASHSIZE]; // External Symbol Table
struct esNode** sortedESTAB; // for printing out the ESTAB
int sestabSize = 0; // how much space allocated sortedESTAB
int estabCount = 0; // how many entries in ESTAB

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
    int currADR = 0; // address we are currently working at
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
                currADR = newHexToInt(buffer+1, ADDRLEN); // read address for first instruction
                currLEN = newHexToInt(buffer+ADDRLEN+1, 2); // read length of record

                /* load text record */
                while(k - ADDRLEN - 3 < 2 * currLEN){ // 2 * currLEN because 2 char per byte
                    /* object code is in half-bytes,
                       thus 2 entries from the buffer
                       are inserted per memory location
                    */
                    MEMORY[CSADDR+currADR] = newHexToInt(&buffer[k], 2);
                    currADR++;
                    k += 2;
                }
            }
            else if(buffer[0] == 'M'){ // modification record
                hbFlag = 'f'; // set half-byte flag to false
                refNum = newHexToInt(buffer+10, 2); // grab the reference number for the reference record
                if(refRecord[refNum]){
                    currADR = newHexToInt(buffer+1, ADDRLEN); // get address to change
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
                            instructionADDR += MEMORY[CSADDR+currADR] & 0x0F;
                        else
                            instructionADDR += MEMORY[CSADDR+currADR+i];
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
                            MEMORY[CSADDR+currADR] = MEMORY[CSADDR+currADR] & 0xF0; // remove lower half
                            /* add only lower half-byte by first shifting the bits
                               the needed amount to the right and doing bitwise AND to
                               only store the intended byte
                            */
                            MEMORY[CSADDR+currADR] += (instructionADDR >> 8 * (currLEN - 1)) & 0x0F;
                        }
                        else{ // add the full byte to memory
                            MEMORY[CSADDR+currADR+i] = (instructionADDR >> 8 * (currLEN - i - 1)) & 0xFF;
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
