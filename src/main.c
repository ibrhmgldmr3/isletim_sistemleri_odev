#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>

using namespace std;

void execute_command(const char *cmd, int input_fd, int output_fd) {
    pid_t pid = fork();
    if (pid == 0) {
        if (input_fd != 0) {
            dup2(input_fd, 0);
            close(input_fd);
        }
        if (output_fd != 1) {
            dup2(output_fd, 1);
            close(output_fd);
        }
        execlp("/bin/sh", "sh", "-c", cmd, (char *)NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    }
}

int main() {
    int pipe1[2], pipe2[2];

    pipe(pipe1);
    execute_command("find /etc", 0, pipe1[1]);
    close(pipe1[1]);

    pipe(pipe2);
    execute_command("grep ssh", pipe1[0], pipe2[1]);
    close(pipe1[0]);
    close(pipe2[1]);

    execute_command("grep conf", pipe2[0], 1);
    close(pipe2[0]);

    for (int i = 0; i < 3; ++i) {
        wait(NULL);
    }

    return 0;
}