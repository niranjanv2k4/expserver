#include<arpa/inet.h>
#include<netdb.h>
#include<netinet/in.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<unistd.h>
#include<pthread.h>

#define PORT 8080
#define BUFF_SIZE 10000

typedef struct {
    char message[BUFF_SIZE];    
    struct sockaddr_in client_addr;
    int sockfd;
    socklen_t addr_len;
} client_data_t;


void strrev(char *s){
    int n = strlen(s);
    for(int i = 0; i<n/2; i++){
        char temp = s[i];
        s[i]=s[n-1-i];
        s[n-1-i]=temp;
    }
}

void* handle_client(void* arg){
    client_data_t* data = (client_data_t *)arg;

    printf("[CLIENT MESSAGE] %s\n", data->message);
    strrev(data->message);

    sendto(data->sockfd, data->message, strlen(data->message), 0, (struct sockaddr *)&(data->client_addr), data->addr_len);

    free(data);
    pthread_exit(NULL);
}


int main(){

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server_addr;
    char buffer[BUFF_SIZE];
    memset(buffer, 0, BUFF_SIZE);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    printf("[INFO] server listening on port %d\n",PORT);

    while(1){
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        ssize_t read_n = recvfrom(sockfd, buffer, BUFF_SIZE, 0, (struct sockaddr *)&(client_addr), &addr_len);
        buffer[read_n] ='\0';

        client_data_t *data = (client_data_t *)malloc(sizeof(client_data_t));
        strcpy(data->message, buffer);
        data->client_addr = client_addr;
        data->sockfd = sockfd;
        data->addr_len = addr_len;
        
        pthread_t thread_id;
        if(pthread_create(&thread_id, NULL, handle_client, (void *)data)!=0){
            printf("Failed to create thread\n");
            free(data);
        };
        pthread_detach(thread_id);
    }

    return 0;
}