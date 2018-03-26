#include "20162004.h"

/* This file hold all the functions regarding the opcodelist:
 * opInsert
 * opMnem
 * opSearch
 * opPrintOpcodelist
 * hashcode
 */

int opInsert(void){
    // reads file and inserts information about opcodes to hashtable
    FILE *fp;
    char buffer[MAXBUF] = "";
    struct opcodeNode* newNode;
    char *token;
    char delim[] = " \n\t\b";
    int index;
    struct opcodeNode *ptr;
    int status = 1;

    if(!(fp = fopen("opcode.txt", "r"))){
        return 0; // if file was not found
    }

    while(!feof(fp) && status){
        status = readline(buffer, fp);
        if(status == 0) break;
        if(status < 0) return 0;
        if(!(newNode = malloc(sizeof(struct opcodeNode)))){
            printf("Error allocating memory for opcodelist node.\n");
            return -2;
        }
        // initialize a new node for the hashtable
        // using data read from file
        token = strtok(buffer, delim);
        newNode->hex = hexToInt(token);
        strcpy(newNode->op, strtok(NULL, delim));
        strcpy(newNode->format, strtok(NULL, delim));
        newNode->next = NULL; newNode->last = NULL;
        index = hashcode(newNode->op);
        // if there was a collision we add the new node to the end of the list
        if(HASHTABLE[index]){
            ptr = HASHTABLE[index];
            if(ptr->last) ptr = ptr->last;
            ptr->next = newNode;
            newNode->last = ptr;
            HASHTABLE[index]->last = newNode;
        }
        else HASHTABLE[index] = newNode;
    }
    return 1;
}

struct opcodeNode* opSearch(char *key, int hashcode){
    struct opcodeNode* ptr = HASHTABLE[hashcode];

    // searches for the key in HASHTABLE[hashcode]
    // if it finds the key it returns a pointer to its location
    while(ptr){
        if(!(strcmp(ptr->op, key))){
            return ptr;
        }
        if(!(ptr = ptr->next)) break;
    }
    return NULL;
}

int opMnem(char** args){
    // searches for & prints mnemonics from hashtable
    struct opcodeNode* node;
    node = opSearch(args[1], hashcode(args[1]));
    if(!node){
        printf("Error: mnemonic not found.\n");
        return -2;
    }
    printf("opcode is %02X\n", node->hex);
    return 1;
}

int opPrintOpcodelist(void){
    // prints all nodes in the hashtable
    int i;
    struct opcodeNode* ptr;
    for(i = 0; i < HASHSIZE; i++){
        ptr = HASHTABLE[i];
        if(ptr){
            printf("%d : ", i);
            while(ptr){
                printf("[%s,%02X]", ptr->op, ptr->hex);
                ptr = ptr->next;
                if(ptr) printf(" -> ");
                else printf("\n");
            }
        }
        else printf("%d :\n", i);
    }
    return 1;
}

int hashcode(char* string){
    // this function takes a string,
    // converts it to an integer and determines its hashcode
    int result = 0;
    int i = 0;
    while(*(string + i)){
        result += (unsigned int)*(string + i++);
    }
    return result % HASHSIZE;
}
