#ifndef XPS_FILE_H
#define XPS_FILE_H

#include "../xps.h"

struct xps_file_s {
    xps_core_t *core;
    const char *file_path;
    xps_pipe_source_t *source;
    FILE *file_struct;
    ssize_t size;
    const char *mime_type;
};

struct xps_keyval_s {
  char *key;
  char *val;
};

xps_file_t *xps_file_create(xps_core_t *core, const char *file_path, int *error);
void xps_file_destroy(xps_file_t *file);

#endif