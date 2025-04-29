#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

/* CONST VARS */
const int max_line = 1024;
const int max_commands = 10;
#define max_redirections 3 //stdin, stdout, stderr
#define max_args 15

/* VARS TO BE USED FOR THE STUDENTS */
char * argvv[max_args];
char * filev[max_redirections];
int background = 0; 

/**
 * This function splits a char* line into different tokens based on a given character
 * @return Number of tokens 
 */
int tokenizar_linea(char *linea, char *delim, char *tokens[], int max_tokens) {
    int i = 0;
    char *token = strtok(linea, delim);
    while (token != NULL && i < max_tokens - 1) {
        tokens[i++] = token;
        token = strtok(NULL, delim);
    }
    tokens[i] = NULL;
    return i;
}

/**
 * This function processes the command line to evaluate if there are redirections. 
 * If any redirection is detected, the destination file is indicated in filev[i] array.
 * filev[0] for STDIN
 * filev[1] for STDOUT
 * filev[2] for STDERR
 */
void procesar_redirecciones(char *args[]) {
    //initialization for every command
    filev[0] = NULL;
    filev[1] = NULL;
    filev[2] = NULL;
    //Store the pointer to the filename if needed.
    //args[i] set to NULL once redirection is processed
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) {
            filev[0] = args[i+1];
            args[i] = NULL;
            args[i + 1] = NULL;
        } else if (strcmp(args[i], ">") == 0) {
            filev[1] = args[i+1];
            args[i] = NULL;
            args[i + 1] = NULL;
        } else if (strcmp(args[i], "!>") == 0) {
            filev[2] = args[i+1];
            args[i] = NULL; 
            args[i + 1] = NULL;
        }
    }
}

/**
 * This function processes the input command line and returns in global variables: 
 * argvv -- command an args as argv 
 * filev -- files for redirections. NULL value means no redirection. 
 * background -- 0 means foreground; 1 background.
 */
int procesar_linea(char *linea) {
    char *comandos[max_commands];
    int num_comandos = tokenizar_linea(linea, "|", comandos, max_commands);

    // Check if background is indicated
    if (strchr(comandos[num_comandos - 1], '&')) {
        background = 1;
        char *pos = strchr(comandos[num_comandos - 1], '&'); 
        // remove character 
        *pos = '\0';
    } else {
        background = 0;
    }
    
    pid_t child_pids[max_commands];
    int pipeline[max_commands-1][2];
    
    for (int i = 0; i < num_comandos - 1; i++) {
        if (pipe(pipeline[i]) < 0) {
            perror("Pipe creation error");
            return -1;
        }
    }
    
    // Execute each command in the pipeline
    for (int cmd_index = 0; cmd_index < num_comandos; cmd_index++) {
        tokenizar_linea(comandos[cmd_index], " \t\n", argvv, max_args);
        procesar_redirecciones(argvv);
        
        child_pids[cmd_index] = fork();
        
        if (child_pids[cmd_index] < 0) {
            // Fork failed
            perror("Fork failed");
            return -1;
        }
        
        if (child_pids[cmd_index] == 0) {
            // child
            
            if (num_comandos > 1) {
                if (cmd_index > 0) {
                    dup2(pipeline[cmd_index-1][0], STDIN_FILENO);
                }
                
                if (cmd_index < num_comandos - 1) {
                    dup2(pipeline[cmd_index][1], STDOUT_FILENO);
                }
            }
            for (int j = 0; j < num_comandos - 1; j++) { // Close all pipe FDs
                close(pipeline[j][0]);
                close(pipeline[j][1]);
            }
            
            //input redirection
            if (filev[0] != NULL) {
                int fd_in = open(filev[0], O_RDONLY);
                if (fd_in < 0) {
                    perror("Cannot open input file");
                    exit(2);
                }
                dup2(fd_in, STDIN_FILENO);
                close(fd_in);
            }
            
            //output redirection
            if (filev[1] != NULL) {
                int fd_out = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd_out < 0) {
                    perror("Cannot open output file");
                    exit(2);
                }
                dup2(fd_out, STDOUT_FILENO);
                close(fd_out);
            }
            
            // error redirection
            if (filev[2] != NULL) {
                int fd_err = open(filev[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd_err < 0) {
                    perror("Cannot open error file");
                    exit(2);
                }
                dup2(fd_err, STDERR_FILENO);
                close(fd_err);
            }
    
            execvp(argvv[0], argvv);
            perror("execution failed");
            exit(1);
        }
    }
    
    for (int i = 0; i < num_comandos - 1; i++) {
        close(pipeline[i][0]);
        close(pipeline[i][1]);
    }
    
    //background execution
    if (background) {
        printf("[%d]\n", child_pids[num_comandos-1]);
    } else {
        for (int i = 0; i < num_comandos; i++) {
            int status;
            waitpid(child_pids[i], &status, 0);
        }
    }
    
    return num_comandos;
}

int main(int argc, char *argv[]) {
    int fd;
    char line[max_line];
    char c;
    int i, bytes_read;
    
    if (argc != 2) {
        perror("File not provided");
        return -1;
    }
    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        return -1;
    }
    

    i = 0;
    while (1) {
        bytes_read = read(fd, &c, 1);
        if (bytes_read <= 0 || c == '\n') {
            break;
        }
        line[i++] = c;
        if (i >= max_line - 1) break;
    }
    line[i] = '\0';

    const char *expected_header = "## Script de SSOO";
    int is_valid = 1; // Indicator for valid header

    for (int idx = 0; ; idx++) {
        if (expected_header[idx] == '\0' && line[idx] == '\0') {
            break;
        }
        if (expected_header[idx] != line[idx]) { // Check for differences
            is_valid = 0;
            break;
        }
    }

    if (!is_valid) {
        perror("First line must be '## Script de SSOO'");
        close(fd);
        return -1;
    }

    //signal handler for background processes
    signal(SIGCHLD, SIG_IGN);
    
    while (1) {
        i = 0;
        while (1) { // read line
            bytes_read = read(fd, &c, 1);
            if (bytes_read <= 0 || c == '\n') {
                break;
            }
            line[i++] = c;
            if (i >= max_line - 1) break;
        }
        if (bytes_read <= 0 && i == 0) { //EOF
            break;
        }
        line[i] = '\0'; //null terminator
        if (i == 0) {
            perror("Empty line");
            close(fd);
            return -1;
        }
    
        procesar_linea(line);
        if (bytes_read <= 0) {
            break;
        }
    }
    
    close(fd);
    return 0;
}
