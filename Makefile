#Makefile
CC = gcc
INCLUDE = /usr/lib
LIBS =
OBJS = 
CFLAGS = -ggdb3

all: server

server:
	$(CC) -o proxyserver proxy_server.c $(CFLAGS) $(LIBS)	

clean:
	rm -f proxyserver
