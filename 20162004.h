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

// Virtual Memory
extern unsigned char MEMORY[MEGA];

// Hash table
extern struct opcodeNode* HASHTABLE[HASHSIZE];

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
int hashcode(char*);

// general shell command function
int funcHelp(void);
int funcDir(void);
int funcQuit(void);
int funcHistory(char*);
int funcType(char**);

#endif
