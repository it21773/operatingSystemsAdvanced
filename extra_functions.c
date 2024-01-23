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

#define PARENT_W 0
#define CHILD_W 1

#define TO_READ 0
#define TO_WRITE 1

typedef struct Child_info {
    pid_t pid;
    char* name;
    int** pipe;
} Child_info;

int children;
Child_info* child_info;
int fd;
char parentmessage[100];


void exitGracefully(char *exit_message, int exnum){
    printf("%s", exit_message);
    exit(exnum);
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

int parentWriteEnd(Child_info* child_info) {
    return child_info->pipe[PARENT_W][TO_WRITE];
}

int childWriteEnd(Child_info* child_info) {
    return child_info->pipe[CHILD_W][TO_WRITE];
}

int parentReadEnd(Child_info* child_info) {
    return child_info->pipe[CHILD_W][TO_READ];
}

int childReadEnd(Child_info* child_info) {
    return child_info->pipe[PARENT_W][TO_READ];
}