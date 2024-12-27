#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#define MAX_COMMANDS 100
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
void execute_command(char *command) {
    char *args[MAX_COMMANDS];
    char *output_file = NULL;
    int i = 0;

    args[i] = strtok(command, " ");
    while (args[i] != NULL) {
        i++;
        args[i] = strtok(NULL, " ");
    }

    if (i > 2 && strcmp(args[i - 2], ">") == 0) {
        output_file = args[i - 1];
        args[i - 2] = NULL;
    }

    if (output_file) {
        execute_with_redirection(args, output_file);
    } else {
        if (fork() == 0) {
            execvp(args[0], args);
            perror("execvp");
            _exit(1);
        } else {
            wait(NULL);
        }
    }
}

void execute_with_pipe(char *input_buffer) {
    int i, n = 1, input = 0, first = 1;
    char *cmd_exec[MAX_COMMANDS];

    cmd_exec[0] = strtok(input_buffer, "|");
    while ((cmd_exec[n] = strtok(NULL, "|")) != NULL) n++;
    cmd_exec[n] = NULL;

    for (i = 0; i < n; i++) {
        int pipettes[2];
        pipe(pipettes);

        if (fork() == 0) {
            if (first == 1 && n > 1) {
                dup2(pipettes[1], STDOUT_FILENO);
            } else if (i == n - 1) {
                dup2(input, STDIN_FILENO);
            } else {
                dup2(input, STDIN_FILENO);
                dup2(pipettes[1], STDOUT_FILENO);
            }

            close(pipettes[0]);
            close(pipettes[1]);

            char *args[MAX_COMMANDS];
            int j = 0;
            args[j] = strtok(cmd_exec[i], " ");
            while (args[j] != NULL) {
                j++;
                args[j] = strtok(NULL, " ");
            }

            execvp(args[0], args);
            perror("execvp");
            exit(EXIT_FAILURE);
        } else {
            wait(NULL);
            close(pipettes[1]);
            input = pipettes[0];
            first = 0;
        }
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
