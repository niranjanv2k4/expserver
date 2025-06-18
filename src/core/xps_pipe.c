#include "../xps.h"

xps_pipe_t *xps_pipe_create(xps_core_t *core, size_t buff_thresh, xps_pipe_source_t *source, xps_pipe_sink_t *sink){
    assert(core!=NULL);
    assert(buff_thresh>0);
    assert(source!=NULL);
    assert(sink!=NULL);

    xps_pipe_t *pipe = malloc(sizeof(xps_pipe_t));
    if(pipe==NULL){
        logger(LOG_ERROR, "xps_pipe_create()", "malloc() failed for 'pipe");
        perror("Error message");
        return NULL;
    }

    pipe->buff_list = xps_buffer_list_create();
    if(pipe->buff_list==NULL){
        logger(LOG_ERROR, "xps_pipe_create()", "buffer_list_create() failed");
        free(pipe);
        return NULL;
    }

    pipe->core = core;
    pipe->source = NULL;
    pipe->sink = NULL;
    pipe->buff_thresh = buff_thresh;                    //CHECK

    vec_push(&(core->pipes), pipe);

    source->pipe = pipe;
    sink->pipe = pipe;

    source->active = true;
    sink->active = true;

    logger(LOG_DEBUG, "xps_pipe_create()", "pipe created");

    return pipe;
}

void xps_pipe_destroy(xps_pipe_t *pipe){
    assert(pipe!=NULL);

    for(int i = 0; i<pipe->core->pipes.length; i++){
        xps_pipe_t *curr = pipe->core->pipes.data[i];
        if(curr==pipe){
            pipe->core->pipes.data[i] = NULL;
            pipe->core->n_null_pipes++;
            break;
        }
    }

    xps_buffer_list_destroy(pipe->buff_list);
    free(pipe);
    logger(LOG_DEBUG, "xps_pipe_destroy()", "pipe destroyed");
}

bool xps_pipe_is_readable(xps_pipe_t *pipe){
    return pipe->buff_list->len>0;
}

bool xps_pipe_is_writable(xps_pipe_t *pipe){
    return pipe->buff_list->len<pipe->buff_thresh;
}

int xps_pipe_attach_source(xps_pipe_t *pipe, xps_pipe_source_t *source){
    assert(pipe!=NULL);
    assert(source!=NULL);

    if(pipe->source!=NULL)
        return E_FAIL;
    
    pipe->source = source;
    source->pipe = pipe;

    return E_SUCCESS;
}

int xps_pipe_attach_sink(xps_pipe_t *pipe, xps_pipe_sink_t *sink){
    assert(pipe!=NULL);
    assert(sink!=NULL);

    if(pipe->sink != NULL)  
        return E_FAIL;

    pipe->sink = sink;
    sink->pipe = pipe;

    return E_SUCCESS;    
}

int xps_pipe_detach_source(xps_pipe_t *pipe){
    assert(pipe != NULL);

    if(pipe->source == NULL)
        return E_FAIL;

    pipe->source->pipe = NULL;
    pipe->source = NULL;

    return E_SUCCESS;
}

int xps_pipe_detach_sink(xps_pipe_t *pipe){
    assert(pipe != NULL);

    if(pipe->sink == NULL)
        return E_FAIL;

    pipe->sink->pipe = NULL;
    pipe->sink = NULL;

    return E_SUCCESS;
}

xps_pipe_source_t *xps_pipe_source_create(void *ptr, xps_handler_t handler_cb, xps_handler_t close_cb){
    assert(ptr!=NULL);
    assert(handler_cb!=NULL);
    assert(close_cb!=NULL);

    xps_pipe_source_t *source = malloc(sizeof(xps_pipe_source_t));
    if(source==NULL){
        logger(LOG_ERROR, "xps_pipe_source_create()", "malloc() failed for 'source'");
        perror("Error message");
        return NULL;
    }

    source->pipe = NULL;
    source->active = false;
    source->ready = false;

    source->ptr = ptr;
    source->close_cb = close_cb;
    source->handler_cb = handler_cb;

    logger(LOG_DEBUG, "xps_pipe_source_create()", "create pipe_source");

    return source;
}

void xps_pipe_source_destroy(xps_pipe_source_t *source){
    assert(source != NULL);

    if(source->pipe!=NULL){
        if(source->pipe->source == source){
            source->pipe->source = NULL;
        }
        source->pipe = NULL;
    }

    free(source);

    logger(LOG_DEBUG, "xps_pipe_source_destroy()", "source destroyed");
}

int xps_pipe_source_write(xps_pipe_source_t *source, xps_buffer_t *buff){
    assert(source!=NULL);
    assert(buff!=NULL);

    if(source->pipe==NULL){
        logger(LOG_ERROR, "xps_pipe_source_write()", "source is not attached to a pipe");
        return E_FAIL;
    }

    if(!xps_pipe_is_writable(source->pipe)){
        logger(LOG_ERROR, "xps_pipe_source_attach()", "pipe is not writable");
        return E_FAIL;
    }

    xps_buffer_t *dup_buff = xps_buffer_duplicate(buff);
    if(dup_buff==NULL){
        logger(LOG_ERROR, "xps_pipe_source_write()", "xps_buffer_duplicate() failed");
        return E_FAIL;
    }

    xps_buffer_list_append(source->pipe->buff_list, dup_buff);
    return E_SUCCESS;
}

xps_pipe_sink_t *xps_pipe_sink_create(void *ptr, xps_handler_t handler_cb, xps_handler_t close_cb){
    assert(ptr!=NULL);
    assert(handler_cb!=NULL);
    assert(close_cb!=NULL);

    xps_pipe_sink_t *sink = malloc(sizeof(xps_pipe_sink_t));
    if(sink==NULL){
        logger(LOG_ERROR, "xps_pipe_sink_create()", "malloc() failed for 'sink'");
        perror("Error message");
        return NULL;
    }

    sink->pipe = NULL;
    sink->active = false;
    sink->ready = false;

    sink->ptr = ptr;
    sink->close_cb = close_cb;
    sink->handler_cb = handler_cb;

    logger(LOG_DEBUG, "xps_pipe_sink_create()", "create pipe_sink");

    return sink;
}

void xps_pipe_sink_destroy(xps_pipe_sink_t *sink){
    assert(sink != NULL);

    if(sink->pipe!=NULL){
        if(sink->pipe->sink == sink){
            sink->pipe->sink = NULL;
        }
        sink->pipe = NULL;
    }

    free(sink);

    logger(LOG_DEBUG, "xps_pipe_sink_destroy()", "sink destroyed");
}


xps_buffer_t *xps_pipe_sink_read(xps_pipe_sink_t *sink, size_t len){
    assert(sink!=NULL);
    assert(len>0);

    if(sink->pipe==NULL){
        logger(LOG_ERROR, "xps_pipe_sink_read()", "sink is not attached to a pipe");
        return NULL;
    }

    if(sink->pipe->buff_list->len<len){
        logger(LOG_ERROR, "xps_pipe_sink_read()", "requested length more than available");
        return NULL;
    }

    xps_buffer_t *buff = xps_buffer_list_read(sink->pipe->buff_list, len);
    if(buff==NULL){
        logger(LOG_ERROR, "xps_pipe_sink_read()", "xps_buffer_list_read() failed");
        return NULL;
    }

    return buff;
}

int xps_pipe_sink_clear(xps_pipe_sink_t *sink, size_t len){
    assert(sink!=NULL);
    assert(len>0);

    if(sink->pipe==NULL){
        logger(LOG_ERROR, "xps_pipe_sink_clear()", "sink is not attached to a pipe");
        return E_FAIL;
    }

    if(sink->pipe->buff_list->len<len){
        logger(LOG_ERROR, "xps_pipe_sink_clear()", "requested length more than available");
        return E_FAIL;
    }

    if(xps_buffer_list_clear(sink->pipe->buff_list, len)!=E_SUCCESS){
        logger(LOG_ERROR, "xps_pipe_sink_clear()", "xps_buffer_list_clear() failed");
        return E_FAIL;
    }

    return E_SUCCESS;
}
