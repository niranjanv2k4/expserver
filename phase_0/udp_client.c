#include<arpa/inet.h>
#include<netdb.h>
#include<netinet/in.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<unistd.h>

#define SERVER_PORT 8080
#define BUFF_SIZE 10000

int main(){
    char buffer[BUFF_SIZE];
    char message[BUFF_SIZE];

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    while(1){
        printf("Enter a string: ");
        fgets(message, BUFF_SIZE, stdin);

        sendto(sockfd, message, strlen(message)-1, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

        ssize_t n = recvfrom(sockfd, buffer, BUFF_SIZE, 0, NULL, NULL);

        buffer[n] = '\0';

        printf("[SERVER MESSAGE] %s\n", buffer);

    }

    close(sockfd);
    return 0;

}