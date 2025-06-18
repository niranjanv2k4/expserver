#include "../xps.h"

void vec_filter_null(vec_void_t *vec);

loop_event_t *loop_event_create(u_int fd, void *ptr, xps_handler_t read_cb, xps_handler_t write_cb, xps_handler_t close_cb){
    assert(ptr!=NULL);

    loop_event_t *event = malloc(sizeof(loop_event_t));
    if(event==NULL){
        logger(LOG_ERROR, "loop_event_create()", "malloc() failed for 'event'");
        return NULL;
    }

    event->fd = fd;
    event->ptr = ptr;
    event->read_cb = read_cb;
    event->write_cb = write_cb;
    event->close_cb = close_cb;

    logger(LOG_DEBUG, "loop_event_create()", "event created for fd %d", fd);
    return event;
}

void loop_event_destroy(loop_event_t *event){
    assert(event!=NULL);

    free(event);

    logger(LOG_DEBUG, "loop_event_destroy()", "event destroyed");
}

xps_loop_t *xps_loop_create(xps_core_t *core){
    assert(core!=NULL);

    xps_loop_t *loop = malloc(sizeof(xps_loop_t));
    if(loop==NULL){
        logger(LOG_ERROR, "xps_loop_create()", "malloc() failed for 'loop'");
        return NULL;
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        logger(LOG_ERROR, "xps_loop_create()", "epoll_create1() failed");
        free(loop);
        return NULL;
    }

    loop->core = core;
    loop->epoll_fd = epoll_fd;
    loop->n_null_events = 0;

    memset(loop->epoll_events, 0, sizeof(loop->epoll_events));
    vec_init(&loop->events);

    logger(LOG_DEBUG, "xps_loop_create()", "loop created with epoll_fd %d", epoll_fd);

    return loop;
}

void xps_loop_destroy(xps_loop_t *loop){
    assert(loop!=NULL);

    for(int i = 0; i<loop->events.length; i++){
        loop_event_destroy(loop->events.data[i]);
    }

    vec_deinit(&loop->events);
    close(loop->epoll_fd);
    free(loop);
}

int xps_loop_attach(xps_loop_t *loop, u_int fd, int event_flags, void *ptr, xps_handler_t read_cb, xps_handler_t write_cb, xps_handler_t close_cb){
    assert(loop!=NULL);
    assert(ptr!=NULL);

    loop_event_t *loop_event = loop_event_create(fd, ptr, read_cb, write_cb, close_cb);
    if(loop_event==NULL){
        logger(LOG_ERROR, "xps_loop_attach()", "loop_event_create() failed");
        perror("Error message");
        return E_FAIL;
    }

    struct epoll_event event;
    event.events = event_flags;
    event.data.ptr = loop_event;

    if(epoll_ctl(loop->epoll_fd, EPOLL_CTL_ADD, fd, &event)==-1){
        logger(LOG_ERROR, "xps_loop_attach()", "epoll_ctl() failed");
        perror("Error message");
        free(loop_event);
        return E_FAIL;
    }

    vec_push(&loop->events, loop_event);

    logger(LOG_DEBUG, "xps_loop_attach()", "loop attached for fd %d", fd);

    return E_SUCCESS;
}

int xps_loop_detach(xps_loop_t *loop, u_int fd){
    assert(loop!=NULL);

    if(epoll_ctl(loop->epoll_fd, EPOLL_CTL_DEL, fd, NULL)==-1){
        logger(LOG_ERROR, "xps_loop_detach()", "epoll_ctl() failed");
        perror("Error message");
        return E_FAIL;
    };
    
    loop_event_t *temp;
    for(int i = 0; i<loop->events.length; i++){
        temp = loop->events.data[i];
        if(temp!=NULL && temp->fd==fd){
            loop->n_null_events += 1;
            loop_event_destroy(temp);
            loop->events.data[i] = NULL;
            break;
        }
    }

    logger(LOG_DEBUG, "xps_loop_detach()", "loop detached");
    return E_SUCCESS;
}

void xps_loop_run(xps_loop_t *loop){
    assert(loop!=NULL);

    logger(LOG_DEBUG, "xps_loop_run()", "starting to run loop");

    while(1){
        logger(LOG_DEBUG, "xps_loop_run()", "loop top");

        bool has_ready_pipes = handle_pipes(loop);

        int timeout = has_ready_pipes?0:-1;

        logger(LOG_DEBUG, "xps_loop_run()", "epoll waiting");
        int n_events = epoll_wait(loop->epoll_fd, loop->epoll_events, MAX_EPOLL_EVENTS, timeout);
        logger(LOG_DEBUG, "xps_loop_run()", "epoll wait over");

        if(n_events<0){
            logger(LOG_ERROR, "xps_loop_run()", "epoll_wait() error");
        }

        if(n_events>0){
            handle_epoll_events(loop, n_events);
        }

        filter_nulls(loop->core);
    }
}

void handle_epoll_events(xps_loop_t *loop, int n_events){
    assert(loop!=NULL);

    logger(LOG_DEBUG, "handle_epoll_events()", "handling %d events", n_events);

    for(int i = 0; i<n_events; i++){
        logger(LOG_DEBUG, "handle_epoll_events()", "handling event no. %d", i+1);

        struct epoll_event curr_epoll_event = loop->epoll_events[i];
        loop_event_t *curr_event = curr_epoll_event.data.ptr;

        int curr_event_idx = -1;
        for(int idx = 0; idx<loop->events.length; idx++){

            loop_event_t *loop_event = loop->events.data[idx];
            if(loop_event!=NULL && loop_event==curr_event){
                curr_event_idx = idx;
                break;
            }
        }

        if(curr_event_idx == -1){
            logger(LOG_ERROR, "handle_epoll_events()", "event not found. skipping...");
            continue;
        }

        if(curr_epoll_event.events & (EPOLLERR | EPOLLHUP)){
            logger(LOG_DEBUG, "handle_epoll_events()", "EVENT / close");
            if(curr_event!=NULL && curr_event->close_cb!=NULL){
                curr_event->close_cb(curr_event->ptr);
            }
            continue;
        }

        if(curr_epoll_event.events & EPOLLIN){
            logger(LOG_DEBUG, "handle_epoll_events()", "EVENT / read");
            if(curr_event!=NULL && curr_event->read_cb!=NULL){
                curr_event->read_cb(curr_event->ptr);
            }
            continue;
        }

        if(curr_epoll_event.events & EPOLLOUT){
            logger(LOG_DEBUG, "handle_epoll_events()", "EVENT / write");
            if(curr_event!=NULL && curr_event->write_cb!=NULL){
                curr_event->write_cb(curr_event->ptr);
            }
        }
    }
}

bool handle_pipes(xps_loop_t *loop){
    assert(loop!=NULL);

    for(int i = 0; i<loop->core->pipes.length; i++){
        xps_pipe_t *pipe = loop->core->pipes.data[i];
        if(pipe==NULL)
            continue;

        if(pipe->source!=NULL && pipe->source->ready && xps_pipe_is_writable(pipe)){
            pipe->source->handler_cb(pipe->source);
        }

        if(pipe->sink !=NULL && pipe->sink->ready && xps_pipe_is_readable(pipe)){
            pipe->sink->handler_cb(pipe->sink);
        }

        if(pipe->source!=NULL && pipe->sink==NULL){
            pipe->source->active = false;
            pipe->source->close_cb(pipe->source);
        }

        if(pipe->sink!=NULL && pipe->source == NULL && xps_pipe_is_readable(pipe)){
            pipe->sink->active = false;
            pipe->sink->close_cb(pipe->sink);
        }
    }

    for(int i = 0; i<loop->core->pipes.length; i++){
        xps_pipe_t *pipe = loop->core->pipes.data[i];
        if(pipe==NULL){
            logger(LOG_DEBUG, "handle_pipes", "pipe is null");
            continue;
        }

        if(pipe->source!=NULL && pipe->source->ready && xps_pipe_is_writable(pipe))
            return true;

        if(pipe->sink != NULL && pipe->sink->ready && xps_pipe_is_readable(pipe)){
            return true;
        }

        if(pipe->source!=NULL && pipe->sink==NULL){
            return true;
        }

        if(pipe->sink!=NULL && pipe->source == NULL && !xps_pipe_is_readable(pipe)){
            return true;
        }
    }
    return false;
}


void filter_nulls(xps_core_t *core){
    assert(core!=NULL);

    if(core->loop->n_null_events > DEFAULT_NULL_THRESH){
        vec_filter_null(&(core->loop->events));
        core->loop->n_null_events = 0;
    }

    if(core->n_null_listeners > DEFAULT_NULL_THRESH){
        vec_filter_null(&(core->listeners));
        core->n_null_listeners = 0;
    }

    if(core->n_null_connections > DEFAULT_NULL_THRESH){
        vec_filter_null(&(core->connections));
        core->n_null_connections = 0;
    }

    if(core->n_null_pipes > DEFAULT_NULL_THRESH){
        vec_filter_null(&(core->pipes));
        core->n_null_pipes = 0;
    }
}