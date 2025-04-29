#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>


int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s <ruta_fichero> <cadena_busqueda>\n", argv[0]);
        return -1;
    }


    char buffer[1024];
    char line[1024];
    int fd, found = 0;
    fd = open(argv[1], O_RDONLY);

    if (fd == -1) {
        perror("Error opening file\n");
        exit(-1);
    }
    ssize_t bytes_read;
    int line_pos = 0;
    char current_char;

    while ((bytes_read = read(fd, &current_char, 1)) > 0) {
        // Add current char to buffer
        line[line_pos++] = current_char;

        if (current_char == '\n' || line_pos >= 1023) {
            line[line_pos] = '\0'; 

            if (strstr(line, argv[2]) != NULL) {
                write(STDOUT_FILENO, line, line_pos);
                found = 1;
            }

            // Reset buffer
            line_pos = 0;
        }
    } 
    // last line
    if (line_pos > 0) {
        line[line_pos] = '\0';
        if (strstr(line, argv[2])) {
            write(STDOUT_FILENO, line, line_pos);
            found = 1;
        }
    }
    if (bytes_read == -1) {
        perror("Error opening file\n");
        close(fd);
        exit(-1);
    }
    close(fd);

    // no matches
    if (!found) {
        write(STDOUT_FILENO, "\"", 1);
        write(STDOUT_FILENO, argv[2], strlen(argv[2]));
        write(STDOUT_FILENO, "\" not found.\n", 13);
    }

    return 0;
}


   

