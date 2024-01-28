
#include "add.h"
#include "../extra_functions.h"

void *
add_1_svc(duration *argp, struct svc_req *rqstp)
{
	static char * result;

	// IMPORTANT, children no longer write on file, number of children is also predefined
	// known bugs: for each client call, the server reruns the whole code

    pid_t init = 1;
    char readmessage[100];

	children = 3;

    child_info = (Child_info*) malloc(children*sizeof(Child_info));

    for(int i=0; i<children; i++){
        child_info[i].pipe = create_2d_INT_array(2,2);
    }

    for (int i=0; i < children ; i++){
        if(pipe(child_info[i].pipe[PARENT_W])) {
            // close(fd);
            perror("pipes");
            exitGracefully("Error with pipes", EXIT_FAILURE);
        }
        if(pipe(child_info[i].pipe[CHILD_W])) {
            // close(fd);
            perror("pipes");
            exitGracefully("Error with pipes", EXIT_FAILURE);
        }
    }


    //this is earlier we can handle the signal right? Otherwise we need more signal handling functions
    // in which they close fewer things since we havent yet opened everything
    signal(SIGINT, (void (*)(int))on_sig_term_parent);


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

            // close(fd);

            exitGracefully("",0);
        }
    }

    //Only parent is operating the code below this point

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
            printf("Attempting cycle %d: child #%d\n", count++, i);
            bool check = is_read_pipe_ready(child_info[i].pipe[CHILD_W], 3);
            if (check) { //if child has responded with done
                read(parentReadEnd(&child_info[i]), readmessage, sizeof(readmessage)); //Child's "done"

                int intMessage = argp->seconds;
                sprintf(parentmessage, "%d", intMessage);
                printf("Message to send: %s\n", parentmessage);
                write(parentWriteEnd(&child_info[i]), parentmessage, strlen(parentmessage)+1);
                printf("Parent send sleep message to child #%d\n", i);

				return (void *) &result;
            } else {
                printf("Parent SKIPPED child #%d\n", i);
            }
        }
    }

    //Unreachable code now that we got the infinite loop
    
    printf("Parent no longer waits for messages!!\n");

    // waitpid(cpid[i], NULL, 0);
    while ((wait(NULL)) > 0); //waiting for all children to finish


    for (int i=0;i<children;i++){
        //closing the pipes completely from parent
        close(parentWriteEnd(&child_info[i]));
        close(parentReadEnd(&child_info[i]));

        free_2d_INT_array(child_info[i].pipe, 2);
    }
    free(child_info);

    printf("end of program\n");
	// return (void *) &result;
}
