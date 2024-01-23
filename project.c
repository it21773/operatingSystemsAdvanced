#include <extra_functions.h>

// those commented below are included by extra_functions.h
// #define PARENT_W 0
// #define CHILD_W 1

// #define TO_READ 0
// #define TO_WRITE 1

// typedef struct Child_info {
//     pid_t pid;
//     char* name;
//     int** pipe;
// } Child_info;

// int children;
// Child_info* child_info;
// int fd;
// char parentmessage[100];

// notes/ to-do:
// remains: incorporate RPC -> 
//      SO FAR: attempted to make a function and call on it from fakeServers fork(). It doesnt seem to work so far. THE PIPE WRITES IN STDOUT AKA THE TERMINAL!
//          1. as far as i understand it, that is called by server when client send a request now all children are awaiting sleep commands from the start, 
//              they receive them from rpc, the server sends to where the parent is hearing(see writer/reader IPC example)
// 
//          2. OR create function "sendSleepOrder(char* sleepTime)" ??
//          and he sends it to the first child he finds available
//      implement exec/exev(?)(this will also improve resource clean-up(with singals)),
//      make code more readable
// 
// even with malloc i can't seem to create 1000 children

// int sleepPipe[2];

// int sendSleepOrder(int sleepTime);
// int readSleepOrder();

int readerIPC(int uniquefd) {
    // // Create a named pipe (FIFO)
    // const char* fifoPath = "/tmp/my_fifo";
    // mkfifo(fifoPath, 0666);

    // // Open the named pipe for reading
    // int fd = open(fifoPath, O_RDONLY);

    // Receive the number from the other process
    int receivedNumber;
    read(uniquefd, &receivedNumber, sizeof(int));

    // Print the received number
    printf("Received number: %d\n", receivedNumber);

    // // Close the pipe and remove it
    // close(fd);
    // unlink(fifoPath);

    return receivedNumber;
}

int start(int argc, char *argv[]) {
    //REMOVE THIS AFTER TESTING
    // Create a named pipe (FIFO)
    const char* fifoPath = "/tmp/my_fifo";
    mkfifo(fifoPath, 0666);

    // Open the named pipe for reading
    int uniquefd = open(fifoPath, O_RDONLY);
    

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

    // if(pipe(sleepPipe)) {
    //     close(fd);
    //     perror("pipes");
    //     exitGracefully("Error with pipes\n", EXIT_FAILURE);
    // }

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

            close(parentWriteEnd(&child_info[i])); // So you can only read on this pipe as a child (close write side)
            close(parentReadEnd(&child_info[i])); // So you can only write on this pipe as a child (close read side)

            read(childReadEnd(&child_info[i]), readmessage, sizeof(readmessage)); //Message of name bestowed unto the child
            
            name = readmessage+sizeof("Hello child, I am your father and I call you: ")-1; //extracting the name
            printf("I'm child #%d and my name is %s\n", i, name);

            //writing on file
            lockFile(fd);
            sprintf(childWrite, "%d -> %s\n",getpid(), name);
            write(fd, childWrite, strlen(childWrite));
            unlockFile(fd);
            //end of write on file

            write(childWriteEnd(&child_info[i]), childmessage, sizeof(childmessage)); //Child's writing "done"

            //reading second message
            read(childReadEnd(&child_info[i]), readmessage, sizeof(readmessage)); //Parent's "good job"

            //reading messages and sleeping appropriately
            while(true) {
                write(childWriteEnd(&child_info[i]), childmessage, sizeof(childmessage)); //Child's writing "done"

                printf("Child #%d awaiting orders no.%d\n", i, count++);
                read(childReadEnd(&child_info[i]), readmessage, sizeof(readmessage)); //reads sleep orders
                
                // to_sleep = (atoi(readmessage)) % 5;
                to_sleep = atoi(readmessage);
                printf("Child #%d sleeping for: %d\n", i, to_sleep);
                sleep(to_sleep);
                printf("Child #%d awoke from slumber\n", i);
            }

            //Unreachable code now that we got the infinite loop 

            //closing the pipes completely from child
            close(childReadEnd(&child_info[i]));
            close(childWriteEnd(&child_info[i]));
        
            // this is necessary
            free_2d_INT_array(child_info[i].pipe, 2);
            
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
        close(childReadEnd(&child_info[i])); // So you can only write on this pipe as a parent (close read side)
        close(childWriteEnd(&child_info[i])); // So you can only read on this pipe as a parent (close write side)

        printf("Parent naming child %d\n", i);
        sprintf(parentmessage, "Hello child, I am your father and I call you: name%d",i); //writing the message(naming)
        write(parentWriteEnd(&child_info[i]), parentmessage, strlen(parentmessage)+1); //Name of the child, +1 to include null terminator \0 although is works fine without it
    }

    for(int i=0; i<children; i++){
        read(parentReadEnd(&child_info[i]), readmessage, sizeof(readmessage)); //Child's "done"
        // printf("Parent reading message from child %d: %s\n", i, readmessage);

        sprintf(parentmessage, "Good job!"); //writing the second message
        // printf("Parent sending second message to child %d : %s\n", i, parentmessage[i]);
        write(parentWriteEnd(&child_info[i]), parentmessage, strlen(parentmessage)+1); //Name of the child, +1 to include null terminator \0 although is works fine without it
        
    }

    //Start of infinite messages
    while(true) {
        for(int i=0; i<children; i++){
            printf("Attempting cycle %d for child #%d\n", count++, i);
            bool check = is_read_pipe_ready(child_info[i].pipe[CHILD_W], 3);
            if (check) { //if child has responded with done
                read(parentReadEnd(&child_info[i]), readmessage, sizeof(readmessage)); //Child's "done"
                // printf("\nAt 'start' int sleepTime = readSleepOrder();\n");
                // int sleepTime = readSleepOrder();
                // printf("\nAt 'start' below int sleepTime = readSleepOrder();\n");
                // sprintf(parentmessage, "%d", sleepTime); //writing for how many seconds to sleep
                int intMessage = readerIPC(uniquefd);
                sprintf(parentmessage, "%d", intMessage);
                printf("Message to send: %s\n", parentmessage);
                // sprintf(parentmessage, "%d", i*i); //writing for how many seconds to sleep
                write(parentWriteEnd(&child_info[i]), parentmessage, strlen(parentmessage)+1); //Name of the child, +1 to include null terminator \0 although is works fine without it
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
        close(parentWriteEnd(&child_info[i]));
        close(parentReadEnd(&child_info[i]));

        free_2d_INT_array(child_info[i].pipe, 2);
    }
    free(child_info);

    printf("end of program\n");

    exit(0);
}

// int sendSleepOrder(int sleepTime) {
//     char writeOrder[10];
//     sprintf(writeOrder, "%d", sleepTime);
//     printf("\nStarting to send the sleep order1: %d\n", sleepTime);
//     printf("\nStarting to send the sleep order2: %s\n", writeOrder);
//     printf("\npipe: %ls\n", sleepPipe);
//     ssize_t bytesWritten = write(sleepPipe[TO_WRITE], writeOrder, strlen(writeOrder));
//     if (bytesWritten == -1) {
//         perror("Error writing to pipe");
//     } else {
//         printf("\nSuccessfully wrote %zd bytes\n", bytesWritten);
//     }

//     printf("\nSend order: %d\n", atoi(writeOrder));
//     return atoi(writeOrder);
// }

// int readSleepOrder() {
//     printf("\nReading sleep order\n");
//     char sleepMessage[10];
    
//     ssize_t bytesRead = read(sleepPipe[TO_READ], sleepMessage, sizeof(sleepMessage));
//     if (bytesRead == -1) {
//         perror("Error reading from pipe");
//     }
//     sleepMessage[bytesRead] = '\0';
//     printf("Can we reach here?!");
//     printf("\nGiving the read order1: %s\n", sleepMessage);
//     printf("\nGiving the read order2: %d\n", atoi(sleepMessage));

//     return atoi(sleepMessage);
// }