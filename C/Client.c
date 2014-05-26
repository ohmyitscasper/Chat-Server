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

#define RECVBUFSIZE 200
#define SENDBUFSIZE 100


int sockFd;
int loggedOn = 0;
pthread_t network_thread;

void Die(char *);
void ctrlCHandler(int);
void *sendFunc(void *);

int main(int argc, char **argv) {

  //Lets just set up our signal handler over here
  signal(SIGINT, ctrlCHandler);

  if(argc!=3) {
    printf("Usage: %s <IP> <port-num>\n", argv[0]);
    exit(1);
  }

  int port; 
  struct sockaddr_in serv_addr;
  char recvBuf[RECVBUFSIZE];

  //Now we have the port number
  port = atoi(argv[2]);
  sockFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  memset(&serv_addr, 0, sizeof(struct sockaddr_in));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  serv_addr.sin_addr.s_addr = inet_addr(argv[1]);

 //Connection of the client to the socket 
  if (connect(sockFd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))<0) 
  	Die("connect() failed");

  pthread_create(&network_thread, NULL, sendFunc, NULL);


  memset(recvBuf, 0, RECVBUFSIZE);
  while(1) {
  	read(sockFd, &recvBuf, RECVBUFSIZE);
  	printf("%s", recvBuf);
  	fflush(stdout);
 	
 	//If we see this. lets just log out.
 	if(!strncmp(recvBuf, "Logging off", 11)) {
      break;
    }
    memset(recvBuf, 0, RECVBUFSIZE);
  }

  printf("\nExitting the client.\n");
  close(sockFd);
  //Cancel/join the recv thread.
  pthread_cancel(network_thread);
  return 0;
}

void *sendFunc(void *arg) {
  char sendBuf[SENDBUFSIZE];
  int sendLen;

  //This loop handles all communication with the user. 
  while(1) {
  	//Ask user for input
  	memset(sendBuf, 0, SENDBUFSIZE);
  	fgets(sendBuf, SENDBUFSIZE, stdin);
  	sendLen = strlen(sendBuf);
  	write(sockFd, sendBuf, sendLen);
  }
  return NULL;
}

void Die(char *message) {
  printf("%s\n", message);
  exit(1);
}


void ctrlCHandler(int num) {
  printf("Exiting the client.\n");
  close(sockFd);
  exit(1);
}
