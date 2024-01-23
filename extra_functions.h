#ifndef EXTRA_FUNCTIONS_H
#define EXTRA_FUNCTIONS_H

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

extern int children;
extern Child_info* child_info;
extern int fd;
extern char parentmessage[100];

void exitGracefully(char *exit_message, int exnum);
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
int parentWriteEnd(Child_info* child_info);
int childWriteEnd(Child_info* child_info);
int parentReadEnd(Child_info* child_info);
int childReadEnd(Child_info* child_info);
#endif