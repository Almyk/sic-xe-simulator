#include "20162004.h"

void getInput(char* command){
    /* this function gets user input until newline is enter */
    char c;
    int i = 0;
    int onlySpace = 1;

    while((c = getchar()) != '\n' && i < MAXBUF - 1){
        if(c != ' ') onlySpace = 0;
        if(!onlySpace) command[i++] = c;
    }
    if(onlySpace) command[0] = '\0';
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
        if(strcmp(arguments[0], "loader")){ // and command is not loader
            *n = -1;
        }
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
    case -7: printf("Error: file not found.\n"); break;
    case -8: printf("Error: file is not a '.asm' file.\n"); break;
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
            // if reached end of line
            if(c == '\n' || c == '\0'){
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
    return -1;
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

int cmpExtension(const char* filename, const char* extension){
    int filenameLen = strlen(filename);
    int extLen = strlen(extension);

    return strcmp(filename+filenameLen-extLen, extension);
}

int stringToInt(char* string){
    int len = strlen(string);
    int i;
    int result = 0;
    if(len > 0){
        for(i = len - 1; i >= 0; i--){
            result += (string[i] - '0') * power(10, len-i-1);
        }
    }
    return result;
}

int skipSpaces(const char* buffer, int* index){
    /* this function increments index until it finds a non-space character */
    /* returns how many iterations it performed */
    int count = 0;
    while((buffer[*index] == ' ' || buffer[*index] == '\t') && buffer[*index] != '\0'){
        *index += 1;
        count++;
    }
    if(buffer[*index] == '\0') count = -2;
    return count;
}

int getToken(const char* buffer, char* token, int tokIndex, int* bufIndex){
    /*
      This function copies one string from buffer to token,
      using spaces as a delimiter.
      tokIndex is given if an offset is needed for the token string,
      and bufIndex is given as an offset for the buffer string.
    */
    int i = tokIndex;

    if(buffer[*bufIndex] == '.'){ // if line is a comment
        token[i] = '.';
        token[i+1] = '\0';
        return -1;
    }
    while(i < TOKLEN && buffer[*bufIndex] != ' ' && buffer[*bufIndex] != '\0'){
        token[i] = buffer[*bufIndex];
        *bufIndex += 1; i++;
    }
    if(buffer[*bufIndex] != '\0') skipSpaces(buffer, bufIndex);
    token[i] = '\0';
    return i;
}

struct textRecordNode* TRALLOC(void){
    return (struct textRecordNode*) calloc(1, sizeof(struct textRecordNode));
}

int isNumber(char* string){
    int i;
    int result = 1;
    for(i = 0; string[i] != '\0'; i++){
        if(string[i] >= '0' && string[i] <= '9'){
            ;
        }
        else{
            result = -1;
            break;
        }
    }
    return result;
}

int newHexToInt(char* string, int n){
    /* returns integer representation of a hexadecimal number
     * given as a character string.
     * this is a new version which can accept strings of any lengths.
     */
    int result = 0;
    int i;

    for(i = n-1; i >= 0; i--){
        // 0 <= x <= 9
        if(string[i] >= '0' && string[i] <= '9'){
            result += (string[i] - '0') * power(16, n - i - 1);
        }
        // A <= x <= F
        else if(string[i] >= 'A' && string[i] <= 'F'){
            result += (string[i] - 'A' + 10) * power(16, n - i - 1);
        }
        // a <= x <= f
        else if(string[i] >= 'a' && string[i] <= 'f'){
            result += (string[i] - 'a' + 10) * power(16, n - i - 1);
        }
        // not a valid hexadecimal
        else{
            return -4;
        }
    }

    return result;
}

int newStringToInt(char* string, int n){
    /* similar to stringToInt, but in this version you can specify string length */
    int i;
    int result = 0;
    for(i = 0; i < n; i++){
        result += (string[i] - '0') * power(10, n-i-1);
    }
    return result;
}
