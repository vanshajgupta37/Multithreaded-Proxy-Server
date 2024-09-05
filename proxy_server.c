#include "http_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <semaphore.h>
#include <pthread.h>


#define MAX_CLIENTS 20

pthread_t t_id[MAX_CLIENTS];
int port_number = 8080;
int proxy_socket_ID;
sem_t semaphore;
pthread_mutex_t lock;
 
typedef struct cache_store cache_store;

struct cache_store{
    char* data;
    int len;
    char* url;
    time_t lru_time_record;
    cache_store* next;
};


cache_store* find(char* url);
int add_cache_store(char* data, int size, char* url);
void remove_cache_store();

cache_store* head;
int cache_size;

int main(int argc, char* argv[]){
    int client_socketID, client_len;
    struct sockaddr_in server_addr, client_addr;
    sem_init(&semaphore,0,  MAX_CLIENTS);

    pthread_mutex_init(&lock, NULL);  // using NULL because in C their exists garbage value by default
    if(argv == 2){
        port_number = atoi(argv[1]);
        // ./proxy 8282 (for example)
        // so, here port_number = 8282 (for this particular case only)
    }
    else{
        printf("Arguments provided are less than required\nTry Again with proper arguments!!\n");
        exit(1);
    }

    printf("Starting Proxy Server at Port: %d\n", port_number);
    proxy_socket_ID = socket(AF_INET, SOCK_STREAM, 0);
    if(proxy_socket_ID<0){
        perror("Failed to create a Socket\n");
        exit(1);
    }
    
    int reuse = 1;
    if(setsocketopt(proxy_socket_ID, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse))<0){
        perror("setSockOpt failed\n");
    }

    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number);
    server.sin_addr.s_addr  INADDR_ANY;
    if(bind(proxy_socket_ID, (struct sockaddr*)&server_addr, sizeof(server_addr)<0)){
        perror("Port is not available, Kindly try another Port Number!!\n");
        exit(1);
    }
    prtinf("Binding on port %d\n", port_num);
    int listen_status = listen(proxy_socket_ID, MAX_CLIENTS);
    if(listen_status<0){
        perror("Error in listening\n");
        exit(1);
    }

    int i = 0;
    int Connected_SocketID[MAX_CLIENTS];

    while(1){
        bzero((char *)&client_addr, sizeof(client_addr));
        client_len = sizeof(client_addr);
        client_socketID = accept(proxy_socket_ID, (struct sockaddr *)&client_addr, (socklen_t*)&client_len);
        if(client_socketID<0){
            printf("Not able to connect");
            exit(1);
        }
        else{
            Connected_SocketID[i] = client_socketID;
        }

        struct sockaddr_in * client_pt = (struct sockaddr_in *)&client_addr;
        struct in_addr ip_addr = client_pt -> sin_addr;
        char str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &ip_addr, str, INET6_ADDRSTRLEN);
        printf("Client is connected with port number %d and ip address is %s\n", ntohs(client_addr.sin_port), str);

        pthread_create(&t_id[i], NULL, thread_fn, (void *)&Connected_SocketID[i]);
        i++;
    }
    
    close(proxy_socket_ID);
    return 0;

}