#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>

const size_t MAX_BUFF_SIZE = 4096;

void snd_pid (pid_t rcv_pid);
void snd_file (FILE* input_file, pid_t rcv_pid);

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf ("Usage: ./sig_snd filename rcv_pid\n");
        return 0;
    }
    pid_t rcv_pid = atoi(argv[2]);

    FILE* input_file = fopen(argv[1], "rb");
    assert (input_file != NULL);


    snd_pid(rcv_pid);

    snd_file(input_file, rcv_pid);

    fclose (input_file);
}

void snd_file (FILE* input_file, pid_t rcv_pid) {
    union sigval val = {};
    char buffer[MAX_BUFF_SIZE];
    int num_readed = 0;

    while (num_readed = fread(buffer, sizeof(char), MAX_BUFF_SIZE, input_file)) {
        for (int i = 0; num_readed - i > 4; i+=4) {
            val.sival_int = *(int*)(buffer + i);
            sigqueue (rcv_pid, SIGUSR1, val);
            pause();
        }

        for (; i < num_readed; i++) {
        }
    }

    kill(rcv_pid, SIGUSR2);

    return ;
}

void snd_pid (pid_t rcv_pid) {
    union sigval val;
    val.sival_int = getpid();
    sigqueue(rcv_pid, SIGINT, val);
}
