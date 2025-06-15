#include"../xps.h"


void connection_loop_read_handler(void *ptr);
void connection_loop_write_handler(void *ptr);
void connection_loop_close_handler(void *ptr);

void connection_read_handler(void *ptr);
void connection_write_handler(void *ptr);

void strrev(char *s){
    int n = strlen(s)-1;
    for(int i = 0; i<n/2; i++){
        char temp = s[i];
        s[i]=s[n-1-i];
        s[n-1-i]=temp;
    }
}

/*responsible for creating a connection instance by allocating it the required memory and attaching the created instance to the event loop.*/
xps_connection_t *xps_connection_create(xps_core_t *core, u_int sock_fd, xps_listener_t *listener){

    xps_connection_t *connection = (xps_connection_t *)malloc(sizeof(xps_connection_t));
    if(connection==NULL){
        logger(LOG_ERROR, "xps_connection_create()", "malloc() failed for 'connection'");
        return NULL;
    }

    connection->core = core;
    connection->sock_fd = sock_fd;
    connection->listener = NULL;
    connection->remote_ip = get_remote_ip(sock_fd);
    connection->write_buff_list = xps_buffer_list_create();

    connection->read_ready = false;
    connection->write_ready = false;
    connection->send_handler = connection_write_handler;
    connection->recv_handler = connection_read_handler;


    xps_loop_attach(core->loop, sock_fd, EPOLLIN | EPOLLOUT | EPOLLET, connection, connection_loop_read_handler, connection_loop_write_handler, connection_loop_close_handler);

    vec_push(&(core->connections), connection);

    logger(LOG_DEBUG, "xps_connection_create()", "created connection");

    return connection;
}


/*takes in a connection and destroys it by detaching it from the loop and de-allocating the memory consumed by it.*/
void xps_connection_destroy(xps_connection_t *connection){

    assert(connection!=NULL);

    for(int i = 0; i<connection->core->connections.length; i++){
        xps_connection_t *curr = connection->core->connections.data[i];
        if(curr==connection){
            connection->core->connections.data[i]=NULL;
            break;
        }
    }

    xps_loop_detach(connection->core->loop, connection->sock_fd);
    close(connection->sock_fd);
    free(connection);

    logger(LOG_DEBUG, "xps_connection_destroy()", "destroyed connection");
}


void connection_read_handler(void *ptr){
    assert(ptr!=NULL);
    xps_connection_t *connection = ptr;

    while(1){

    xps_buffer_t *buffer = xps_buffer_create(DEFAULT_BUFFER_SIZE, 0, NULL);
    if(buffer==NULL){
        logger(LOG_ERROR, "connection_loop_read_handler()", "xps_buffer_create() failed");
        perror("Error message");
        return;
    }
    long read_n = recv(connection->sock_fd, buffer->data, buffer->size, 0);

    if(read_n<0){
        if(errno==EAGAIN || errno == EWOULDBLOCK){
            connection->read_ready = false;
        }else{
            logger(LOG_ERROR, "connection_loop_read_handler()", "recv() failed");
            perror("Error message");
            xps_buffer_destroy(buffer);
            xps_connection_destroy(connection);
        }
        return;
    }

    if(read_n==0){
        logger(LOG_INFO, "xps_connection_read_handler()", "peer closed the connection");
        xps_buffer_destroy(buffer);
        xps_connection_destroy(connection);
        return;
    }

    buffer->len = read_n;
    buffer->data[read_n] = '\0';

    printf("[CLIENT MESSAGE] %s", buffer->data);
    
    strrev(buffer->data);

    xps_buffer_list_append(connection->write_buff_list, buffer);
}

    logger(LOG_DEBUG, "connection_loop_read_handler()", "data read complete");
}

void connection_write_handler(void *ptr){
    assert(ptr!=NULL);
    xps_connection_t *connection = ptr;

    if(connection->write_buff_list->len==0){
        return;
    }

    xps_buffer_t *buffer = xps_buffer_list_read(connection->write_buff_list, connection->write_buff_list->len);
    if(buffer==NULL){
        logger(LOG_ERROR, "connection_loop_read_handler()", "xps_buffer_list_read() failed");
        perror("Error message");
        return;
    }

    ssize_t n = send(connection->sock_fd, buffer->data, buffer->len, 0);

    if(n==-1){
        if(errno == EAGAIN || errno == EWOULDBLOCK){
            connection->write_ready = false;
        }else{
            logger(LOG_ERROR, "connection_loop_write_handler()", "send() failed");
            perror("Error message");
            xps_connection_destroy(connection);
        }
        return;
    }

    xps_buffer_list_clear(connection->write_buff_list, n);
    
    logger(LOG_DEBUG, "connection_write_handler()", "write handler complete");
}


void connection_loop_close_handler(void *ptr){
    assert(ptr!=NULL);
    xps_connection_t *connection = ptr;
    xps_connection_destroy(connection);
}

void connection_loop_read_handler(void *ptr){
    assert(ptr!=NULL);
    xps_connection_t *connection = ptr;
    connection->read_ready = true;
}

void connection_loop_write_handler(void *ptr){
    assert(ptr!=NULL);
    xps_connection_t *connection = ptr;
    connection->write_ready = true;
}