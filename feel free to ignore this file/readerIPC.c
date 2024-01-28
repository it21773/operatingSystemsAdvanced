#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int main() {
    // Create a named pipe (FIFO)
    const char* fifoPath = "/tmp/my_fifo";
    mkfifo(fifoPath, 0666);

    // Open the named pipe for reading
    int fd = open(fifoPath, O_RDONLY);

    // Receive the number from the other process
    int receivedNumber;
    read(fd, &receivedNumber, sizeof(int));

    // Print the received number
    printf("Received number: %d\n", receivedNumber);

    // Close the pipe and remove it
    close(fd);
    unlink(fifoPath);

    return 0;
}
