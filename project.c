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
// supported N messages(kinda), if i initiate with "./a.out asd.txt 4" it exits for no reason after
// parent tries to write on pipe(after a few writes already), can/should i clean the pipe somehow?
// 
// even with malloc i can't seem to create 1000 children
// 
// extra:
// when using exitGracefully, replace exnum=1 with EXIT_FAILURE
// should i use exitGracefully so sparingly?

#define PARENT_W 0
#define CHILD_W 1

#define TO_READ 0
#define TO_WRITE 1

#define N 3

bool is_read_pipe_ready(int* read_pipe, int timelimit);
int* create_1d_int_array(int index);
int** create_2d_INT_array(int index, int jndex);
char** create_2d_CHAR_array(int index, int jndex);
void free_2d_INT_array(int** array, int index);
void free_2d_CHAR_array(char** array, int index);
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


    // Save the current stdout, for debugging purposes
    FILE *original_stdout = stdout;
    // Open a file for writing (replace "output.txt" with your desired file name), for debugging purposes
    FILE *new_stdout = fopen("stdout.txt", "w");
    // stdout_to_file(new_stdout); //to print results on file instead of terminal for better debugging


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
            // printf("Child %d reading message from parent(extracted): %s\n", i, name);
            
            //writing on file
            lockFile(fd);
            char childWrite[100];
            sprintf(childWrite, "%d -> %s\n",getpid(), name);
            write(fd, childWrite, strlen(childWrite));
            unlockFile(fd);
            //end of write on file

            char childmessage[5] = "done";
            // printf("Child %d writing message to parent.\n", i);
            write(child_info[i].pipe[CHILD_W][TO_WRITE], childmessage, sizeof(childmessage)); //Child's writing "done"

            //reading second message
            read(child_info[i].pipe[PARENT_W][TO_READ], readmessage, sizeof(readmessage));
            // printf("Child %d reading 2nd message from parent: %s\n", i, readmessage);

            //reading N(=infinite after testing) messages and sleeping appropriately
            for(int j=0;j<N;j++) {
                printf("Child %d STATS: j: %d, i: %d\n", i, j, i);
                printf("Child %d writing message to parent.\n", i);
                write(child_info[i].pipe[CHILD_W][TO_WRITE], childmessage, sizeof(childmessage)); //Child's writing "done"

                printf("Child %d attempting to read #%d message from parent.\n", i, j);
                read(child_info[i].pipe[PARENT_W][TO_READ], readmessage, sizeof(readmessage)); //reads sleep orders

                printf("Child %d read #%d message from parent: %s\n", i, j, readmessage);
                
                int to_sleep = (atoi(readmessage) + i) % 5;
                printf("Child %d sleeping for: %d\n", i, to_sleep);
                sleep(to_sleep);
            }


            //closing the pipes completely from child
            close(child_info[i].pipe[PARENT_W][TO_READ]);
            close(child_info[i].pipe[CHILD_W][TO_WRITE]);

            // Close the file stream
            fclose(new_stdout);
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
        // printf("Parent sending message to child %d\n", i);
        sprintf(parentmessage[i], "Hello child, I am your father and I call you: name%d",i); //writing the message(naming)
        write(child_info[i].pipe[PARENT_W][TO_WRITE], parentmessage[i], strlen(parentmessage[i])+1); //Name of the child, +1 to include null terminator \0 although is works fine without it
        
    }


    for(int i=0; i<children; i++){
        read(child_info[i].pipe[CHILD_W][TO_READ], readmessage, sizeof(readmessage)); //Child's "done"
        // printf("Parent reading message from child %d: %s\n", i, readmessage);

        sprintf(parentmessage[i], "Good job!"); //writing the second message
        // printf("Parent sending second message to child %d : %s\n", i, parentmessage[i]);
        write(child_info[i].pipe[PARENT_W][TO_WRITE], parentmessage[i], strlen(parentmessage[i])+1); //Name of the child, +1 to include null terminator \0 although is works fine without it
        
    }

    int count = 2*N*children;
    printf("The parent cicle should be repeated for %d\n", count);
    count = 0;
    //Start of N messages
    for(int j=0;j<2*N;j++) { //after testing this will infinite loop
        for(int i=0; i<children; i++){
            printf("C%d Parent j: %d, i: %d\n", count++, j, i);
            bool check = is_read_pipe_ready(child_info[i].pipe[CHILD_W], 3);
            if (check) { //if child has responded with done
                read(child_info[i].pipe[CHILD_W][TO_READ], readmessage, sizeof(readmessage)); //Child's "done"
                printf("Parent %d reading #%d message from child: %s\n", i, j, readmessage);

                sprintf(parentmessage[i], "3"); //writing for how many seconds to sleep
                printf("Parent %d sending #%d message(to sleep) to child : %s\n", i, j, parentmessage[i]);
                write(child_info[i].pipe[PARENT_W][TO_WRITE], parentmessage[i], strlen(parentmessage[i])+1); //Name of the child, +1 to include null terminator \0 although is works fine without it
                printf("Parent %d send #%d message succesfully : %s\n", i, j, parentmessage[i]);
            } else {
                printf("Parent %d SKIPPED child\n", i);
            }

        }
    }

    printf("Parent no longer waits for messages!!\n");

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


    // Close the file stream
    fclose(new_stdout);
    printf("end of program\n");

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
        exit(EXIT_FAILURE);
    }

    // Check if the pipe's read end is ready for reading
    if (FD_ISSET(read_pipe[TO_READ], &read_fds)) {

        return true;

        //we are simply checking here, no need to actually read the pipe
    //     char buffer[256];
    //     ssize_t bytes_read = read(read_pipe[TO_READ], buffer, sizeof(buffer) - 1);

    //     if (bytes_read == -1) {
    //         perror("read");
    //         exit(EXIT_FAILURE);
    //     }

    //     if (bytes_read == 0) {
    //         printf("Child process received no message from the parent.\n");
    //     } else {
    //         buffer[bytes_read] = '\0';
    //         printf("Child process received the message: %s\n", buffer);
    //     }
    } else {
        // printf("Child process did not receive any message within the timeout.\n");
        return false;
    }
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

bool stdout_to_file(FILE *new_stdout){

    // Check if the file opening was successful
    if (new_stdout == NULL) {
        fprintf(stderr, "Failed to open the file for writing.\n");
        return false; // Return an error code
    }

    // Redirect stdout to the new file stream
    if (dup2(fileno(new_stdout), fileno(stdout)) == -1) {
        fprintf(stderr, "Failed to redirect stdout.\n");
        return false; // Return an error code
    }

}

void restore_stdout(FILE *new_stdout) {
    fflush(stdout);
    freopen("/dev/tty", "w", stdout); // On Linux, use "/dev/tty" to reopen the console
}