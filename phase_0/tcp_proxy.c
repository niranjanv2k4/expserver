#include<arpa/inet.h>
#include<netdb.h>
#include<netinet/in.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<unistd.h>
#include<sys/epoll.h>
#include<stdbool.h>

#define PORT 8080
#define BUFF_SIZE 10000
#define MAX_ACCEPT_BACKLOG 5
#define MAX_EPOLL_EVENTS 10
#define UPSTREAM_PORT 3000
#define MAX_SOCKS 10

int listen_sock_fd, epoll_fd;
struct epoll_event events[MAX_EPOLL_EVENTS];
int route_table[MAX_SOCKS][2], route_table_size = 0;


/*--------HELPER FUNCTIONS--------*/
int find_pair(int sock_fd, bool flag){

    int idx = flag?0:1, res = flag?1:0;

    for(int i = 0; i<route_table_size; i++){
        if(route_table[i][idx]==sock_fd){
            return route_table[i][res];
        }
    }
    return -1;
}

int get_fd_role(int sock_fd){
    for(int i = 0; i<route_table_size; i++){
        if(route_table[i][0]==sock_fd)return 1;
        if(route_table[i][1]==sock_fd)return 0;
    }
    return -1;
}


/*--------REQUIRED FUNCTIONS--------*/
int create_loop(){
    return epoll_create1(0);
}

void loop_attach(int epoll_fd, int fd, int events){
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}

int create_server(){

    listen_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    int enable = 1;
    setsockopt(listen_sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    bind(listen_sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    listen(listen_sock_fd, MAX_ACCEPT_BACKLOG);
    printf("[INFO] Server listening on port %d...\n", PORT);

    return listen_sock_fd;
}

int connect_upstream(){
    int upstream_sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(UPSTREAM_PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(upstream_sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    return upstream_sock_fd;
}

void accept_connection(int listen_sock_fd){

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int conn_sock_fd = accept(listen_sock_fd, (struct sockaddr *)&client_addr, &client_addr_len);

    printf("[INFO] Client connected to the server.\n");

    loop_attach(epoll_fd, conn_sock_fd, EPOLLIN);

    int upstream_sock_fd = connect_upstream();

    loop_attach(epoll_fd, upstream_sock_fd, EPOLLIN);

    route_table[route_table_size][0] = conn_sock_fd;
    route_table[route_table_size][1] = upstream_sock_fd;
    route_table_size+=1;

}

void handle_client(int conn_sock_fd){
    char buff[BUFF_SIZE];

    int read_n = recv(conn_sock_fd, buff, sizeof(buff), 0);

    if(read_n<=0){
        printf("[ERROR] Error encountered, Closing connection (handle client)\n");
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, conn_sock_fd, NULL);
        close(conn_sock_fd);
        return;
    }

    printf("[CLIENT MESSAGE] %s", buff);

    /* find the right upstream socket from the route table */
    int upstream_sock_fd = find_pair(conn_sock_fd, true);

    int bytes_written = 0;
    int message_len = read_n;

    /*This loop ensures reliable transmission of the full message.*/
    while(bytes_written<message_len){
        int n = send(upstream_sock_fd, buff+bytes_written, message_len-bytes_written, 0);
        bytes_written+=n;
    }
}

void handle_upstream(int upstream_sock_fd){
    char buff[BUFF_SIZE];

    int read_n = recv(upstream_sock_fd, buff, sizeof(buff), 0);

    if(read_n<=0){
        printf("[ERROR] Error encountered, Closing connection (handle upstream)\n");
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, upstream_sock_fd, NULL);
        close(upstream_sock_fd);
        return;
    }

    /* find the right client socket from the route table */
    int client_sock_fd = find_pair(upstream_sock_fd, false);

    int bytes_written = 0;
    int message_len = read_n;

    /*This loop ensures reliable transmission of the full message.*/
    while(bytes_written<message_len){
        int n = send(client_sock_fd, buff+bytes_written, message_len-bytes_written, 0);
        bytes_written+=n;
    }
}


void loop_run(int epoll_fd){
    while(1){
        printf("[DEBUG] Epoll wait\n");
        int n_ready_fds = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, -1);

        for(int i = 0; i<n_ready_fds; i++){
            int curr_fd = events[i].data.fd;

            if(curr_fd == listen_sock_fd){
                accept_connection(listen_sock_fd);

            }

            /*Checks if it is a client_sock_fd or upstream_sock_fd*/
            int role = get_fd_role(curr_fd);

            if(role==1){
                handle_client(curr_fd);
            }else if(role==0){
                handle_upstream(curr_fd);
            }
        }
    }
}

int main(){
    listen_sock_fd = create_server();
    epoll_fd = create_loop();
    loop_attach(epoll_fd, listen_sock_fd, EPOLLIN);
    loop_run(epoll_fd);
    return 0;
}