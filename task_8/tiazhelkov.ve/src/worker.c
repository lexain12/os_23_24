#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "../include/worker.h"

#define UDP_PORT_RECEIVE 8123  // Port for receiving UDP messages
#define UDP_PORT_SEND 8123// Port for sending UDP responses
#define WORKER_PORT 8123// Port for worker TCP connections
#define MAX_CORES 8            // Maximum number of cores (for demonstration)
#define BUF_SIZE 4096

int main() {
    // UDP socket for receiving UDP messages
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket == -1) {
        perror("UDP socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in udpReceiveAddr;
    memset(&udpReceiveAddr, 0, sizeof(udpReceiveAddr));
    udpReceiveAddr.sin_family = AF_INET; udpReceiveAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    udpReceiveAddr.sin_port = htons(UDP_PORT_RECEIVE);

    if (bind(udpSocket, (struct sockaddr *)&udpReceiveAddr, sizeof(udpReceiveAddr)) == -1) {
        perror("UDP bind failed");
        close(udpSocket);
        exit(EXIT_FAILURE);
    }

    printf("Listening for UDP messages...\n");

    // Receive UDP message to get client address
    char udpBuffer[BUF_SIZE];
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    if (recvfrom(udpSocket, udpBuffer, sizeof(udpBuffer), 0, (struct sockaddr *)&clientAddr, &clientAddrLen) == -1) {
        perror("UDP receive failed");
        close(udpSocket);
        exit(EXIT_FAILURE);
    }

    udpBuffer[BUF_SIZE - 1] = '\0';

    // Get client IP and port from the received UDP message
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
    int clientPort = ntohs(clientAddr.sin_port);

    printf("Received UDP message from %s:%d\n", clientIP, clientPort);

    // Get the number of CPU cores (for demonstration, using a constant value)
    int numCores = MAX_CORES;

    // UDP socket for sending UDP response
    int udpSendSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSendSocket == -1) {
        perror("UDP send socket creation failed");
        close(udpSocket);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in udpSendAddr;
    memset(&udpSendAddr, 0, sizeof(udpSendAddr));
    udpSendAddr.sin_family = AF_INET;
    udpSendAddr.sin_port = htons(UDP_PORT_SEND);
    inet_pton(AF_INET, clientIP, &udpSendAddr.sin_addr);

    sleep(1);
    // Send the number of CPU cores back through UDP
    char coresBuffer[16];
    snprintf(coresBuffer, sizeof(coresBuffer), "%d", numCores);
    if (sendto(udpSendSocket, coresBuffer, strlen(coresBuffer), 0, (struct sockaddr *)&udpSendAddr, sizeof(udpSendAddr)) == -1) {
        perror("UDP send failed");
        close(udpSendSocket);
        close(udpSocket);
        exit(EXIT_FAILURE);
    }

    printf("Sent number of cores: %d through UDP\n", numCores);

    close(udpSendSocket);
    close(udpSocket);

    // TCP socket for accepting TCP connections from the system
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == -1) {
        perror("TCP socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in workerAddr;
    memset(&workerAddr, 0, sizeof(workerAddr));
    workerAddr.sin_family = AF_INET;
    workerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    workerAddr.sin_port = htons(WORKER_PORT);

    if (bind(listenSocket, (struct sockaddr *)&workerAddr, sizeof(workerAddr)) == -1) {
        perror("TCP bind failed");
        close(listenSocket);
        exit(EXIT_FAILURE);
    }

    if (listen(listenSocket, 5) == -1) {
        perror("Listen failed");
        close(listenSocket);
        exit(EXIT_FAILURE);
    }

    printf("Worker is now listening for TCP connections...\n");

    while (1) {
        struct sockaddr_in clientTcpAddr;
        socklen_t clientTcpAddrLen = sizeof(clientTcpAddr);

        int clientSocket = accept(listenSocket, (struct sockaddr *)&clientTcpAddr, &clientTcpAddrLen);
        if (clientSocket == -1) {
            perror("Accept failed");
            close(listenSocket);
            exit(EXIT_FAILURE);
        }

        // Handle the TCP connection (For demonstration, simply print client information)
        char clientTcpIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientTcpAddr.sin_addr, clientTcpIP, INET_ADDRSTRLEN);
        int clientTcpPort = ntohs(clientTcpAddr.sin_port);
        sleep(4);
        char buff[BUF_SIZE];
        int len = read(clientSocket, buff, sizeof(buff));
        buff[len] = '\0';

        printf("Accepted TCP connection from %s:%d\n", clientTcpIP, clientTcpPort);
        printf ("Message from server %s", buff);
        struct Task task = {};

        sscanf(buff, "%lf %lf %lf %lf %lld", &task.x1, &task.y1, &task.x2, &task.y2, &task.num_of_points);
        sleep(1);
        double result = monte_carlo(task, MAX_CORES);
        write(clientSocket, &result, sizeof(double));


        close(clientSocket);
        break;
    }

    close(listenSocket);
    return 0;
}

