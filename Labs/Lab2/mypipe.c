#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
    int pipefd[2];
    char message[] = "hello";
    char buffer[256];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        close(pipefd[0]);

        write(pipefd[1], message, sizeof(message));

        close(pipefd[1]);

        exit(EXIT_SUCCESS);
    } else {
        close(pipefd[1]);
        ssize_t bytesRead = read(pipefd[0], buffer, sizeof(buffer));

        if (bytesRead == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        printf("Received message from child: %s\n", buffer);
        close(pipefd[0]);

        waitpid(pid, NULL, 0);

        exit(EXIT_SUCCESS);
    }

    return 0;
}