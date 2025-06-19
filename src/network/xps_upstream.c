#include "../xps.h"

xps_connection_t *xps_upstream_create(xps_core_t *core, const char *host, u_int port){
    assert(core!=NULL);

    if(!is_valid_port(port)){
        logger(LOG_ERROR, "xps_upstream_create()", "Invalid port");
        return NULL;
    }

    int upstream_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(upstream_sock_fd<0){
        logger(LOG_ERROR, "xps_upstream_create()", "socket creation failed");
        return NULL;
    }

    struct addrinfo *addr_info = xps_getaddrinfo(host, port);
    if(addr_info==NULL){
        logger(LOG_ERROR, "xps_upstream_create()", "xps_getaddrinfo() failed");
        close(upstream_sock_fd);
        return NULL;
    }

    int connect_error = connect(upstream_sock_fd, addr_info->ai_addr, addr_info->ai_addrlen);
    freeaddrinfo(addr_info);

    if(!(connect_error == 0 || errno == EINPROGRESS)){
        logger(LOG_ERROR, "xps_upstream_create()", "connect() failed");
        perror("Error message");
        close(upstream_sock_fd);
        return NULL;
    }

    xps_connection_t *connection = xps_connection_create(core, upstream_sock_fd);
    if(connection==NULL){
        logger(LOG_ERROR, "xps_upstream_create()", "xps_connection_create() failed");
        perror("Error message");
        close(upstream_sock_fd);
        return NULL;
    }

    logger(LOG_DEBUG, "xps_upstream_create()", "upstream created");
    
    return connection;
}