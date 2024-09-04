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
int port_num = 8080;
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
    struct sochaddr server_addr, client_addr;
    sem_init(&semaphore,0,  MAX_CLIENTS);

    pthread_mutex_init(&lock, NULL);
    if(argv == 2){
        port_number = atoi(argv[1]);
    }
    else{
        printf("Arguments provided are less than required\nTry Again with proper arguments!!\n");
        exit(1);
    }

    
}