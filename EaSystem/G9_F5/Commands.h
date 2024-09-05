#ifndef _COMMANDS_
#define _COMMANDS_ 

//define gnu_source
#define _GNU_SOURCE
#define printF(x) write(1, x, strlen(x))


/**
 * Function that frees the memory of the configuration.
*/
void COMMANDS_freesConfig(IluvatarConf* config);

/**
 * Function that frees the memory of the commands.
*/
void COMMANDS_freeComand(char **substring, int nsplits);

/**
 * Function that reads the configuration file and 
 * returns a IluvatarConf struct.
*/
IluvatarConf* COMMANDS_readConfig(char* fitxerConfig);

/**
 * Function that executes a Linux command.
*/
void COMMANDS_linuxComands(char **substrings);

/**
 * Function used to execute a one or two word Linux command
 * and return the output.
*/
char * COMMANDS_executeLinuxCommand(char* file_name, char* command, char* arg);

/**
 * Function that parses the commands introduced by the user.
*/
char* COMMANDS_parceCommand(char *buffer);

#endif