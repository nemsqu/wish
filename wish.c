#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

struct command {
    int number;
    char **params;
    char *outputFile;
    struct command *pNext;
};
typedef struct command COMMAND;

struct path {
    char *path;
    struct path *pNext;
};
typedef struct path PATH;

COMMAND *add_command_to_list(COMMAND *pCommand, char **params, int commandNro, char *outputFile){
    COMMAND *pNew = NULL, *ptr = NULL;

    if((pNew = malloc(sizeof(COMMAND))) == NULL){
        char error_message[50] = "Error allocating memory when adding command\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }
    pNew->number = commandNro;
    pNew->outputFile = outputFile;
    pNew->pNext = NULL;

    // initialize memory for parameters
    if ((pNew->params = malloc(sizeof(char*))) == NULL) {
        char error_message[50] = "Error allocating memory when adding command\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }

    // add parameters
    int i = 0;
    while(params[i] != NULL){
        if((pNew->params = realloc(pNew->params, (i+1)*sizeof(char*))) == NULL){
            char error_message[50] = "Error allocating memory when adding command\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
        pNew->params[i] = params[i];
        i++;
    }
   if((pNew->params = realloc(pNew->params, (i+1)*sizeof(char*))) == NULL){
        char error_message[50] = "Error allocating memory when adding command\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }
    pNew->params[i] = NULL;

    // add command to list
    if(pCommand == NULL){
        pCommand = pNew;
    } else {
        ptr = pCommand;
        while(ptr->pNext != NULL){
            ptr = ptr->pNext;
        }
        ptr->pNext = pNew;
    }
    return pCommand;
}

COMMAND *empty_commands(COMMAND *pCommand){
    COMMAND *ptr;

    while(pCommand != NULL){
        ptr = pCommand;
        pCommand = ptr->pNext;
        free(ptr->params);
        free(ptr);
    }
    
    return pCommand;
}


PATH *empty_paths(PATH *path){
    PATH *ptr;

    while(path != NULL){
        ptr = path->pNext;
        free(path);
        path = ptr;
    }
    return path;
}

PATH *add_paths(char *buffer, PATH *path){
    // remove newline so that it doesn't mess up the (last) path
    buffer[strcspn(buffer, "\n")] = 0;

    // cut away command "path"
    strtok(buffer, " ");
    char *newPath = strtok(NULL, " ");
    PATH *ptr;

    // delete all paths if no new path has been implemented
    if(newPath == NULL){
        if(path != NULL){
            path = empty_paths(path);
        }
    } else {
        // clear old paths
        if(path != NULL){
            path = empty_paths(path);
        }
        while(newPath != NULL){
            PATH *pNew = NULL;
            if((pNew = malloc(sizeof(PATH))) == NULL){
                char error_message[50] = "Error allocating memory when adding path\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
                exit(1);
            }
            if((pNew->path = malloc(strlen(newPath) * sizeof(char))) == NULL){
                char error_message[50] = "Error allocating memory when adding path\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
                exit(1);
            }
            strcpy(pNew->path, newPath);
            pNew->pNext = NULL;

            // add path to list
            if(path == NULL){
                path = pNew;
            } else {
                ptr = path;
                while(ptr->pNext != NULL){
                    ptr = ptr->pNext;
                }
                ptr->pNext = pNew;
            }
            newPath = strtok(NULL, " ");
        }
    }
    return path;
}


void run_shell(int batchMode, FILE *input){
    char *buffer = NULL;
    size_t len = 0, read;
    // use char pointer array https://newton.ex.ac.uk/teaching/resources/jmr/p2.html
    char **params = NULL;
    char *tok, *defaultPath = "/bin";
    int status;
    PATH *path = NULL;
    COMMAND *pCommand = NULL;

    // only print prompt in interactive mode
    if(batchMode == 0){
        printf("Wish> ");
    }
    // initialize path
    if((path = malloc(sizeof(PATH))) == NULL){
        char error_message[50] = "Error allocating memory when adding path\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }
    if((path->path = malloc(strlen(defaultPath) * sizeof(char))) == NULL){
        char error_message[50] = "Error allocating memory when adding path\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }
    strcpy(path->path, defaultPath);
    path->pNext = NULL;

    // run shell
    while((read = getline(&buffer, &len, input)) != -1){
        //check built-in commands
        char command[5] = "";
        strncpy(command, buffer, 4);
        command[4] = '\0';
        if(strcmp(command, "path") == 0){
            path = add_paths(buffer, path);
            if(batchMode == 0){
                printf("Wish> ");
            }
            continue;
        }
        if(strcmp(command, "exit") == 0){
            // Inputing stuff after exit is an error
            if(read > 5){
                char error_message[30] = "Incorrect input\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
                if(batchMode == 0){
                    printf("Wish> ");
                }
                continue;
            }
            empty_commands(pCommand);
            if(params != NULL){
                free(params);
            }
            if(path != NULL){
                path = empty_paths(path);
            }
            exit(0);
        }
        
        char cdCommand[3] = "";
        strncpy(cdCommand, command, 2);
        cdCommand[2] = '\0';
        
        if(strcmp(cdCommand, "cd") == 0){
            //cut away command part "cd"
            strtok(buffer, " ");
            char *arg = NULL;
            arg = strtok(NULL, "\n");

            if(arg == NULL || strtok(NULL, " ") != NULL){
                char error_message[30] = "Incorrect input\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
            } else {
                if(chdir(arg) != 0){
                    char error_message[50] = "Error with changing directory\n";
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
            }
            if(batchMode == 0){
                printf("Wish> ");
            }
            continue;
        }
        //remove newline so that it doesn't confuse execv https://stackoverflow.com/questions/2693776/removing-trailing-newline-character-from-fgets-input
        buffer[strcspn(buffer, "\n")] = 0;
        tok = strtok(buffer, " ");
        
        params = malloc(sizeof(char *));
        int i = 1, fileOutputError = 0, commands = 0;
        char *outputFile = NULL, *commandPart = NULL;
        // parse through commands and parameters
        while(tok != NULL){
            if(strcmp(tok, ">") == 0){
                if(params[0] == NULL){
                    // Invalid input, no command to output to the file
                    fileOutputError = 1;
                    char error_message[30] = "Incorrect input\n";
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    break;
                }
                outputFile = strtok(NULL, " ");
                // check if there's data after the filename
                tok = strtok(NULL, " ");
                if((outputFile == NULL) || (strcmp(outputFile, ">") == 0) || (tok != NULL && (strcmp(tok, "&") != 0))){
                    outputFile = NULL;
                    fileOutputError = 1;
                    char error_message[30] = "Incorrect input\n";
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    break;
                }
                continue;
            } else if(strcmp(tok, "&") == 0){
                // new command, add previous command and parameters to list
                params[i-1] = NULL;
                commands++;
                pCommand = add_command_to_list(pCommand, params, commands, outputFile);
                free(params);
                if((params = malloc(sizeof(char*))) == NULL){
                    char error_message[50] = "Error allocating memory when adding params\n";
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    exit(1);
                }
                i = 1;
                tok = strtok(NULL, " ");
                continue;
            } else if(strchr(tok, '>') != NULL){
                // user didn't include a whitespace before token
                commandPart = strtok(tok, ">");
                outputFile = strtok(NULL, " ");
                if((outputFile == NULL) || (strcmp(outputFile, ">") == 0) || (strtok(NULL, " ") != NULL)){
                    outputFile = NULL;
                    fileOutputError = 1;
                    char error_message[30] = "Incorrect input\n";
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
                if((params = realloc(params, (i+1)*sizeof(char *))) == NULL){
                    char error_message[50] = "Error allocating memory when adding params\n";
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    exit(1);
                }
                params[i-1] = commandPart;
                i++;
                break;
            }
            else if(strchr(tok, '&') != NULL){
                // user inputted multiple commands without a whitespace in between
                char *nextround = strtok(NULL, " ");
                char *param = strtok(tok, "&");
                char *string = strtok(NULL, " ");
                // add command and parameters to list
                params[i-1] = param;
                if((params = realloc(params, (i+1)*sizeof(char *))) == NULL){
                    char error_message[50] = "Error allocating memory when adding params\n";
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    exit(1);
                }
                i++;
                params[i-1] = NULL;
                commands++;
                pCommand = add_command_to_list(pCommand, params, commands, outputFile);
                // initialize params for next command
                free(params);
                if((params = malloc(sizeof(char*))) == 0){
                    char error_message[50] = "Error allocating memory when adding params\n";
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    exit(1);
                }
                i = 1;
                // check if there are more commands
                while(strchr(string, '&') != NULL){
                    param = strtok(string, "&");
                    string = strtok(NULL, "");
                    params[i-1] = param;
                    if((params = realloc(params, (i+1)*sizeof(char *))) == NULL){
                        char error_message[50] = "Error allocating memory when adding params\n";
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        exit(1);
                    }
                    i++;
                    params[i-1] = NULL;
                    commands++;
                    pCommand = add_command_to_list(pCommand, params, commands, outputFile);
                    free(params);
                    params = malloc(sizeof(char*));
                    i = 1;
                }
                // adding last command to list
                if(string != NULL){
                    params[i-1] = string;
                    if((params = realloc(params, (i+1)*sizeof(char *))) == 0){
                        char error_message[50] = "Error allocating memory when adding params\n";
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        exit(1);
                    }
                    i++;
                    params[i-1] = NULL;
                    commands++;
                    pCommand = add_command_to_list(pCommand, params, commands, outputFile);
                    free(params);
                    if((params = malloc(sizeof(char*))) == NULL){
                        char error_message[50] = "Error allocating memory when adding params\n";
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        exit(1);
                    }
                    i = 1;
                }
                tok = nextround;
                continue;
            }

            // first command or parameter related to previous command
            if((params = realloc(params, (i+1)*sizeof(char *))) == NULL){
                char error_message[50] = "Error allocating memory when adding params\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
                exit(1);
            }
            params[i-1] = tok;
            i++;
            tok = strtok(NULL, " ");
        }
        // continue to next round if there was a problem with output file
        if(fileOutputError == 1){
            if(batchMode == 0){
                printf("Wish> ");
            }
            continue;
        }
        // add last command to list
        if((params = realloc(params, (i+1)*sizeof(char *))) == NULL){
            char error_message[50] = "Error allocating memory when adding params\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
        params[i-1] = NULL;
        commands++;
        pCommand = add_command_to_list(pCommand, params, commands, outputFile);

        // create a thread for each command
        pid_t pids[commands];
        for(int nPid = 0; nPid<commands; nPid++){
            pids[nPid] = fork();

            switch (pids[nPid]) {
                case -1: {
                    char error_message[50] = "An error has occurred when starting thread\n";
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
                case 0: {
                    COMMAND *wanted = pCommand;

                    wanted = pCommand;
                    // find the command this thread should execute
                    while(wanted->number != nPid+1 && wanted != NULL){
                        wanted = wanted->pNext;
                    }

                    // command couldn't be found, something went wrong
                    if(wanted == NULL){
                        char error_message[50] = "Could not find command to execute\n";
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        break;
                    }

                    // check through all paths if command is available
                    int available = 0;
                    char fullPath[264] = "";
                    PATH *pathptr = path;
                    while(pathptr != NULL){
                        strcpy(fullPath, "");
                        strcat(fullPath, pathptr->path);
                        strcat(fullPath, "/");
                        strcat(fullPath, wanted->params[0]);

                        if(access(fullPath, X_OK) == 0){
                            available = 1;
                            break;
                        }
                        pathptr = pathptr->pNext;
                    }

                    if(available == 0){
                        char error_message[30] = "Command not in path\n";
                        write(STDERR_FILENO, error_message, strlen(error_message));
                    }
                    else {
                        // check which output to use
                        if(wanted->outputFile != NULL){
                            FILE *pFile = fopen(wanted->outputFile, "w");
                            if (pFile==NULL) {
                                char error_message[50] = "Could not open output file\n";
                                write(STDERR_FILENO, error_message, strlen(error_message));
                                break;
                            }
                            //change output of execv from stdout to file https://stackoverflow.com/questions/2605130/redirecting-exec-output-to-a-buffer-or-file
                            dup2(fileno(pFile), fileno(stdout));
                            fclose(pFile);
                        }
                        
                        if(execv(fullPath, wanted->params)  == -1) {
                            char error_message[30] = "An error has occurred\n";
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            break;
                        }
                        //program should not continue after execv: something went wrong if this part is reached
                        char error_message[30] = "An error has occurred\n";
                        write(STDERR_FILENO, error_message, strlen(error_message));
                    }
                }
            }
        }
        int n = commands;
        pid_t pid;
        // wait for threads to finish
        while(n > 0){
            if ((pid = wait(&status)) == -1) {
                    exit(1);
            }
            n--;
        }

        // empty previous commands before next round
        pCommand = empty_commands(pCommand);
        if(batchMode == 0){
            printf("Wish> ");
        }
    }
}

int main(int argc, char *argv[]){
    // too many parameters
    if(argc > 2){
        char error_message[30] = "Invalid input\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }
    else if(argc == 1){
        // interactive mode
        run_shell(0, stdin);
    }
    else {
        // batch mode
        FILE *pfile;
        if((pfile = fopen(argv[1], "r")) == NULL){
            char error_message[30] = "Could not open batch file\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
        run_shell(1, pfile);
        fclose(pfile);
    }
    return 0;
}