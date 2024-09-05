#ifndef _CONNECTION_
#define _CONNECTION_ 

#include "SharedFunc.h"

// Trama Types
#define TT_NEW_CONNECTION 1
#define TT_UPDATE_LIST 2
#define TT_SEND_MSG 3
#define TT_SEND_FILE 4
#define TT_MD5 5
#define TT_EXIT 6
#define TT_ERROR 7
#define TT_COUNT_MESSAGE 8

// Trama Set Headers
#define THS_NEW_SON 1
#define THS_CONOK 2
#define THS_CONKO 3
#define THS_LIST_REQUEST 4
#define THS_LIST_RESPONSE 5
#define THS_MSG 6
#define THS_MSGOK 7
#define THS_NEW_FILE 8
#define THS_FILE_DATA 9
#define THS_CHECK_OK 10
#define THS_CHECK_KO 11
#define THS_NEW_MSG 12
#define THS_EXIT 13
#define THS_UNKNOWN 14

// Trama Headers
#define TH_NEW_SON "[NEW_SON]"
#define TH_CONOK "[CONOK]"
#define TH_CONKO "[CONKO]"
#define TH_LIST_REQUEST "[LIST_REQUEST]"
#define TH_LIST_RESPONSE "[LIST_RESPONSE]"
#define TH_MSG "[MSG]"
#define TH_MSGOK "[MSGOK]"
#define TH_NEW_FILE "[NEW_FILE]"
#define TH_FILE_DATA "[FILE_DATA]"
#define TH_CHECK_OK "[CHECK_OK]"
#define TH_CHECK_KO "[CHECK_KO]"
#define TH_NEW_MSG "[NEW_MSG]"
#define TH_EXIT "[EXIT]"
#define TH_UNKNOWN "[UNKNOWN]"

// Messages
#define TRAMA_ERROR "\nTRAMA REBUDA INCORRECTE...\n"
#define TRAMA_NOT_DONE "\nTRAMA REBUDA NO IMPLEMENTADA...\n"

// Structs
typedef struct{
    char* name;
    char* ip;
    char* port;
    char* pid;
    int connected;
    int fd;
    pthread_t id_thread;
} Client;

// TRAMA'S STRUCT
typedef struct{
    char type;
    char* header;
    short lenght;
    char* data;
} Trama;

// MESSAGE'S STRUCT
typedef struct{
    char* originUser;
    char* message;
} Message;

// METAFILE'S STRUCT
typedef struct{
    char *originUser;
    char *fileName;
    int fileSize;
    char *fileHash;
} MetaFile;

// THREAD'S STRUCT
typedef struct{
    int clientFD;
    char *dirName;
    Client **clientList;
    int nClients;
} ThreadArgs;


/**
 * Function that sends a Trama through a socketFD
 * using seralization.
 * In case of size != -1, the lenght of the data is set to size.
 * In case of size == -1, the lenght of the data is set to strlen(trama->data).
*/
void CONNECTION_sendTrama(Trama *trama, int socketFD, short size);

/**
 * Function that returns a Trama with
 * the Type and Header requested.
*/
Trama * CONNECTION_setTramaTypeHeader(char type, char* header);

/**
 * Function that reads and saves in dynamic memory 
 * an incoming trama.
*/
Trama * CONNECTION_getTrama(char c, int clientFD);

/**
 * Function that returns a Client with the data
 * from a data Trama.
*/
Client * CONNECTION_dataToClient(char *trama_data);

/**
 * Function that returns a Cliets array
 * from the trama data.
*/
Client ** CONNECTION_dataToClients(char *trama_data, int *nClients);

/**
 * Function that prints the information of a Trama.
*/
void CONNECTION_showTrama(Trama *trama);

/**
 * Function that prints the information of a Client.
*/
void CONNECTION_showClient(Client *client);

/**
 * Function useful for debugging that shows the list of clients
 * stored in the program.
*/
void CONNECTION_showClientsList(Client **clients, int nClients);

/**
 * Function that frees the memory of a Trama.
*/
void CONNECTION_freeTrama(Trama *trama);

/**
 * Function that frees the memory of a Client.
*/
void CONNECTION_freeClient(Client *client);

/**
 * Function that frees the memory of n Client.
*/
void CONNECTION_freeClients(Client **client, int nClients);

/**
 * Function that returns a Message with the data
 * from a data Trama.
*/
Message * CONNECTION_dataToMessage(char *trama_data);

/**
 * Function that frees the memory of a Message.
*/
void CONNECTION_freeMessage(Message *message);

/**
 * Function that frees the memory of a MetaFile.
*/
void CONNECTION_freeMetaFile(MetaFile *meta_file);

/**
 * Function that returns 1 if the directory exists,
 * 0 if not.
*/
int COMMANDS_checkDirectory(char *directory);

#endif