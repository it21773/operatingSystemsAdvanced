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

    // Open the named pipe for writing
    int fd = open(fifoPath, O_WRONLY);

    // Send a number to the other process
    int numberToSend = 42;
    write(fd, &numberToSend, sizeof(int));

    // Close the pipe and remove it
    close(fd);
    unlink(fifoPath);

    return 0;
}
