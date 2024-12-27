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
void execute_with_redirection(char **args, char *output_file);

int command(int input, int first, int last, char *cmd) {
    int pipettes[2];
    pid_t pid;

    pipe(pipettes);
    pid = fork();

    if (pid == 0) {
        if (first == 1 && last == 0 && input == 0) {
            dup2(pipettes[1], STDOUT_FILENO);
        } else if (first == 0 && last == 0 && input != 0) {
            dup2(input, STDIN_FILENO);
            dup2(pipettes[1], STDOUT_FILENO);
        } else {
            dup2(input, STDIN_FILENO);
        }

        if (execvp(cmd, (char *const[]){cmd, NULL}) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } else if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (input != 0) {
        close(input);
    }

    close(pipettes[1]);

    if (last == 1) {
        close(pipettes[0]);
    }

    return pipettes[0];
}

void with_pipe_execute(char *input_buffer) {
    int i, n = 1, input = 0, first = 1;
    char *cmd_exec[MAX_COMMANDS];

    cmd_exec[0] = strtok(input_buffer, "|");
    while ((cmd_exec[n] = strtok(NULL, "|")) != NULL) n++;
    cmd_exec[n] = NULL;

    for (i = 0; i < n - 1; i++) {
        input = command(input, first, 0, cmd_exec[i]);
        first = 0;
    }
    command(input, first, 1, cmd_exec[i]);
}
// Pipe içeren komutları çalıştıran fonksiyon
void pipe_execute(char *input_buffer) {
    int i, n = 1, input = 0, first = 1;
    char *cmd_exec[MAX_COMMANDS];

    cmd_exec[0] = strtok(input_buffer, "|");
    while ((cmd_exec[n] = strtok(NULL, "|")) != NULL) n++;
    cmd_exec[n] = NULL;

    for (i = 0; i < n - 1; i++) {
        input = command(input, first, 0, cmd_exec[i]);
        first = 0;
    }
    command(input, first, 1, cmd_exec[i]);
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
