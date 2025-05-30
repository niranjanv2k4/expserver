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

void strrev(char *s){
    int n = strlen(s)-1;
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

    int epoll_fd = epoll_create1(0);
    struct epoll_event event, events[MAX_EPOLL_EVENTS];
    event.events = EPOLLIN;
    event.data.fd = listen_socket_fd;

    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_socket_fd, &event);

    while(1){
        printf("[DEBUG] Epoll wait\n");
        int n_ready_fds = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, -1);

        for(int i = 0; i<n_ready_fds; i++){
            int curr_fd = events[i].data.fd;

            if(curr_fd == listen_socket_fd){
                struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                int conn_sock_fd = accept(curr_fd, (struct sockaddr *)&client_addr, &client_addr_len);

                printf("[INFO] Client connected to the server.\n");

                struct epoll_event ev;
                ev.events = EPOLLIN;
                ev.data.fd = conn_sock_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_sock_fd, &ev);

            }else{
                char buff[BUFF_SIZE];
                memset(buff, 0, BUFF_SIZE);

                ssize_t read_n = recv(curr_fd, buff, sizeof(buff), 0);

                if(read_n<=0){
                    if(read_n==0){
                        printf("[INFO] Client disconnected.\n");
                    }else{
                        printf("[INFO] Error occured. Closing connection\n");
                    }
                    close(curr_fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, curr_fd, NULL);
                    continue;
                }

                printf("[CLIENT MESSAGE] %s", buff);
                strrev(buff);
                send(curr_fd, buff, read_n, 0);
            }


        }
    }

    return 0;
}