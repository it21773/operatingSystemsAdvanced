#include <stdio.h>
#include <unistd.h>

int main() {
    // Save the current stdout
    FILE *original_stdout = stdout;

    // Open a file for writing (replace "output.txt" with your desired file name)
    FILE *new_stdout = fopen("output.txt", "w");

    // Check if the file opening was successful
    if (new_stdout == NULL) {
        fprintf(stderr, "Failed to open the file for writing.\n");
        return 1; // Return an error code
    }

    // Redirect stdout to the new file stream
    if (dup2(fileno(new_stdout), fileno(stdout)) == -1) {
        fprintf(stderr, "Failed to redirect stdout.\n");
        return 1; // Return an error code
    }

    // Now, stdout is redirected to the file "output.txt"
    printf("This will be written to the file.\n");

    // Restore the original stdout
    fflush(stdout);
    freopen("/dev/tty", "w", stdout); // On Linux, use "/dev/tty" to reopen the console

    printf("This will be back to the original stdout.\n");

    // Close the file stream
    fclose(new_stdout);

    return 0; // Return success code
}
