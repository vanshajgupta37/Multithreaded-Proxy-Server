CC=g++
CFLAGS= -g -Wall 

all: proxy

proxy: proxy_server.c
	$(CC) $(CFLAGS) -o http_parser.o -c http_parser.c -lpthread
	$(CC) $(CFLAGS) -o proxy.o -c proxy_server.c -lpthread
	$(CC) $(CFLAGS) -o proxy http_parser.o proxy.o -lpthread

clean:
	rm -f proxy *.o

tar:
	tar -cvzf ass1.tgz proxy_server.c README Makefile http_parser.c http_parser.h