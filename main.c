#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h> // Required for clock_gettime

#define buffer_size 128
#define WelcomeMSG "$ ./enseash \nWelcome to ENSEA Tiny Shell. \nType 'exit' to quit. \n"
#define Prompt "enseash %"
#define ByeMSG "Bye bye...\n"

// Function to calculate elapsed time in milliseconds
long get_elapsed_ms(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;
}

static void print_welcome(void) {
    write(STDOUT_FILENO, WelcomeMSG, strlen(WelcomeMSG));
}

static void print_prompt_with_status(int status, int has_status, long duration_ms) {
    char msg[128];

    if (has_status) {
        if (WIFEXITED(status)) {
            int len = snprintf(msg, sizeof(msg),
                               "enseash [exit:%d|%ldms] %% ",
                               WEXITSTATUS(status), duration_ms);
            write(STDOUT_FILENO, msg, len);
            return;
        }
        if (WIFSIGNALED(status)) {
            int len = snprintf(msg, sizeof(msg),
                               "enseash [sign:%d|%ldms] %% ",
                               WTERMSIG(status), duration_ms);
            write(STDOUT_FILENO, msg, len);
            return;
        }
    }
    // Initial prompt
    write(STDOUT_FILENO, Prompt " ", strlen(Prompt) + 1);
}

int main(void) {
    int status;
    char buffer[buffer_size];
    int has_status = 0;
    long last_duration = 0;
    struct timespec start_time, end_time;

    print_welcome();
    print_prompt_with_status(0, has_status, 0);

    while (read(STDIN_FILENO, buffer, buffer_size) > 0) {
        buffer[strcspn(buffer, "\n")] = '\0';

        if (strlen(buffer) == 0) {
            print_prompt_with_status(status, has_status, last_duration);
            continue;
        }

        if (strcmp(buffer, "exit") == 0) {
            write(STDOUT_FILENO, ByeMSG, strlen(ByeMSG));
            break;
        }

        // Start timer before forking
        clock_gettime(CLOCK_MONOTONIC, &start_time);

        pid_t pid = fork();
        if (pid == 0) {
            char *argv[] = {buffer, NULL};
            execvp(buffer, argv);
            perror("execvp failed");
            exit(1);
        } else {
            waitpid(pid, &status, 0);

            // End timer after child finishes
            clock_gettime(CLOCK_MONOTONIC, &end_time);

            last_duration = get_elapsed_ms(start_time, end_time);
            has_status = 1;
            print_prompt_with_status(status, has_status, last_duration);
        }
    }

    return 0;
}