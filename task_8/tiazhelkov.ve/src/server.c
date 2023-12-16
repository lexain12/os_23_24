/*
** broadcaster.c -- дейтаграммный “клиент” подобный talker.c, но
** этот может вещать
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#define SERVERPORT 8123// порт для подключения пользователей
#define UDP_PORT_RECEIVE 8123// Порт для приема ip от всех worker
#define MAXLINE 1024 
#define BUF_SIZE 1024
#define IP4_LEN 15
#define MAX_WORKERS 10

struct Worker {
    char ip[IP4_LEN];
    int num_of_cores;
};

struct Server {
    struct Worker workers[MAX_WORKERS];
    int num_of_workers;
};

int send_invite_to_workers() {
	int sockfd;
	int broadcast = 1;
	struct hostent *myNet;

    struct sockaddr_in servaddr; 
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { // creating a socket for broadcast
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

	if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast,
        sizeof broadcast) == -1) {
        perror("setsockopt (SO_BROADCAST)");
        exit(1);
    }

    int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        perror("Setsockopt failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    } 

    char buffer[MAXLINE] = {};
    if (gethostname(buffer, MAXLINE) == -1) {
        perror("Unable to gethostname");
        exit(EXIT_FAILURE);
    }

	if ((myNet = gethostbyname(buffer)) == NULL) { // получить информацию хоста
        perror("gethostbyname");
        exit(1);
	}
    
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVERPORT);
    servaddr.sin_addr.s_addr = inet_addr("192.168.1.255");
	memset(servaddr.sin_zero, '\0', sizeof servaddr.sin_zero);
    
    int numbytes = 0;
    printf("Sending invites");
	if ((numbytes=sendto(sockfd, "Hello", strlen("Hello"), 0,
    	(struct sockaddr *)&servaddr, sizeof servaddr)) == -1) {
        perror("sendto");
        exit(1); 
    }

    close(sockfd);
}

int rcv_ip_from_workers(struct Server* server) {
    sleep(10);
	int receiveSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (receiveSocket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int reuse = 1;
    if (setsockopt(receiveSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        perror("Setsockopt failed");
        close(receiveSocket);
        exit(EXIT_FAILURE);
    } 

    struct sockaddr_in receiveAddr;
    memset(&receiveAddr, 0, sizeof(receiveAddr));
    receiveAddr.sin_family = AF_INET;
    receiveAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    receiveAddr.sin_port = htons(UDP_PORT_RECEIVE);

    if (bind(receiveSocket, (struct sockaddr*)&receiveAddr, sizeof(receiveAddr)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    printf("Listening for responses...\n");

    char buffer[BUF_SIZE];
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);

    struct timeval startTime, currentTime;
    gettimeofday(&startTime, NULL);

    while (1) {
        gettimeofday(&currentTime, NULL);
        int elapsedSeconds = currentTime.tv_sec - startTime.tv_sec;

        if (elapsedSeconds >= 5) {
            printf("Received responses for 5 seconds. Exiting...\n");
            break;
        }

        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(receiveSocket, &readSet);

        int ready = select(receiveSocket + 1, &readSet, NULL, NULL, NULL);
        if (ready == -1) {
            perror("Select error");
            exit(EXIT_FAILURE);
        }

        if (ready > 0) {
            ssize_t recvBytes = recvfrom(receiveSocket, buffer, BUF_SIZE - 1, 0, (struct sockaddr*)&clientAddr, &addrLen);
            if (recvBytes == -1) {
                perror("Recvfrom failed");
                exit(EXIT_FAILURE);
            }

            buffer[recvBytes] = '\0';

            // Store received worker information in the server struct
            if (server->num_of_workers< MAX_WORKERS) {
                struct Worker newWorker;
                strcpy(newWorker.ip, inet_ntoa(clientAddr.sin_addr));
                newWorker.num_of_cores = atoi(buffer);

                server->workers[server->num_of_workers] = newWorker;
                server->num_of_workers++;
				printf("New worker\n");
            } else {
                printf("Maximum workers reached. Ignoring additional responses.\n");
            }
        }
    }

    close(receiveSocket);
}
int main(int argc, char *argv[])
{
    struct Server server = {};
    send_invite_to_workers();
    rcv_ip_from_workers(&server);
//    split_tasks();
//    establish_connections();
//    send_task();
//    gather_results();


    return 0;
} 
