#include"../xps.h"

void strrev(char *s){
    int n = strlen(s)-1;
    for(int i = 0; i<n/2; i++){
        char temp = s[i];
        s[i]=s[n-1-i];
        s[n-1-i]=temp;
    }
}

/*responsible for creating a connection instance by allocating it the required memory and attaching the created instance to the event loop.*/
xps_connection_t *xps_connection_create(int epoll_fd, int sock_fd){

    xps_connection_t *connection = (xps_connection_t *)malloc(sizeof(xps_connection_t));
    if(connection==NULL){
        logger(LOG_ERROR, "xps_connection_create()", "malloc() failed for 'connection'");
        return NULL;
    }

    xps_loop_attach(epoll_fd, sock_fd, EPOLLIN);

    connection->epoll_fd = epoll_fd;
    connection->sock_fd = sock_fd;
    connection->listener = NULL;
    connection->remote_ip = get_remote_ip(sock_fd);

    vec_push(&connections, connection);

    logger(LOG_DEBUG, "xps_connection_create()", "created connection");

    return connection;
}


/*takes in a connection and destroys it by detaching it from the loop and de-allocating the memory consumed by it.*/
void xps_connection_destroy(xps_connection_t *connection){

    assert(connection!=NULL);

    for(int i = 0; i<connections.length; i++){
        xps_connection_t *curr = connections.data[i];
        if(curr==connection){
            connections.data[i]=NULL;
            break;
        }
    }

    xps_loop_detach(connection->epoll_fd, connection->sock_fd);
    close(connection->sock_fd);
    free(connection->remote_ip);
    free(connection);
    logger(LOG_DEBUG, "xps_connection_destroy()", "destroyed connection");

}


/*With the connection instances attached to the epoll, we will get notification from the event loop if there is a read event. To handle this, we’ll create a function xps_connection_read_handler()*/
void xps_connection_read_handler(xps_connection_t *connection){

    assert(connection!=NULL);

    char buff[DEFAULT_BUFFER_SIZE];
    long read_n = recv(connection->sock_fd, buff, sizeof(buff), 0);

    if(read_n<0){
        logger(LOG_ERROR, "xps_connection_read_handler()", "recv() failed");
        perror("Error message");
        return;
    }

    if(read_n==0){
        logger(LOG_INFO, "xps_connection_read_handler()", "peer closed the connection");
        xps_connection_destroy(connection);
        return;
    }

    buff[read_n] = '\0';

    printf("[CLIENT MESSAGE] %s", buff);
    
    strrev(buff);

    long bytes_written = 0;
    long message_len = read_n;
    while(bytes_written<message_len){
        long write_n = send(connection->sock_fd, buff+bytes_written, message_len-bytes_written, 0);
        if(write_n<0){
            logger(LOG_ERROR, "xps_connection_read_handler()", "send() failed");
            perror("Error message");
            xps_connection_destroy(connection);
            return;
        }
        bytes_written+=write_n;
    }

}
