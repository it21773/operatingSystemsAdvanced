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
    pid_t term;
    for (int i=0;i<children;i++){
        cpid[i]=-42;
    }

    int pw_pipefds[children][2];
    int cw_pipefds[children][2];
    int pw_pipesuccess[children];
    int cw_pipesuccess[children];
    char parentmessage[children][100];
    char childmessage[5] = "done";
    char readmessage[100];
    
    int count = 0;

    int fd = open(argv[1], O_CREAT | O_TRUNC | O_WRONLY, 00600);
    if (fd == -1) { //checking if open succeeded
        perror("open");
        exitGracefully("open error\n",1);
    }

    for (int i=0; i < children ; i++){
        pw_pipesuccess[i] = pipe(pw_pipefds[i]);
        if (pw_pipesuccess[i] == -1){
            exitGracefully("Error with pipes",1);
        }
        cw_pipesuccess[i] = pipe(cw_pipefds[i]);
        if (cw_pipesuccess[i] == -1){
            exitGracefully("Error with pipes",1);
        }
    }

    //writing as a Parent in the beginning of the file
    char parentWrite[50];
    sprintf(parentWrite, "[PARENT] -> %d\n", getpid());
    write(fd, parentWrite, strlen(parentWrite));

    while (init != 0 && count<children) {
        init = fork();
        cpid[count] = init;
        if (cpid[count] < 0) {
            perror("fork");
            exitGracefully("fork error\n",1);
        } else if (cpid[count] > 0) { //Parent
            close(pw_pipefds[count][0]); // So you can only write on this pipe as a parent (close read side)
            close(cw_pipefds[count][1]); // So you can only read on this pipe as a parent (close write side)

            sprintf(parentmessage[count], "Hello child, I am your father and I call you: name%d",count); //writing the message(naming)
            write(pw_pipefds[count][1], parentmessage[count], sizeof(parentmessage[count])); //Name of the child
            read(cw_pipefds[count][0], readmessage, sizeof(readmessage)); //Child's "done"

            // waitpid(cpid[count], NULL, 0);
        } else { //Child
            close(pw_pipefds[count][1]); // So you can only read on this pipe as a child (close write side)
            close(cw_pipefds[count][0]); // So you can only write on this pipe as a child (close read side)

            read(pw_pipefds[count][0], readmessage, sizeof(readmessage)); //Message of name bestowed unto the child
            char* name = readmessage+sizeof("Hello child, I am your father and I call you: ")-1; //extracting the name
            
            //writing on file
            lockFile(fd);
            char childWrite[100];
            sprintf(childWrite, "\n%d -> %s\n",getpid(), name);
            write(fd, childWrite, strlen(childWrite));
            unlockFile(fd);
            //end of write on file
            write(cw_pipefds[count][1], childmessage, sizeof(childmessage)); //Child's writing "done"
            exitGracefully("",0);
        }
        count++;
    }

    while ((term = wait(NULL)) > 0); //waiting for all children to finish, still younger children write last, shouldn't it be mixed(random)?
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