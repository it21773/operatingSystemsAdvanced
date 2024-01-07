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

// notes/ to-do:
// improve the pipe messages (name, can you work, yes i can, then go to work) AND incorporate select.c
// continue further down the exercise(page 8, incorporate select() or pselect() instead of write/read)
// even with malloc i can't seem to create 1000 children
// 
// extra:
// when using exitGracefully, replace exnum=1 with EXIT_FAILURE
// should i use exitGracefully so sparingly?

#define PARENT_W 0
#define CHILD_W 1

#define TO_READ 0
#define TO_WRITE 1

int* create_1d_int_array(int index);
int** create_2d_INT_array(int index, int jndex);
char** create_2d_CHAR_array(int index, int jndex);
void free_2d_INT_array(int** array, int index);
void free_2d_CHAR_array(char** array, int index);
void lockFile(int fileDescriptor);
void unlockFile(int fileDescriptor);
bool endswith(char *string, char *end);
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

    typedef struct Child_info {
        pid_t pid;
        char* name;
        int** pipe;
    } Child_info;

    Child_info* child_info = (Child_info*) malloc(children*sizeof(Child_info));

    for(int i=0; i<children; i++){
        child_info[i].pipe = create_2d_INT_array(2,2);
    }

    pid_t init = 1;

    int* pw_pipesuccess = create_1d_int_array(children); //check for successful creation then release
    int* cw_pipesuccess = create_1d_int_array(children); //check for successful creation then release

    char readmessage[100];

    int fd = open(argv[1], O_CREAT | O_TRUNC | O_WRONLY, 00600);
    if (fd == -1) { //checking if open succeeded
        perror("open");
        exitGracefully("open error\n",1);
    }


    for (int i=0; i < children ; i++){
        pw_pipesuccess[i] = pipe(child_info[i].pipe[PARENT_W]);
        if (pw_pipesuccess[i] == -1){
            exitGracefully("Error with pipes",1);
        }
        cw_pipesuccess[i] = pipe(child_info[i].pipe[CHILD_W]);
        if (cw_pipesuccess[i] == -1){
            exitGracefully("Error with pipes",1);
        }
    }
    free(pw_pipesuccess);
    free(cw_pipesuccess);

    //writing as a Parent in the beginning of the file
    char parentWrite[50];
    sprintf(parentWrite, "[PARENT] -> %d\n \n\n\n\n\n\n\n\n", getpid()); //\n helps with seeing ordinality(1,2,3) -> child0 in line 10, child2 in line 12
    write(fd, parentWrite, strlen(parentWrite));

    for (int i = 0; i<children;i++) {
        init = fork();
        child_info[i].pid = init;
        if (init < 0) {
            perror("fork");
            exitGracefully("fork error\n",1);
        } else if (init > 0) { //Parent
            //do nothing, just birth children
        } else { //Child

            close(child_info[i].pipe[PARENT_W][TO_WRITE]); // So you can only read on this pipe as a child (close write side)
            close(child_info[i].pipe[CHILD_W][TO_READ]); // So you can only write on this pipe as a child (close read side)

            read(child_info[i].pipe[PARENT_W][TO_READ], readmessage, sizeof(readmessage)); //Message of name bestowed unto the child
            
            char* name = readmessage+sizeof("Hello child, I am your father and I call you: ")-1; //extracting the name
            printf("Child %d reading message from parent(extracted): %s\n", i, name);
            
            //writing on file
            lockFile(fd);
            char childWrite[100];
            sprintf(childWrite, "%d -> %s\n",getpid(), name);
            write(fd, childWrite, strlen(childWrite));
            unlockFile(fd);
            //end of write on file

            char childmessage[5] = "done";
            printf("Child %d writing message to parent.\n", i);
            write(child_info[i].pipe[CHILD_W][TO_WRITE], childmessage, sizeof(childmessage)); //Child's writing "done"

            //reading second message
            read(child_info[i].pipe[PARENT_W][TO_READ], readmessage, sizeof(readmessage));
            printf("Child %d reading 2nd message from parent: %s\n", i, readmessage);

            //closing the pipes completely from child
            close(child_info[i].pipe[PARENT_W][TO_READ]);
            close(child_info[i].pipe[CHILD_W][TO_WRITE]);
            exitGracefully("",0);
        }
    }

    //Only parent is operating the code below this point


    char** parentmessage = create_2d_CHAR_array(children, 100);
    //Sending first message
    for(int i=0; i<children; i++){
        close(child_info[i].pipe[PARENT_W][TO_READ]); // So you can only write on this pipe as a parent (close read side)
        close(child_info[i].pipe[CHILD_W][TO_WRITE]); // So you can only read on this pipe as a parent (close write side)

        //these should be done randomly i think the professor said?
        printf("Parent sending message to child %d\n", i);
        sprintf(parentmessage[i], "Hello child, I am your father and I call you: name%d",i); //writing the message(naming)
        write(child_info[i].pipe[PARENT_W][TO_WRITE], parentmessage[i], strlen(parentmessage[i])+1); //Name of the child, +1 to include null terminator \0 although is works fine without it
        
    }


    for(int i=0; i<children; i++){
        read(child_info[i].pipe[CHILD_W][TO_READ], readmessage, sizeof(readmessage)); //Child's "done"
        printf("Parent reading message from child %d: %s\n", i, readmessage);

        sprintf(parentmessage[i], "Good job!"); //writing the second message
        printf("Parent sending second message to child %d : %s\n", i, parentmessage[i]);
        write(child_info[i].pipe[PARENT_W][TO_WRITE], parentmessage[i], strlen(parentmessage[i])+1); //Name of the child, +1 to include null terminator \0 although is works fine without it
        
    }

    
    // waitpid(cpid[i], NULL, 0);
    while ((wait(NULL)) > 0); //waiting for all children to finish


    // these frees could potentially happen sooner?
    for (int i=0;i<children;i++){
        //closing the pipes completely from parent
        close(child_info[i].pipe[PARENT_W][TO_WRITE]);
        close(child_info[i].pipe[CHILD_W][TO_READ]);

        free(child_info[i].pipe[PARENT_W]);
        free(child_info[i].pipe[CHILD_W]);
    }
    free(child_info);
    free_2d_CHAR_array(parentmessage, children);

    printf("end of program\n");

}


int* create_1d_int_array(int index) {

    int* array = (int*)malloc(index * sizeof(int));

    return array;
}

int** create_2d_INT_array(int index, int jndex) {

    int** array = (int**)malloc(index * sizeof(int*));
    if (array == NULL) {
        perror("Memory allocation failed");
        exitGracefully("Memory allocation failed\n",1);
    }
    for (int i = 0; i < index; ++i) {
        array[i] = (int*)malloc(jndex * sizeof(int));
        if (array[i] == NULL) {
            perror("Memory allocation failed");
            exitGracefully("Memory allocation failed\n",1);
        }
    }

    return array;
}

char** create_2d_CHAR_array(int index, int jndex) {

    char** array = (char**)malloc(index * sizeof(char*));
    if (array == NULL) {
        perror("Memory allocation failed");
        exitGracefully("Memory allocation failed\n",1);
    }
    for (int i = 0; i < index; ++i) {
        array[i] = (char*)malloc(jndex * sizeof(char));
        if (array[i] == NULL) {
            perror("Memory allocation failed");
            exitGracefully("Memory allocation failed\n",1);
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

void free_2d_CHAR_array(char** array, int index) {
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
        exit(EXIT_FAILURE); //** should this be exit gracefully?
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
        exit(EXIT_FAILURE); //** should this be exit gracefully?
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