#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

// Çıkış yönlendirme ve komut çalıştırma için temel fonksiyon
void execute_with_redirection(char **args, char *output_file) {
    pid_t pid = fork();
    if (pid == 0) { // Child process
        if (output_file) {
            int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("Failed to open file");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO); // Redirect stdout to the file
            close(fd);
        }
        execvp(args[0], args); // Execute the command with arguments
        perror("Execution failed");
        exit(EXIT_FAILURE);
    } else if (pid > 0) { // Parent process
        wait(NULL); // Wait for the child process to finish
    } else {
        perror("Fork failed");
    }
}

// Komut parse edip çalıştırma
void parse_and_execute(char *input) {
    char *command_part = strtok(input, ">");
    char *output_file = strtok(NULL, ">");

    if (command_part) {
        // Trim whitespace for output_file
        if (output_file) {
            while (*output_file == ' ') output_file++;
        }

        // Split command into arguments
        char *args[128]; // Assuming a maximum of 128 arguments
        int i = 0;
        char *token = strtok(command_part, " ");
        while (token != NULL && i < 128 - 1) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL; // Null-terminate the argument list

        execute_with_redirection(args, output_file);
    } else {
        printf("Invalid command format. Use: command > output_file\n");
    }
}

int main() {
    // Kullanıcıya komut girdisi için basit bir kabuk
    while (1) {
        char input[256];
        printf("Enter a command: ");
        if (!fgets(input, sizeof(input), stdin)) {
            perror("Failed to read input");
            break;
        }

        // Remove trailing newline
        input[strcspn(input, "\n")] = 0;

        // Exit the shell
        if (strcmp(input, "exit") == 0) {
            printf("Exiting shell...\n");
            break;
        }

        // Execute the command
        parse_and_execute(input);
    }

    return 0;
}
