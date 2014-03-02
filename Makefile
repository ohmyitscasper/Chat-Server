CC=gcc
CFLAGS= -Wall
LDFLAGS= -lpthread

default:all

Server: Server.c LinkedList.c

Client: Client.c

.PHONY: all
all: Server Client

.PHONY: clean
clean:
	rm Server Client
