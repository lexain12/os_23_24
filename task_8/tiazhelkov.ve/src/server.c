/*
** broadcaster.c -- дейтаграммный “клиент” подобный talker.c, но
** этот может вещать
*/
#include <bits/types/struct_timeval.h>
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
#define BUF_SIZE 4096
#define IP4_LEN 15
#define MAX_WORKERS 10

struct Worker {
    char ip[IP4_LEN];
    int num_of_cores;
    int socket;
    double result;
};

struct Task {
    double x1;
    double x2;
    double y1;
    double y2;
    long long num_or_points;
};

struct Server {
    struct Worker workers[MAX_WORKERS];
    int num_of_workers;
    struct Task* tasks;
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

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 1;

        int ready = select(receiveSocket + 1, &readSet, NULL, NULL, &timeout);
        if (ready == -1) {
            perror("Select error");
            exit(EXIT_FAILURE);
        }

        if (ready > 0) {
            ready = 0;
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
				printf("New worker %d %s\n", newWorker.num_of_cores, newWorker.ip);
            } else {
                printf("Maximum workers reached. Ignoring additional responses.\n");
            }
        }
    }

    close(receiveSocket);
}

void establish_connections(struct Server *server) {
    for (int i = 0; i < server->num_of_workers; ++i) {
        int workerSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (workerSocket == -1) {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }

        struct sockaddr_in workerAddr;
        memset(&workerAddr, 0, sizeof(workerAddr));
        workerAddr.sin_family = AF_INET;
        workerAddr.sin_port = htons(SERVERPORT);
        inet_pton(AF_INET, server->workers[i].ip, &workerAddr.sin_addr);

        if (connect(workerSocket, (struct sockaddr *)&workerAddr, sizeof(workerAddr)) == -1) {
            perror("Connection failed");
            close(workerSocket);
            exit(EXIT_FAILURE);
        }

        // Connection established, store the socket in the worker struct
        server->workers[i].socket = workerSocket;
    }
}

void split_tasks(struct Server* server, struct Task main_task) {
    int num_of_cores = 0;
    for (int i = 0; i < server->num_of_workers; i++) {
        num_of_cores += server->workers[i].num_of_cores;
    }

    server->tasks = (struct Task*) calloc(num_of_cores, sizeof(struct Task));

    for (int i = 0; i < num_of_cores; i++) {
        server->tasks[i].x1 = (main_task.x2 - main_task.x1) / num_of_cores * i + main_task.x1;
        server->tasks[i].y1 = main_task.y1;
        server->tasks[i].x2 = (main_task.x2 - main_task.x1) / num_of_cores * (i + 1) + main_task.x1;
        server->tasks[i].y2 = main_task.y2;
        server->tasks[i].num_or_points = main_task.num_or_points / num_of_cores;
    }
}
int print_one_task(char* dest, struct Task task) {
    fprintf(stderr, "task %lf %lf %lf %lf %lld \n", task.x1, task.y1, task.x2, task.y2, task.num_or_points);
    return sprintf(dest, "%lf %lf %lf %lf %lld \n", task.x1, task.y1, task.x2, task.y2, task.num_or_points);
}

void send_task(struct Server *server) {
    char buffer[BUF_SIZE];
    int task_number;
    for (int i = 0; i < server->num_of_workers; ++i) {
        int offset = 0;
        for (int j = 0; j < server->workers[i].num_of_cores; j++) {
            offset += print_one_task(buffer + offset, server->tasks[task_number]);
            task_number += 1;
        }

        fprintf(stderr, "%s\n", buffer);
        write(server->workers[i].socket, buffer, sizeof(buffer));
    }
    printf ("Number of tasks %d\n", task_number);
}


void gather_results(struct Server* server) {
    fd_set readSet;
    FD_ZERO(&readSet);
    int maxfd = server->workers[server->num_of_workers - 1].socket;

    // Add all worker sockets to the file descriptor set and find the maximum file descriptor
    for (int i = 0; i < server->num_of_workers; ++i) {
        FD_SET(server->workers[i].socket, &readSet);
        if (server->workers[i].socket > maxfd) {
            maxfd = server->workers[i].socket;
        }
    }

    // Track the number of responses received from workers
    int responsesReceived = 0;

    while (responsesReceived < server->num_of_workers) {
        // Set the timeout (adjust as needed)
        struct timeval timeout;
        timeout.tv_sec = 5; // 5 seconds timeout (adjust as needed)
        timeout.tv_usec = 0;

        // Call select
        int ready = select(maxfd + 1, &readSet, NULL, NULL, &timeout);
        if (ready == -1) {
            perror("Select error");
            exit(EXIT_FAILURE);
        }

        if (ready == 0) {
            printf("Timeout: No additional responses received.\n");
            break; // Timeout reached
        }

        // Process ready sockets
        for (int i = 0; i < server->num_of_workers; ++i) {
            if (FD_ISSET(server->workers[i].socket, &readSet)) {
                // Handle data reception from worker socket
                double receivedValue;
                int bytesRead = recv(server->workers[i].socket, &receivedValue, sizeof(double), 0);

                if (bytesRead == sizeof(double)) {
                    printf("Received value from worker %d: %f\n", i + 1, receivedValue);
                    server->workers[i].result = receivedValue;
                    responsesReceived++;
                    FD_CLR(server->workers[i].socket, &readSet); // Remove socket from the set
                } else if (bytesRead == 0) {
                    printf("Worker %d closed the connection.\n", i + 1);
                    FD_CLR(server->workers[i].socket, &readSet); // Remove socket from the set
                } else {
                    perror("Recv error");
                    // Handle recv error or connection issue with the worker
                }
            }
        }
    }

    // All workers have responded, process the results if needed
    for (int i = 0; i < server->num_of_workers; ++i) {
        printf("Worker %d result: %f\n", i + 1, server->workers[i].result);
    }
}

int main(int argc, char *argv[])
{
    struct Task main_task = {.x1 = 0.0, .y1 = 0.0, .x2 = 3.0, .y2 = 3.0, .num_or_points=10000};

    struct Server server = {};
    send_invite_to_workers();
    rcv_ip_from_workers(&server);
    split_tasks(&server, main_task);
    establish_connections(&server);
    sleep(1);
    send_task(&server);
    gather_results(&server);


    return 0;
} 
