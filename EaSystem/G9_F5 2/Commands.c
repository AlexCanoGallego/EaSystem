#include "Connection.h"
#include "SharedFunc.h"

IluvatarConf *config;           // File configuration.
int numClients;                 // Number of users.

/**
 * Function that frees the memory of the configuration.
*/
void COMMANDS_freesConfig(IluvatarConf* config){
    // File config frees
    free(config->user_name);
    free(config->files_dir);
    free(config->arda_server->ip);
    free(config->arda_server->port);
    free(config->arda_server);
    free(config->comun_server->ip);
    free(config->comun_server->port);
    free(config->comun_server);
    free(config);
}

/**
 * Function that frees the memory of the commands.
*/
void COMMANDS_freeComand(char **substring, int nsplits){
    for(int i=0; i<=nsplits; i++){
        //free(substring[i]);
    }
    free(substring[nsplits]);
    free(substring);
}

/**
 * Function that reads the configuration file and 
 * returns a IluvatarConf struct.
*/
IluvatarConf* COMMANDS_readConfig(char* fitxerConfig){
    IluvatarConf* config = (IluvatarConf*) malloc (sizeof(IluvatarConf));
    int fd = open(fitxerConfig, O_RDONLY);

    if(fd > 0){
        config->user_name = SHAREDFUNC_readUntil(fd, '\n');
        config->files_dir = SHAREDFUNC_readUntil(fd, '\n');
        config->arda_server = (IpPort*)malloc(sizeof(IpPort));
        config->arda_server->ip = SHAREDFUNC_readUntil(fd, '\n');
        config->arda_server->port = SHAREDFUNC_readUntil(fd, '\n');
        config->comun_server = (IpPort*)malloc(sizeof(IpPort));
        config->comun_server->ip = SHAREDFUNC_readUntil(fd, '\n');
        config->comun_server->port = SHAREDFUNC_readUntil(fd, '\n');
        
        close(fd); // Tanquem fitxer;
    }else{
        // Error d'obertura del fitxer de configuracio
        write (1, ERROR_FILE_NO_OPEN, strlen(ERROR_FILE_NO_OPEN));
    }

    return config;
}

/**
 * Function that executes a Linux command.
*/
void COMMANDS_linuxComands(char **substrings){
	int linuxPipe[2];
    pipe(linuxPipe);

    pid_t child = fork();

	switch (child) {
        int espera;
		case -1:
        // Fork error
        write(1, ERROR_FORK, strlen(ERROR_FORK));
        break;
        case 0:
            close(linuxPipe[0]);
            dup2(linuxPipe[1], STDERR_FILENO);

            char *linuxArr[] = {"/bin/sh", "-c", *substrings, NULL};
            execv("/bin/sh", linuxArr);

            break;  
        default:
            // Primer s'espera a que el fill acabi
            close(linuxPipe[1]);

            wait(&espera);
            int returnEspera = WEXITSTATUS(espera);
            if(returnEspera == 127) write(1, UNKNOWN_COMMAND, strlen(UNKNOWN_COMMAND));

            close(linuxPipe[0]);

            break;
    }
}


/**
 * Function used to execute a one or two word Linux command
 * and return the output.
*/
char * COMMANDS_executeLinuxCommand(char* file_name, char* command, char* arg){
    // Create a pipe
    int fd[2];
    if (pipe(fd) == -1){
        return NULL;
    }
    // Create a child process
    pid_t pid = fork();
    if (pid == -1) {
        return NULL;
    }
    // Child process
    if (pid == 0) {
        // Close the read end of the pipe
        close(fd[0]);
        // Redirect stdout to the write end of the pipe
        dup2(fd[1], STDOUT_FILENO);

        // Execute command with argument or not 
        if(arg != NULL){
            char *info[] = {command, arg, file_name, NULL};
            execvp(command, info);
        }else{
            char *info[] = {command, file_name, NULL};
            execvp(command, info);
        }

        // Close the write end of the pipe
        close(fd[1]);
    }else {
        // Close the write end of the pipe
        close(fd[1]);
        // Read from the read end of the pipe
        char *hash=NULL;
        char c=0;
        int i=0;

        // Read pipe until space
        while (read(fd[0], &c, sizeof(char))) {
            if(hash ==  NULL) hash = (char*)malloc(sizeof(char));
            hash = (char*)realloc(hash, sizeof(char) * (i + 2));
            hash[i++] = c;
        }
        hash[i] = '\0';
        // Close the read end of the pipe
        close(fd[0]);
        wait(NULL);

        return hash;
    }

    return NULL;
}

/**
 * Function that parses the commands introduced by the user.
*/
char* COMMANDS_parceCommand(char *buffer){
    char* string = (char*)malloc(sizeof(char));
    int j = 0;
    int parce = 0;

    for(int i = 0; i < (int)strlen(buffer); i++){
        if(buffer[i] == '"'){
            parce++;
        }
        if(parce == 0 || (buffer[i] != '&' && buffer[i] != '#')){
            string = (char*)realloc(string, sizeof(char) * (j + 2));
            string[j++] = buffer[i];
        }
    }
    string[j] = '\0';
    return string;
}

/**
 * Function that returns 1 if the directory exists,
 * 0 if not.
*/
int COMMANDS_checkDirectory(char *directory){
    char *temp_file_dir = SHAREDFUNC_readString(directory, '\0', 1); // Remove the first '/'
    int fd = open(temp_file_dir, O_DIRECTORY);
    free(temp_file_dir);

    // If the directory does not exist
    if (fd == -1) return 0;

    // If the directory exists
    close(fd);
    return 1;    
}