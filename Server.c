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

#include "Server.h"
#include "LinkedList.h"

#define FILENAME "user_pass.txt"
#define FILELINES 9
#define MAXRECVBUF 1000
#define BCASTSIZE 200
#define MSGSIZE 200

/* Just uselessly made character constants for the commands. */
#define WHOELSE   7
#define WHOLASTHR 9
#define BROADCAST 9
#define MESSAGE   7
#define BLOCK     5
#define UNBLOCK   7
#define LOGOUT    6


#define BLOCK_TIME 60       //THIS QUANTITY IS IN SECS. IF YOU EDIT IT PLEASE LEAVE IT IN SECS AS I USE IT AS SECS.
#define LAST_HOUR  0.02     //THIS QUANTITY IS IN HOURS. IF YOU EDIT IT PLEASE LEAVE IT IN HOURS AS I USE IT AS HOURS.
                            //  you can just do 0.5 or whatever you want.
#define TIME_OUT   30       //THIS QUANTITY IS IN MINS. IF YOU EDIT IT PLEASE LEAVE IT IN MINS AS I USE IT AS MINS.

#define LAST_HR_SECS (LAST_HOUR*3600)
#define TIME_OUT_SECS (TIME_OUT*60)

//Some global variables to be used
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
char usernames[FILELINES][MAXCHARS];		
char passwords[FILELINES][MAXCHARS]; 	
List *allUsers;
List *threads;
List *blocks;
int servFd;


//Function declarations
void Die(char *);
void *threadFn(void *);
void broadcast(void *, char *, int);
int checkUserName(char *);
int checkPassword(char *, int);
void ctrlCHandler(int);
void threadCleanup(void *);

int main(int argc, char **argv) {

  //Lets just set up our signal handler over here
  signal(SIGINT, ctrlCHandler);

  if(argc!=2) {
    printf("Usage: %s <port-num>\n", argv[0]);
    exit(1);
  }

  int a;
  int cliFd;
  int portNum;
  int cliLen;

  FILE *fp;

  //The structures that hold all of the data 
  struct sockaddr_in serv_addr;
  struct sockaddr_in cli_addr;
  pthread_t *thread;
  Request *request;
  UserData *user;


  //Allocating the three lists
  allUsers = malloc(sizeof(List));
  threads = malloc(sizeof(List));
  blocks = malloc(sizeof(List));

  //Initializing variables as well as the lists
  cliLen = sizeof(cli_addr);
  portNum = atoi(argv[1]);
  initialize(allUsers);
  initialize(threads);
  initialize(blocks);

  //setting the uname and pw arrays to 0
  int y,z;
  for(z = 0; z<FILELINES; z++) {
    for(y = 0; y < MAXCHARS; y++) { 
      usernames[z][y] = 0;
      passwords[z][y] = 0;
    }
  }

  fp = fopen(FILENAME, "r");
  if (fp == NULL) {
    Die("fopen() failed");
  }

  //Reading from the input file
  for(a = 0; a < FILELINES; a++) {
    fscanf(fp, "%s %s", usernames[a], passwords[a]);
    user = malloc(sizeof(UserData));
    memset(user, 0, sizeof(UserData));
    memcpy(user->userName, usernames[a], strlen(usernames[a]));
    printf("length: %d\n", strlen(usernames[a]));
    insert(allUsers, user, &mutex);
  }

  //Don't need to read/write to the file anymore so we can close it.
  fclose(fp);

  //Filling out the sockaddr_in for the server
  servFd = socket(AF_INET, SOCK_STREAM, 0);
  memset(&serv_addr, 0, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(portNum);

  if(bind(servFd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
    Die("bind() failed");

  if(listen(servFd, MAXUSERS) < 0) 
    Die("listen() failed");


  printf("Server started. Listening on port %d\n", portNum);

  while(1) {
    //Accept a connection
    if((cliFd = accept(servFd, (struct sockaddr *)&cli_addr, (socklen_t *)&cliLen)) < 0) 
      Die("accept() failed");

    //Allocate space for the request as well as the thread
    request = malloc(sizeof(Request));
    thread = malloc(sizeof(pthread_t));

    request->IP = cli_addr.sin_addr.s_addr;
    request->sockNum = cliFd;

    printf("Received request from ip: %lu\tDispatching new thread with descriptor: %d\n", request->IP, request->sockNum);
    pthread_create(thread, NULL, &threadFn, request);

    //Inserting this thread into the list of threads.
    insert(threads, (void *)thread, &mutex);
  }


  close(servFd);
}


/* Something to consider to be futuristic.
 * Might help with logging out

void threadLogIn(unsigned long myIP, mySock, )
*/


/* Handles all of the communication for the threads
 
   Input arg: 

 */
void *threadFn(void *arg) {

  //Lets detach this thread first
  pthread_detach(pthread_self());

  Request *request = (Request *) arg;
  int mySock = request->sockNum;  /* HAVE TO WORRY ABOUT CLOSING THE SOCKET TOO */
  unsigned long myIP = request->IP;

  //Now that we have acquired the data, we can free the request structure.
  free(request);

  printf("In the thread %d\nRequest IP: %lu Request Socknum: %d\n\n\n", pthread_self(), myIP, mySock);

  time_t now;
  int tempRecvBufSize;
  char tempUnameBuf[MAXCHARS];
  char tempPassBuf[MAXCHARS];
  char dataRecvBuf[MAXRECVBUF];
  int unameLen;
  int uindex, passright;
  int sendDataLen = 0;
  int newUser = 1;  //Used to determine later on if we need to allocate memory for a new user OR just use the same memory.
  UserData* currentUser = NULL;

  //This list needs to remain local to the thread as per assignment 
  List *names = malloc(sizeof(List));
  initialize(names);

  pthread_cleanup_push(threadCleanup, names);

  //This loop handles all logging in and blocking users. 
  while(1) {

    //Zeroing out the two input buffers
    memset(tempUnameBuf, 0, MAXCHARS);
    memset(tempPassBuf, 0, MAXCHARS);
    

    //Prompting for username
    write(mySock, "Username: ", 10);
    if((tempRecvBufSize=recv(mySock, tempUnameBuf, MAXCHARS, 0)) < 0)
      Die("recv failed");
    uindex = checkUserName(tempUnameBuf);
    unameLen = strlen(tempUnameBuf);
    printf("Username buf: %s\n", tempUnameBuf);

    //Prompt the user for the password
    write(mySock, "Password: ", 10);
    if((tempRecvBufSize=recv(mySock, tempPassBuf, MAXCHARS, 0)) < 0)
      Die("recv failed");
    passright = checkPassword(tempPassBuf, uindex);

    /* At this point we know if the user should be authenticated or not. */

    //If the login information is correct
    if(!passright) {
      printf("This nigga's password is right: %s\n", tempUnameBuf);

      //Checking if the user exists in the list     
      currentUser = findUser(allUsers, tempUnameBuf, &mutex);
      if(currentUser) {
        //Checking if that user is already logged in.
        printf("User exists\n");
        if(currentUser->loggedIn) {
          write(mySock, "User already logged in.\n", 24);
          continue;
        }
        //The user isn't logged in, so lets go ahead and log him in.
        newUser=0;
        break;
      }
    
      //First check if the user is already on the blocked list
      BlockedUsers *blocked = findBlocked(blocks, tempUnameBuf, myIP, &mutex);
      //The user does exist on the blocked list
      if(blocked) {
        now = time(NULL);
  
        //The user's time is still not up
        if(now<blocked->until) {
          printf("User on block list: %s\n", tempUnameBuf);
          printf("Time now: %d, time until unblocked: %d\n", now, blocked->until);
          write(mySock, "Still blocked\n", 14);
          continue;
        }
        else {  //The user doesn't need to be blocked anymore. 
          printf("Removing this user from blocked list: %s\n", tempUnameBuf);

          //Remove his name from the blocked users list
          removeItem(blocks, blocked, &mutex);
          free(blocked);
        }
      }
      //If we're logging in, then we should just get rid of the entire wrongcount list
      //This will be done in the thread cleanup handler thats called by pthread_cleanup_pop
    //  deleteList(names);
    //  free(names);
      break;
    }
    //Otherwise 

    //First check if the user is already on the blocked list
    BlockedUsers *blocked = findBlocked(blocks, tempUnameBuf, myIP, &mutex);
    WrongCounts *usercount = findWrongCount(names, tempUnameBuf, &mutex); //And find if they're already on the wrong count list

    //The user does exist on the blocked list
    if(blocked) {
      now = time(NULL);

      //The user's time is still not up
      if(now<blocked->until) {
        printf("User on block list: %s\n", tempUnameBuf);
        printf("Time now: %d, time until unblocked: %d\n", now, blocked->until);
        write(mySock, "Still blocked\n", 14);
        continue;
      }
      else {  //The user doesn't need to be blocked anymore. 
        printf("Removing this user from the blocked list: %s\n", tempUnameBuf);
        removeItem(blocks, blocked, &mutex);
        free(blocked);
        if(usercount) {
          usercount->wrongCount=1;
          continue;
        }
      }
    }

    /* 
     * Checking the names list to see if this incorrect login attempt user already exists or not
     *
     */
    //This user isn't on the wrong list yet.
    if(!usercount) {
      WrongCounts *thisUser = malloc(sizeof(WrongCounts));
      memcpy(thisUser->userName, tempUnameBuf, unameLen);
      thisUser->wrongCount=1;
      printf("Starting incorrect login count for: %s\n", tempUnameBuf);
      insert(names, (void *)thisUser, &mutex);
      continue;
    }
    else {  //We found this bastard on the list
      usercount->wrongCount++;
      printf("Count is: %d for this user: %s\n", usercount->wrongCount, tempUnameBuf);

      if(usercount->wrongCount>=3) {
        now = time(NULL);

        //Create and set the blockeduser struct.
        BlockedUsers *blockeduser = malloc(sizeof(BlockedUsers));
        blockeduser->IP = myIP;
        memcpy(blockeduser->userName, tempUnameBuf, unameLen);
        blockeduser->until = now+BLOCK_TIME;

        printf("3 incorrect login attemps from this user, so blocked: %s\n", tempUnameBuf);
        //Add this blocked user struct to the list of blocked users.
        insert(blocks, (void *)blockeduser, &mutex);
      }
    }
  }
  printf("User logged in: %s\n", tempUnameBuf);  
  
  /* This thread handler was only there to enforce that the list for wrong counts gets freed,
     since that was a local list. We can just pop that handler off now
   */
  pthread_cleanup_pop(1); 

  now = time(NULL);

  if(newUser) { //We're dealing with a new user here, so we have to allocate memory for him
    //Now that we have authenticated a user, we are ready to add the user to our list of users. 
    currentUser = (UserData *)malloc(sizeof(UserData));
    memset(currentUser, 0, sizeof(UserData));
    currentUser->IP = myIP;
    currentUser->sockNum = mySock;
    currentUser->lastLogin = now;
    currentUser->lastLogout = 0;
    memcpy(currentUser->userName, tempUnameBuf, unameLen);
    currentUser->loggedIn = 1;
  
    insert(allUsers, currentUser, &mutex);
  } else {  //We're dealing with a user who is already in our system.
    currentUser->IP = myIP;
    currentUser->sockNum = mySock;
    currentUser->lastLogin = now;
    currentUser->loggedIn = 1;
  }
  write(mySock, "Welcome to the chat server!\n", 28);

  while(1) {
    printf("\nTraversing all users:\n");
    traverse(allUsers);
    printf("\n");
    now = time(NULL);

    memset(dataRecvBuf, 0, MAXRECVBUF);

    write(mySock, "Say a command.\n", 15);
    if((tempRecvBufSize=recv(mySock, dataRecvBuf, MAXRECVBUF, 0)) < 0)
      Die("recv failed");



    /* Implementation of whoelse command */
    if(!strncmp(dataRecvBuf, "whoelse", WHOELSE)) {

      write(mySock, "\n", 1);

      //Allocate a buffer here
      char listOfUsers[MAXCHARS*MAXUSERS+MAXUSERS];
      memset(listOfUsers, 0, MAXCHARS*MAXUSERS+MAXUSERS);

      //Send it to whoelse to get it filled in
      //Also give whoelse our pointer so it knows not to get our own names
      whoelse(allUsers, listOfUsers, (void *)currentUser, &mutex);  
      sendDataLen = strlen(listOfUsers);

      //Send that buffer
      write(mySock, listOfUsers, sendDataLen);
      write(mySock, "\n", 1);
      continue; //Only allow 1 command per round
    }


    /* Implementation of wholasthr command */
    if(!strncmp(dataRecvBuf, "wholasthr", WHOLASTHR)) {

      write(mySock, "\n", 1);

      //Allocate a buffer here
      char listOfUsers[MAXCHARS*MAXUSERS+MAXUSERS];
      memset(listOfUsers, 0, MAXCHARS*MAXUSERS+MAXUSERS);

      //Send it to wholasthr to get filled in.
      //Also give whoelse our pointer so it knows 
      wholasthr(allUsers, listOfUsers, (void *)currentUser, LAST_HR_SECS, &mutex);
      sendDataLen = strlen(listOfUsers);

      //Send the buffer
      write(mySock ,listOfUsers, sendDataLen);
      write(mySock, "\n", 1);
      continue; //Only allow 1 command per round
    }


    /* Implementation of broadcast command */
    if(!strncmp(dataRecvBuf, "broadcast", BROADCAST)) {
      //Do the broadcast stuff here

      //First need to parse the buffer for the actual message. 
      char message[BCASTSIZE];  //Hardcoded. Plz fix  
      memset(message, 0, BCASTSIZE);

      //Adding our name to the message so everyone knows who its from. 

      memcpy(message, tempUnameBuf, unameLen-1);
      message[unameLen-1] = ':';
      message[unameLen] = ' ';

      char tempBuf[BCASTSIZE];
      sscanf(dataRecvBuf, "broadcast %[^\n]s", tempBuf);
      strcat(message, tempBuf);

      int messageLen = strlen(message);

      printf("Broadcast message:\n%s\n", message);

      broadcastMessage(allUsers, message, messageLen, broadcast, &mutex);
      continue; //Only allow 1 command per round
    }


    if(!strncmp(dataRecvBuf, "message", MESSAGE)) {
      //Do the whoelse stuff here
      char user[MAXCHARS];  //The user to whom we're gonna send this message
      char message[MSGSIZE]; 
      char tempBuf[MSGSIZE];
      
      memset(user, 0, MAXCHARS);
      memset(message, 0, MSGSIZE);

      sscanf(dataRecvBuf, "message %s %[^\n]s", user, tempBuf);
      printf("To which user shall we deliver? This one: %s\n", user);

      memcpy(message, tempUnameBuf, unameLen-1);
      message[unameLen-1] = ':';
      message[unameLen] = ' ';

      strcat(message, tempBuf);
      printf("Private message.\n%s\n", message);
      int messageLen = strlen(message);

      //Find the user first
      UserData *toUser = findUser(allUsers, user, &mutex);
      if(toUser && toUser!=currentUser) {  //If the user exists
        //Checking if the user is logged in or not
        if(toUser->loggedIn) {
          //Normal user messaging procedure
          write(toUser->sockNum, "\n", 1);
          write(toUser->sockNum, message, messageLen);
          write(toUser->sockNum, "\n\n", 2);
        } 
        else {  
          //Offline messaging procedure

        }

      } else {  //The user don't exist or trying to message myself
        if(toUser==currentUser)
          write(mySock, "Can't message yourself.\n", 24);
        else
          write(mySock, "Sorry, the user doesn't exist.\n", 31);
      }
      continue; //Only allow 1 command per round
    }




    if(!strncmp(dataRecvBuf, "block", BLOCK)) {
      //Do the whoelse stuff here



      continue; //Only allow 1 command per round
    }
    if(!strncmp(dataRecvBuf, "unblock", UNBLOCK)) {
      //Do the whoelse stuff here



      continue; //Only allow 1 command per round
    }
    if(!strncmp(dataRecvBuf, "logout", LOGOUT)) {
      //Do the whoelse stuff here
      now = time(NULL);
      currentUser->lastLogout=now;
      currentUser->loggedIn=0;
      break;
    }

  }

  //Remove the thread from the list of threads first. 
  removeThread(threads, pthread_self(), &mutex);
  pthread_exit(NULL);
  return NULL;
}

/* The function that gets called by the list */
void broadcast(void *ptr, char *message, int len) {
  UserData *data = (UserData *)ptr;
  int sock = data->sockNum;
  write(sock, "\n", 1);
  write(sock, message, len);
  write(sock, "\n", 1);
}


/* This function goes through the list of usernames and check them against the one
   supplied in the argument. 

   Also sanitizes the username if for example it contains a \n at the end

   Return value: -1 if not found
   		Index in username array if found
*/
int checkUserName(char *uname) {
  int found = -1;
  int i;
  int len;


  int unameLen = strlen(uname);

  //If uname contains \n at the end
  if(uname[unameLen-1]=='\n' || uname[unameLen-1]=='\r')  //Also check for carriage return
    uname[unameLen-1]=0;

  for(i = 0; i < FILELINES; i++) {
    len = strlen(usernames[i]);
    if(strncmp(uname, usernames[i], len)==0)
      found = i;
  }
  return found;
}

/* Supplied with the password string from the user and the index at which it resides,
   it checks the password

   Return value: 1 if NOT found
   		 0 if found
*/
int checkPassword(char *passwd, int num) {
  int a;
  int pwLen = strlen(passwords[num]);
  
  //If the index that we're given is -1 then screw it
  if(num==-1) 
    return 1;
  a = strncmp(passwd, passwords[num], pwLen);

  //Making sure its either a 0 or a 1
  if(a<0)
    a*=-1;
  return a;
}

void Die(char *message) {
  //Cancelling all running threads
  cancelThreads(threads);
  
  deleteList(allUsers);
  free(allUsers);
  deleteList(threads);
  free(threads);
  deleteList(blocks);
  free(blocks);
  printf("%s\n", message);
  exit(1);
}

void ctrlCHandler(int sig) {

  //Have to cancel all of the threads here. 
  cancelThreads(threads);

  deleteList(allUsers);
  free(allUsers);
  deleteList(threads);
  free(threads);
  deleteList(blocks);
  free(blocks);
  printf("Ctrl-c Exit. Freed Everything.\n");
  close(servFd);

  //giving threads time to finish
  sleep(1);

  exit(1);
}


void threadCleanup(void *arg) {
  printf("In the thread cleanup handler\n");
  deleteList(arg);
  free(arg);
}

