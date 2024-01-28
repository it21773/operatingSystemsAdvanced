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


void lockFile(int fd);
void unlockFile(int fd);
// void exitProperly(int);

int main(int argc, char *argv[]) {
    // for testing & debugging purposes
    argc = 2;
    argv[0] = "name of file";
    argv[1] = "ztest.txt";
    argv[2] = "50";

    int children = atoi(argv[2]);

    pid_t init = 1;
    pid_t cpid[children];
    int stringSize = 18;

    // printf("Initializing...\n");
    int count = 0;

    // int fd = open(argv[1], O_CREAT | O_TRUNC | O_WRONLY, 00600);
    // if (fd == -1) { //checking if open succeeded
    //     perror("open");
    //     printf("open error");
    // }

    FILE *file = fopen(argv[1], "w");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }
    
    // //writing as a Parent in the beginning of the file
    char parentWrite[stringSize];
    sprintf(parentWrite, "[PARENT] -> %d\n", getpid());

    // lockFile(fd);
    // write(fd, parentWrite, stringSize);
    // unlockFile(fd);
    fprintf(file, parentWrite); //this prints things more than once even though fork is after this code


    while (init != 0 && count<children) {
        init = fork();
        cpid[count] = init;
        if (cpid[count] < 0) {
            perror("fork");
            //handling error...
        } else if (cpid[count] > 0) { //Parent
            // printf("parent's pid %d, child's %d, %d\n", getpid(), init, cpid[count]);
            // strcpy(parentWrite, "Parent's pid ");
            // sprintf(pidString, "%d", getpid());
            // strcat(parentWrite, pidString);
            // strcat(parentWrite, ", child's ");
            // sprintf(pidString, "%d", init);
            // strcat(parentWrite, pidString);
            // strcat(parentWrite, ", ");
            // sprintf(pidString, "%d", cpid[count]);
            // strcat(parentWrite, pidString);
            // strcat(parentWrite, "\n");
            // sprintf(parentWrite, "parent's pid %d, child's %d, %d\n", getpid(), init, cpid[count]);

            //do parent stuff 
            // lockFile(fd);
            // write(fd, parentWrite, 41);
            // unlockFile(fd);
            waitpid(cpid[count], NULL, 0);
        } else { //Child
            // printf("child's pid %d, res_fork = %d, %d\n", getpid(), init, cpid[count]);
            //do child stuff

            // lockFile(fd);
            // char childWrite[stringSize];
            // sprintf(childWrite, "\n[CHILD] -> %d\n", getpid());
            // write(fd, childWrite, stringSize);
            // unlockFile(fd);
        }
        count++;
    }

    // for (int i = 0; i < argv[1][0]; i++) { //this performs worse
    //     cpid[i] = fork();
    //     if (cpid[i] < 0) {
    //         perror("fork");
    //     } else if (cpid[i] > 0) { //Parent
    //         printf("parent %d\n", cpid[i]);
    //         waitpid(cpid[i], NULL, 0);
    //     } else { //Child
    //         printf("child %d, mypid = %d\n", cpid[i], getpid());
    //     }
    // }

    // init = fork();
    // if (init < 0) {
    //     perror("fork");
    // } else if (init > 0) { //Parent
    //     printf("parent %d\n", init);
    //     waitpid(init, NULL, 0);
    // } else { //Child
    //     printf("child %d, mypid = %d\n", init, getpid());
    // }




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