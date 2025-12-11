#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

#define buffer_size 128
#define WelcomeMSG "$ ./enseaSH \nWelcome to ENSEA Tiny Shell. \nType 'exit' to quit. \n"
#define Prompt "enseash %"
#define ByeMSG "Bye bye..."

static void print_welcome(void) {
    write(STDOUT_FILENO, WelcomeMSG, strlen(WelcomeMSG));
}

static void print_prompt(void) {
    write(STDOUT_FILENO, Prompt, strlen(Prompt));
}

int main(void) {
    int status;
    char buffer[buffer_size];

    print_welcome();
    print_prompt();

    while ((read(STDIN_FILENO, buffer, buffer_size)) > 0) {
        buffer[strcspn(buffer, "\n")] = '\0';  // Remove newline character
        if (strlen(buffer) == 0) {
            print_prompt();
            continue;
        }

        if (strcmp(buffer,"exit")==0) {
            write (STDOUT_FILENO, ByeMSG, strlen(ByeMSG));
            break;
        }

        pid_t pid = fork();
        if (pid != 0) {
            waitpid(pid, &status, 0);
            print_prompt();
        } else {
            char *argv[] = {buffer, NULL};
            execvp(buffer, argv);
            perror("execvp failed");
            exit(1);
        }
    }

    return 0;
}
