#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

int main() {
    int pipe_fd[2];
    pid_t child_pid;

    // Create a pipe
    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Fork a child process
    child_pid = fork();

    if (child_pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) {
        // Child process

        // Close write end of the pipe
        close(pipe_fd[1]);

        // Read data from the pipe
        char message[100];
        printf("Child sleeping\n");
        sleep(8);
        printf("Child woke\n");
        ssize_t bytes_read = read(pipe_fd[0], message, sizeof(message));

        if (bytes_read == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        printf("Child received message: %.*s\n", (int)bytes_read, message);

        // Close read end of the pipe
        close(pipe_fd[0]);

        exit(EXIT_SUCCESS);
    } else {
        // Parent process

        // Close read end of the pipe
        close(pipe_fd[0]);

        // Use select to check if it's possible to write to the pipe
        fd_set write_fds;
        FD_ZERO(&write_fds);
        FD_SET(pipe_fd[1], &write_fds);

        struct timeval timeout;
        timeout.tv_sec = 3;  // Set timeout to 3 seconds (less than child's sleep duration)
        timeout.tv_usec = 0;

        char message[] = "Hello, child one!";
        ssize_t bytes_written;

        bytes_written = write(pipe_fd[1], message, sizeof(message));
        sleep(2);
        
        int ready = select(pipe_fd[1] + 1, NULL, &write_fds, NULL, &timeout);
        printf("%d\n", ready);
        if (ready == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        if (ready > 0) {
            // It's possible to write to the pipe

            // Write message to the pipe
            sprintf(message, "Hello, child two!");
            bytes_written = write(pipe_fd[1], message, sizeof(message));

            if (bytes_written == -1) {
                perror("write");
                exit(EXIT_FAILURE);
            }

            printf("Parent sent message to child\n");
        } else {
            printf("Timeout. Parent did not write to child\n");
        }

        // Close write end of the pipe
        close(pipe_fd[1]);

        // Wait for the child to finish
        wait(NULL);

        exit(EXIT_SUCCESS);
    }

    return 0;
}
