#ifndef _SEND_
#define _SEND_

#include "Connection.h"
#include "SharedFunc.h"
#include "Commands.h"

#define DATA_CHUNK_SIZE 496
// Trama = 512Bytes
// Type = 1Byte
// Header = 1Byte(TT_SEND_FILE) + 12Bytes(TH_FILE_DATA)
// Length = 2Bytes
// Data = 496Bytes to use

/**
 * Function used to check if the message format is correct.
*/
int SEND_checkMSGFormat(char **substring, int nsplits);

/**
 * Function used to check if the file exists
 * and if it has an extension.
*/
int SEND_checkFile(char* file_name, IluvatarConf *iluvatarConf);

/**
* Function used to send a file to other Iluvatar client.
*/
void SEND_sendFile(char **substring, int nsplits,  int clientFD, IluvatarConf *config, int mailbox);

/**
* Function used to send a message to other Iluvatar client.
*/
void SEND_sendMsg(char **substring, int nsplits, int iluvatarFD, IluvatarConf *config, int mailbox, int socketFD);


/**
 * Function used to send the file info to other Iluvatar client.
*/
int SEND_sendFileMeta(char **substring, int nsplits, IluvatarConf *config, int clientFD);

/**
 * Function used to send the file data to other Iluvatar client.
*/
void SEND_sendFileData(char *file_name, IluvatarConf *config, int clientFD);

/**
* This function is used to recive a file from 
* other Iluvatar.
*/
void SEND_reciveFile(int clientFD, Trama *trama_recived, char* dir_name);

/**
* This function is used to recive a msg from
* other Iluvatar.
*/
void SEND_reciveMsg(int clientFD, Trama *trama_recived);

/**
* This function is used called from a thread to recive a 
* file or message from other Iluvatar.
*/
void * SEND_reciveFromIluvatar(void* tmpFD);

/**
 * This function is used to show a message in the terminal.
*/
void SEND_showMsg(Message *message_recived, int i);

/**
* This function is used to show a file in the terminal.
*/
void SEND_showFileMeta(MetaFile *meta_file);

/**
* This function is used to connect to other Iluvatar
* and get the client iluvatarFD.
*/
int SEND_connectToOtherIluvatar(int i);

/**
* This function is used to recive the file metadata from other Iluvatar client.
*/
MetaFile * SEND_reciveFileMeta(Trama *trama_recived);

/**
* Function that send the "Trama NEW_MSG" to Arda.
*/
void SEND_countMSG(int socketFD, IluvatarConf *config);

#endif