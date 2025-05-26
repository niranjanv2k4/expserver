#include<arpa/inet.h>
#include<netdb.h>
#include<netinet/in.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<unistd.h>

#define PORT 8080
#define BUFF_SIZE 100000
#define MAX_ACCEPT_BACKLOG 5

void strrev(char *s){
    int n = strlen(s);
    for(int i = 0; i<n/2; i++){
        char temp = s[i];
        s[i]=s[n-1-i];
        s[n-1-i]=temp;
    }
}

int main(){

    int listen_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    int enable = 1;
    setsockopt(listen_socket_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    bind(listen_socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    listen(listen_socket_fd, MAX_ACCEPT_BACKLOG);
    printf("[INFO] Server listening on port %d...\n", PORT);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len;

    while(1){
        int conn_sock_fd = accept(listen_socket_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        printf("[INFO] Client connected to server\n");

        while(1){

            char buff[BUFF_SIZE];
            memset(buff, 0, BUFF_SIZE);

            ssize_t read_n = recv(conn_sock_fd, buff, sizeof(buff), 0);

            if(read_n<0){
                printf("[INFO] Error occured. Closing server\n");
                close(conn_sock_fd);
                exit(1);
            }
            if(read_n==0){
                printf("[INFO] Client disconnected. Closing server\n");
                close(conn_sock_fd);
                break;
            }

            printf("[CLIENT MESSAGE] %s\n", buff);

            strrev(buff);

            send(conn_sock_fd, buff, read_n, 0);
        }
    }
    return 0;
}