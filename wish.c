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
    char path[264];
    struct path *pNext;
};
typedef struct path PATH;

COMMAND *add_command_to_list(COMMAND *pCommand, char **params, int commandNro, char *outputFile){
    printf("ADDING\n");
    COMMAND *pNew = NULL, *ptr = NULL;

    if((pNew = malloc(sizeof(COMMAND))) == NULL){
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
    }
    printf("malloc 1\n");
    pNew->number = commandNro;
    pNew->outputFile = outputFile;
    pNew->pNext = NULL;

    if ((pNew->params = malloc(sizeof(char*))) == NULL) {
        fprintf(stderr, "Malloc failed.\n");
        exit(1);
    }
    printf("malloc 2\n");
    int i = 0;
    while(params[i] != NULL){
        pNew->params = realloc(pNew->params, (i+1)*sizeof(char*));
        pNew->params[i] = params[i];
        i++;
    }
    pNew->params = realloc(pNew->params, (i+1)*sizeof(char*));
    pNew->params[i] = NULL;

    if(pCommand == NULL){
        printf("**************** YES IT IS ********************\n");
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
        if(ptr == NULL){
            printf("should not happen");
        }
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
}

PATH *add_paths(char *buffer, PATH *path){
    //remove newline so that it doesn't mess up the (last) path
    buffer[strcspn(buffer, "\n")] = 0;

    //cut away command "path"
    strtok(buffer, " ");
    char *newPath = strtok(NULL, " ");
    PATH *ptr;

    if(newPath == NULL){
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
    } else {
        if(path != NULL){
            path = empty_paths(path);
        }
        while(newPath != NULL){
            PATH *pNew = NULL;
            if((pNew = malloc(sizeof(PATH))) == NULL){
                //ERROR
            }
            strcpy(pNew->path, newPath);
            printf("%ld\n", strlen(newPath));
            pNew->pNext = NULL;

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
    /*int z = 0;
    while(z < paths){
        printf("%s\n", path[z]);
        z++;
    }*/
    return path;
}


void run_shell(){
    char *buffer = NULL;
    size_t len = 0;
    size_t read;
    char **params = NULL;
    char *tok;
    pid_t pid;
    int status;
    char *defaultPath = "/bin";
    PATH *path = NULL;
    COMMAND *pCommand = NULL;

    printf("Wish> ");
    path = malloc(sizeof(PATH)); // TODO: ADD ERROR CHECK
    strcpy(path->path, defaultPath);
    path->pNext = NULL;
    while((read = getline(&buffer, &len, stdin)) != -1){

        //check built-in commands
        char command[5] = "";
        strncpy(command, buffer, 4);
        command[4] = '\0';
        if(strcmp(command, "path") == 0){
            printf("Path?\n");
            path = add_paths(buffer, path);
            printf("Wish> ");
            continue;
        }
        if(strcmp(command, "exit") == 0){
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
            //cut away command "cd"
            strtok(buffer, " ");
            char *arg = NULL;
            arg = strtok(NULL, "\n");
            printf("%s\n", arg);
            if(arg == NULL || strtok(NULL, " ") != NULL){
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
            } else {
                printf("Changing directories with chdir()\n");
                if(chdir(arg) != 0){
                    char error_message[30] = "An error has occurred\n";
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
            }
            printf("Wish> ");
            continue;
        }
        //remove newline so that it doesn't confuse execv
        buffer[strcspn(buffer, "\n")] = 0;
        tok = strtok(buffer, " ");
        //command[strlen(command)-1] = '\0';
        // tok = strtok(NULL, " ");
        // printf("param: %s\n", tok);
        params = malloc(sizeof(char *));
        int i = 1, fileOutputError = 0, commands = 1;
        char *outputFile = NULL;
        while(tok != NULL){
            if(strcmp(tok, ">") == 0){
                outputFile = strtok(NULL, " ");
                if((strcmp(outputFile, ">") == 0) || (strtok(NULL, " ") != NULL)){
                    outputFile = NULL;
                    fileOutputError = 1;
                    char error_message[30] = "An error has occurred\n";
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
                break;
            } else if(strcmp(tok, "&") == 0){
                params[i-1] = NULL;
                pCommand = add_command_to_list(pCommand, params, commands, outputFile);
                commands++;
                free(params);
                params = malloc(sizeof(char*));
                i = 1;
                tok = strtok(NULL, " ");
                continue;
            }
            //printf("realloc next new %ld old %ld\n", (i+1)*sizeof(char *), sizeof(params));
            params = realloc(params, (i+1)*sizeof(char *)); //TODO error handling
            //printf("realloc 2\n");
            params[i-1] = tok;
            i++;
            tok = strtok(NULL, " ");
        }
        if(fileOutputError == 1){
            printf("Wish> ");
            continue;
        }
        params = realloc(params, (i+1)*sizeof(char *)); //TODO error handling
        printf("realloc 3\n");
        params[i-1] = NULL;
        pCommand = add_command_to_list(pCommand, params, commands, outputFile);
        //debug: check params
        COMMAND *testi;
        testi = pCommand;
        while(testi != NULL){
            int f = 0;
            printf("Valmis: %d ", testi->number);
            while(testi->params[f] != NULL){
                printf("%s ", testi->params[f]);
                f++;
            }
            printf("\n");
            testi = testi->pNext;
        }
        pid_t pids[commands];
        for(int nPid = 0; nPid<commands; nPid++){
            pids[nPid] = fork();
            printf("%d\n", pids[nPid]);
            printf("nPid: %d, commands: %d\n",nPid, commands);
            switch (pids[nPid]) {
                case -1: {
                    char error_message[30] = "An error has occurred\n";
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
                case 0: {
                    COMMAND *wanted = pCommand;

                    wanted = pCommand;
                    while(wanted->number != nPid+1){
                        //printf("Loopissa: %d\n", wanted->number);
                        wanted = wanted->pNext;
                    }

                    int available = 0;
                    char fullPath[264] = "";
                    PATH *pathptr = path;
                    while(pathptr != NULL){
                    //for (int x = 0; x < strlen(path); x++){
                        printf("Path[x] = %s\n", pathptr->path);
                        strcpy(fullPath, "");
                        strcat(fullPath, pathptr->path);
                        //fullPath = realloc(fullPath, sizeof(fullPath) + sizeof("/"));
                        strcat(fullPath, "/");
                        //fullPath = realloc(fullPath, sizeof(fullPath) + sizeof(params[0]));
                        strcat(fullPath, wanted->params[0]);
                        printf("Path: %s\n", fullPath);
                        if(access(fullPath, X_OK) == 0){
                            available = 1;
                            break;
                        }
                        pathptr = pathptr->pNext;
                    }
                    printf("%d\n", available);
                    if(available == 0){
                        char error_message[30] = "An error has occurred\n";
                        write(STDERR_FILENO, error_message, strlen(error_message));
                    }
                    else {
                        if(wanted->outputFile != NULL){
                            FILE *pFile = fopen(wanted->outputFile, "w");
                            if (pFile==NULL) {
                                char error_message[30] = "An error has occurred\n";
                                write(STDERR_FILENO, error_message, strlen(error_message));
                            }
                            //change output from stdout to file
                            dup2(fileno(pFile), fileno(stdout));
                            fclose(pFile);
                        }
                        printf("executing\n");
                        if(execv(fullPath, wanted->params)  == -1) {
                            char error_message[30] = "An error has occurred\n";
                            write(STDERR_FILENO, error_message, strlen(error_message));
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
        while(n > 0){
                if ((pid = wait(&status)) == -1) {
                        perror("wait");
                        exit(1);
                }
                printf("Child %ld died, status: 0x%X\n", (long)pid, status);
                break;
            n--;
        }

        pCommand = empty_commands(pCommand);
        
        printf("Memory freed\n");
        printf("Wish> ");
    }
}

int main(int argc, char *argv[]){
    if(argc > 2){
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
    }
    else if(argc == 1){
        run_shell();
    }
    else {
        // batch mode
    }

    return 0;
}