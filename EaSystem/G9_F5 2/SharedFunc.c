#include "SharedFunc.h"

/**
 * Function that reads a line from a file until it finds the final character,
 * in addition, in case it finds a '&' it omits it.
*/
char* SHAREDFUNC_readUntil(int fd, char end) {
    char* string = NULL;
    int i=0, size;
    char c = '\0';

    while(1){
        size = read(fd, &c, sizeof(char));
        if(string ==  NULL) string = (char*)malloc(sizeof(char));
        if (c != end && size > 0 && c != '&') {
            string = (char*)realloc(string, sizeof(char) * (i + 2));
            string[i++] = c;
        } else if (c != '&'){
            break;
        }
    }
    string[i] = '\0';
    return string;
}

/**
 * Function that returns the number of "del" inside a string.
*/
int SHAREDFUNC_nChar(char buffer[MAX_BUFFER], char del){       
    int n=0,i=0;
    if((int)strlen(buffer)<=1) return 0;
    while (buffer[i] != '\0'){
        if(buffer[i] == del) n++;
        i++;
    }
    return n+1;
}

/**
 * Temporal function for testing.
*/
char* SHAREDFUNC_readLine(int fd, char delimiter) {
    char *msg = (char*)malloc(sizeof(char));
    char current;
    int i=0;
    int len=0;

    while((len+= read(fd, &current, sizeof(char))) > 0){
        msg[i] = current;
        msg = (char *) realloc(msg, ++i + (sizeof(char)));
        if(current == delimiter) break;
    }

    msg[i] = '\0';

    return msg;
}

/**
 * Function that parses the command and saves it by substrings.
*/
char** SHAREDFUNC_splitFunc(char *buffer,int *nsplits, char del){     
    int n = SHAREDFUNC_nChar(buffer, del);
    char **substrings;
    int i=0;

    substrings = (char**) malloc (sizeof(char *) * (n+2));
    substrings[i] = strtok(buffer, " ");

    while(substrings[i] != NULL) {
        i++;
        substrings[i] = strtok(NULL, " ");
    }

    substrings[i]=NULL;

    *nsplits=i;
    return substrings;
}

/**
 * Function that parses a string indicating the byte to start as well as the char to finish.
*/
char* SHAREDFUNC_readString(char *buffer, char del, int start){
    char *string = NULL;
    int j = 0;

    for(int i = start; i < (int) strlen(buffer); i++){
        if(buffer[i] == del){
            break;
        } else{
            if(string == NULL) string = (char*)malloc(sizeof(char));
            else string = (char*)realloc(string, sizeof(char) * (j + 2));

            string[j] = buffer[i];
            j++;
        }
    }

    string[j] = '\0';
    return string;
}

/*
* Private function to print the dollar sign.
*/ 
void SHAREDFUNC_printDollar(){
    char *buf;
    asprintf(&buf, "%s$ %s",COLOR_BLUE, COLOR_RESET);
    write(STDOUT_FILENO, buf, strlen(buf));
    free(buf);
    write(1, COLOR_GREEN, strlen(COLOR_GREEN));
}