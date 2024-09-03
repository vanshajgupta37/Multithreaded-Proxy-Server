#include "http_parser.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#define MAX_CLIENTS 20

pthread_t t_id[MAX_CLIENTS];
int port_num = 8080;
int proxy_socket_ID;

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
