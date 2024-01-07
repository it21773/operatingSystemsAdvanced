#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define MESSAGE_SIZE 50

int main() {
    int parent_to_child_pipe[2];
    int child_to_parent_pipe[2];

    if (pipe(parent_to_child_pipe) == -1 || pipe(child_to_parent_pipe) == -1) {
        perror("Pipe creation failed");
        exit(EXIT_FAILURE);
    }

    pid_t child_pid = fork();

    if (child_pid == -1) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) {
        // Child process
        close(parent_to_child_pipe[1]);  // Close the write end of parent-to-child pipe
        close(child_to_parent_pipe[0]);  // Close the read end of child-to-parent pipe

        char parent_message[MESSAGE_SIZE];
        char child_reply[MESSAGE_SIZE];

        // Read message from the parent
        read(parent_to_child_pipe[0], parent_message, sizeof(parent_message));
        printf("Child received: %s\n", parent_message);

        // Reply to the parent
        // snprintf(child_reply, sizeof(child_reply), "Hello from child!");
        sprintf(child_reply, "Hello from child!");
        write(child_to_parent_pipe[1], child_reply, sizeof(child_reply));

        // Read the parent's second message
        read(parent_to_child_pipe[0], parent_message, sizeof(parent_message));
        printf("Child received: %s\n", parent_message);

        close(parent_to_child_pipe[0]);  // Close the read end of parent-to-child pipe
        close(child_to_parent_pipe[1]);  // Close the write end of child-to-parent pipe

        exit(EXIT_SUCCESS);
    } else {
        // Parent process
        close(parent_to_child_pipe[0]);  // Close the read end of parent-to-child pipe
        close(child_to_parent_pipe[1]);  // Close the write end of child-to-parent pipe

        char parent_message[MESSAGE_SIZE];
        char child_reply[MESSAGE_SIZE];

        // Send message to the child
        // snprintf(parent_message, sizeof(parent_message), "Hello from parent!");
        sprintf(parent_message, "Hello from parent!");
        write(parent_to_child_pipe[1], parent_message, sizeof(parent_message));

        // Read the child's reply
        read(child_to_parent_pipe[0], child_reply, sizeof(child_reply));
        printf("Parent received: %s\n", child_reply);

        // Reply to the child again
        // snprintf(parent_message, sizeof(parent_message), "Another message from parent!");
        sprintf(parent_message, "Another message from parent!");
        write(parent_to_child_pipe[1], parent_message, sizeof(parent_message));

        close(parent_to_child_pipe[1]);  // Close the write end of parent-to-child pipe
        close(child_to_parent_pipe[0]);  // Close the read end of child-to-parent pipe

        wait(NULL);  // Wait for the child process to finish

        exit(EXIT_SUCCESS);
    }

    return 0;
}
