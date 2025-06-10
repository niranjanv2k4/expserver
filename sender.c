#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8001
#define BUFFER_SIZE 1024

int main(){
    int sock = 0;
    struct sockaddr_in server_addr;
    char buff[BUFFER_SIZE] = {0};
    char input[BUFFER_SIZE];

    if((sock=socket(AF_INET, SOCK_STREAM, 0))<0){
        perror("Error message");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if(inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr)<0){
        perror("Invalid address / Not supported");
        return -1;
    }

    if(connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr))<0){
        perror("Connection failed");
        return -1;
    }

    while (1){
        printf("Enter the message to send: ");
        fgets(input, BUFFER_SIZE, stdin);

        int send_result = send(sock, input, strlen(input), 0);
        if(send_result == -1){
            perror("Send failed");
        }else{
            printf("Message sent to server\n");
        }
    }

    close(sock);
    return 0;
}