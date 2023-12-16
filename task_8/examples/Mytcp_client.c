#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8124

#define MAX_SZ 4096

void f(int sockfd) {
    char buf[MAX_SZ];
    while (1) {
        printf ("Enter the message...\n");
        buf[0] = '\0';
        scanf("%s", buf);
        write(sockfd, buf, strlen(buf));
        buf[0] = '\0';
        read(sockfd, buf, sizeof(buf));
        printf("%s\n", buf);
    }

    return;
}

int main() {
    int sockfd = 0;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1) {
        perror("Socket");
        return 1;
    }

    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;  // Семейство протоколов
    servaddr.sin_addr.s_addr = inet_addr("192.168.166.193"); // Конвертация в нужный формат
    servaddr.sin_port = htons(PORT); // host to network short
    
    if (connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) != 0) {
        perror("connect");
        return 1;
    }

    printf("Connection...\n");
    f(sockfd);
}
