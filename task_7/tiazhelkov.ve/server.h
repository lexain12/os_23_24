#include "C-Thread-Pool/thpool.h"
#define MAX_NUM_OF_CLIENTS 64
#define BUF_SIZE 1024

struct Client {
    int tx_fd;
    char* rx_filename;
};

struct Server {
    struct Client clients[MAX_NUM_OF_CLIENTS];
    int   rx;
    int   tx;  
    int   max_fd;
    int   client_num;
};

union send_file_args {
    char* buf;
    int   fd ;
};

int  server_init (struct Server* server, char* tx_name, char* rx_name);
void server_deinit (struct Server* server, char* tx_name, char* rx_name);
void reinit_select (struct Server* server, fd_set* rfds);

int parse_cmd (int fd, char** first_file, char** second_file);
void add_client (struct Server* server, char* tx_filename, char* rx_filename);
int server_cmd (struct Server* server);
int client_request (struct Server* server, threadpool, int);
void send_file (void*);
