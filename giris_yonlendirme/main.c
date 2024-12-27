#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MAX_CMD_LEN 1024

void execute_command(char *cmd);

int main() {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while (1) {
        // Prompt
        printf("> ");
        fflush(stdout);

        // Read input
        read = getline(&line, &len, stdin);
        if (read == -1) {
            perror("getline");
            exit(EXIT_FAILURE);
        }

        // Remove newline character
        line[strcspn(line, "\n")] = '\0';

        // Check for quit command
        if (strcmp(line, "quit") == 0) {
            break;
        }

        // Execute the command
        execute_command(line);
    }

    // Free the allocated memory for line
    free(line);

    return 0;
}

void execute_command(char *cmd) {
    char *input_file = NULL;
    char *command = strtok(cmd, "<");

    // Check if there is an input redirection
    if (command != NULL) {
        input_file = strtok(NULL, "<");
    }

    if (input_file != NULL) {
        // Remove leading and trailing whitespaces
        while (*input_file == ' ') input_file++;
        input_file[strcspn(input_file, " ")] = '\0';
    }

    // Execute the command with input redirection if needed
    if (input_file != NULL) {
        // Check if the input file exists
        int fd = open(input_file, O_RDONLY);
        if (fd == -1) {
            printf("Giriş dosyası bulunamadı.\n");
            return;
        }

        // Redirect stdin to the input file
        if (dup2(fd, STDIN_FILENO) == -1) {
            perror("dup2");
            close(fd);
            return;
        }

        close(fd);
    }

    // Execute the command
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return;
    } else if (pid == 0) {
        // Child process
        execlp(command, command, (char *)NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
    }
}