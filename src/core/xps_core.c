#include "../xps.h"

xps_core_t *xps_core_create(){
    xps_core_t *core = malloc(sizeof(xps_core_t));
    if(core==NULL){
        logger(LOG_ERROR, "xps_core_create()", "malloc() failed for 'core'");
        perror("Error message");
        return NULL;
    }

    xps_loop_t *loop = xps_loop_create(core);
    if(loop==NULL){
        logger(LOG_ERROR, "xps_core_create()", "xps_loop_create() failed");
        perror("Error message");
        free(core);
        return NULL;
    }

    core->loop = loop;

    vec_init(&(core->listeners));
    vec_init(&(core->connections));
    vec_init(&(core->pipes));

    core->n_null_listeners = 0;
    core->n_null_connections = 0;
    core->n_null_pipes = 0;


    logger(LOG_DEBUG, "xps_core_create()", "core created");
    return core;
}

void xps_core_destroy(xps_core_t *core){
    assert(core!=NULL);

    for(int i = 0; i<core->connections.length; i++){
        if(core->connections.data[i]!=NULL){
            xps_connection_destroy(core->connections.data[i]);
        }
    }
    vec_deinit(&(core->connections));

    for(int i = 0; i<core->listeners.length; i++){
        if(core->listeners.data[i]!=NULL){
            xps_listener_destroy(core->listeners.data[i]);
        }
    }
    vec_deinit(&(core->listeners));

    for(int i = 0; i<core->pipes.length; i++){
        if(core->pipes.data[i]!=NULL){
            xps_pipe_destroy(core->pipes.data[i]);
        }
    }
    vec_deinit(&(core->pipes));

    free(core);

    logger(LOG_DEBUG, "xps_core_destroy()", "core destroyed");
}

void xps_core_start(xps_core_t *core){
    assert(core!=NULL);

    logger(LOG_DEBUG, "xps_core_start()", "core starting...");

    for(int port = 8001; port<=8004; port++){
        xps_listener_create(core, "127.0.0.1", port);
        logger(LOG_INFO, "xps_core_create()", "Server listening on [http://0.0.0.0:%u](http://0.0.0.0:%u/)", port, port);
    }

    xps_loop_run(core->loop);
}