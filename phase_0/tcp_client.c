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

    int client_sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(SERVER_PORT);

    if(connect(client_sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr))!=0){
        printf("[ERROR] Failed to connect to tcp server\n");
        exit(1);
    }else{
        printf("[INFO] Connected to tcp server\n");
    }

    while(1){
        char *line;
        size_t line_len = 0;
        ssize_t read_n = getline(&line, &line_len, stdin);

        if(read_n==-1){
            printf("[ERROR] Error encountered\n");
            exit(1);
        }

        send(client_sock_fd, line, line_len, 0);

        char buff[BUFF_SIZE];
        memset(buff, 0, BUFF_SIZE);

        read_n = recv(client_sock_fd, buff, sizeof(buff), 0);

        if(read_n<=0){
            printf("[ERROR] Error encountered, closing the connection\n");
            close(client_sock_fd);
            exit(1);
        }

        printf("[SERVER MESSAGE] %s", buff);

    }

    close(client_sock_fd);
    return 0;

}