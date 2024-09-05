#ifndef _SHARED_FUNC_
#define _SHARED_FUNC_ 

//define gnu_source
#define _GNU_SOURCE
#define printF(x) write(1, x, strlen(x))

//includes
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <time.h>
#include <pthread.h>
#include <sys/msg.h>
#include <sys/stat.h>




// Colors defines
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_RESET   "\x1b[0m"

// Error defines
#define ERROR_ARGS "Error, wrong number of arguments!\n"
#define ERROR_FILE_NO_OPEN "Error while opening configuration file!\n"
#define ERROR_FORK "Error ocurred while creating a subrprocess!\n"
#define EXIT_MSG "Disconnecting from Arda. See you soon, son of Iluvatar\n\n"
#define ERROR_SOCKET "Error creating the socket\n"
#define ERROR_IP "Error configuring IP\n"
#define ERROR_ARDA "Error connecting with Arda\n"
#define ERROR_DISCONECTION "Disconnection request denied by Arda.\n"
#define ERROR_ILUVATAR_SERVER "Error connecting with Iluvatar's server\n"
#define ERROR_SENDING "Wait until the file is sent!\n"
#define USER_NOT_FOUND "User not found\n\n"
#define ERROR_BIND "Error doing the bind\n"
#define ERROR_LISTEN "Error doing the listen\n"
#define ERROR_MSG "Error, incorrect message format\n"
#define ERROR_FILE_EXTENSION "Error, file don't have extension\n"
#define ERROR_FILE "Error, file don't exists\n"
#define FILE_FAIL "File failed to sent\n"
#define ERROR_LINUX_COMMANDS "Error, executing the linux commands\n"
#define ERROR_OPENING_FILE "Error opening the file\n"
#define DISCONNECTING_ARDA "\nDisconnecting Arda from all Iluvatar’s children\n"
#define CLOSE_SERVER "Closing server\n"
#define ARDA_READING_FILE "\nARDA SERVER\nReading configuration file\nWaiting for connections...\n\n"

// Messages defines
#define UNKNOWN_COMMAND "Unknown command\n"
#define USERS_LIST_UPDATED "Users list updated\n"
#define MESSAGE_SENT "Message correctly sent\n"
#define FILE_SENT "File correctly sent\n"
#define HELPER "\nMaybe you want say:\n\t-SEND MSG\n\t-SEND FILE\n\n"
#define LIST_USERS "There are 4 children of Iluvatar connected:\n\
1. Galadriel 172.16.205.3    8020    matagalls     2379\n\
2. Isildur   172.16.205.17   9015    puigpedros   16619\n\
3. Arondir   172.16.205.17   9035    puigpedros   16620\n\
4. Elrond    172.16.205.4    9435    montserrat   16649\n\
5. Miriel    172.16.205.4    9535    montserrat   16619\n\n"

// Commands defines
#define OPT_UPDATE_USERS "UPDATE USERS"
#define OPT_LIST_USERS "LIST USERS"
#define OPT_SEND_MSG "SEND MSG"
#define OPT_SEND_FILE "SEND FILE"
#define OPT_EXIT "EXIT"
#define NEXT_LINE "\n"

// Size defines
#define MAX_BUFFER  300
#define MAX_SEND_TYPE  10


// PORT'S STRUCT
typedef struct{
    char* ip;
    char* port;
} IpPort;

// ILUVATAR'S STRUCT
typedef struct{
    char* user_name;
    char* files_dir;
    IpPort* arda_server;
    IpPort* comun_server;
} IluvatarConf;

// ARDA'S STRUCT
typedef struct {
    IpPort* server;
    char* files_dir;
} ArdaServer;

/**
 * Function that reads a line from a file until it finds the final character,
 * in addition, in case it finds a '&' it omits it.
*/
char* SHAREDFUNC_readUntil(int fd, char end);

/**
 * Function that returns the number of "del" inside a string.
*/
int SHAREDFUNC_nChar(char buffer[MAX_BUFFER], char del);

/**
 * Temporal function for testing.
*/
char* SHAREDFUNC_readLine(int fd, char delimiter);

/**
 * Function that parses the command and saves it by substrings.
*/
char** SHAREDFUNC_splitFunc(char *buffer,int *nsplits, char del);

/**
 * Function that parses a string indicating the byte to start as well as the char to finish.
*/
char* SHAREDFUNC_readString(char *buffer, char del, int start);

/*
 * Function that prints a dollar sign. 
*/
void SHAREDFUNC_printDollar();


#endif