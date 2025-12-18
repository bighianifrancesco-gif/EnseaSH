#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>

#define buffer_size 128
#define MAX_ARGS 10
#define WelcomeMSG "$ ./enseash \nWelcome to ENSEA Tiny Shell. \nType 'exit' to quit. \n"
#define Prompt "enseash %"
#define ByeMSG "Bye bye...\n"

long get_elapsed_ms(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;
}

static void print_prompt(int status, int has_status, long duration_ms) {
    char msg[128];
    if (has_status) {
        if (WIFEXITED(status)) {
            int len = snprintf(msg, sizeof(msg), "enseash [exit:%d|%ldms] %% ", WEXITSTATUS(status), duration_ms);
            write(STDOUT_FILENO, msg, len);
        } else if (WIFSIGNALED(status)) {
            int len = snprintf(msg, sizeof(msg), "enseash [sign:%d|%ldms] %% ", WTERMSIG(status), duration_ms);
            write(STDOUT_FILENO, msg, len);
        }
    } else {
        write(STDOUT_FILENO, Prompt " ", strlen(Prompt) + 1);
    }
}

int main(void) {
    int status = 0;
    char buffer[buffer_size];
    int has_status = 0;
    long last_duration = 0;
    struct timespec start_time, end_time;

    write(STDOUT_FILENO, WelcomeMSG, strlen(WelcomeMSG));
    print_prompt(0, has_status, 0);

    /* --- MAIN SHELL ENGINE --- */
    ssize_t n;
    while ((n = read(STDIN_FILENO, buffer, buffer_size - 1)) > 0) {
        buffer[n] = '\0';
        buffer[strcspn(buffer, "\n\r")] = '\0'; // Cleans both Unix (\n) and Windows (\r) line endings

        if (strlen(buffer) == 0) {
            print_prompt(status, has_status, last_duration);
            continue;
        }

        if (strcmp(buffer, "exit") == 0) {
            write(STDOUT_FILENO, ByeMSG, strlen(ByeMSG));
            break;
        }

        /* --- STRING TOKENIZATION --- */
        char *argv[MAX_ARGS];
        int i = 0;
        char *token = strtok(buffer, " ");
        while (token != NULL && i < MAX_ARGS - 1) {
            argv[i++] = token;
            token = strtok(NULL, " ");
        }
        argv[i] = NULL;

        if (argv[0] == NULL) {
            print_prompt(status, has_status, last_duration);
            continue;
        }

        clock_gettime(CLOCK_MONOTONIC, &start_time);

        /* --- PROCESS EXECUTION --- */
        pid_t pid = fork();
        if (pid == 0) {
            // Scan argv for redirection operators BEFORE executing
            for (int j = 0; argv[j] != NULL; j++) {

                if (strcmp(argv[j], ">") == 0) {
                    /* O_CREAT: create file if missing. O_TRUNC: wipe file if exists. 0644: rw-r--r-- */
                    int fd = open(argv[j+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd < 0) { perror("open"); exit(1); }

                    dup2(fd, STDOUT_FILENO); // Point Standard Output to our file descriptor
                    close(fd);               // Clean up the original extra reference
                    argv[j] = NULL;          // Truncate argv: execvp only sees the command, not the redirection symbols
                    break;
                }

                else if (strcmp(argv[j], "<") == 0) {
                    int fd = open(argv[j+1], O_RDONLY);
                    if (fd < 0) { perror("open"); exit(1); }

                    dup2(fd, STDIN_FILENO);  // Point Standard Input to our file descriptor
                    close(fd);
                    argv[j] = NULL;
                    break;
                }
            }

            execvp(argv[0], argv); // Replaces child process image with the new program
            perror("execvp failed");
            exit(1);
        }

        else {
            /* --- PARENT WAIT & TELEMETRY --- */
            waitpid(pid, &status, 0);
            clock_gettime(CLOCK_MONOTONIC, &end_time);

            last_duration = get_elapsed_ms(start_time, end_time);
            has_status = 1;
            print_prompt(status, has_status, last_duration);
        }
    }
    return 0;
}