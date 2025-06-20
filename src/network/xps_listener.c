#include"../xps.h"

void listener_connection_handler(void *ptr);

xps_listener_t* xps_listener_create(xps_core_t *core, const char *host, u_int port){
    assert(host!=NULL);
    assert(is_valid_port(port));

    int sock_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if(sock_fd<0){
        logger(LOG_ERROR, "xps_listener_create()", "socket() failed");
        perror("Error message");
        return NULL;
    }

    const int enable = 1;
    if(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))<0){
        logger(LOG_ERROR, "xps_listener_create()", "setsockopt() failed");
        perror("Error message");
        close(sock_fd);
        return NULL;
    }

    struct addrinfo *addr_info = xps_getaddrinfo(host, port);
    if(addr_info==NULL){
        logger(LOG_ERROR, "xps_listener_create()", "xps_getaddrinfo() failed");
        close(sock_fd);
        return NULL;
    }

    if(bind(sock_fd, addr_info->ai_addr, addr_info->ai_addrlen)<0){
        logger(LOG_ERROR, "xps_listener_create()", "failed to bind() to %s:%u", host, port);
        perror("Error message");
        freeaddrinfo(addr_info);
        close(sock_fd);
        return NULL;
    }

    freeaddrinfo(addr_info);

    if(listen(sock_fd, DEFAULT_BACKLOG)<0){
        logger(LOG_ERROR, "xps_listener_create()", "listen() failed");
        perror("Error message");
        close(sock_fd);
        return NULL;
    }

    xps_listener_t *listener = malloc(sizeof(xps_listener_t));
    if(listener==NULL){
        logger(LOG_ERROR, "xps_listener_create()", "malloc() failed for listener");
        close(sock_fd);
        return NULL;
    }

    listener->core = core;
    listener->host = host;
    listener->port = port;
    listener->sock_fd = sock_fd;

    if(xps_loop_attach(core->loop, sock_fd, EPOLLIN | EPOLLET, listener, listener_connection_handler, NULL, NULL)==E_FAIL){
        logger(LOG_ERROR, "xps_listener_create()", "falied to attach to loop");
        free(listener);
        close(sock_fd);
        return NULL;
    }

    vec_push(&(core->listeners), listener);

    logger(LOG_DEBUG, "xps_listener_create()", "created listener port on %d", port);

    return listener;
}

void xps_listener_destroy(xps_listener_t *listener){

    assert(listener!=NULL);
    
    xps_loop_detach(listener->core->loop, listener->sock_fd);

    for(int i = 0; i<listener->core->listeners.length; i++){
        xps_listener_t *curr = listener->core->listeners.data[i];
        if(curr==listener){
            listener->core->listeners.data[i]=NULL;
            break;
        }
    }
    
    close(listener->sock_fd);

    logger(LOG_DEBUG, "xps_listener_destroy()", "destroyed listener or port %d", listener->port);

    free(listener);
}

void listener_connection_handler(void *ptr){
    assert(ptr!=NULL);
    xps_listener_t *listener = ptr;

    while(1){
        struct sockaddr conn_addr;
        socklen_t conn_addr_len = sizeof(conn_addr);

        int conn_sock_fd = accept(listener->sock_fd, (struct sockaddr *)&conn_addr, &conn_addr_len);
        if(conn_sock_fd<0){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                break;
            }
            else{
                logger(LOG_ERROR, "listener_connection_handler()", "accept() failed");
                perror("Error message");
                return;
            }
        }

        if(make_socket_non_blocking(conn_sock_fd)<0){
            logger(LOG_ERROR, "listener_connection_handler()", "Failed to change to non-blocking");
            perror("Error message");
            return;
        }

        xps_connection_t *client = xps_connection_create(listener->core, conn_sock_fd);
        if(client==NULL){
            logger(LOG_ERROR, "listener_connection_handler()", "xps_connection_create() failed");
            close(conn_sock_fd);
            return;
        }

        client->listener = listener;

        if(listener->port==8001){

            xps_connection_t *upstream_conn = xps_upstream_create(listener->core, "127.0.0.1", 3000);
            xps_pipe_t *client_to_upstream = xps_pipe_create(listener->core, DEFAULT_BUFFER_SIZE, client->source, upstream_conn->sink);
            xps_pipe_t *upstream_to_client = xps_pipe_create(listener->core, DEFAULT_BUFFER_SIZE, upstream_conn->source, client->sink);
            
            if(!client_to_upstream || !upstream_to_client){
                logger(LOG_ERROR, "listener_connection_handler()", "xps_pipe_create() failed");
                perror("Error message");
                close(conn_sock_fd);
                return;
            }
        }
        else if(listener->port==8002){

            int error;
            xps_file_t *file = xps_file_create(listener->core, "../public/sample.txt", &error);
            
            if(!xps_pipe_create(listener->core, DEFAULT_BUFFER_SIZE, file->source, client->sink)){
                logger(LOG_ERROR, "listener_connection_handler()", "xps_pipe_create() failed");
                perror("Error message");
                return;
            }
            
            /*Calling the file handler functions*/

            file->source->handler_cb(file->source);
            file->source->close_cb(file->source);
        }
        else{
            xps_pipe_create(listener->core, DEFAULT_BUFFER_SIZE, client->source, client->sink);
        }

        logger(LOG_INFO, "listener_connection_handler()", "new connection accepted");
    }
}