#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <signal.h>
#include <string.h>


//to-do:
// handle errors and user mistakes


void lockFile(int fileDescriptor);
void unlockFile(int fileDescriptor);

int main(int argc, char *argv[]) {
    //for testing & debugging purposes
    // argc = 2;
    // argv[0] = "project.c";
    // argv[1] = "asd.txt";
    // argv[2] = "100";

    if (argc < 3) {
        prinf("Wrong arguments");
    }
    //handle wrong arguments

    int children = atoi(argv[2]);

    pid_t init = 1;
    pid_t cpid[children];
    int stringSize = 18;
    
    int count = 0;

    int fd = open(argv[1], O_CREAT | O_TRUNC | O_WRONLY, 00600);
    if (fd == -1) { //checking if open succeeded
        perror("open");
        printf("open error");
    }

    //writing as a Parent in the beginning of the file
    char parentWrite[stringSize];
    sprintf(parentWrite, "[PARENT] -> %d\n", getpid());
    write(fd, parentWrite, stringSize);

    while (init != 0 && count<children) {
        init = fork();
        cpid[count] = init;
        if (cpid[count] < 0) {
            perror("fork");
            //handling error...
        } else if (cpid[count] > 0) { //Parent
            waitpid(cpid[count], NULL, 0);
        } else { //Child
            lockFile(fd);
            char childWrite[stringSize];
            sprintf(childWrite, "\n[CHILD] -> %d\n", getpid());
            write(fd, childWrite, stringSize);
            unlockFile(fd);
        }
        count++;
    }
}

void lockFile(int fileDescriptor) {
    struct flock fl;
    fl.l_type = F_WRLCK;  // Write lock
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;  // Lock the entire file

    if (fcntl(fileDescriptor, F_SETLKW, &fl) == -1) {
        perror("Error locking file");
        exit(EXIT_FAILURE);
    }
}

void unlockFile(int fileDescriptor) {
    struct flock fl;
    fl.l_type = F_UNLCK;  // Unlock
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;  // Unlock the entire file

    if (fcntl(fileDescriptor, F_SETLKW, &fl) == -1) {
        perror("Error unlocking file");
        exit(EXIT_FAILURE);
    }
}