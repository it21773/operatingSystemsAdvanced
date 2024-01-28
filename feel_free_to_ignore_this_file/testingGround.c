#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>

// notes/ to-do:
// this is outdated by project.c

#define PARENT_W 0
#define CHILD_W 1

#define TO_READ 0
#define TO_WRITE 1

#define N 3


typedef struct Child_info {
    pid_t pid;
    char* name;
    int** pipe;
} Child_info;

int children;
Child_info* child_info;
int fd;
char parentmessage[100];


void on_sig_term_parent(int sig);
void on_sig_term_child(int sig);
void free_resources_parent();
void free_resources_child();
bool is_read_pipe_ready(int* read_pipe, int timelimit);
int** create_2d_INT_array(int index, int jndex);
void free_2d_INT_array(int** array, int index);
void lockFile(int fileDescriptor);
void unlockFile(int fileDescriptor);
bool endswith(char *string, char *end);
void exitGracefully(char *exit_message, int exnum);
bool stdout_to_file(FILE *new_stdout);
void restore_stdout(FILE *new_stdout);

int main(int argc, char *argv[]) {

    long num; //for input checking
    char *text; //for input checking
    if (argc != 3) {
        exitGracefully("Must input exactly 2 arguments, name of txt file and number of children.\n", EXIT_FAILURE);
    }
    if (!endswith(argv[1],".txt")) {
        exitGracefully("First input must be a .txt file.\n", EXIT_FAILURE);
    }
    num = strtol (argv[2], &text, 10);
    if ((text == argv[2]) || (*text != '\0')) {
        exitGracefully("Second input must be a number.\n", EXIT_FAILURE);
    } else {
        if (num < 0) {
            exitGracefully("Second input must be a positive number.\n", EXIT_FAILURE);
        }
    }

    pid_t init = 1;
    char readmessage[100];

    children = atoi(argv[2]); //to make sure we get the right number

    child_info = (Child_info*) malloc(children*sizeof(Child_info));

    for(int i=0; i<children; i++){
        child_info[i].pipe = create_2d_INT_array(2,2);
    }


    fd = open(argv[1], O_CREAT | O_TRUNC | O_WRONLY, 00600);
    if (fd == -1) { //checking if open succeeded
        perror("open");
        exitGracefully("Open error\n", EXIT_FAILURE);
    }


    for (int i=0; i < children ; i++){
        if(pipe(child_info[i].pipe[PARENT_W])) {
            close(fd);
            perror("pipes");
            exitGracefully("Error with pipes", EXIT_FAILURE);
        }
        if(pipe(child_info[i].pipe[CHILD_W])) {
            close(fd);
            perror("pipes");
            exitGracefully("Error with pipes", EXIT_FAILURE);
        }
    }


    //this is earlier we can handle the signal right? Otherwise we need more signal handling functions
    // in which they close fewer things since we havent yet opened everything
    signal(SIGINT, (void (*)(int))on_sig_term_parent);


    //writing as a Parent in the beginning of the file
    char parentWrite[50];
    sprintf(parentWrite, "[PARENT] -> %d\n \n\n\n\n\n\n\n\n", getpid()); //\n helps with seeing ordinality(1,2,3) -> child0 in line 10, child2 in line 12
    write(fd, parentWrite, strlen(parentWrite));

    for (int i = 0; i<children;i++) {
        init = fork();
        child_info[i].pid = init;
        if (init < 0) {
            //can the error be handled better?
            perror("fork");
            exitGracefully("Fork error\n", EXIT_FAILURE);
        } else if (init > 0) { //Parent
            //do nothing, just birth children
        } else { //Child
            signal(SIGINT, (void (*)(int))on_sig_term_child);

            char* name;
            int count=0;
            int to_sleep;
            char childWrite[100];
            char childmessage[5] = "done";

            close(child_info[i].pipe[PARENT_W][TO_WRITE]); // So you can only read on this pipe as a child (close write side)
            close(child_info[i].pipe[CHILD_W][TO_READ]); // So you can only write on this pipe as a child (close read side)

            read(child_info[i].pipe[PARENT_W][TO_READ], readmessage, sizeof(readmessage)); //Message of name bestowed unto the child
            
            name = readmessage+sizeof("Hello child, I am your father and I call you: ")-1; //extracting the name
            printf("I'm child #%d and my name is %s", i, name);

            //writing on file
            lockFile(fd);
            sprintf(childWrite, "%d -> %s\n",getpid(), name);
            write(fd, childWrite, strlen(childWrite));
            unlockFile(fd);
            //end of write on file

            write(child_info[i].pipe[CHILD_W][TO_WRITE], childmessage, sizeof(childmessage)); //Child's writing "done"

            //reading second message
            read(child_info[i].pipe[PARENT_W][TO_READ], readmessage, sizeof(readmessage)); //Parent's "good job"

            //reading messages and sleeping appropriately
            while(true) {
                write(child_info[i].pipe[CHILD_W][TO_WRITE], childmessage, sizeof(childmessage)); //Child's writing "done"

                printf("Child #%d awaiting orders no.%d\n", i, count++);
                read(child_info[i].pipe[PARENT_W][TO_READ], readmessage, sizeof(readmessage)); //reads sleep orders
                
                to_sleep = (atoi(readmessage)) % 5;
                printf("Child #%d sleeping for: %d\n", i, to_sleep);
                sleep(to_sleep);
                printf("Child #%d awoke from slumber\n", i);
            }

            //Unreachable code now that we got the infinite loop 

            //closing the pipes completely from child
            close(child_info[i].pipe[PARENT_W][TO_READ]);
            close(child_info[i].pipe[CHILD_W][TO_WRITE]);
        
            // this is necessary
            free(child_info[i].pipe[PARENT_W]);
            free(child_info[i].pipe[CHILD_W]);
            
            // is this neccessary? prob yes
            free(child_info);

            close(fd);

            exitGracefully("",0);
        }
    }

    //Only parent is operating the code below this point

    //closing the file now that all children were born and have already shared that
    close(fd);

    //if i use exec these can go up since the children wont know about them anyway
    int count=0;

    //Sending first message
    for(int i=0; i<children; i++){
        close(child_info[i].pipe[PARENT_W][TO_READ]); // So you can only write on this pipe as a parent (close read side)
        close(child_info[i].pipe[CHILD_W][TO_WRITE]); // So you can only read on this pipe as a parent (close write side)

        printf("Parent naming child %d\n", i);
        sprintf(parentmessage, "Hello child, I am your father and I call you: name%d",i); //writing the message(naming)
        write(child_info[i].pipe[PARENT_W][TO_WRITE], parentmessage, strlen(parentmessage)+1); //Name of the child, +1 to include null terminator \0 although is works fine without it
    }

    for(int i=0; i<children; i++){
        read(child_info[i].pipe[CHILD_W][TO_READ], readmessage, sizeof(readmessage)); //Child's "done"
        // printf("Parent reading message from child %d: %s\n", i, readmessage);

        sprintf(parentmessage, "Good job!"); //writing the second message
        // printf("Parent sending second message to child %d : %s\n", i, parentmessage[i]);
        write(child_info[i].pipe[PARENT_W][TO_WRITE], parentmessage, strlen(parentmessage)+1); //Name of the child, +1 to include null terminator \0 although is works fine without it
        
    }

    //Start of infinite messages
    while(true) {
        for(int i=0; i<children; i++){
            printf("Attempting cycle %d for child #%d\n", count++, i);
            bool check = is_read_pipe_ready(child_info[i].pipe[CHILD_W], 3);
            if (check) { //if child has responded with done
                read(child_info[i].pipe[CHILD_W][TO_READ], readmessage, sizeof(readmessage)); //Child's "done"

                sprintf(parentmessage, "%d", i); //writing for how many seconds to sleep
                write(child_info[i].pipe[PARENT_W][TO_WRITE], parentmessage, strlen(parentmessage)+1); //Name of the child, +1 to include null terminator \0 although is works fine without it
                printf("Parent send sleep message to child #%d\n", i);
            } else {
                printf("Parent SKIPPED child #%d\n", i);
            }
        }
    }

    //Unreachable code now that we got the infinite loop
    
    printf("Parent no longer waits for messages!!\n");

    // waitpid(cpid[i], NULL, 0);
    while ((wait(NULL)) > 0); //waiting for all children to finish


    // these frees could potentially happen sooner?
    for (int i=0;i<children;i++){
        //closing the pipes completely from parent
        close(child_info[i].pipe[PARENT_W][TO_WRITE]);
        close(child_info[i].pipe[CHILD_W][TO_READ]);

        free_2d_INT_array(child_info[i].pipe, 2);
    }
    free(child_info);

    printf("end of program\n");

    exit(0);
}


bool is_read_pipe_ready(int* read_pipe, int timelimit){

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(read_pipe[TO_READ], &read_fds);  // Monitor the read end of the pipe

    // Set timeout
    struct timeval timeout;
    timeout.tv_sec = timelimit;
    timeout.tv_usec = 0;

    int ready_fds = select(read_pipe[TO_READ] + 1, &read_fds, NULL, NULL, &timeout);

    if (ready_fds == -1) {
        perror("select");
        exitGracefully("Select failed", EXIT_FAILURE);
    }

    // Check if the pipe's read end is ready for reading
    if (FD_ISSET(read_pipe[TO_READ], &read_fds)) {
        return true;
    } else {
        return false;
    }
}

int** create_2d_INT_array(int index, int jndex) {

    int** array = (int**)malloc(index * sizeof(int*));
    if (array == NULL) {
        perror("Memory allocation failed");
        exitGracefully("Memory allocation failed\n", EXIT_FAILURE);
    }
    for (int i = 0; i < index; ++i) {
        array[i] = (int*)malloc(jndex * sizeof(int));
        if (array[i] == NULL) {
            perror("Memory allocation failed");
            exitGracefully("Memory allocation failed\n", EXIT_FAILURE);
        }
    }
    return array;
}

void free_2d_INT_array(int** array, int index) {
    for (int i = 0; i < index; ++i) {
        free(array[i]);
    }
    free(array);
}

void lockFile(int fileDescriptor) {
    struct flock fl;
    fl.l_type = F_WRLCK;  // Write lock
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;  // Lock the entire file

    if (fcntl(fileDescriptor, F_SETLKW, &fl) == -1) {
        perror("Error locking file");
        exitGracefully("Error locking file", EXIT_FAILURE);
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
        exitGracefully("Error unlocking file", EXIT_FAILURE);
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

void free_resources_child(){
    for (int i = 0; i < children; i++) {
        //closing the pipes completely from child
        close(child_info[i].pipe[PARENT_W][TO_READ]);
        close(child_info[i].pipe[CHILD_W][TO_WRITE]);

        free_2d_INT_array(child_info[i].pipe, 2);
    }
    free(child_info);
    if (fd != -1) {
        close(fd);
    }
}

void free_resources_parent(){
    for (int i=0;i<children;i++){
        //closing the pipes completely from parent
        close(child_info[i].pipe[PARENT_W][TO_WRITE]);
        close(child_info[i].pipe[CHILD_W][TO_READ]);

        free_2d_INT_array(child_info[i].pipe, 2);
    }
    free(child_info);
    if (fd != -1) {
        close(fd);
    }
}

void on_sig_term_parent(int sig) {
    while ((wait(NULL)) > 0); //waiting for all children to finish their clean up
    printf("Children cleaned up. Exiting..\n");
    free_resources_parent();
    exit(0);
}

void on_sig_term_child(int sig) {
    printf("Child terminating due to SIGINT(Ctrl+C) signal\n");
    
    free_resources_child();
    exit(0);
}

void exitGracefully(char *exit_message, int exnum){
    printf("%s", exit_message);
    exit(exnum);
}