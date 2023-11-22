#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

void sigusr1_handler(int sig, siginfo_t* info, void* context) {
}

void sigusr2_handler(int sig, siginfo_t* info, void* context) {
}

void sigint_handler(int sig, siginfo_t* info, void* context) {
}

int main(int argc, char* argv[]) {

    if (argc != 2) {
        printf ("Usage: ./sig_rcv outputFileName\n");
        return 0;
    }

    

}
