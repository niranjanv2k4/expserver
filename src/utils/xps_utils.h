#ifndef XPS_UTILS_H
#define XPS_UTILS_H

#include"../xps.h"

// Sockets
bool is_valid_port(u_int port);
int make_socket_non_blocking(u_int sock_fd);
struct addrinfo *xps_getaddrinfo(const char *host, u_int port);
char *get_remote_ip(u_int sock_fd);
void vec_filter_null(vec_void_t *v);
const char *get_file_text(const char *file_path);

// Other functions

#endif