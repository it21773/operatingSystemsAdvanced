#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
    fd_set readfds;
    struct timeval timeout;
    int result;

    // Set up the file descriptor set with stdin
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    // Set up the timeout (5 seconds)
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    // Wait for input on stdin or timeout
    result = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);

    if (result == -1) {
        perror("select");
        exit(EXIT_FAILURE);
    } else if (result == 0) {
        printf("Timeout reached. No input received.\n");
    } else {
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            printf("Input is available on stdin. Reading...\n");

            char buffer[256];
            ssize_t bytesRead = read(STDIN_FILENO, buffer, sizeof(buffer));

            if (bytesRead == -1) {
                perror("read");
                exit(EXIT_FAILURE);
            } else if (bytesRead == 0) {
                printf("End of file (EOF) received.\n");
            } else {
                buffer[bytesRead] = '\0';
                printf("Read from stdin: %s", buffer);
            }
        }
    }

    return 0;
}
