#include "SharedFunc.h"
#include "Connection.h"
#include "semaphore_v2.h"


// VARIABLES GLOBALSs
ArdaServer *ardaConfig;         // Configuration of the Arda server.
struct sockaddr_in servidor;    // Server struct logic.
int listenFD;                   // FD of server listener.
Client **clients;               // Llista de clients.
int nClients;                   // Nombre de clients a la llista.
int clientFD;                   // FD of the clients
pthread_t thread;               // The starter Thread
semaphore sem;                  // The Arda's semaphore 
int countMSG;                   // The messages counter
// VARIABLES GLOBALS

/**
 * Fucntion used to free the memory
 * used to save the server config.
*/
void ARDA_freeArdaConfig(){
    free(ardaConfig->server->ip);
    free(ardaConfig->server->port);
    free(ardaConfig->server);
    free(ardaConfig->files_dir);
    free(ardaConfig);
}

/**
* Function that close all clients threads.
*/
void ARDA_closeThreadsFD(){
    close(clientFD);                // Closes de client FD

    for(int i = 0; i < nClients; i++){
        close(clients[i]->fd);
        pthread_cancel(clients[i]->id_thread);
        pthread_join(clients[i]->id_thread, NULL);
        pthread_detach(clients[i]->id_thread);
    }
}

/**
* Function that print the counter of MSG as well as save the value in a file.
*/
void ARDA_showAndSaveMSG(){
    char *output;

    asprintf(&output, "%d Messages have been sent through the network\n", countMSG);
    write(STDOUT_FILENO, output, strlen(output));
    free(output);

    int fd_countMSG = open("countMSG.txt", O_WRONLY|O_CREAT, 0600);

    if(fd_countMSG >= 0){
        asprintf(&output, "%d\n", countMSG);
        write(fd_countMSG, output, strlen(output));
        free(output);
        close(fd_countMSG);
    }
}

/**
 * Function that is executed in case of Arda server
 * closing (SIGINT).
*/
void ARDA_signalArdaExit(){
    printF(DISCONNECTING_ARDA);
    ARDA_showAndSaveMSG();
    printF(CLOSE_SERVER);
    ARDA_closeThreadsFD();
    ARDA_freeArdaConfig();
    CONNECTION_freeClients(clients, nClients);
    close(listenFD);
    SEM_destructor(&sem);

    exit(1);
}

/**
* Function that controls with a semaphore the counter MSG from Iluvatars.
*/
void ARDA_countMSG(Trama *trama){

    if((strcmp(trama->header, TH_NEW_MSG) != 0) || (trama->data == NULL) || (trama->lenght <= 0)){
        write(STDOUT_FILENO, TRAMA_ERROR, strlen(TRAMA_ERROR));
        return;
    } else{
        SEM_wait(&sem);
        countMSG++;
        SEM_signal(&sem);
    }
}

/**
 * Function used to make the file read of Arda server.
*/
ArdaServer* ARDA_readArdaConfig(char* fitxerConfig){
    ArdaServer* config = (ArdaServer*) malloc (sizeof(ArdaServer));
    int fd = open(fitxerConfig, O_RDONLY);

    if(fd > 0){
        config->server = (IpPort*)malloc(sizeof(IpPort));
        config->server->ip = SHAREDFUNC_readUntil(fd, '\n');
        config->server->port = SHAREDFUNC_readUntil(fd, '\n');
        config->files_dir = SHAREDFUNC_readUntil(fd, '\n');
        close(fd); // Tanquem fitxer;
    }else{
        // Error d'obertura del fitxer de configuracio
        write (1, ERROR_FILE_NO_OPEN, strlen(ERROR_FILE_NO_OPEN));
    }
    return config;
}

/**
* Function that reads the file for the counter MSG or creates a new file for that.
*/
void ARDA_readFdCountMSG(){
    char* counter;
    int fd_countMSG = open("countMSG.txt", O_RDONLY|O_CREAT, 0600);

    if(fd_countMSG >= 0){
        counter = SHAREDFUNC_readUntil(fd_countMSG, '\n');
        countMSG = atoi(counter);
        free(counter);
        close(fd_countMSG);
    } else{
        countMSG = 0;
        //es crea el fitxer per escriure els missatges.
    }
}

/**
 * Function used to generate the server logic using the IP and
 * port of the read config file.
*/
int ARDA_configArdaServer(){
    int bool=1;
    if( (listenFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        printF(ERROR_SOCKET);
        bool=0;
        close(listenFD);
    }

    bzero(&servidor, sizeof(servidor));
    servidor.sin_port = htons(atoi(ardaConfig->server->port));
    servidor.sin_family = AF_INET;
    servidor.sin_addr.s_addr = inet_addr(ardaConfig->server->ip);

    if(bind(listenFD, (struct sockaddr*) &servidor, sizeof(servidor)) < 0){
        printF(ERROR_BIND);
        close(listenFD);
        bool=0;
    }

    if(listen(listenFD, 10) < 0){
        printF(ERROR_LISTEN);
        close(listenFD);
        bool=0;
    }
    return bool;
}

/**
 * Function that searializes the clients array 
 * and adds to the trama data.
*/
void ARDA_addSerializedClients(Trama *trama){
    trama->data = NULL;

    for(int i=0; i<nClients; i++){
        if(clients[i]->connected){
            if(trama->data == NULL){
                asprintf(&trama->data, "%s&%s&%s&%s", clients[i]->name, clients[i]->ip, clients[i]->port, clients[i]->pid);   
            }else{
                char *tmp = trama->data;
                asprintf(&trama->data, "%s#%s&%s&%s&%s", tmp, clients[i]->name, clients[i]->ip, clients[i]->port, clients[i]->pid);
                free(tmp);
            }
        }
    }
}

/**
 * Function used to mark a client as disconnected.
*/
void ARDA_removeClient(Trama *trama){
    for(int i=0; i<nClients; i++){
        // Set client as disconnected
        if(strcmp(clients[i]->name, trama->data)==0)clients[i]->connected=0;
    }
}

/**
 * Function used to add a client to the array of clients,
 * if the client is already in the array, it will be updated
 * to connected.
*/
void ARDA_addClient(Client *client, int clientFD){
    int i=0;
    // Check if is already in the array disconnected
    while (i < nClients){
        if(strcmp(clients[i]->name, client->name)==0){
            clients[i]->connected=1;
            // Update PID
            strcpy(clients[i]->pid, client->pid);
            // Update Client FD
            clients[i]->fd = client->fd;
            // Update Thread ID
            clients[i]->id_thread = pthread_self();
            return;
        }
        i++;
    }

    // If the client is not in the array, we add it.
    nClients++;
    client->fd = clientFD;
    client->id_thread = pthread_self();
    client->connected=1; // Set client as connected
    clients = (Client**)realloc(clients, sizeof(Client*) * nClients);
    clients[nClients-1] = client;
}

/**
 * Function that returns an unknown trama
*/
void ARDA_unknownTrama(Trama *trama, int clientFD){
    ARDA_removeClient(trama);

    Trama *tmp = CONNECTION_setTramaTypeHeader(TT_ERROR, TH_UNKNOWN);
    tmp->data=NULL;
    CONNECTION_sendTrama(tmp, clientFD, -1);
    CONNECTION_freeTrama(tmp);
}

/**
 * Function used to add the new client to the array list.
*/
void ARDA_welcomeClient(Trama *trama, int clientFD){

    if((strcmp(trama->header, TH_NEW_SON) != 0) || (trama->data == NULL) || (trama->lenght <= 0)){ 
        write(STDOUT_FILENO, TRAMA_ERROR, strlen(TRAMA_ERROR));
        return;
    } else{
        Client *client = CONNECTION_dataToClient(trama->data);
        char *output;
        
        ARDA_addClient(client, clientFD);

        // Accept the client
        Trama *tmp = CONNECTION_setTramaTypeHeader(TT_NEW_CONNECTION, TH_CONOK);  // Set Type & Header
        ARDA_addSerializedClients(tmp); // Data added
        CONNECTION_sendTrama(tmp, clientFD, -1); // Send trama
        CONNECTION_freeTrama(tmp);

        SEM_wait(&sem);
        asprintf(&output, "New login: %s, IP: %s, port: %s, PID %s\nUpdating user’s list\nSending user’s list\nResponse sent\n\n", client->name, client->ip, client->port, client->pid);
        write(STDOUT_FILENO, output, strlen(output));
        SEM_signal(&sem);
        free(output);
    }
}

/**
 * Funciton used to send the updated user list to client-
*/
void ARDA_updateList(Trama *trama, int clientFD){

    if((strcmp(trama->header, TH_LIST_REQUEST) != 0) || (trama->data == NULL) || (trama->lenght <= 0)){
        write(STDOUT_FILENO, TRAMA_ERROR, strlen(TRAMA_ERROR));
        return;
    } else{
        char *output;

        Trama *tmp = CONNECTION_setTramaTypeHeader(TT_UPDATE_LIST, TH_LIST_REQUEST);
        ARDA_addSerializedClients(tmp);
        CONNECTION_sendTrama(tmp, clientFD, -1);
        CONNECTION_freeTrama(tmp);

        SEM_wait(&sem);
        asprintf(&output, "New petition: %s demands the user’s list\nSending user’s list to %s\n\n", trama->data, trama->data);
        write(1, output, strlen(output));
        SEM_signal(&sem);
        free(output);
    }    
}

/**
 * Function used to respond an exit request
 * from a client.
*/
void ARDA_exitPetition(Trama *trama, int clientFD){

    if((strcmp(trama->header, TH_EXIT) != 0) || (trama->data == NULL) || (trama->lenght <= 0)){
        write(STDOUT_FILENO, TRAMA_ERROR, strlen(TRAMA_ERROR));
        return;
    } else{
        char *output;

        // Set client who wants to desconnect as disconnected
        ARDA_removeClient(trama);

        // Acceptem la petició de l'usuari
        Trama *tmp = CONNECTION_setTramaTypeHeader(TT_EXIT, TH_CONOK);  // Set Type & Header
        tmp->data=NULL;
        CONNECTION_sendTrama(tmp, clientFD, -1);
        CONNECTION_freeTrama(tmp);

        SEM_wait(&sem);
        asprintf(&output, "New exit petition: %s has left Arda\nUpdating user’s list\nSending user’s list\nResponse sent\n\n", trama->data);
        write(STDOUT_FILENO, output, strlen(output));
        SEM_signal(&sem);
        free(output);
    }
}

/**
 * Function used to respond to each client request.
*/
int ARDA_processTrama(Trama *trama, int clientFD){
    int exit=0;

    switch (trama->type){
        case TT_NEW_CONNECTION:
            ARDA_welcomeClient(trama, clientFD);
            break;
        case TT_UPDATE_LIST:
            ARDA_updateList(trama, clientFD);
            break;
        case TT_EXIT:
            ARDA_exitPetition(trama, clientFD);
            exit=1;
            break;
        case TT_COUNT_MESSAGE:
            ARDA_countMSG(trama);
            break;
        default:
            write(STDOUT_FILENO, TRAMA_ERROR, strlen(TRAMA_ERROR));
            break;
    }
    return exit;
}

/**
 * Function that makes the new thread wait new communications.
*/
void * ARDA_funcioThread(void* tmpFD){
    int clientFD = *((int*)tmpFD);
    char c;

    while(1){
        // Waiting for a new trama
        if(read(clientFD, &c, sizeof(char)) != 0){ 
            Trama *trama = CONNECTION_getTrama(c, clientFD);
            if(ARDA_processTrama(trama, clientFD)){
                CONNECTION_freeTrama(trama);
                break;
            } else{
                CONNECTION_freeTrama(trama);
            }
        }
    }
    close(clientFD);                // Closes de client FD
    pthread_cancel(pthread_self()); // Kills the thread
    pthread_join(pthread_self(), NULL);
    pthread_detach(pthread_self()); // Frees the thread memory
    
    return NULL;
}

/**
 * Function used to wait new client connections.
*/
void ARDA_waitingConnections(){
    printF(ARDA_READING_FILE);

    while(1){
        clientFD = accept(listenFD, (struct sockaddr*) NULL, NULL);
        pthread_create(&thread, NULL, ARDA_funcioThread, &clientFD);
    }
}

/**
* Function that initializes the semaphore.
*/
void ARDA_startSemaphore(){
    SEM_constructor(&sem);
    SEM_init(&sem, 1);
}

/**
 * Main function of Arda server. 
*/
int main(int argc, char *argv[]){
    signal(SIGINT, ARDA_signalArdaExit);
    nClients = 0;

    if(argc == 2){
        // File config reading
        ardaConfig = ARDA_readArdaConfig(argv[1]);
        if(ARDA_configArdaServer()){
            ARDA_readFdCountMSG();
            ARDA_startSemaphore();
            ARDA_waitingConnections();
        }
        // S'executa en cas de no poderse connectar
        ARDA_freeArdaConfig();
    }else{
        write (1, ERROR_ARGS,strlen(ERROR_ARGS));
    }
    return 0;
}