#include "20162004.h"

void getInput(char* command){
    /* this function gets user input until newline is enter */
    char c;
    int i = 0;

    while((c = getchar()) != '\n' && i < MAXBUF - 1){
        command[i++] = c;
    }
    command[i] = '\0';

    // flushes stdin
    if(i >= MAXBUF - 1)
        while((c = getchar()) != '\n');
}

char** parseInput(char* command, int* n){
    /* this function tokenizes the input,
     * and does some initial validation on the input */
    char** arguments = (char**) malloc(sizeof(char*) * TOKENBUF+1);
    char* token;
    char* tmpString = (char*) calloc(MAXBUF, sizeof(char));
    char delim[] = " \t\b\n";
    int len = 0;
    int i = 0;
    int commas = 0;

    // if no input was given
    len = strlen(command);
    if(len < 1){
        *n = -2;
        arguments = NULL;
        return arguments;
    }

    // tokenizes the recieved string
    strcpy(tmpString, command);
    token = strtok(tmpString, delim);
    len = strlen(token);
    // removes the commas from the string
    commas = removeCommas(tmpString+len+2);
    if(commas < 0){ // if < 0 it means the string is in an invalid format
        *n = commas;
        return arguments;
    }

    while(token != NULL){
        arguments[i] = token;
        i++;

        if(i > TOKENBUF){
            // too many arguments
            *n = -1;
            return arguments;
        }
        token = strtok(NULL, delim);
    }
    arguments[i] = NULL; 
    *n = i;

    if(*n > 2 && commas != *n - 2){
        // not enough commas for amount of arguments
        *n = -1;
    }

    return arguments;
}

int removeCommas(char* string){
    /* this function replaces commas with spaces
     * and also checks for consecutive commas
     */ 
    int i;
    int commaCount = 0, conComma = 0;

    for(i = 0; string[i] != '\0'; i++){
        if(string[i] == ','){
            commaCount++; conComma++;
            string[i] = ' ';
            if(conComma > 1) return -1;
        }
        else if(string[i] != ' ' && string[i] != '\n'){
            conComma = 0;
        }
    }
    return commaCount;
}

void addHistory(char* string){
    /* this function adds a new node to the history list */
    struct historyNode* head = getSetHistHead();
    struct historyNode* ptr = head->last;
    struct historyNode* newNode;

    // allocate memory for new node
    newNode = malloc(sizeof(struct historyNode));
    if(!newNode){
        printf("Error allocating memory for history node.\n");
        return;
    }
    head->n++;
    head->last = newNode;
    newNode->n = head->n;
    newNode->next = NULL;
    strcpy(newNode->name, string);

    // not the first entry in linked list
    if(ptr){
        newNode->last = ptr;
        ptr->next = newNode;
    }
    // if it is the first entry
    else{
        head->next = newNode;
        newNode->last = NULL;
    }
}

int hexToInt(char* string){
    /* returns integer representation of a hexadecimal number
     * given as a character string
     */
    int result = 0;
    int i;
    int len = strlen(string);
    if(len > 5) return -5; // string too long

    for(i = len-1; i >= 0; i--){
        // 0 <= x <= 9
        if(string[i] >= '0' && string[i] <= '9'){
            result += (string[i] - '0') * power(16, len - i - 1);
        }
        // A <= x <= F
        else if(string[i] >= 'A' && string[i] <= 'F'){
            result += (string[i] - 'A' + 10) * power(16, len - i - 1);
        }
        // a <= x <= f
        else if(string[i] >= 'a' && string[i] <= 'f'){
            result += (string[i] - 'a' + 10) * power(16, len - i - 1);
        }
        // not a valid hexadecimal
        else{
            return -4;
        }
    }

    return result;
}

int power(int base, int exp){
    /* raises base to the exp */
    int i = 1;
    int result = base;
    if(exp == 0) return 1;
    if(exp == 1) return base;
    for(i = 1; i < exp; i++){
        result = result * base;
    }
    return result;
}

void printError(int n){
    switch(n){
        case -1: printf("Error: Not a valid command, please try 'help'.\n"); break;
        case -3: printf("Error: End address < Start address.\n"); break;
        case -4: printf("Error: Not a valid hexadecimal.\n"); break;
        case -5: printf("Error: Hexadecimals have to be in the range [0x00000, 0xFFFFF].\n");
                 printf("Please make sure your hexadecimals are no longer than 5 signs.\n");
                 break;
        case -6: printf("Error: Value to be stored exceeds the range [0x0,0xFF].\n"); break;
    }
}
void freeHistory(void){
    /* this function free's the memory allocated for history list */
    struct historyNode* head = getSetHistHead();
    struct historyNode* ptr = head->next;
    struct historyNode* delete;

    while(ptr){
        delete = ptr;
        ptr = ptr->next;
        free(delete);
    }
    free(head);
}
void freeHashtable(void){
    /* this function free's the memory for the opcodelist */
    struct opcodeNode* ptr;
    struct opcodeNode* delete;
    int i;

    for(i = 0; i < HASHSIZE; i++){
        if(HASHTABLE[i]){
            ptr = HASHTABLE[i];
            while(ptr){
                delete = ptr;
                ptr = ptr->next;
                free(delete);
            }
        }
    }
}
void freeArguments(char** args){
    /* frees the tokenized arguments
     * since strtok() only returns pointers pointing to the given string + offset
     * free(args[0]) implicitly free's all the tokens
     */
    if(args[0]) free(args[0]);
    if(args) free(args);
}

int readline(char* buffer, FILE* fp){
    int i;
    char c;

    for(i = 0; i < MAXBUF; i++){
        c = fgetc(fp);

        if(!feof(fp)){
            // reached end of line
            if(c == '\n'){
                buffer[i] = '\0';
                return 1;
            }
            else{
                buffer[i] = c;
            }
        }
        // reached end of file
        else return 0;
    }
    return 0;
}

struct historyNode* getSetHistHead(void){
    /* This function returns the head node for the history list
     * and if the head node have not been initialized, it initializes it
     */
    static struct historyNode* head;

    if(!head){
        head = malloc(sizeof(struct historyNode));
        if(!head){
            printf("Error allocating memory for history list's head node.\n");
            return NULL;
        }
        head->n = 0;
        head->next = NULL;
        head->last = NULL;
    }
    return head;
}

int findFile(char* filename){
    /* searches for file in current directory
     * uses the dirent.h library functions.
     * returns 1 if found, 0 if not found.
     */

    DIR *dirp;
    struct dirent *dir;
    int found = 0;

    dirp = opendir(".");
    while(!found && (dir = readdir(dirp)) != NULL){
        if(!(strcmp(dir->d_name, filename))){
            found = 1;
        }
    }
    closedir(dirp);
    return found;
}
