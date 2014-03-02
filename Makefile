CC=/usr/local/bin/gcc-4.6
CFLAGS= -Wall
LDFLAGS= -lpthread

default:all

Server: Server.c LinkedList.c LinkedList.h

Client: Client.c

.PHONY: all
all: Server Client

.PHONY: clean
clean:
	rm Server Client
