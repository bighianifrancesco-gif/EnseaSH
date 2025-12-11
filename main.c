#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define WelcomeMSG "$ ./enseaSH \nWelcome to ENSEA Tiny Shell. \nType 'exit' to quit. \n"
#define Prompt "enseash %"
static void print_welcome(void) {
    write(STDOUT_FILENO, WelcomeMSG, strlen(WelcomeMSG));
    }
static void print_prompt(void) {
    write(STDOUT_FILENO, Prompt, strlen(Prompt));
}


int main() {
    print_welcome();
    print_prompt();
}