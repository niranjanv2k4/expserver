#include"./xps.h"

int epoll_fd;
struct epoll_event events[MAX_EPOLL_EVENTS];
vec_void_t listeners;
vec_void_t connections;

int xps_loop_create(){
    return epoll_create1(0);
}

void xps_loop_attach(int epoll_fd, int fd, int events){
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}

void xps_loop_detach(int epoll_fd, int fd){
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
}

void xps_loop_run(int epoll_fd){
    while(1){
        logger(LOG_DEBUG, "xps_loop_run()", "epoll wait");
        int n_read_fds = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, -1);
        logger(LOG_DEBUG, "xps_loop_run()", "epoll wait over");

        for(int idx = 0; idx<n_read_fds; idx++){
            int curr_fd = events[idx].data.fd;

            xps_listener_t* listener = NULL;
            for(int i = 0; i<listeners.length; i++){
                xps_listener_t *curr = listeners.data[i];
                if(curr!=NULL && curr->sock_fd==curr_fd){
                    listener = curr;
                    break;
                }
            }

            if(listener){
                xps_listener_connection_handler(listener);
                continue;
            }

            xps_connection_t *connection = NULL;
            for(int i = 0; i<connections.length; i++){
                xps_connection_t *curr = connections.data[i];
                if(curr!=NULL && curr->sock_fd == curr_fd){
                    connection = curr;
                    break;
                }
            }

            if(connection){
                xps_connection_read_handler(connection);
            }
        }
    }
}


int main(){

    epoll_fd = xps_loop_create();

    vec_init(&listeners);
    vec_init(&connections);

    const char *host = "127.0.0.1";
    for(int port = 8001; port<=8004; port++){
        xps_listener_create(epoll_fd, host, port);
        logger(LOG_INFO, "main()", "Server listening to port %u", port);
    }

    xps_loop_run(epoll_fd);
}