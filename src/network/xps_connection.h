#ifndef XPS_CONNECTION_H
#define XPS_CONNECTION_H

#include "../xps.h"

struct xps_connection_s {
  xps_core_t *core;
  int sock_fd;
  xps_listener_t *listener;
  char *remote_ip;

  xps_pipe_source_t *source;
  xps_pipe_sink_t *sink;
};

xps_connection_t *xps_connection_create(xps_core_t *core, u_int sock_fd);
void xps_connection_destroy(xps_connection_t *connection);

#endif