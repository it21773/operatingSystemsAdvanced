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


int main() {
    
    printf("the can only be one "); //using this it will be printed twice
    // printf("the can only be one\n"); //using this it will be printed just once

    pid_t cpid = fork();
    if (cpid < 0) {
        perror("fork");
    } else if (cpid > 0) { //Parent
        printf("parent %d\n", cpid);
        waitpid(cpid, NULL, 0);
    } else { //Child
        printf("child %d, mypid = %d\n", cpid, getpid());
    }
}