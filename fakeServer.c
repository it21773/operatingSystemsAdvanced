#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <project.h>

int main(int argc, char *argv[]) {

    printf("argc: %d ", argc);
    for(int i=0;i<argc;i++){
        printf("argv[%d]: %s | ", i, argv[i]);
    }
    printf("\n\n\n");
    start(argc, argv);

    // int init = fork();
    // if (init == -1) {
    //     printf("fork error\n");
    //     exit(1);
    // } else if (init == 0) { //Parent
    //     start(argc, argv);
    //     while ((wait(NULL)) > 0);
    // } else { //Child
    //     // sleep(5);
    //     // printf("\nfakeServer sending order\n");
    //     // sendSleepOrder(6);
    //     // printf("\nfakeServer send the order\n");

    //     // Create a named pipe (FIFO)
    //     const char* fifoPath = "/tmp/my_fifo";
    //     mkfifo(fifoPath, 0666);

    //     // Open the named pipe for writing
    //     int fd = open(fifoPath, O_WRONLY);

    //     // Send a number to the other process
    //     int numberToSend = 42;
    //     write(fd, &numberToSend, sizeof(int));

    //     // Close the pipe and remove it
    //     close(fd);
    //     unlink(fifoPath);

    //     return 0;
    // }
    
}