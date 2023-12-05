#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "server.h"
#include "log.h"

int main () {
    struct Server server = {};
    int error = server_init(&server, "server_tx_fifo", "server_rx_fifo");

    if (error) 
        return error;


    fd_set rfds;
    struct timeval tv;
    int retval;

    FD_ZERO(&rfds);
    FD_SET(server.rx, &rfds);
    
    while (1) {
        retval = select(server.max_fd + 1, &rfds, NULL, NULL, NULL);

        if (retval == -1){
            perror("select()");
            return 1;
        }

        if (FD_ISSET(server.rx, &rfds)) {
            char* tx_filename = NULL;
            char* rx_filename = NULL;
            server_cmd(&server);
        }

        fprintf(stderr, "Number of clients %d\n", server.client_num);
        for (int i = 0; i < server.client_num; i++) {
            fprintf (stderr, "%d %d\n", i, server.clients[i].tx_fd);
            if (FD_ISSET(server.clients[i].tx_fd, &rfds) && server.clients[i].tx_fd != 0) {
                fprintf(stderr, "Client request %d\n", i);
                client_request(&server, i);
                FD_CLR(server.clients[i].tx_fd, &rfds);
            }
        }

        reinit_select(&server, &rfds);
    }

    
    server_deinit(&server, "server_tx_fifo", "server_rx_fifo");
}

int server_cmd(struct Server* server) {
    char buf[BUF_SIZE];
    char cmd[BUF_SIZE];
    char tx_fifo[BUF_SIZE];
    char rx_fifo[BUF_SIZE];
    read(server->rx, buf, BUF_SIZE);

    int num_readed = sscanf(buf, "%s %s %s", cmd, tx_fifo, rx_fifo);
    LOG("%s %s %s\n", cmd, tx_fifo, rx_fifo);
    if (num_readed <= 0) {
        fprintf (stderr, "Error\n");
        perror("sscanf");
    } 
    if (strcmp(cmd, "OFF") == 0) {
        fprintf(stderr, "HERE\n");
        LOG("shutting down\n");
        server_deinit(server, "server_tx_fifo", "server_rx_fifo");
        exit(0);
    }

    if (strcmp(cmd, "REG") != 0) {
        fprintf (stderr, "Unknown cmd 404\n");
        assert(1);
        return 1;
    }

    add_client(server, tx_fifo, rx_fifo);

    return 0;
}

int client_request(struct Server* server, int client_num) {
    char buf[BUF_SIZE] = {};
    char cmd[BUF_SIZE] = {};
    char file[BUF_SIZE] = {};
    read(server->clients[client_num].tx_fd, buf, BUF_SIZE);
    fprintf (stderr, "client cmd: %s\n", buf);

    int num_readed = sscanf(buf, "%s %s ", cmd, file);
    if (num_readed <= 0) {
        fprintf (stderr, "Error\n");
        perror("sscanf");
    } 

    if (strcmp(cmd, "GET") != 0) {                  // change here
        fprintf(stderr, "WRONG CMD\n");
        return 1;
    }
    fprintf(stderr, "OK\n");
    
    fprintf(stderr, "HEY %s\n", server->clients[client_num].rx_filename);
    int dest_fd = open(server->clients[client_num].rx_filename, O_WRONLY);
    assert(dest_fd >= 0);
    send_file(dest_fd, file);
    close(dest_fd);

    return 0;
}

void send_file (int fd, char* fileName) {
    fprintf(stderr, "Writing file %s\n", fileName);
    char buff[BUF_SIZE] = {};

    FILE* fileptr = fopen(fileName, "r");
    perror("Fopen");
    assert(fileptr != NULL);
    int num_readed = 0;

    while (num_readed = fread(buff, sizeof(char), BUF_SIZE, fileptr)) {
        write(fd, buff, num_readed);
        fprintf(stderr, "%d %s", num_readed, buff);
    }
    fprintf(stderr, "Writing file %d\n", num_readed);
    perror("Reading");

    fclose(fileptr);

    return;
}

void add_client (struct Server* server, char* tx_filename, char* rx_filename) {
    fprintf(stderr, "adding a client %s %s\n", tx_filename, rx_filename);
    LOG("%s, %s\n", tx_filename, rx_filename);

    // Update maximum
    int tx = open(tx_filename, O_RDWR);

    server->clients[server->client_num].tx_fd = tx;
    server->clients[server->client_num].rx_filename = (char*) calloc(sizeof(char), sizeof(rx_filename));
    strcpy(server->clients[server->client_num].rx_filename, rx_filename);

    if (tx > server->max_fd) server->max_fd = tx;

    server->client_num += 1;
}

void reinit_select (struct Server* server, fd_set* rfds) {
    FD_ZERO(rfds);
    FD_SET(server->rx, rfds);

    for (int i = 0; i < server->client_num; i++) {
        FD_SET(server->clients[i].tx_fd, rfds);
    }
    

    printf("Reinit\n");
    return;
}


int server_init (struct Server *server, char* tx_name, char* rx_name) {
    int ret = mkfifo(tx_name, 0666);
    ret    |= mkfifo(rx_name, 0666);

    if (ret != 0) {
        perror("Unable to make server fifo");
        return 1;
    }

    int rx_fd = open(rx_name, O_RDWR);
    int tx_fd = open(tx_name, O_RDWR);

    if (tx_fd < 0 || rx_fd < 0) {
        perror("Unable to open server fifo");
        return 1;
    }

    server->rx = rx_fd;
    server->tx = tx_fd;
    server->max_fd = tx_fd;

    return 0;
}

void server_deinit (struct Server* server, char* tx_name, char* rx_name) {
    close(server->tx);
    close(server->rx);
    unlink(tx_name);
    unlink(rx_name);
}
