#include "Mailbox.h"


/*
* Function used to show the msg trama data.
*/
void MAILBOX_showTrama(MsgTrama mailbox_trama){
    char *output;
    write(1, COLOR_RESET, strlen(COLOR_RESET));
    asprintf(&output, "data: %s\nidtrama: %ld\ntype: %d\n\n", mailbox_trama.data, mailbox_trama.idtrama, mailbox_trama.type);
    write(STDOUT_FILENO, output, strlen(output));
    free(output);
}

/*
* Funcion used to create and send msg to the mailbox.
*/
void MAILBOX_sendMailboxTrama(int idtrama, char *origPort, char *destPort, char type, char *data, int mailbox){
    MsgTrama mailbox_trama;
    memset(&mailbox_trama, 0, sizeof(MsgTrama));    
    mailbox_trama.idtrama = idtrama;                    // Set the idtrama (unused)
    strcpy(mailbox_trama.destPort, destPort);           // Set the destination port
    strcpy(mailbox_trama.origPort, origPort);           // Set the origin port
    mailbox_trama.type = type;                          // Set the type
    strcpy(mailbox_trama.data, data);                   // Set the msg data
    // Send the trama using msg mailbox
    msgsnd(mailbox, &mailbox_trama, sizeof(MsgTrama) - sizeof(long), IPC_NOWAIT);
}

/*
* Funcion used to create and send file to the mailbox.
*/
FileTrama MAILBOX_sendFileMailboxTrama(int idtrama, char *destPort, char *hash, char *name, char type, char *data, int mailbox, int size, int fullSize, char *originUserName){
    FileTrama mailbox_trama;
    memset(&mailbox_trama, 0, sizeof(FileTrama)); 
    mailbox_trama.idtrama = idtrama;
    strcpy(mailbox_trama.destPort, destPort);
    strcpy(mailbox_trama.hash, hash);
    strcpy(mailbox_trama.fileName, name);
    mailbox_trama.type = type;
    strcpy(mailbox_trama.data, data);
    mailbox_trama.size = size;
    mailbox_trama.fullSize = fullSize;
    strcpy(mailbox_trama.originUserName, originUserName);
    // Send the trama using file mailbox
    msgsnd(mailbox, &mailbox_trama, sizeof(FileTrama) - sizeof(long), IPC_NOWAIT);
    return mailbox_trama;
}


/*
* Funcion used to send msg to the mailbox.
*/
void MAILBOX_sendMsg(char **substring, int nsplits, char *portid, IluvatarConf *config, int mailbox, int socketFD){
    // Create the msg data
    char *msg;
    asprintf(&msg, "%s&", config->user_name);
    for(int i = 3; i < nsplits; i++){
        char *tmp = msg;
        asprintf(&msg, "%s%s ", tmp, substring[i]);
        free(tmp);
    }

    // Send msg
    MAILBOX_sendMailboxTrama(
    atoi(portid),               // Unused
    config->comun_server->port, // Origin port the reciver of the MSG
    portid,                     // Destination port the reciver of the MSG
    MTT_MSG,                    // Type of the trama (MSG)
    msg,                        // Data of the trama
    mailbox);                   // Mailbox to send the trama

    free(msg);

    SEND_countMSG(socketFD, config);
}

/*
* Funcion used to send file metadata to the mailbox 
* with the destination port.
*/
FileTrama MAILBOX_sendFileMeta(char **substring, char *portid, IluvatarConf *config, int mailbox){
    char *file_dir;
    char *temp_file_dir = SHAREDFUNC_readString(config->files_dir, '\0', 1); // Remove the first '/'
    asprintf(&file_dir, "%s/%s", temp_file_dir, substring[3]);
    
    char *full_md5sum=COMMANDS_executeLinuxCommand(file_dir, "md5sum", NULL);
    char *full_wc=COMMANDS_executeLinuxCommand(file_dir, "wc", "-c");
    if(full_md5sum == NULL || full_wc == NULL){
        printF("Error: Executing the linux commands\n");
        free(file_dir);
        FileTrama aux;
        aux.idtrama = -1;
        return aux;
    }

    // Clean the hash and size string output
    char *hash = SHAREDFUNC_readString(full_md5sum, ' ', 0);
    char *size_wc = SHAREDFUNC_readString(full_wc, ' ', 0);
    free(temp_file_dir);
    free(full_md5sum);
    free(full_wc);

    FileTrama filetrama = MAILBOX_sendFileMailboxTrama(
    1,               // Unused
    portid,          // Origin port the reciver of the MSG
    hash,            // Destination port the reciver of the MSG
    substring[3],    // Type of the trama (MSG)
    MTT_NEW_FILE,
    "data_chunk",    // Data of the trama
    mailbox,
    0,
    atoi(size_wc),
    config->user_name); // Mailbox to send the trama

    free(file_dir);
    free(size_wc);
    free(hash);

    return filetrama;
}


/*
* Send a file to the destination using the mailbox.
*/
void MAILBOX_sendFile(char **substring, char *portid, IluvatarConf *config, int mailbox){
    FileTrama filetrama = MAILBOX_sendFileMeta(substring, portid, config, mailbox); 
    if(filetrama.idtrama == -1){
        return;
    }

    char *file_dir;
    char *temp_file_dir = SHAREDFUNC_readString(config->files_dir, '\0', 1); // Remove the first '/'
    asprintf(&file_dir, "%s/%s", temp_file_dir, substring[3]);

    // Open the file
    int file = open(file_dir, O_RDONLY);
    if(file <= 0){
        printF("Error: Opening the file\n");
        return;
    }


    int fi=0;   // Flag to know if the file is finished
    int ntrama = 0;

    // Read the file until the end
    while (fi == 0){
        // Create new data_chunk to send
        char data_chunk[FILE_DATA_CHUNK_SIZE];
        //memset(data_chunk, "\0", FILE_DATA_CHUNK_SIZE);
        char c = 0;
        int i = 0;
        int size = 0;

        while (i < FILE_DATA_CHUNK_SIZE-1) { // -1 because of the '\0'
            size = read(file, &c, sizeof(char));
            if(size == 0){
                data_chunk[i] = '\0';
                fi=1;
                break;
            }
            data_chunk[i++] = c;
        }
        data_chunk[i] = '\0';
        ntrama++;

        MAILBOX_sendFileMailboxTrama(
        ntrama,               // Unused
        portid, // Origin port the reciver of the MSG
        filetrama.hash,                     // Destination port the reciver of the MSG
        substring[3],                    // Type of the trama (MSG)
        MTT_DATA_FILE,
        data_chunk,                        // Data of the trama
        mailbox,
        i,
        filetrama.fullSize,
        config->user_name);                   // Mailbox to send the trama
    }

    free(file_dir);
    free(temp_file_dir);
    close(file);
}

/*
* This function will recive a msg.
*/
void MAILBOX_reciveMsg(MsgTrama mailbox_trama, int mailbox, IluvatarConf *config){
    Message *message_recived = CONNECTION_dataToMessage(mailbox_trama.data);
    SEND_showMsg(message_recived, 0);
    CONNECTION_freeMessage(message_recived);

    // Send ACK
    MAILBOX_sendMailboxTrama(
    atoi(mailbox_trama.origPort),   // Unused
    config->comun_server->port,     // Origin port the reciver of the MSG
    mailbox_trama.origPort,         // Destination port the reciver of the MSG
    MTT_ACK_MSG,                    // Type of the trama (MSG)
    "NULL",                         // Data of the trama
    mailbox);                       // Mailbox to send the trama
}


/*
* This function will recive a msg trama and will 
* execute the correct function.
*/
void MAILBOX_msgRecived(MsgTrama mailbox_trama, IluvatarConf *_config, int _mailbox){

    switch(mailbox_trama.type){
        case MTT_MSG:
            MAILBOX_reciveMsg(mailbox_trama, _mailbox, _config);
            SHAREDFUNC_printDollar();
            write(1, COLOR_GREEN, strlen(COLOR_GREEN));
            break;
        case MTT_ACK_MSG:
            write(1, COLOR_RESET, strlen(COLOR_RESET));
            printF("Message correctly sent\n\n");
            SHAREDFUNC_printDollar();
            write(1, COLOR_GREEN, strlen(COLOR_GREEN));
            break;
        default:
            printF("Recived an unknown trama\n");
            break;
    }
}

/*
* Function to show the file trama recived.
*/
void MAILBOX_showFile(FileTrama trama){
    char *output;
    write(1, COLOR_RESET, strlen(COLOR_RESET));
    asprintf(&output, "\n\nNew file received!\nYour neighbor %s has sent:\n%s\n\n", trama.originUserName, trama.fileName);
    write(STDOUT_FILENO, output, strlen(output));
    free(output);
}


/*
*  This fucntion will be called by a thread to recive the file.
*/
void * MAILBOX_reciveFile(void* args){
    ThreadArgMailbox threadArgs = *((ThreadArgMailbox*)args);

    int sum_size=0;

    char *file_dir;
    char *temp_file_dir = SHAREDFUNC_readString(threadArgs.config->files_dir, '\0', 1); // Remove the first '/'
    asprintf(&file_dir, "%s/%s", temp_file_dir, threadArgs.file_mailbox_trama.fileName);

    // 1. Create the file
    //int fd = open(file_dir, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    int fd = open(file_dir, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        printF("Error: File could not be created\n");
        free(temp_file_dir);
        free(file_dir);
        pthread_cancel(pthread_self());
        pthread_detach(pthread_self());
        return NULL;
    }

    int index = 0;
    FileTrama aux;

    while(1){
        FileTrama aux_trama;
        
        // Poll the file mailbox
        msgrcv(threadArgs.file_mailbox, &aux_trama, sizeof(FileTrama) - sizeof(long),0,0);
        if(strcmp(aux_trama.destPort, threadArgs.config->comun_server->port)==0
        && strcmp(threadArgs.file_mailbox_trama.fileName, aux_trama.fileName)==0
        && aux_trama.idtrama == index+1){
            index = aux_trama.idtrama;
            write(fd, aux_trama.data, aux_trama.size);
            sum_size += aux_trama.size;
            aux = aux_trama;
            if(sum_size == threadArgs.file_mailbox_trama.fullSize)break;
        }else{
            // Case where the msg is for another Iluvatar
            msgsnd(threadArgs.file_mailbox, &aux_trama, sizeof(FileTrama)- sizeof(long), IPC_NOWAIT);
        }
    }

    int j;
    for(int i = 0; i < threadArgs.nclients; i++){
        if(strcmp(threadArgs.clients[i]->name, aux.originUserName) == 0){
            j=i;
        }
    }

    char *full_md5sum=COMMANDS_executeLinuxCommand(file_dir, "md5sum", NULL);
    char *md5sum = SHAREDFUNC_readString(full_md5sum, ' ', 0);
    free(full_md5sum);

    MAILBOX_showFile(threadArgs.file_mailbox_trama);

    if(strcmp(md5sum, threadArgs.file_mailbox_trama.hash)==0){
        // Case where the file is correct (MTT_OK_FILE)
        MAILBOX_sendFileMailboxTrama(
        1,
        threadArgs.clients[j]->port,
        aux.hash,
        aux.fileName,
        MTT_OK_FILE,
        "NULL",
        threadArgs.file_mailbox,
        1,
        aux.fullSize,
        threadArgs.config->user_name);
    }else{
        // Case where the file is corrupt (MTT_KO_FILE)
        MAILBOX_sendFileMailboxTrama(
        1,
        threadArgs.clients[j]->port,
        aux.hash,
        aux.fileName,
        MTT_KO_FILE,
        "NULL",
        threadArgs.file_mailbox,
        1,
        aux.fullSize,
        threadArgs.config->user_name);
        printF("The received file is corrupt, the MD5 hashes don’t match\n");
    }
    SHAREDFUNC_printDollar();

    free(md5sum);

    close(fd);
    free(file_dir);
    free(temp_file_dir);

    pthread_cancel(pthread_self());
    pthread_detach(pthread_self());

    return NULL;
}