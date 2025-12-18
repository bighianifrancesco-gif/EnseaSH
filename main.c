#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

#define buffer_size 128
#define WelcomeMSG "$ ./enseaSH \nWelcome to ENSEA Tiny Shell. \nType 'exit' to quit. \n"
#define Prompt "enseash %"
#define ByeMSG "Bye bye...\n"

static void print_welcome(void) {
    write(STDOUT_FILENO, WelcomeMSG, strlen(WelcomeMSG));
}

//static void print_prompt(void) {
  //  write(STDOUT_FILENO, Prompt, strlen(Prompt));
//}

static void print_prompt_with_status(int status, int has_status) {
    char msg[64];

    if (has_status) {
        // If process exited normally
        if (WIFEXITED(status)) {
            int len = snprintf(msg, sizeof(msg),
                               "enseash [exit:%d] %% ",
                               WEXITSTATUS(status));
            write(STDOUT_FILENO, msg, len);
            return;
        }
        // If process was killed by a signal
        if (WIFSIGNALED(status)) {
            int len = snprintf(msg, sizeof(msg),
                               "enseash [sign:%d] %% ",
                               WTERMSIG(status));
            write(STDOUT_FILENO, msg, len);
            return;
        }
    }
    // Default prompt if no status information
    write(STDOUT_FILENO, Prompt " ", strlen(Prompt) + 1);
}


int main(void) {
    int status;
    char buffer[buffer_size];
    int last_status = 0;
    int has_status = 0;

    print_welcome();
    print_prompt_with_status(last_status, has_status);

    while ((read(STDIN_FILENO, buffer, buffer_size)) > 0) {
        buffer[strcspn(buffer, "\n")] = '\0';  // Remove newline character

        if (strlen(buffer) == 0) {
            print_prompt_with_status(last_status, has_status);
            continue;
        }

        if (strcmp(buffer, "exit") == 0) {
            write(STDOUT_FILENO, ByeMSG, strlen(ByeMSG));
            break;
        }

        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            char *argv[] = {buffer, NULL};
            execvp(buffer, argv);
            perror("execvp failed");
            exit(1);
        } else {
            // Parent process
            waitpid(pid, &status, 0);
            last_status = status;
            has_status = 1;
            print_prompt_with_status(last_status, has_status);
        }
    }

    return 0;
}
