#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

// CAN I MAKE THIS SO I KNOW WHICH CHILD IS READY? I NEED TO REVERSE THIS
// SO THAT THE PARENT SELECT TO READ FROM THE CHILDREN MESSAGES AND
// TRY TO THEN DETECT WHICH CHILD SEND THE MESSAGE

// key notes to remember!
// FD_ZERO(fd_set *set): Clears all file descriptors from the set.
// FD_SET(int fd, fd_set *set): Adds the specified file descriptor to the set.
// FD_CLR(int fd, fd_set *set): Removes the specified file descriptor from the set.
// FD_ISSET(int fd, fd_set *set): Checks if the specified file descriptor is a member of the set.


int main() {
    // Create a pipe
    int pipe_fd[2][2];
    for (int j = 0; j<2;j++) {
        if (pipe(pipe_fd[j]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    // Fork to create a child process

    for (int i = 0; i<2;i++) {
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {  // Parent process
            // Close the read end of the pipe (not used by the parent)
            close(pipe_fd[i][0]);


            sleep(6); //this will trigger the timeout. pipe needs to remain open

            // Send a message through the pipe
            const char *message = "Hello, child process!";
            ssize_t bytes_written = write(pipe_fd[i][1], message, strlen(message));

            if (bytes_written == -1) {
                perror("write");
                exit(EXIT_FAILURE);
            }

            sleep(6);
            // Close the write end of the pipe without writing any message
            close(pipe_fd[i][1]); //pipe needs to remain open to trigger the timeout

        } else {  // Child process
            // Close the write end of the pipe (not used by the child)
            close(pipe_fd[i][1]);

            // Set up file descriptor set for select
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(pipe_fd[i][0], &read_fds);  // Monitor the read end of the pipe

            printf("Child process waiting for a message from the parent...\n");

            // Set timeout for 5 seconds
            struct timeval timeout;
            timeout.tv_sec = 5;
            timeout.tv_usec = 0;

            // Wait for input on the pipe or timeout
            int ready_fds = select(pipe_fd[i][0] + 1, &read_fds, NULL, NULL, &timeout);

            if (ready_fds == -1) {
                perror("select");
                exit(EXIT_FAILURE);
            }

            // Check if the pipe's read end is ready for reading
            if (FD_ISSET(pipe_fd[i][0], &read_fds)) {
                char buffer[256];
                ssize_t bytes_read = read(pipe_fd[i][0], buffer, sizeof(buffer) - 1);

                if (bytes_read == -1) {
                    perror("read");
                    exit(EXIT_FAILURE);
                }

                if (bytes_read == 0) {
                    printf("Child process received no message from the parent.\n");
                } else {
                    buffer[bytes_read] = '\0';
                    printf("Child process received the message: %s\n", buffer);
                }
            } else {
                printf("Child process did not receive any message within the timeout.\n");
            }

            printf("Child process waiting for 2nd time.\n");

            //wait again:
            FD_ZERO(&read_fds);
            FD_SET(pipe_fd[i][0], &read_fds);  // Monitor the read end of the pipe
            timeout.tv_sec = 5;
            timeout.tv_usec = 0;

            ready_fds = select(pipe_fd[i][0] + 1, &read_fds, NULL, NULL, &timeout);

            if (ready_fds == -1) {
                perror("select");
                exit(EXIT_FAILURE);
            }

            // Check if the pipe's read end is ready for reading
            if (FD_ISSET(pipe_fd[i][0], &read_fds)) {
                char buffer[256];
                ssize_t bytes_read = read(pipe_fd[i][0], buffer, sizeof(buffer) - 1);

                if (bytes_read == -1) {
                    perror("read");
                    exit(EXIT_FAILURE);
                }

                if (bytes_read == 0) {
                    printf("Child process received no message from the parent.\n");
                } else {
                    buffer[bytes_read] = '\0';
                    printf("Child process received the message: %s\n", buffer);
                }
            } else {
                printf("Child process did not receive any message within the timeout.\n");
            }

            // printf("Child reading pipe anyway\n");
            // char buffer[256];
            // read(pipe_fd[i][0], buffer, sizeof(buffer) - 1);
            // printf("Child process received the message: %s\n", buffer);

            // Close the read end of the pipe after reading
            close(pipe_fd[i][0]);
            exit(0);
        }
    }

    while ((wait(NULL)) > 0);
    printf("Parent process did its job.\n");
    return 0;
}
