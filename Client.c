/* This will probably have to be multithreaded as well because it's going to have to recv and send */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#include <pthread.h>
#include <signal.h>

void ctrlCHandler(int);

int main(int argc, char **argv) {

  //Lets just set up our signal handler over here
  signal(SIGINT, ctrlCHandler);

  if(argc!=3) {
    printf("Usage: %s <IP> <port-num>\n", argv[0]);
    exit(1);
  }



}


void ctrlCHandler(int num) {
	printf("Exiting the client.\n");
	exit(1);
}
