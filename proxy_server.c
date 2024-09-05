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
pthread_mutex_t lock; // lock is used for locking cache
 
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


cache_store* find(char* url){

// Checks for url in the cache if found returns pointer to the respective cache element or else returns NULL
    cache_store* site=NULL;
	//sem_wait(&cache_lock);
    int temp_lock_val = pthread_mutex_lock(&lock);
	printf("Remove Cache Lock Acquired %d\n",temp_lock_val); 
    if(head!=NULL){
        site = head;
        while (site!=NULL)
        {
            if(!strcmp(site->url,url)){
				printf("LRU Time Track Before : %ld", site->lru_time_record);
                printf("\nurl found\n");
				// Updating the time_track
				site->lru_time_record = time(NULL);
				printf("LRU Time Track After : %ld", site->lru_time_record);
				break;
            }
            site=site->next;
        }       
    }
	else {
    printf("\nurl not found\n");
	}
	//sem_post(&cache_lock);
    temp_lock_val = pthread_mutex_unlock(&lock);
	printf("Remove Cache Lock Unlocked %d\n",temp_lock_val); 
    return site;
}




void remove_cache_store(){
    // If cache is not empty searches for the node which has the least lru_time_record and deletes it
    cache_store * p ;  	// cache_store Pointer (Prev. Pointer)
	cache_store * q ;		// cache_store Pointer (Next Pointer)
	cache_store * temp;	// Cache element to remove
    //sem_wait(&cache_lock);
    int temp_lock_val = pthread_mutex_lock(&lock);
	printf("Remove Cache Lock Acquired %d\n",temp_lock_val); 
	if( head != NULL) { // Cache != empty
		for (q = head, p = head, temp =head ; q -> next != NULL; 
			q = q -> next) { // Iterate through entire cache and search for oldest time track
			if(( (q -> next) -> lru_time_record) < (temp -> lru_time_record)) {
				temp = q -> next;
				p = q;
			}
		}
		if(temp == head) { 
			head = head -> next; /*Handle the base case*/
		} else {
			p->next = temp->next;	
		}
		cache_size = cache_size - (temp -> len) - sizeof(cache_store) - 
		strlen(temp -> url) - 1;     //updating the cache size
		free(temp->data);     		
		free(temp->url); // Free the removed element 
		free(temp);
	} 
	//sem_post(&cache_lock);
    temp_lock_val = pthread_mutex_unlock(&lock);
	printf("Remove Cache Lock Unlocked %d\n",temp_lock_val); 
}





int add_cache_store(char* data,int size,char* url){
    // Adds element to the cache
	// sem_wait(&cache_lock);
    int temp_lock_val = pthread_mutex_lock(&lock);
	printf("Add Cache Lock Acquired %d\n", temp_lock_val);
    int element_size=size+1+strlen(url)+sizeof(cache_store); // Size of the new element which will be added to the cache
    if(element_size>MAX_ELEMENT_SIZE){
		//sem_post(&cache_lock);
        // If element size is greater than MAX_ELEMENT_SIZE we don't add the element to the cache
        temp_lock_val = pthread_mutex_unlock(&lock);
		printf("Add Cache Lock Unlocked %d\n", temp_lock_val);
		// free(data);
		// printf("--\n");
		// free(url);
        return 0;
    }
    else
    {   while(cache_size+element_size>MAX_SIZE){
            // We keep removing elements from cache until we get enough space to add the element
            remove_cache_store();
        }
        cache_store* element = (cache_store*) malloc(sizeof(cache_store)); // Allocating memory for the new cache element
        element->data= (char*)malloc(size+1); // Allocating memory for the response to be stored in the cache element
		strcpy(element->data,data); 
        element -> url = (char*)malloc(1+( strlen( url )*sizeof(char)  )); // Allocating memory for the request to be stored in the cache element (as a key)
		strcpy( element -> url, url );
		element->lru_time_record=time(NULL);    // Updating the time_track
        element->next=head; 
        element->len=size;
        head=element;
        cache_size+=element_size;
        temp_lock_val = pthread_mutex_unlock(&lock);
		printf("Add Cache Lock Unlocked %d\n", temp_lock_val);
		//sem_post(&cache_lock);
		// free(data);
		// printf("--\n");
		// free(url);
        return 1;
    }
    return 0;
}








int sendErrorMessage(int socket, int status_code)
{
	char str[1024];
	char currentTime[50];
	time_t now = time(0);

	struct tm data = *gmtime(&now);
	strftime(currentTime,sizeof(currentTime),"%a, %d %b %Y %H:%M:%S %Z", &data);

	switch(status_code)
	{
		case 400: snprintf(str, sizeof(str), "HTTP/1.1 400 Bad Request\r\nContent-Length: 95\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>400 Bad Request</TITLE></HEAD>\n<BODY><H1>400 Bad Rqeuest</H1>\n</BODY></HTML>", currentTime);
				  printf("400 Bad Request\n");
				  send(socket, str, strlen(str), 0);
				  break;

		case 403: snprintf(str, sizeof(str), "HTTP/1.1 403 Forbidden\r\nContent-Length: 112\r\nContent-Type: text/html\r\nConnection: keep-alive\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>403 Forbidden</TITLE></HEAD>\n<BODY><H1>403 Forbidden</H1><br>Permission Denied\n</BODY></HTML>", currentTime);
				  printf("403 Forbidden\n");
				  send(socket, str, strlen(str), 0);
				  break;

		case 404: snprintf(str, sizeof(str), "HTTP/1.1 404 Not Found\r\nContent-Length: 91\r\nContent-Type: text/html\r\nConnection: keep-alive\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD>\n<BODY><H1>404 Not Found</H1>\n</BODY></HTML>", currentTime);
				  printf("404 Not Found\n");
				  send(socket, str, strlen(str), 0);
				  break;

		case 500: snprintf(str, sizeof(str), "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 115\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>500 Internal Server Error</TITLE></HEAD>\n<BODY><H1>500 Internal Server Error</H1>\n</BODY></HTML>", currentTime);
				  //printf("500 Internal Server Error\n");
				  send(socket, str, strlen(str), 0);
				  break;

		case 501: snprintf(str, sizeof(str), "HTTP/1.1 501 Not Implemented\r\nContent-Length: 103\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>404 Not Implemented</TITLE></HEAD>\n<BODY><H1>501 Not Implemented</H1>\n</BODY></HTML>", currentTime);
				  printf("501 Not Implemented\n");
				  send(socket, str, strlen(str), 0);
				  break;

		case 505: snprintf(str, sizeof(str), "HTTP/1.1 505 HTTP Version Not Supported\r\nContent-Length: 125\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>505 HTTP Version Not Supported</TITLE></HEAD>\n<BODY><H1>505 HTTP Version Not Supported</H1>\n</BODY></HTML>", currentTime);
				  printf("505 HTTP Version Not Supported\n");
				  send(socket, str, strlen(str), 0);
				  break;

		default:  return -1;

	}
	return 1;
}


int connectRemoteServer(char* host_addr, int port_num)
{
	// Creating Socket for remote server ---------------------------

	int remoteSocket = socket(AF_INET, SOCK_STREAM, 0);

	if( remoteSocket < 0)
	{
		printf("Error in Creating Socket.\n");
		return -1;
	}
	
	// Get host by the name or ip address provided

	struct hostent *host = gethostbyname(host_addr);	
	if(host == NULL)
	{
		fprintf(stderr, "No such host exists.\n");	
		return -1;
	}

	// inserts ip address and port number of host in struct `server_addr`
	struct sockaddr_in server_addr;

	bzero((char*)&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port_num);

	bcopy((char *)host->h_addr,(char *)&server_addr.sin_addr.s_addr,host->h_length);

	// Connect to Remote server ----------------------------------------------------

	if( connect(remoteSocket, (struct sockaddr*)&server_addr, (socklen_t)sizeof(server_addr)) < 0 )
	{
		fprintf(stderr, "Error in connecting !\n"); 
		return -1;
	}
	// free(host_addr);
	return remoteSocket;
}


int handle_request(int clientSocket, ParsedRequest *request, char *tempReq)
{
	char *buf = (char*)malloc(sizeof(char)*MAX_BYTES);
	strcpy(buf, "GET ");
	strcat(buf, request->path);
	strcat(buf, " ");
	strcat(buf, request->version);
	strcat(buf, "\r\n");

	size_t len = strlen(buf);

	if (ParsedHeader_set(request, "Connection", "close") < 0){
		printf("set header key not work\n");
	}

	if(ParsedHeader_get(request, "Host") == NULL)
	{
		if(ParsedHeader_set(request, "Host", request->host) < 0){
			printf("Set \"Host\" header key not working\n");
		}
	}

	if (ParsedRequest_unparse_headers(request, buf + len, (size_t)MAX_BYTES - len) < 0) {
		printf("unparse failed\n");
		//return -1;				// If this happens Still try to send request without header
	}

	int server_port = 80;				// Default Remote Server Port
	if(request->port != NULL)
		server_port = atoi(request->port);

	int remoteSocketID = connectRemoteServer(request->host, server_port);

	if(remoteSocketID < 0)
		return -1;

	int bytes_send = send(remoteSocketID, buf, strlen(buf), 0);

	bzero(buf, MAX_BYTES);

	bytes_send = recv(remoteSocketID, buf, MAX_BYTES-1, 0);
	char *temp_buffer = (char*)malloc(sizeof(char)*MAX_BYTES); //temp buffer
	int temp_buffer_size = MAX_BYTES;
	int temp_buffer_index = 0;

	while(bytes_send > 0)
	{
		bytes_send = send(clientSocket, buf, bytes_send, 0);
		
		for(int i=0;i<bytes_send/sizeof(char);i++){
			temp_buffer[temp_buffer_index] = buf[i];
			// printf("%c",buf[i]); // Response Printing
			temp_buffer_index++;
		}
		temp_buffer_size += MAX_BYTES;
		temp_buffer=(char*)realloc(temp_buffer,temp_buffer_size);

		if(bytes_send < 0)
		{
			perror("Error in sending data to client socket.\n");
			break;
		}
		bzero(buf, MAX_BYTES);

		bytes_send = recv(remoteSocketID, buf, MAX_BYTES-1, 0);

	} 
	temp_buffer[temp_buffer_index]='\0';
	free(buf);
	add_cache_store(temp_buffer, strlen(temp_buffer), tempReq);
	printf("Done\n");
	free(temp_buffer);
	
	
 	close(remoteSocketID);
	return 0;
}


int checkHTTPversion(char *msg)
{
	int version = -1;

	if(strncmp(msg, "HTTP/1.1", 8) == 0)
	{
		version = 1;
	}
	else if(strncmp(msg, "HTTP/1.0", 8) == 0)			
	{
		version = 1;										// Handling this similar to version 1.1
	}
	else
		version = -1;

	return version;
}

void* thread_fn(void* socketNew)
{
	sem_wait(&seamaphore); 
	int p;
	sem_getvalue(&seamaphore,&p);
	printf("semaphore value:%d\n",p);
    int* t= (int*)(socketNew);
	int socket=*t;           // Socket is socket descriptor of the connected Client
	int bytes_send_client,len;	  // Bytes Transferred

	
	char *buffer = (char*)calloc(MAX_BYTES,sizeof(char));	// Creating buffer of 4kb for a client
	
	
	bzero(buffer, MAX_BYTES);								// Making buffer zero
	bytes_send_client = recv(socket, buffer, MAX_BYTES, 0); // Receiving the Request of client by proxy server
	
	while(bytes_send_client > 0)
	{
		len = strlen(buffer);
        //loop until u find "\r\n\r\n" in the buffer
		if(strstr(buffer, "\r\n\r\n") == NULL)
		{	
			bytes_send_client = recv(socket, buffer + len, MAX_BYTES - len, 0);
		}
		else{
			break;
		}
	}

	// printf("--------------------------------------------\n");
	// printf("%s\n",buffer);
	// printf("----------------------%d----------------------\n",strlen(buffer));
	
	char *tempReq = (char*)malloc(strlen(buffer)*sizeof(char)+1);
    //tempReq, buffer both store the http request sent by client
	for (int i = 0; i < strlen(buffer); i++)
	{
		tempReq[i] = buffer[i];
	}
	
	//checking for the request in cache 
	struct cache_store* temp = find(tempReq);

	if( temp != NULL){
        //request found in cache, so sending the response to client from proxy's cache
		int size=temp->len/sizeof(char);
		int pos=0;
		char response[MAX_BYTES];
		while(pos<size){
			bzero(response,MAX_BYTES);
			for(int i=0;i<MAX_BYTES;i++){
				response[i]=temp->data[pos];
				pos++;
			}
			send(socket,response,MAX_BYTES,0);
		}
		printf("Data retrived from the Cache\n\n");
		printf("%s\n\n",response);
		// close(socketNew);
		// sem_post(&seamaphore);
		// return NULL;
	}
	
	
	else if(bytes_send_client > 0)
	{
		len = strlen(buffer); 
		//Parsing the request
		ParsedRequest* request = ParsedRequest_create();
		
        //ParsedRequest_parse returns 0 on success and -1 on failure.On success it stores parsed request in
        // the request
		if (ParsedRequest_parse(request, buffer, len) < 0) 
		{
		   	printf("Parsing failed\n");
		}
		else
		{	
			bzero(buffer, MAX_BYTES);
			if(!strcmp(request->method,"GET"))							
			{
                
				if( request->host && request->path && (checkHTTPversion(request->version) == 1) )
				{
					bytes_send_client = handle_request(socket, request, tempReq);		// Handle GET request
					if(bytes_send_client == -1)
					{	
						sendErrorMessage(socket, 500);
					}

				}
				else
					sendErrorMessage(socket, 500);			// 500 Internal Error

			}
            else
            {
                printf("This code doesn't support any method other than GET\n");
            }
    
		}
        //freeing up the request pointer
		ParsedRequest_destroy(request);

	}

	else if( bytes_send_client < 0)
	{
		perror("Error in receiving from client.\n");
	}
	else if(bytes_send_client == 0)
	{
		printf("Client disconnected!\n");
	}

	shutdown(socket, SHUT_RDWR);
	close(socket);
	free(buffer);
	sem_post(&seamaphore);	
	
	sem_getvalue(&seamaphore,&p);
	printf("Semaphore post value:%d\n",p);
	free(tempReq);
	return NULL;
}


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