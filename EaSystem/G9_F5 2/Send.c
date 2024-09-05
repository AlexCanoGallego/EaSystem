#include "Send.h"

Client **clients;       // Llista de clients
int nClients;           // Number of Clients
int clientFD;           // FD from the clients

/**
 * Function used to check if the message format is correct.
*/
int SEND_checkMSGFormat(char **substring, int nsplits){
    int x=0;
    char *msg=NULL;

    for(int i = 3; i < nsplits; i++){
        char *tmp = msg;
        asprintf(&msg, "%s%s ", tmp, substring[i]);
        free(tmp);
    }
    
    for(int i = 0; i < (int) strlen(msg); i++){
        if(msg[i] == '"') x++;
    }
    free(msg);

    if(x != 2){
        printF(ERROR_MSG);
        return 0;
    }
    return 1;
}

/**
 * Function used to check if the file exists
 * and if it has an extension.
*/
int SEND_checkFile(char* file_name, IluvatarConf *config){
    // Set the file and directory path
    char *temp_file_dir = SHAREDFUNC_readString(config->files_dir, '\0', 1); // Remove the first '/'
    char *file_dir;
    asprintf(&file_dir, "%s/%s", temp_file_dir, file_name);

    // 1. Check if file has an extension
    // Find the last '.' character in the string
    char* extension = strrchr(file_name, '.');
    // If the string does not contain a '.' character 
    // or the '.' character is the last character in the string
    if (extension == NULL || extension[1] == '\0') {
        printF(ERROR_FILE_EXTENSION);
        free(temp_file_dir);
        free(file_dir);
        return 0;
    }

    // 2. Check if file exists
    // Check if file exists
    int file = open(file_dir, O_RDONLY);
    if(file <= 0){
        printF(ERROR_FILE);
        free(temp_file_dir);
        free(file_dir);
        return 0;
    }
    close(file);
    free(temp_file_dir);
    free(file_dir);

    return 1;
}

/**
* Function that send the "Trama NEW_MSG" to Arda.
*/
void SEND_countMSG(int socketFD, IluvatarConf *config){
    Trama *trama = CONNECTION_setTramaTypeHeader(TT_COUNT_MESSAGE, TH_NEW_MSG);  // Set Type & Header
    asprintf(&trama->data, "%s", config->user_name);
    CONNECTION_sendTrama(trama, socketFD, -1);
    CONNECTION_freeTrama(trama);
}


/**
* Function used to send a file to other Iluvatar client.
* In case mailbox=1, the file will be sent to the mailbox.
* In case mailbox=-1, the file will be sent using sockets.
*/
void SEND_sendFile(char **substring, int nsplits, int iluvatarFD, IluvatarConf *config, int mailbox){
    if(mailbox == -1){
        // 1. Send file info
        if (SEND_sendFileMeta(substring, nsplits, config, iluvatarFD) == -1){
            return;

        }
        // 2. Send file
        SEND_sendFileData(substring[3], config, iluvatarFD);
        
        // 3. Wait for the ACK
        char c;         
        if(read(iluvatarFD, &c, sizeof(char)) != 0){ 
            Trama *test = CONNECTION_getTrama(c, iluvatarFD);
            if(strcmp(test->header, TH_CHECK_OK) == 0){
                printF("File correctly sent\n");
            }else{
                printF("File failed to sent\n");
            }
            CONNECTION_freeTrama(test);
        }
    }else{
        return;
    }


}

/**
* Function used to send a message to other Iluvatar client.
*/
void SEND_sendMsg(char **substring, int nsplits, int iluvatarFD, IluvatarConf *config, int mailbox, int socketFD){

    
    char *msg;
    asprintf(&msg, "%s&", config->user_name);
    for(int i = 3; i < nsplits; i++){
        char *tmp = msg;
        asprintf(&msg, "%s%s ", tmp, substring[i]);
        free(tmp);
    }
    
    Trama *trama = CONNECTION_setTramaTypeHeader(TT_SEND_MSG, TH_MSG);  // Set Type & Header
    asprintf(&trama->data, "%s", msg);

    if(mailbox == -1){
        // Send the message using socket
        CONNECTION_sendTrama(trama, iluvatarFD, -1);
    }

    CONNECTION_freeTrama(trama);
    free(msg);

    // Wait for the ACK   
    char c;         
    if(read(iluvatarFD, &c, sizeof(char)) != 0){ 
        Trama *test = CONNECTION_getTrama(c, iluvatarFD);
        if(strcmp(test->header, TH_MSGOK) == 0){
            printF(MESSAGE_SENT);
            SEND_countMSG(socketFD, config);
        }else{
            printF("Error sending the message\n");
        }
        CONNECTION_freeTrama(test);
    }
}

/**
 * Function used to send the file info to other Iluvatar client.
*/
int SEND_sendFileMeta(char **substring, int nsplits, IluvatarConf *config, int iluvatarFD){
    char *data = NULL;

    if(data == NULL) asprintf(&data, "%s&", config->user_name);
    for(int i = 3; i < nsplits; i++){
        char *tmp = data;
        asprintf(&data, "%s%s&", tmp, substring[i]);
        free(tmp);
    }

    // Set the file and directory path
    char *temp_file_dir = SHAREDFUNC_readString(config->files_dir, '\0', 1); // Remove the first '/'
    char *file_dir;
    asprintf(&file_dir, "%s/%s", temp_file_dir, substring[3]);

    // Calculate hash and size of size
    char *full_md5sum=COMMANDS_executeLinuxCommand(file_dir, "md5sum", NULL);
    char *full_wc=COMMANDS_executeLinuxCommand(file_dir, "wc", "-c");
    if(full_md5sum == NULL || full_wc == NULL){
        printF(ERROR_LINUX_COMMANDS);
        free(data);
        free(file_dir);
        free(temp_file_dir);
        return -1;
    }

    // Clean the hash and size string output
    char *hash = SHAREDFUNC_readString(full_md5sum, ' ', 0);
    char *size = SHAREDFUNC_readString(full_wc, ' ', 0);
    free(temp_file_dir);
    free(full_md5sum);
    free(file_dir);
    free(full_wc);

    // Add hash and size to data
    char *tmp = data;
    asprintf(&data, "%s%s&%s", tmp, size, hash);
    free(tmp);
    free(hash);
    free(size);
    
    // Add the data to the trama and send it
    Trama *trama = CONNECTION_setTramaTypeHeader(TT_SEND_FILE, TH_NEW_FILE);  // Set Type & Header
    trama->data = data;
    CONNECTION_sendTrama(trama, iluvatarFD, -1);
    CONNECTION_freeTrama(trama);

    return 1;
}

/**
 * Function used to send the file data to other Iluvatar client.
*/
void SEND_sendFileData(char *file_name, IluvatarConf *config, int iluvatarFD){
    // Set the file and directory path
    char *temp_file_dir = SHAREDFUNC_readString(config->files_dir, '\0', 1); // Remove the first '/'
    char *file_dir;
    asprintf(&file_dir, "%s/%s", temp_file_dir, file_name);
    free(temp_file_dir);


    // Open the file
    int file = open(file_dir, O_RDONLY);
    if(file <= 0){
        printF(ERROR_OPENING_FILE);
        return;
    }

    int fi=0;   // Flag to know if the file is finished

    // Read the file until the end
    while (fi == 0){
        // Create new data_chunk to send
        char *data_chunk = malloc(DATA_CHUNK_SIZE * sizeof(char)); // dat_chunk size = 496
        char c = 0;
        int i = 0;
        int size = 0;

        while (i < DATA_CHUNK_SIZE-1) { // -1 because of the '\0'
            size = read(file, &c, sizeof(char));
            if(size == 0){
                data_chunk[i] = '\0';
                fi=1;
                break;
            }
            data_chunk[i++] = c;
        }
        data_chunk[i] = '\0';

        // Create the trama and send it
        Trama *trama = CONNECTION_setTramaTypeHeader(TT_SEND_FILE, TH_FILE_DATA);  // Set Type & Header
        trama->data = data_chunk;
        CONNECTION_sendTrama(trama, iluvatarFD, i);
        CONNECTION_freeTrama(trama);
    }

    close(file);
    free(file_dir);
}


/**
 * Function used to recive the file info from other Iluvatar client
 * and return the MetaFile struct.
*/
MetaFile * SEND_reciveFileMeta(Trama *trama_recived){
    MetaFile *meta_file = (MetaFile*)malloc(sizeof(MetaFile));

    // Get the data from the trama

    // 1. Get the origin user
    meta_file->originUser = SHAREDFUNC_readString(trama_recived->data, '&', 0);
    int start = (int)strlen(meta_file->originUser) + 1;
    // 2. Get the file name
    meta_file->fileName = SHAREDFUNC_readString(trama_recived->data, '&', start);
    start += (int)strlen(meta_file->fileName) + 1;
    // 3. Get the file size
    char *aux_size = SHAREDFUNC_readString(trama_recived->data, '&', start);
    meta_file->fileSize = atoi(aux_size);
    start += (int)strlen(aux_size) + 1;
    free(aux_size);

    // 4. Get the file hash
    meta_file->fileHash = SHAREDFUNC_readString(trama_recived->data, '\0', start);

   return meta_file;
}

/**
 * Function used to send the file data to other Iluvatar client.
*/
void SEND_reciveFileData(MetaFile *meta_file, int clientFD, char *files_dir){
    char c;
    int sum_size=0;

    // Set the right file path of the file
    char *temp_file_dir = SHAREDFUNC_readString(files_dir, '\0', 1); // Remove the first '/'
    char *file_dir;
    asprintf(&file_dir, "%s/%s", temp_file_dir, meta_file->fileName);

    // 1. Create the file
    int fd = open(file_dir, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        printF("Error: File could not be created\n");
        free(temp_file_dir);
        free(file_dir);
        return;
    }

    // 2. Read the data while the total size of the MetaFile 
    // struct is not reached.
    while(1){
        // 1. Get the trama
        if(read(clientFD, &c, sizeof(char)) != 0){
            Trama *trama_recived = CONNECTION_getTrama(c, clientFD);
            sum_size += trama_recived->lenght;
            // 2. Write the data to the file
            write(fd, trama_recived->data, trama_recived->lenght);
            // 3. Free the trama
            CONNECTION_freeTrama(trama_recived);
            if(sum_size == meta_file->fileSize)break;
        }
    }

    close(fd); // Close the file
    free(temp_file_dir);
    free(file_dir);
}

/**
* This function is used to check the MD5SUM of the file,
* and compare it with the one recived, 
* to know if the file was recived correctly.
* Return 1 if it was recived correctly, 0 otherwise.
*/
int SEND_checkRecivedFileHash(char *dir_name, MetaFile *meta_file){
    // 1. Get the MD5SUM of the file
    char *file_dir;
    char *temp_file_dir = SHAREDFUNC_readString(dir_name, '\0', 1); // Remove the first '/'
    asprintf(&file_dir, "%s/%s", temp_file_dir, meta_file->fileName);
    char *full_md5sum=COMMANDS_executeLinuxCommand(file_dir, "md5sum", NULL);
    char *md5sum = SHAREDFUNC_readString(full_md5sum, ' ', 0);
    free(full_md5sum);
    free(file_dir);
    free(temp_file_dir);

    // 2. Compare the MD5SUM of the file with the one recived
    if(strcmp(md5sum, meta_file->fileHash) == 0){
        free(md5sum);
        return 1;
    }else{
        free(md5sum);
        return 0;
    }
}

/**
* This function is used to recive a file from 
* other Iluvatar.
*/
void SEND_reciveFile(int clientFD, Trama *trama_recived, char* dir_name){

    // 1. Get the meta data of the file
    MetaFile *meta_file = SEND_reciveFileMeta(trama_recived);

    // 2. Get the file data
    SEND_reciveFileData(meta_file, clientFD, dir_name);

    // 3. Check the MD5SUM of the file
    Trama *trama_ack;
    if(SEND_checkRecivedFileHash(dir_name, meta_file)){
        // File was recived correctly
        trama_ack = CONNECTION_setTramaTypeHeader(TT_MD5, TH_CHECK_OK);
    }else{
        // File was not recived correctly
        trama_ack = CONNECTION_setTramaTypeHeader(TT_MD5, TH_CHECK_KO);
    }

    SEND_showFileMeta(meta_file); // When the file is recived, show the recive message
    CONNECTION_freeMetaFile(meta_file);

    // 4. Send the ACK
    trama_ack->data = NULL;
    CONNECTION_sendTrama(trama_ack, clientFD, -1);
    CONNECTION_freeTrama(trama_ack);
}

/**
* This function is used to recive a msg from
* other Iluvatar.
*/
void SEND_reciveMsg(int clientFD, Trama *trama_recived){
    // Get and show MSG
    Message *message_recived = CONNECTION_dataToMessage(trama_recived->data);
    SEND_showMsg(message_recived, 1);
    CONNECTION_freeMessage(message_recived);

    // Send MSG ACK
    Trama *trama_ack = CONNECTION_setTramaTypeHeader(TT_SEND_MSG, TH_MSGOK);  // Set Type & Header
    trama_ack->data = NULL;
    CONNECTION_sendTrama(trama_ack, clientFD, -1);
    CONNECTION_freeTrama(trama_ack);
}

/**
* This function is used to show a "$" message in the terminal.
*/
void printDollar(){
    char *buf;
    asprintf(&buf, "%s$ %s",COLOR_BLUE, COLOR_RESET);
    write(STDOUT_FILENO, buf, strlen(buf));
    free(buf);
    write(1, COLOR_GREEN, strlen(COLOR_GREEN));
}


/**
* This function is used called from a thread to recive a 
* file or message from other Iluvatar.
*/
void * SEND_reciveFromIluvatar(void* args){
    ThreadArgs threadArgs = *((ThreadArgs*)args);
    clientFD = threadArgs.clientFD;
    clients = threadArgs.clientList;
    nClients = threadArgs.nClients;
    
    char c;

    // Wait for the first trama
    if(read(clientFD, &c, sizeof(char)) != 0){ 
        Trama *trama_recived = CONNECTION_getTrama(c, clientFD); // Get trama from Iluvatar
        if(strcmp(trama_recived->header, TH_MSG) == 0){ 
            SEND_reciveMsg(clientFD, trama_recived);
        }else if(trama_recived->type == TT_SEND_FILE /*&& (strcmp(trama_recived->header, TH_NEW_FILE) == 0)*/){
            SEND_reciveFile(clientFD, trama_recived, threadArgs.dirName);
        }
        CONNECTION_freeTrama(trama_recived); // Free first trama recived
    }

    close(clientFD); // Closes this client FD
    printDollar();
    // Finish thread used to recive messages or files
    pthread_cancel(pthread_self()); // Kills the thread
    pthread_detach(pthread_self()); // Frees the thread memory

    return NULL;
}

/**
 * This function is used to show a message in the terminal.
 * If i == 1, the message is from same machine.
 * If i == 0, the message is from other machine.
*/
void SEND_showMsg(Message *message_recived, int i){
    char *output;
    char *ip = "Unknown";

    for(int i = 0; i < nClients; i++){
        if(strcmp(clients[i]->name, message_recived->originUser) == 0){
            ip = clients[i]->ip;
        }
    }

    write(1, COLOR_RESET, strlen(COLOR_RESET));
    if(i == 1){
        asprintf(&output, "\n\nNew message received!\n%s, from %s\nsays:\n%s\n\n", message_recived->originUser, ip, message_recived->message);
    }else{
        asprintf(&output, "\n\nNew message received!\nYour neighbor %s says:\n%s\n\n", message_recived->originUser, message_recived->message);
    }
    write(STDOUT_FILENO, output, strlen(output));
    free(output);
}

/**
* This function is used to show a file in the terminal.
*/
void SEND_showFileMeta(MetaFile *meta_file){
    char *output;
    char *ip = "Unknown";
    
    for(int i = 0; i < nClients; i++){
        if(strcmp(clients[i]->name, meta_file->originUser) == 0){
            ip = clients[i]->ip;
        }
    }

    write(1, COLOR_RESET, strlen(COLOR_RESET));
    asprintf(&output, "\n\nNew file received!\n%s, from %s has\nsent:\n%s\n\n", meta_file->originUser, ip, meta_file->fileName);
    write(STDOUT_FILENO, output, strlen(output));
    free(output);
}



