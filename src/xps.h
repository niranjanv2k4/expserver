#ifndef XPS_H
#define XPS_H
// #define _POSIX_C_SOURCE 200112L
#define _GNU_SOURCE

// Header files
#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

// 3rd party libraries
#include "lib/vec/vec.h" // https://github.com/rxi/vec

// Constants
#define DEFAULT_BACKLOG 64
#define MAX_EPOLL_EVENTS 32
#define DEFAULT_BUFFER_SIZE 100000 // 100 KB
#define DEFAULT_NULL_THRESH 32

// Error constants
#define E_SUCCESS 0
#define E_FAIL -1
#define E_AGAIN -2
#define E_NEXT -3
#define E_NOTFOUND -4
#define E_PERMISSION -5
#define E_EOF -6

// Data types
typedef unsigned char u_char;
typedef unsigned int u_int;
typedef unsigned long u_long;

// Structures
struct xps_core_s;
struct xps_loop_s;
struct xps_listener_s;
struct xps_connection_s;

// Struct typedefs
typedef struct xps_core_s xps_core_t;
typedef struct xps_loop_s xps_loop_t;
typedef struct xps_listener_s xps_listener_t;
typedef struct xps_connection_s xps_connection_t;

typedef struct xps_buffer_s xps_buffer_t;
typedef struct xps_buffer_list_s xps_buffer_list_t ;

typedef struct xps_pipe_s xps_pipe_t;
typedef struct xps_pipe_source_s xps_pipe_source_t;
typedef struct xps_pipe_sink_s xps_pipe_sink_t;

typedef struct xps_file_s xps_file_t;
typedef struct xps_keyval_s xps_keyval_t;


// Function typedefs
typedef void (*xps_handler_t)(void *ptr);

// xps headers
#include "core/xps_core.h"
#include "core/xps_loop.h"
#include "core/xps_pipe.h"
#include "network/xps_connection.h"
#include "network/xps_listener.h"
#include "network/xps_upstream.h"
#include "utils/xps_logger.h"
#include "utils/xps_utils.h"
#include "utils/xps_buffer.h"
#include "disk/xps_mime.h"
#include "disk/xps_file.h"


#endif