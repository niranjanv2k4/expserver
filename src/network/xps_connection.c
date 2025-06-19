#include"../xps.h"


void connection_loop_read_handler(void *ptr);
void connection_loop_write_handler(void *ptr);
void connection_loop_close_handler(void *ptr);

void connection_source_handler(void *ptr);
void connection_source_close_handler(void *ptr);
void connection_sink_handler(void *ptr);
void connection_sink_close_handler(void *ptr);
void connection_close(xps_connection_t *connection, bool peer_closed);


loop_event_t *get_loop_event_by_fd(xps_loop_t *loop, int fd);


void strrev(char *s){
    int n = strlen(s)-1;
    for(int i = 0; i<n/2; i++){
        char temp = s[i];
        s[i]=s[n-1-i];
        s[n-1-i]=temp;
    }
}

/*responsible for creating a connection instance by allocating it the required memory and attaching the created instance to the event loop.*/
xps_connection_t *xps_connection_create(xps_core_t *core, u_int sock_fd){
    assert(core!=NULL);

    xps_connection_t *connection = (xps_connection_t *)malloc(sizeof(xps_connection_t));
    if(connection==NULL){
        logger(LOG_ERROR, "xps_connection_create()", "malloc() failed for 'connection'");
        return NULL;
    }

    xps_pipe_source_t *source = xps_pipe_source_create(connection, connection_loop_read_handler, connection_source_close_handler);
    if(source==NULL){
        logger(LOG_ERROR, "xps_connection_create()", "xps_pipe_source_create() failed");
        free(connection);
        return NULL;
    }

    xps_pipe_sink_t *sink = xps_pipe_sink_create(connection, connection_loop_write_handler, connection_sink_close_handler);
    if(sink==NULL){
        logger(LOG_ERROR, "xps_connection_create()", "xps_pipe_sink_create() failed");
        xps_pipe_source_destroy(source);
        free(connection);
        return NULL;
    }


    connection->core = core;
    connection->sock_fd = sock_fd;
    connection->listener = NULL;
    connection->remote_ip = get_remote_ip(sock_fd);

    connection->source = source;
    connection->sink = sink;

    /*STAGE 6(doubt): string will be echoed back if EPOLLET flag is removed, but not if added, because when edge triggerd the socket become writable and fire the epoll even before the message is sent therfore the when the message is actually sent it won't trigger therefore no reponse will generated*/

    if(xps_loop_attach(core->loop, sock_fd, EPOLLIN | EPOLLOUT , connection, connection_source_handler, connection_sink_handler, connection_loop_close_handler)==E_FAIL){
        logger(LOG_ERROR, "xps_connection_create()", "xps_loop_attach() failed");
        xps_pipe_source_destroy(source);
        xps_pipe_sink_destroy(sink);
        free(connection);
        return NULL;
    }

    vec_push(&(core->connections), connection);

    logger(LOG_DEBUG, "xps_connection_create()", "created connection");

    return connection;
}


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

    xps_pipe_source_destroy(connection->source);
    xps_pipe_sink_destroy(connection->sink);

    close(connection->sock_fd);
    free(connection);

    logger(LOG_DEBUG, "xps_connection_destroy()", "destroyed connection");
}


void connection_source_handler(void *ptr){
    assert(ptr!=NULL);
    
    xps_connection_t *connection = ptr;
    xps_pipe_source_t *source = connection->source;

    xps_buffer_t *buffer = xps_buffer_create(DEFAULT_BUFFER_SIZE, 0, NULL);
    if(buffer==NULL){
        logger(LOG_ERROR, "connection_source_handler()", "xps_buffer_create() failed");
        return;
    }

    long read_n = recv(connection->sock_fd, buffer->data, buffer->size, 0);
    buffer->len = read_n;

    if(read_n<0 && (errno==EAGAIN || errno == EWOULDBLOCK)){
        xps_buffer_destroy(buffer);
        source->ready = false;
        return;
    }

    if(read_n<0){
        xps_buffer_destroy(buffer);
        logger(LOG_INFO, "connection_source_handler()", "recv() failed");
        connection_close(connection, false);
        return;
    }

    if(read_n==0){
        logger(LOG_INFO, "connection_source_handler()", "peer closed the connection");
        xps_buffer_destroy(buffer);
        connection_close(connection, true);
        return;
    }

    buffer->data[read_n] = '\0';
    printf("[CLIENT MESSAGE] %s", buffer->data);

    /*Commented out as a part of upstream module*/
    // strrev(buffer->data);


    if(xps_pipe_source_write(source, buffer)!=E_SUCCESS){
        logger(LOG_ERROR, "xps_source_handler()", "xps_pipe_source_write() failed");
        xps_buffer_destroy(buffer);
        connection_close(connection, false);
        return;
    }

    xps_buffer_destroy(buffer);

    logger(LOG_DEBUG, "connection_source_handler()", "data read complete");
}

void connection_source_close_handler(void *ptr){
    assert(ptr!=NULL);

    xps_connection_t *connection = ptr;
    xps_pipe_source_t *source = connection->source;

    if(!source->active && !connection->sink->active){
        connection_close(connection, true);
    }
}


void connection_sink_handler(void *ptr){
    assert(ptr!=NULL);
    
    xps_connection_t *connection = ptr;
    xps_pipe_sink_t *sink = connection->sink;

    if(sink->pipe->buff_list->len==0){
        return;
    }

    xps_buffer_t *buffer = xps_pipe_sink_read(sink, sink->pipe->buff_list->len);
    if(buffer==NULL){
        logger(LOG_ERROR, "connection_sink_handler()", "xps_pipe_sink_read() failed");
        return;
    }

    ssize_t write_n = send(connection->sock_fd, buffer->data, buffer->len, MSG_NOSIGNAL);
    xps_buffer_destroy(buffer);

    if(write_n<0 && (errno == EAGAIN || errno == EWOULDBLOCK)){
        sink->ready = false;
        return;
    }

    if(write_n<0){
        logger(LOG_ERROR, "connection_sink_handler()", "send() failed");
        connection_close(connection, false);
        return;
    }

    if(write_n==0)
        return;

    if(xps_buffer_list_clear(sink->pipe->buff_list, write_n)!=E_SUCCESS){
        logger(LOG_ERROR, "connection_sink_handler()", "failed to clear %d bytes from sink", write_n);
    }
    
    logger(LOG_DEBUG, "connection_write_handler()", "write handler complete");
}

void connection_sink_close_handler(void *ptr){
    assert(ptr!=NULL);
    
    xps_connection_t *connection = ptr;
    xps_pipe_sink_t *sink = connection->sink;

    if(!sink->active && !connection->source->active){
        connection_close(connection, true);
    }
}

void connection_loop_read_handler(void *ptr){
    assert(ptr!=NULL);
    xps_connection_t *connection = ptr;
    connection->source->ready = true;
}

void connection_loop_write_handler(void *ptr){
    assert(ptr!=NULL);
    xps_connection_t *connection = ptr;
    connection->sink->ready = true;
}

void connection_loop_close_handler(void *ptr){
    assert(ptr!=NULL);
    xps_connection_t *connection = ptr;
    connection_close(connection, true);
}

void connection_close(xps_connection_t *connection, bool peer_closed){
    assert(connection!=NULL);
    logger(LOG_INFO, "connection_close()", peer_closed?"peer closed connection":"closing connection");
    xps_connection_destroy(connection);
}