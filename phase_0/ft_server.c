#include<arpa/inet.h>
#include<netdb.h>
#include<netinet/in.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<unistd.h>
#include<sys/epoll.h>

#define PORT 8080
#define BUFF_SIZE 100000
#define MAX_ACCEPT_BACKLOG 5
#define MAX_EPOLL_EVENTS 10


void write_to_file(int conn_sock_fd){
    char buff[BUFF_SIZE];
    ssize_t bytes_recieved;

    FILE *fp;
    const char *filename = "../files/t2.txt";
    fp = fopen(filename, "w");
    if(fp==NULL){
        perror("[-] Error creating the file.\n");
        exit(EXIT_FAILURE);
    }

    printf("[INFO] Recieving data from the client...\n");

    while((bytes_recieved=recv(conn_sock_fd, buff, sizeof(buff), 0))>0){
        printf("[FILE DATA] %s", buff);
        fprintf(fp, "%s", buff);
        memset(buff, 0, BUFF_SIZE);
    }

    if(bytes_recieved<0){
        printf("[-] Error recieving data\n");
    }

    fclose(fp);
    printf("[INFO] Data transfer successful\n");
}

int main(){
    int listen_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    int enable = 1;
    setsockopt(listen_sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(listen_sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    listen(listen_sock_fd, MAX_ACCEPT_BACKLOG);
    printf("[INFO] Server listening to port %d...\n", PORT);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len;

    int conn_sock_fd = accept(listen_sock_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    printf("[INFO] Client connected to server\n");

    write_to_file(conn_sock_fd);

    return 0;
}
