#ifndef _MAILBOX_
#define _MAILBOX_

#include "Connection.h"
#include "SharedFunc.h"
#include "Commands.h"
#include "Send.h"

#define DATA_CHUNK_SIZE 496
#define FILE_DATA_CHUNK_SIZE 496

#define MTT_MSG 1
#define MTT_ACK_MSG 2
#define MTT_OK 3
#define MTT_KO 4

#define MTT_NEW_FILE 5
#define MTT_DATA_FILE 6
#define MTT_END_FILE 7

#define MTT_OK_FILE 8
#define MTT_KO_FILE 9
#define MTT_CANCEL_FILE 10

// Struct used to send msg to the mailbox.
typedef struct{
    long idtrama;
    char origPort[6];
    char destPort[6];
    char type;
    char data[DATA_CHUNK_SIZE];
} MsgTrama;

// Struct used to send file to the mailbox.
typedef struct{
    long idtrama;
    char destPort[6];
    char originUserName[256];
    char hash[33];
    char fileName[256];
    int size;
    int fullSize;
    char type;
    char data[FILE_DATA_CHUNK_SIZE];
} FileTrama;

// Struct used add arguments in mailbox recive thread.
typedef struct{
    FileTrama file_mailbox_trama;
    IluvatarConf *config;
    int file_mailbox;
    Client **clients;
    int nclients;
} ThreadArgMailbox;

/*
* Function used to show the msg trama data.
*/
void MAILBOX_showTrama(MsgTrama trama);

/*
* Funcion used to send msg to the mailbox.
*/
void MAILBOX_sendMsg(char **substrings, int nsplits, char *pid, IluvatarConf *config, int mailbox, int socketFD);

/*
* Send a file to the destination using the mailbox.
*/
void MAILBOX_sendFile(char **substring, char *portid, IluvatarConf *config, int mailbox);

/*
* This function will recive a msg.
*/
void MAILBOX_msgRecived(MsgTrama mailbox_trama, IluvatarConf *_config, int _mailbox);

/*
*  This fucntion will be called by a thread to recive the file.
*/
void * MAILBOX_reciveFile(void* args);

#endif