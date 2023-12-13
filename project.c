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
#include <stdbool.h>


bool endswith(char *string, char *end);
void lockFile(int fileDescriptor);
void unlockFile(int fileDescriptor);
void exitGracefully(char *exit_message, int exnum);

int main(int argc, char *argv[]) {
    //for testing & debugging purposes
    // argc = 2;
    // argv[0] = "project.c";
    // argv[1] = "asd.txt";
    // argv[2] = "100";

    long num; //for input checking
    char *text; //for input checking

    if (argc != 3) {
        exitGracefully("Must input exactly 2 arguments, name of txt file and number of children.\n",1);
    }
    if (!endswith(argv[1],".txt")) {
        exitGracefully("First input must be a .txt file.\n",1);
    }
    num = strtol (argv[2], &text, 10);
    if ((text == argv[2]) || (*text != '\0')) {
        exitGracefully("Second input must be a number.\n",1);
    } else {
        if (num < 0) {
            exitGracefully("Second input must be a positive number.\n",1);
        }
    }

    int children = atoi(argv[2]); //to make sure we get the right number

    pid_t init = 1;
    pid_t cpid[children];
    int stringSize = 18;

    // // struct PipeInfo {
    // //     int pid;
    // //     int pipefds[2];
    // // };

    // // struct PipeInfo* pw_pipefds[children];
    // // struct PipeInfo* cw_pipefds[children];
    // int pw_pipefds[children][2];
    // int cw_pipefds[children][2];
    // int pw_pipesuccess[children];
    // int cw_pipesuccess[children];
    // char parentmessage[children][50];
    // char childmessage[children][5];
    
    int count = 0;

    int fd = open(argv[1], O_CREAT | O_TRUNC | O_WRONLY, 00600);
    if (fd == -1) { //checking if open succeeded
        perror("open");
        exitGracefully("open error\n",1);
    }

    // for (int i=0; i < children ; i++){
    //     pw_pipesuccess[i] = pipe(pw_pipefds[i]);
    //     // if (pipesuccess[i] == -1){
    //     //     exitGracefully("Error with pipes",1);
    //     // }
    //     cw_pipesuccess[i] = pipe(cw_pipefds[i]);
    // }

    //writing as a Parent in the beginning of the file
    char parentWrite[stringSize];
    sprintf(parentWrite, "[PARENT] -> %d\n", getpid());
    write(fd, parentWrite, stringSize);

    while (init != 0 && count<children) {
        init = fork();
        cpid[count] = init;
        if (cpid[count] < 0) {
            perror("fork");
            exitGracefully("fork error\n",1);
        } else if (cpid[count] > 0) { //Parent
            // for (int i=0; i < children ; i++){
            //     close(pw_pipefds[i][0]); // So you can only write on this pipe as a parent
            //     close(cw_pipefds[i][1]); // So you can only read on this pipe as a parent
            //     write(pw_pipefds[i][1], parentmessage, sizeof(parentmessage)); //Naming the child
            //     read(cw_pipefds[i][0], childmessage, sizeof(childmessage)); //Child's "done"
            //     //PROBLEM, cant know with pipe is with which child, need an identifier, the pid is 
            //     //a good first idea but i only know it after the fork
            // }
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

bool endswith(char *string, char *end){
    int string_length =  strlen(string);
    int end_length = strlen(end);

    if (end_length > string_length) {
        return false;
    }

    for (int i = 0 ; i < end_length ; i++) {
        if (string[string_length - i] != end[end_length - i]) {
            return false;
        }
    }
    return true;
}


void exitGracefully(char *exit_message, int exnum){
    printf("%s", exit_message);
    exit(exnum);
}