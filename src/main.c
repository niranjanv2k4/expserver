#include "xps.h"

xps_core_t *core;

void sigint_handler(int signum);

int main(){

    signal(SIGINT, sigint_handler);
    // printf("main->");
    core = xps_core_create();

    xps_core_start(core);
}

void sigint_handler(int signum){
    logger(LOG_WARNING, "sigint_handler()", "SIGINT recieved");
    xps_core_destroy(core);
    exit(EXIT_SUCCESS);
}