#include "20162004.h" // my header

/* Global Variables */
int PROGADDR = 0; // Program Address pointer

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
        if(CSADDR == -2){
            printf("Error: File empty. (%s)\n", args[i]);
            return 1;
        }
        else if(CSADDR == -3){
            printf("Error: First row not initialized as head record. (%s)\n", args[i]);
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
        CSLTH = newHexToInt(buffer+13, 6);

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
                    for(i = k, j = 0; i < k+6; i++, j++){
                        if(buffer[i] != ' ') progname[j] = buffer[i];
                        else break;
                    }
                    progname[j] = '\0';
                    k += 6; // increment reading position
                    /* read address */
                    for(i = k, j = 0; i < k+6; i++, j++){
                        adrString[j] = buffer[i];
                    }
                    adrString[j] = '\0';
                    address = newHexToInt(adrString, 6);
                    k += 6; // increment reading position

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
    int i;
    struct esNode* ptr;
    printf("Control\t\tSymbol\n");
    printf("section\t\tname\t\tAddress\t\tLength\n");
    printf("------------------------------------------------------\n");
    if(estabCount){ // if ESTAB is not empty
        for(i = 0; i < estabCount; i++){
            ptr = sortedESTAB[i];
            if(ptr->flag == 'c')
                printf("%s\t\t\t\t%d\t\t%d\n",ptr->SYMBOL, ptr->address, ptr->length);
            else
                printf("\t\t%s\t\t%d\n",ptr->SYMBOL, ptr->address);
        }
    }
    printf("------------------------------------------------------\n");
}

void resetESTAB(void){
    int i;
    for(i = 0; i < estabCount; i++){
        free(sortedESTAB[i]);
    }
    free(sortedESTAB);
    estabCount = 0;
    sestabSize = 0;
}
