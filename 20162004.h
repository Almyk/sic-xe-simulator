#ifndef __MYHEADER__
#define __MYHEADER__

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

// Constansts
#define MAXBUF 100
#define TOKENBUF 4
#define MEGA 1048576 // 2^20
#define HASHSIZE 20
#define SYMHASHSIZE 47

// Data Structures
struct historyNode{
    int n;
    char name[MAXBUF];
    struct historyNode *next;
    struct historyNode *last; // only used by head node to quickly find last node in list
};

struct opcodeNode{
    char op[10];
    unsigned char hex;
    char format[4];
    struct opcodeNode* next;
    struct opcodeNode* last;
};

struct symbolNode{
    char label[10];
    unsigned int loc;
    struct symbolNode* next;
    struct symbolNode* last;
};

struct intermediateRecordNode{
    unsigned int linenumber;
    unsigned int loc;
    unsigned int objectCode;
    char label[10];
    char opcode[10];
    char operand[10];
    char flag;
};

extern struct intermediateRecordNode** intermediateRecord;
extern int imIndex;

// Virtual Memory
extern unsigned char MEMORY[MEGA];

// Hash tables
extern struct opcodeNode* HASHTABLE[HASHSIZE]; // Operation Code Table
extern struct symbolNode* SYMTAB[SYMHASHSIZE]; // Symbol Table


/* Function Declerations */
int runCommand(char**, int);

// general functions
void getInput(char*);
char** parseInput(char*, int*);
int removeCommas(char*);
void addHistory(char*);
int hexToInt(char*);
int power(int, int);
void printError(int);
void freeHistory(void);
void freeHashtable(void);
void freeArguments(char**);
int readline(char*, FILE*);
struct historyNode* getSetHistHead(void);
int findFile(char*);
int cmpExtension(const char*, const char*);
int stringToInt(char*);
int skipSpaces(char*, int*);
int getToken(const char*, char*, int*);


// Functions regarding memory
int memDump(char**, int);
int memEdit(char**, int);
int memFill(char**, int);
int memReset(void);

// opcode functions
int opInsert(void);
int opMnem(char**);
struct opcodeNode* opSearch(char*, int);
int opPrintOpcodelist(void);
int hashcode(char*, int);

// general shell command function
int funcHelp(void);
int funcDir(void);
int funcQuit(void);
int funcHistory(char*);
int funcType(char*);

// assembler functions
int asmAssemble(char*);
int asmFirstPass(char*);
int asmSecondPass(char*);
void initializeASM(int);
int asmSymTabInsert(char*, int, int);

#endif
