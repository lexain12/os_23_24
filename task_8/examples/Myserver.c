#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_SZ 4096
#define PORT 8123

void *f(void* args) {
    int* args_int = (int*) args;
    int confd = args_int[0];
    int n_client = args_int[1];
    char buf[MAX_SZ];
    while (1) {
        buf[0] = '\0';
        int num_read = read(confd, buf, sizeof(buf));
        buf[num_read] = '\0';
        printf("From client %d: %s\n", confd, buf);
        write(confd, buf, strlen(buf));
    }
}

int main() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr; 

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return 1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    if ((bind(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr))) != 0) {
        perror("Bind");
        return 1;
    }

    int number_of_clients = 0;
    while (1) {
        if (listen(sockfd, 5) != 0)  {
            perror("Listen");
            return 1;
        }

        memset(&cliaddr, 0, sizeof(cliaddr));
        unsigned int len = sizeof(cliaddr);
        int connfd = accept(sockfd, (struct sockaddr*) &cliaddr, &len);
        number_of_clients += 1;

        if (connfd < 0) {
            perror("accept");
        }

        pthread_t tid;
        int args[2];
        args[0] = connfd;
        args[1] = number_of_clients;
        printf("%s\n", inet_ntoa(cliaddr.sin_addr));
        pthread_create(&tid, NULL, &f, args);

    }
    close(sockfd);
    return 0;

}
