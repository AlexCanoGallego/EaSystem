#include "SharedFunc.h"
#include "Connection.h"
#include "Commands.h"
#include "Send.h"
#include "Mailbox.h"

#define printF(x) write(1, x, strlen(x))

// VARIABLES GLOBALS
IluvatarConf *config;           // Configuration of the Iluvatar
char *buffer2;                  // Global buffer.
Trama *tmp;                     // Global trama temporal.

// Arda server connection
int socketFD;                   // FD of the client from Arda server.
struct sockaddr_in servidor;    // Connection to server logic struct.
Client **clients;               // Clients array.
int nClients;                   // Number of clients in the array.
// Arda server connection

// This Iluvatar server creation
struct sockaddr_in s_addrIluvatar;   // This Iluvatar creation server struct logic.
int socketIluvatar;                  // Socket server to Iluvatar to listen from.
pthread_t accept_connections_thread; // Thread used to accept connections from other Iluvatar.


// This Iluvatar connection to other Iluvatar
struct sockaddr_in iluvatarServer;  // Connection to Iluvatar server logic struct.
int iluvatarFD;                     // FD of the client from other Iluvatar server.
int sending;                        // Flag to know if we are sending a file.


// Mailbox
int msg_mailbox;                // ID of the queue.
int file_mailbox;               // ID of the queue.
pthread_t mailbox_thread;       // Thread used to poll the mailbox and check 
                                // if someone has sent a message to this Iluvatar.

pthread_t file_mailbox_thread;  // Thread used to poll the mailbox and check 
                                // if someone has sent a file to this Iluvatar.


/**
 * Funciton used to say goodbye to the server.
*/
int ILUVATAR_serverGoodBy(){
    int ok=0;
    char c;

    Trama *trama = CONNECTION_setTramaTypeHeader(TT_EXIT, TH_EXIT);  // Set Type & Header
    asprintf(&trama->data, "%s", config->user_name);
    CONNECTION_sendTrama(trama, socketFD, -1);
    CONNECTION_freeTrama(trama);

    // Esperem confirmació del servidor
    if(read(socketFD, &c, sizeof(char)) != 0){ 
        trama = CONNECTION_getTrama(c, socketFD);
        if(strcmp(trama->header, TH_CONOK)==0) ok=1; // S'ha pogut establir la comunicació
        CONNECTION_freeTrama(trama);
    }
    return ok;
}

/**
 * Function that establishes the connection with the Arda server.
*/
int ILUVATAR_connectToServer(){
    if( (socketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        printF(ERROR_SOCKET);
    }

    bzero(&servidor, sizeof(servidor));
    servidor.sin_family = AF_INET;
    servidor.sin_port = htons(atoi(config->arda_server->port));

    if(inet_pton(AF_INET, config->arda_server->ip, &servidor.sin_addr) < 0){
        printF(ERROR_IP);
        return 0;
    }

    if(connect(socketFD, (struct sockaddr*) &servidor, sizeof(servidor)) < 0){ //(struct sockaddr*)
        printF(ERROR_ARDA);
        return 0;
    }

    return 1;
}

/**
 * Function that is executed in case of Iluvatar client
 * closes.
*/
void ILUVATAR_signalExit(int signum){
    
    if(sending){
        printF(ERROR_SENDING);
        return;
    }

    if(signum == 2){
        char *buffer;
        asprintf(&buffer, "%sEXIT%s\n", COLOR_RED, COLOR_RESET);
        write(STDOUT_FILENO, buffer, strlen(buffer));
        free(buffer);
    }
    if(ILUVATAR_serverGoodBy() != 1){
        printF(ERROR_DISCONECTION); 
    }

    // Closing accept connections Iluvatar server thread
    close(socketIluvatar);  // Close this Iluvatar server socket
    pthread_cancel(accept_connections_thread);
    pthread_join(accept_connections_thread, NULL);
    pthread_detach(accept_connections_thread);

    // Closing mailbox thread
    pthread_cancel(mailbox_thread);
    pthread_join(mailbox_thread, NULL);
    pthread_detach(mailbox_thread);

    // Closing file mailbox thread
    pthread_cancel(file_mailbox_thread);
    pthread_join(file_mailbox_thread, NULL);
    pthread_detach(file_mailbox_thread);


    write(1, EXIT_MSG, strlen(EXIT_MSG));
    CONNECTION_freeClients(clients, nClients);
    close(socketFD);        // Close the Arda server socket
    COMMANDS_freesConfig(config);
    exit(1);
}

/**
 * Function used to show the user list 
 * saved in the Iluvadar client.
*/
void ILUVATAR_showListUsers(){
    char *output;

    asprintf(&output, "There are %d children of Iluvatar connected:\n", nClients);
    write(1, output, strlen(output));
    free(output);

    CONNECTION_showClientsList(clients, nClients);
    printF("\n");
}


/**
 * Function used to request a list update
 * from the server.
*/
void ILUVATAR_updateUsers(){
    Trama *trama = CONNECTION_setTramaTypeHeader(TT_UPDATE_LIST, TH_LIST_REQUEST);  // Set Type & Header
    char c;

    asprintf(&trama->data, "%s", config->user_name);
    CONNECTION_sendTrama(trama, socketFD, -1);
    CONNECTION_freeTrama(trama);

    // Waiting for the response
    if(read(socketFD, &c, sizeof(char)) != 0){ 
        trama = CONNECTION_getTrama(c, socketFD);  
        if(strcmp(trama->header, TH_LIST_REQUEST) == 0){
            CONNECTION_freeClients(clients, nClients);
            clients = CONNECTION_dataToClients(trama->data, &nClients);
        }
        CONNECTION_freeTrama(trama);
    }
}


/**
* This function is used to connect to other Iluvatar
* and get the client iluvatarFD.
*/
int ILUVATAR_connectToOtherIluvatar(int i){
    if((iluvatarFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        printF(ERROR_SOCKET);
        return 0;
    }

    bzero(&iluvatarServer, sizeof(iluvatarServer));
    iluvatarServer.sin_family = AF_INET;
    iluvatarServer.sin_port = htons(atoi(clients[i]->port));

    if(inet_pton(AF_INET, clients[i]->ip, &iluvatarServer.sin_addr) < 0){
        printF(ERROR_IP);
        return 0;
    }

    if(connect(iluvatarFD, (struct sockaddr*) &iluvatarServer, sizeof(iluvatarServer)) < 0){
        printF(ERROR_ILUVATAR_SERVER);
        return 0;
    }

    return 1; // Successfull connection to iluvatar server.
}

/**
* Function that select if is a send MSG or a send File.
*/
void ILUVATAR_send(char **substrings, int nsplits){
    int bool=0;
    // Check if the user exists
    for(int i=0; i<nClients; i++){
        if(strcmp(substrings[2], clients[i]->name) == 0) bool=1;
    }
    if(bool == 0){
        printF(USER_NOT_FOUND);
        return;
    } 
    
    int type=2;
    if(strcasecmp(substrings[1], "FILE")==0)type=1;

    // Check the format of the message or the file
    if(type == 2){
        if(SEND_checkMSGFormat(substrings, nsplits)==0) {
            return; 
        }  
    }else{
        // Check if the file exists
        if(SEND_checkFile(substrings[3], config)==0){
            return;
        }
    }

    // In case of the user exists
    for(int i = 0; i < nClients; i++){
        if(strcmp(substrings[2], clients[i]->name) == 0){
            // Connect to the other Iluvatar, save the FD as global variable
            if(strcmp(clients[i]->ip, config->comun_server->ip) == 0){
                // Same machine (use message queue)
                if(type == 2){
                    MAILBOX_sendMsg(substrings, nsplits, clients[i]->port, config, msg_mailbox, socketFD);
                }else{
                    sending = 1;
                    MAILBOX_sendFile(substrings, clients[i]->port, config, file_mailbox);
                    sending = 0;
                }
            }else{
                // Different machine (use sockets)
                if(ILUVATAR_connectToOtherIluvatar(i) == 1){
                    if(type == 2){
                        SEND_sendMsg(substrings, nsplits, iluvatarFD, config, -1, socketFD);
                    }else{
                        sending = 1;
                        SEND_sendFile(substrings, nsplits, iluvatarFD, config, -1);
                        sending = 0;
                    }
                    close(iluvatarFD);
                }
            }


        } 
    }
}


/**
 * Funciton used to select the command functionality.
*/
void ILUVATAR_comandsSelector(char **substrings, int nsplits, char *buf){

    if(nsplits == 0){
        write(1, UNKNOWN_COMMAND, strlen(UNKNOWN_COMMAND));
    } else if(strcasecmp(substrings[0], "SEND")==0 && nsplits < 4){
        write(1, UNKNOWN_COMMAND, strlen(UNKNOWN_COMMAND));
        write(1, HELPER, strlen(HELPER));
    }else if((strcasecmp(substrings[0], "SEND")==0 && strcasecmp(substrings[1], "FILE")==0 && nsplits == 4)
        || (strcasecmp(substrings[0], "SEND")==0 && strcasecmp(substrings[1], "MSG")==0 && nsplits >= 4)){
        ILUVATAR_send(substrings, nsplits);
    }else if(strcasecmp(substrings[0], "UPDATE")==0 && strcasecmp(substrings[1], "USERS")==0 && nsplits == 2){
        ILUVATAR_updateUsers();
        write(1, USERS_LIST_UPDATED, strlen(USERS_LIST_UPDATED));
    }else if(strcasecmp(substrings[0], "LIST")==0 && strcasecmp(substrings[1], "USERS")==0 && nsplits == 2){
        ILUVATAR_showListUsers();
    }else if(strcasecmp(substrings[0], "EXIT")==0 && nsplits == 1){
        COMMANDS_freeComand(substrings, nsplits);
        free(buf);
        free(buffer2);
        ILUVATAR_signalExit(4);
    } else{
        COMMANDS_linuxComands(substrings);
    }
}


/**
 * Function that waits for the user commands
 * intput.
*/
void ILUVATAR_iluvatarComands(IluvatarConf* config){
    char *output;
    int nsplits;
    fd_set descriptors;

    asprintf(&output, "\nWelcome %s, son of Iluvatar\n", config->user_name);
    write(STDOUT_FILENO, output, strlen(output));
    free(output);
    
    SHAREDFUNC_printDollar();

    while(1){
        char *buf;
        FD_ZERO(&descriptors);
        FD_SET(socketFD, &descriptors);
        FD_SET(STDIN_FILENO, &descriptors);
        
        if(select(4, &descriptors, NULL, NULL, NULL)<0){
            write(1, COLOR_RESET, strlen(COLOR_RESET));
            printF("ERROR\n");
            close(socketFD);
        }

        // Polling the Arda_FD and STDIN_FILENO
        while(1){
            if(FD_ISSET(socketFD, &descriptors)){
                write(1, COLOR_RESET, strlen(COLOR_RESET));
                close(socketFD);
                ILUVATAR_signalExit(4);
            }
            if(FD_ISSET(STDIN_FILENO, &descriptors)){
                buffer2 = SHAREDFUNC_readUntil(STDIN_FILENO, '\n');
                buf = COMMANDS_parceCommand(buffer2);
                char **substrings = SHAREDFUNC_splitFunc(buf,&nsplits, ' ');
                write(1, COLOR_RESET, strlen(COLOR_RESET));
                ILUVATAR_comandsSelector(substrings,nsplits,buf);
                free(buf);
                free(buffer2);
                COMMANDS_freeComand(substrings,nsplits);
                SHAREDFUNC_printDollar();
                break;
            }
        }
    }
}

/**
 * Funciton used to send a connection 
 * request to the server.
*/
int ILUVATAR_sendTramaNewConexion(){
    int ok=0;
    char c;

    Trama *trama = CONNECTION_setTramaTypeHeader(TT_NEW_CONNECTION, TH_NEW_SON);  // Set Type & Header
    asprintf(&trama->data, "%s&%s&%s&%d", config->user_name, config->comun_server->ip, config->comun_server->port, getpid());
    CONNECTION_sendTrama(trama, socketFD, -1);
    CONNECTION_freeTrama(trama);

    // Wait for server confirmation
    if(read(socketFD, &c, sizeof(char)) != 0){ 
        tmp = CONNECTION_getTrama(c, socketFD);        
        if(strcmp(tmp->header, TH_CONOK)==0) ok=1;
        // Transform data to clients struct
        clients = CONNECTION_dataToClients(tmp->data, &nClients);
        CONNECTION_freeTrama(tmp);
    }

    return ok; 
}


/**
 * Function used to accept the connections and creates a new thread
 * for each connection who wants to talk with Iluvatar.
*/
void * ILUVATAR_acceptConnections(){

    while(1){
        pthread_t thread; // Dedicated thread for each client who wants to talk with this Iluvatar
        int clientFD = accept(socketIluvatar, (struct sockaddr*) NULL, NULL);
        ThreadArgs threadArgs;
        threadArgs.clientFD = clientFD;
        threadArgs.dirName = config->files_dir;
        threadArgs.clientList = clients;
        threadArgs.nClients = nClients;
        
        pthread_create(&thread, NULL, SEND_reciveFromIluvatar, &threadArgs);
    }

    return NULL;
}

/**
* Function that starts the Iluvatar as server.
*/
int ILUVATAR_startIluvatarServer(){

    if((socketIluvatar = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        printF("Error creant el socket\n");
        close(socketIluvatar);
        return 0;
    }

    bzero(&s_addrIluvatar, sizeof(s_addrIluvatar));
    s_addrIluvatar.sin_port = htons(atoi(config->comun_server->port));
    s_addrIluvatar.sin_family = AF_INET;
    s_addrIluvatar.sin_addr.s_addr = inet_addr(config->comun_server->ip);

    if(bind(socketIluvatar, (struct sockaddr*) &s_addrIluvatar, sizeof(s_addrIluvatar)) < 0){
        printF(ERROR_BIND);
        close(socketIluvatar);
        return 0;
    }

    if(listen(socketIluvatar, 10) < 0){
        printF(ERROR_LISTEN);
        close(socketIluvatar);
        return 0;
    }

    // Start thread to accept connections
    // in case the Iluvatar server has been started
    // successfully.
    pthread_create(&accept_connections_thread, NULL, ILUVATAR_acceptConnections, NULL);

    return 1;
}

/*
* Function used to poll the mailbox asynchronically
* and see if there are any messages for this Iluvatar.
*/
void * ILUVATAR_pollMailbox(){
    MsgTrama msg_mailbox_trama;

    while(1){
        // Poll the msg mailbox
        msgrcv(msg_mailbox, &msg_mailbox_trama, sizeof(MsgTrama) - sizeof(long),0,0);
        if(strcmp(msg_mailbox_trama.destPort, config->comun_server->port)==0){
            // Case where the msg is for me
            MAILBOX_msgRecived(msg_mailbox_trama, config, msg_mailbox);
            //SHAREDFUNC_printDollar();
            write(1, COLOR_GREEN, strlen(COLOR_GREEN));
        }else{
            // Case where the msg is for another Iluvatar
            msgsnd(msg_mailbox, &msg_mailbox_trama, sizeof(MsgTrama)- sizeof(long), IPC_NOWAIT);
        }
    }

    return NULL;
}

/*
 * Function used to poll the file mailbox asynchronically.
 * and see if there are any new file for this Iluvatar.
 * In case there is a new file, a new thread will be created.
*/
void * ILUVATAR_pollFileMailbox(){
    FileTrama file_mailbox_trama;

    while(1){
        // Poll the file mailbox
        msgrcv(file_mailbox, &file_mailbox_trama, sizeof(FileTrama) - sizeof(long),0,0);
        if(strcmp(file_mailbox_trama.destPort, config->comun_server->port)==0
            && file_mailbox_trama.type == MTT_NEW_FILE){
            // Case where the file is for me
            pthread_t recive_file;
            ThreadArgMailbox threadArgs;
            threadArgs.file_mailbox_trama = file_mailbox_trama;
            threadArgs.config = config;
            threadArgs.file_mailbox = file_mailbox;
            threadArgs.clients = clients;
            threadArgs.nclients = nClients;
            // Create a new thread to receive a new file
            pthread_create(&recive_file, NULL, MAILBOX_reciveFile, &threadArgs);
            //write(1, COLOR_GREEN, strlen(COLOR_GREEN));
        }else if(strcmp(file_mailbox_trama.destPort, config->comun_server->port)==0
            && file_mailbox_trama.type == MTT_OK_FILE){
            // Case where the file was received correctly
            write(1, COLOR_RESET, strlen(COLOR_RESET));
            printF("File correctly sent\n");
            SHAREDFUNC_printDollar();
        }else if(strcmp(file_mailbox_trama.destPort, config->comun_server->port)==0
            && file_mailbox_trama.type == MTT_KO_FILE){
            // Case where the file was not received correctly
            write(1, COLOR_RESET, strlen(COLOR_RESET));
            printF("File incorrectly sent, MD5 hashes don’t match\n");
            SHAREDFUNC_printDollar();
        }else{
            // Case where the msg is for another Iluvatar
            msgsnd(file_mailbox, &file_mailbox_trama, sizeof(FileTrama)- sizeof(long), IPC_NOWAIT);
        }
    }

    return NULL;
}

/*
 * Function that creates the msg mailbox using the
 * ftok of IluvatarSon.c. And also creates the polling thread.
*/
int ILUVATAR_createMsgMailbox(){
    key_t key;

    key = ftok("IluvatarSon.c", 0);
    if (key == (key_t)-1) {
        printF("Error creating the key\n");
        return -1;
    }

    msg_mailbox = msgget(key, IPC_CREAT | 0600);
    if (msg_mailbox < 0) {
        printF("Error creating the mailbox\n");
        return -1;
    }
    // Start thread to polll the mailbox
    // in case the Mailbox has been created
    // successfully.
    pthread_create(&mailbox_thread, NULL, ILUVATAR_pollMailbox, NULL);

    return 1;
}

/*
 * Function that creates the file mailbox using the
 * ftok of Arda.c. And also creates the polling thread.
*/
int ILUVATAR_createFileMailbox(){
    key_t key;

    key = ftok("Arda.c", 0);
    if (key == (key_t)-1) {
        printF("Error creating the key\n");
        return -1;
    }

    file_mailbox = msgget(key, IPC_CREAT | 0600);
    if (file_mailbox < 0) {
        printF("Error creating the mailbox\n");
        return -1;
    }

    pthread_create(&file_mailbox_thread, NULL, ILUVATAR_pollFileMailbox, NULL);

    return 1;
}



/**
 * Main function of IluvatarSon. 
*/
int main(int argc, char *argv[]){
    signal(SIGINT, ILUVATAR_signalExit);
    nClients = 0;
    sending = 0;
    if(argc == 2){
        config = COMMANDS_readConfig(argv[1]);
        if(!COMMANDS_checkDirectory(config->files_dir)){ 
            char *output;
            asprintf(&output, "Directory %s doesn't exist\n", config->files_dir);
            write(STDOUT_FILENO, output, strlen(output));
            free(output);
        }else{
            if(ILUVATAR_connectToServer()){
                if(ILUVATAR_sendTramaNewConexion()){
                    if(ILUVATAR_startIluvatarServer()){ 
                        if(ILUVATAR_createMsgMailbox() && ILUVATAR_createFileMailbox()){
                            ILUVATAR_iluvatarComands(config);   
                        }else{
                            printF("Error creating mailbox\n");
                        }
                    } else{
                        close(socketFD);
                        close(socketIluvatar);
                    }
                }else{
                    ILUVATAR_signalExit(4);
                    printF(ERROR_DISCONECTION); 
                }
            }else{
                close(socketFD);
            }
        }

        COMMANDS_freesConfig(config);
    }else{
        write (1, ERROR_ARGS,strlen(ERROR_ARGS));
    }
    return 0;
}