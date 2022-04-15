#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

void run_shell(){
    char *buffer = NULL;
    size_t len = 0;
    size_t read;
    char **params = NULL;
    char *tok;
    pid_t pid;
    int status;
    char *defaultPath = "/bin";
    char **path = NULL;

    printf("Wish> ");
    path = malloc(sizeof(char *));
    path[0] = defaultPath;
    while((read = getline(&buffer, &len, stdin)) != -1){

        //check buil-in commands
        char command[5] = "";
        strncpy(command, buffer, 4);
        command[4] = '\0';
        if(strcmp(command, "path") == 0){
            int paths = 0;
            //cut away command "path"
            strtok(buffer, " ");
            char *newPath = strtok(NULL, " ");

            if(newPath == NULL){
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
            } else {
                while(newPath != NULL){
                    paths++;
                    path = realloc(path, paths*sizeof(char *)); // TODO error check
                    path[paths-1] = newPath;
                    newPath = strtok(NULL, " ");
                }
            }
            /*int z = 0;
            while(z < paths){
                printf("%s\n", path[z]);
                z++;
            }*/
            printf("Wish> ");
            continue;
        }
        if(strcmp(command, "exit") == 0){
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
        buffer[strcspn(buffer, "\n")] = 0;
        tok = strtok(buffer, " ");
        //command[strlen(command)-1] = '\0';
        // tok = strtok(NULL, " ");
        // printf("param: %s\n", tok);
        params = malloc(sizeof(char *));
        int i = 1;
        while(tok != NULL){
            params = realloc(params, (i+1)*sizeof(char *));
            params[i-1] = tok;
            i++;
            tok = strtok(NULL, " ");
        }
        //*params = realloc(params, sizeof(params) + sizeof('\0'));
        params = realloc(params, (i+1)*sizeof(char *));
        params[i-1] = NULL;
        /*char params = strtok(NULL, "\n"); // tee tästä loop, joka lisää parametrit/loppu stringin arrayhyn
        strcat(params, "\0");*/
        
        switch (pid = fork()) {
            case -1: {
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
            }
            case 0: {
                int available = 0;
                char fullPath[264] = "";
                int x = 0;
                while(path[x] != NULL){
                //for (int x = 0; x < strlen(path); x++){
                    strcpy(fullPath, "");
                    strcat(fullPath, path[x]);
                    //fullPath = realloc(fullPath, sizeof(fullPath) + sizeof("/"));
                    strcat(fullPath, "/");
                    //fullPath = realloc(fullPath, sizeof(fullPath) + sizeof(params[0]));
                    strcat(fullPath, params[0]);
                    if(access(fullPath, X_OK) == 0){
                        available = 1;
                        break;
                    }
                    x++;
                }
                printf("%d\n", available);
                if(available == 0){
                    char error_message[30] = "An error has occurred\n";
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
                else {
                    if(execv(fullPath, params)  == -1) {
                        perror("execvp");
                        exit(1);
                    }
                }
                printf("juhuu");
            }
            default:
                printf("Child %d was born, waiting...\n", pid);
                if (wait(&status) == -1) {
                        perror("wait");
                        exit(1);
                }
                printf("Child died, status: 0x%X\n", status);
                break;
        }
        printf("Wish> ");
        free(params);
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