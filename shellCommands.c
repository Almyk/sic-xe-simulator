#include "20162004.h"

/* This file holds all the basic shell functions:
 * Help
 * Dir
 * History
 * Quit
 */


int funcHelp(void){
    // prints the commands
    printf("h[elp]\n");
    printf("d[ir]\n");
    printf("q[uit]\n");
    printf("hi[story]\n");
    printf("du[mp] [start, end]\n");
    printf("e[dit] address, value\n");
    printf("f[ill] start, end, value\n");
    printf("reset\n");
    printf("opcode mnemonic\n");
    printf("opcodelist\n");
    printf("assemble filename\n");
    printf("type filename\n");
    printf("symbol\n");
    return 1;
}

int funcDir(void){
    DIR *dirp;
    struct dirent *dir;
    struct stat sb;


    dirp = opendir(".");
    if(dirp){
        // reading current directory
        while((dir = readdir(dirp)) != NULL){
            // does not print current(.) and the previous(..) directory
            if(strcmp(dir->d_name, "..") && strcmp(dir->d_name, ".")){
                // if directory
                if(dir->d_type == DT_DIR){
                    printf("%s/  ", dir->d_name);
                }
            // if not a directory
            // print a '*' for executables and a space for nonexecutables.
            // stat() used the sb structure to return information about the
            // file and then compares it with the library constant to see if
            // it is an executable or not. It is compared using the bitwise
            // operator AND to see if there is a bit matching, meaning the 
            // file have executable rights
                else{
                    printf("%s%c  ", dir->d_name, stat(dir->d_name, &sb) == 0 && sb.st_mode & S_IXUSR ? '*' : ' ');
                }
            }
        }
        closedir(dirp);
    }
    else{
        printf("error occurred while opening folder\n");
        //exit(EXIT_FAILURE);
        return -2;
    }
    printf("\n");
    return 1;
}

int funcQuit(void){
    return 0;
}

int funcHistory(char *command){
    struct historyNode* ptr = getSetHistHead();
    // only print history when there are at least 1 command saved
    // if there is no command saved, then this history command
    // should not be printed
    if(ptr->n < 1) return 1;
    addHistory(command);
    ptr = ptr->next;
    while(ptr){
        printf("%d\t%s\n", ptr->n, ptr->name);
        ptr = ptr->next;
    }
    return 2;
}

int funcType(char** args){
    DIR *dirp;
    struct dirent *dir;
    FILE *fp;
    char buffer[MAXBUF] = "";
    int found = 0;

    dirp = opendir(".");

    while(!found && (dir = readdir(dirp)) != NULL){
        if(!(strcmp(dir->d_name, args[1]))){
            found = 1;
        }
    }
    if(found){
        printf("\n");
        fp = fopen(args[1], "r");
        while(readline(buffer, fp)){
            printf("\t%s\n", buffer);
        }
        printf("\n");
    }
    else{
        printf("Error: File not found.\n");
        return -2;
    }

    closedir(dirp);
    return 1;
}
