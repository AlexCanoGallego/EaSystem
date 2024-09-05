#include "Connection.h"
#include "SharedFunc.h"


/**
 * Function that sends a Trama through a socketFD
 * using seralization.
 * [TYPE(1 Byte) | HEADER(x Bytes) | LENGHT(2 Bytes) | DATA(Lenght BYTES)]
 * In case of size != -1, the lenght of the data is set to size.
 * In case of size == -1, the lenght of the data is set to strlen(trama->data).
*/
void CONNECTION_sendTrama(Trama *trama, int socketFD, short size){
    char *msg;

    if(size == -1){
        if(trama->data != NULL){
            trama->lenght = strlen(trama->data);
        }else{
            trama->lenght=0;
        }
    }else{
        trama->lenght = size;
    }

    // TYPE (1 Byte) HEADER(x Bytes)
    asprintf(&msg, "%c%s", trama->type, trama->header);

    // LENGHT (2 Bytes)
    char l = (trama->lenght & 0xFF);         // Primer byte
    char h = ((trama->lenght >> 8) & 0xFF);  // Segon byte

    int i = (int)strlen(msg);
    int j = 0;

    // DATA (Lenght BYTES)
    if(trama->lenght > 0){
        if(size == -1){
            j = (int)strlen(trama->data);
        }else{
            j = size;
        }
        //j = (int)strlen(trama->data);

        msg = (char*)realloc(msg,(sizeof(char) * i+j+2) +  sizeof(short));
        msg[i]=l;
        msg[i+1]=h;

        int k=0;
        while(k<j){
            msg[i+2+k]=trama->data[k];
            k++;
        }
        k++;
        msg[i+2+k]='\0';    

        write(socketFD, msg, i+j+2);
    }else{
        write(socketFD, msg, i+1);
    }

    free(msg);
}

/**
 * Function that returns a Trama with
 * the Type and Header requested.
*/
Trama * CONNECTION_setTramaTypeHeader(char type, char* header){
    Trama *trama = (Trama*) malloc(sizeof(Trama));
    trama->type = type;

    char *p = (char*)malloc((int)strlen(header) * sizeof(char)+1);
    strcpy(p, header);
    trama->header = p;

    return trama;
}

/**
 * Function that reads and saves in dynamic memory 
 * an incoming trama.
*/
Trama * CONNECTION_getTrama(char cc, int clientFD){
    Trama *trama = (Trama*) malloc(sizeof(Trama));
    trama->lenght=0;

    // Get Trama Type
    trama->type = cc;

    // Get Trama Header
    trama->header = SHAREDFUNC_readLine(clientFD, ']');

    // Get lenght
    read(clientFD, &trama->lenght, sizeof(short));

    // Get Trama Data
    trama->data=NULL;
    if(trama->lenght > 0){
        trama->data = (char*) malloc(sizeof(char) * (trama->lenght+1));
        read(clientFD, trama->data, sizeof(char) * trama->lenght+1);
        trama->data[trama->lenght] = '\0';
    }

    return trama;
}


/**
 * Function that returns a Message with the data
 * from a Trama with the header [MSG].
*/
Message * CONNECTION_dataToMessage(char *trama_recived){
    Message *message = (Message*)malloc(sizeof(Message));

    message->originUser = SHAREDFUNC_readString(trama_recived, '&', 0);
    int start = (int)strlen(message->originUser) + 1;
    message->message = SHAREDFUNC_readString(trama_recived, '\0', start);

    return message;
}

/**
 * Function that returns a Client with the data
 * from a data Trama.
*/
Client * CONNECTION_dataToClient(char *trama_data){
    Client *client = (Client*)malloc(sizeof(Client));
    int i=0,j=0;
    char *data;

    // Get Name
    data = (char*)malloc(sizeof(char));
    while(1){
        data[i]=trama_data[i];
        i++;
        data = (char*)realloc(data, sizeof(char) * (i+1));
        if(trama_data[i] == '&')break;
    }
    i++;
    data = (char*)realloc(data, sizeof(char) * (i+1));
    data[i-1]='\0';
    client->name=data;

    // Get Ip
    j=0;
    data = (char*)malloc(sizeof(char));
    while(1){
        data[j]=trama_data[i];
        i++;j++;
        data = (char*)realloc(data, sizeof(char) * (j+1));
        if(trama_data[i] == '&')break;
    }
    i++;
    j++;
    data = (char*)realloc(data, sizeof(char) * (j+1));
    data[j-1]='\0';
    client->ip=data;

    // Get Port
    j=0;
    data = (char*)malloc(sizeof(char));
    while(1){
        data[j]=trama_data[i];
        i++;j++;
        data = (char*)realloc(data, sizeof(char) * (j+1));
        if(trama_data[i] == '&')break;
    }
    i++;
    j++;
    data = (char*)realloc(data, sizeof(char) * (j+1));
    data[j-1]='\0';
    client->port=data;

    // Get PID
    j=0;
    data = (char*)malloc(sizeof(char));
    while(1){
        data[j]=trama_data[i];
        i++;j++;
        data = (char*)realloc(data, sizeof(char) * (j+1));
        if(trama_data[i] == '\0')break;
    }
    i++;
    j++;
    data = (char*)realloc(data, sizeof(char) * (j+1));
    data[j-1]='\0';
    client->pid=data;

    return client;
}



/**
* Funciton that splits a string
* into array of dynamic strings.
*/
char ** splitStrings(char *data, char del, int *nUsers){
    int n=0;    // n Substrings
    int i=0;    // To iterate full string
    int j=0;    // To iterate each substring

    // Allocate first substring
    char **substrings = (char**) malloc (sizeof(char*));
    substrings[n] = (char*) malloc (sizeof(char));

    while(data[i] != '\0'){
        if(data[i] == del){
            j++;
            substrings[n] = (char*) realloc (substrings[n], sizeof(char) * (j));
            substrings[n][j-1] = '\0';
            // New string
            n++;
            substrings = (char**) realloc (substrings, sizeof(char*) * (n+1));
            substrings[n] = (char*) malloc (sizeof(char));
            j=0;
        }else{
            // New char
            substrings[n][j] = data[i];
            j++;
            substrings[n] = (char*) realloc (substrings[n], sizeof(char) * (j+2));
        }

        i++;        
    }

    j++;
    substrings[n] = (char*) realloc (substrings[n], sizeof(char) * (j));
    substrings[n][j-1] = '\0';

    *nUsers = n+1;

    return substrings;
}

/**
 * Functions that frees the memory of n substrings.
*/
void freeSubstrings(char **substrings, int n){
    for(int i=0; i<n; i++){
        free(substrings[i]);
    }
    free(substrings);
}

/**
 * Function that returns a Cliets array
 * from the trama data.
*/
Client ** CONNECTION_dataToClients(char *trama_data, int *nClients){    
    int n=0;
    char **substrings=NULL;
    substrings = splitStrings(trama_data, '#',  &n);

    Client **clients = (Client**) malloc (sizeof(Client*) * n);

    for(int i=0;i<n;i++){
        if(substrings[i] != NULL){
            clients[i] = CONNECTION_dataToClient(substrings[i]);
            clients[i]->connected = 1;
        }
    }

    freeSubstrings(substrings, n);

    *nClients=n;

    return clients;
}

/**
 * Function that prints the information of a Trama.
*/
void CONNECTION_showTrama(Trama *trama){
    char *output;
    // %hi = short (signed)
    asprintf(&output, "Type: %d\nHeader: %s\nLenght: %hi\nData: %s\n\n", trama->type, trama->header, trama->lenght, trama->data);
    write(STDOUT_FILENO, output, strlen(output));
    free(output);
}

/**
 * Function that prints the information of a Client.
*/
void CONNECTION_showClient(Client *client){
    char *output=NULL;
    asprintf(&output, "Name: %s, Ip: %s, Port: %s, Pid: %s\n", client->name, client->ip, client->port, client->pid);
    write(STDOUT_FILENO, output, strlen(output));
    free(output);
}

/**
 * Function useful for debugging that shows the list of clients
 * stored in the program.
*/
void CONNECTION_showClientsList(Client **c, int n){
    char *output=NULL;
    struct in_addr addr;

    for(int i=0; i<n; i++){
        if(c[i]->connected){
            asprintf(&output, "%d.\t%s\t%s\t%s\t", i+1, c[i]->name, c[i]->ip, c[i]->port);
            write(STDOUT_FILENO, output, strlen(output));
            free(output);

            inet_aton(c[i]->ip, &addr);
            struct hostent *server = gethostbyaddr(&addr, sizeof(addr), AF_INET);

            asprintf(&output, "%s\t%s\n", server->h_name, c[i]->pid);

            write(STDOUT_FILENO, output, strlen(output));
            free(output);

        }
    }
    if(n==0) printF("\nThe client list is empty!\n");
}

/**
 * Function that frees the memory of a Trama.
*/
void CONNECTION_freeTrama(Trama *trama){
    //if(trama->data != NULL) 
    free(trama->data);
    free(trama->header);    
    free(trama);
    trama=NULL;
}

/**
 * Function that frees the memory of a Message.
*/
void CONNECTION_freeMessage(Message *message){
    free(message->message);
    free(message->originUser);
    free(message);
    message=NULL;
}

/**
 * Function that frees the memory of a Message.
*/
void CONNECTION_freeMetaFile(MetaFile *meta_file){
    free(meta_file->fileHash);
    free(meta_file->fileName);
    free(meta_file->originUser);
    free(meta_file);
    meta_file=NULL;
}

/**
 * Function that frees the memory of a Client.
*/
void CONNECTION_freeClient(Client *client){
    free(client->name);
    free(client->ip);
    free(client->port);
    free(client->pid);
    free(client);
    client=NULL;
}

/**
 * Function that frees the memory of n Client.
*/
void CONNECTION_freeClients(Client **clients, int nClients){
    for(int i=0; i<nClients; i++){
        CONNECTION_freeClient(clients[i]);
    }
    free(clients);
}
