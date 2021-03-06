#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "LinkedList.h"


/* Initializes the head to null */
void initialize(List *list) {
  list->head = NULL;
}

/* Insert an item into the linked list */
void insert(List *list, void *data, pthread_mutex_t *mutex) {
  pthread_mutex_lock(mutex);
  Node *head = list->head;
  Node *newNode = malloc(sizeof(Node));
  newNode->data = data;
  newNode->next = head; 
  list->head = newNode;
  pthread_mutex_unlock(mutex);
}

/* Used to remove a generic item from the list if the pointer to the item is give
 */
void* removeItem(List *list, void *data, pthread_mutex_t *mutex) {
  pthread_mutex_lock(mutex);
  Node *curr, *prv;
  prv=NULL;
  for(curr = list->head; curr!=NULL; prv=curr, curr=curr->next) {
    if(curr->data == data) {
      if(prv == NULL)
        list->head = curr->next;
      else
        prv->next = curr->next;
      void *temp = curr->data;
      free(curr);
      pthread_mutex_unlock(mutex);
      return temp;
    }
  }
  pthread_mutex_unlock(mutex);
  return NULL;
}


/* TODO: Nothing
   Used to remove a thread from the list of threads
 */
void* removeThread(List *list, pthread_t data, pthread_mutex_t *mutex) {
  Node *curr, *prv;
  prv=NULL;

  pthread_mutex_lock(mutex);
  for(curr = list->head; curr!=NULL; prv=curr, curr=curr->next) {
    if(*(pthread_t *)(curr->data) == data) {
      if(prv == NULL)
        list->head = curr->next;
      else	
        prv->next = curr->next;
      void *temp = curr->data;
      free(curr);
      pthread_mutex_unlock(mutex);
      return temp;
    }
  }
  pthread_mutex_unlock(mutex);
  return NULL;
}

/***********************************************************************
    MIGHT NOT NEED THE FOLLOWING FUNCTION BELOW
/* TODO: Figure out what to do with this one

*/
int find(List *list, void *data, pthread_mutex_t *mutex) {
  int found = 0;
  pthread_mutex_lock(mutex);
  Node *temp = list->head;
  while(temp) {
    UserData *tempData = (UserData *)temp->data;
    if(tempData == data) {
      found = 1;
      break;
    }
    temp=temp->next;
  }
  pthread_mutex_unlock(mutex);
  return found;
}
/*******************************************************************END




/** 
  * A specialized find function that can look through a list and find if a given 
  * username is blocked or not by the server. 
  * 
  * Returns: the pointer to the struct if found
  *          otherwise NULL
  */
BlockedUsers* findBlocked(List *list, char *name, unsigned long IP, pthread_mutex_t *mutex) {
  BlockedUsers* found = NULL;
  int len = strlen(name);

  pthread_mutex_lock(mutex);
  Node *temp = list->head;
  while(temp) {
    BlockedUsers *tempData = (BlockedUsers *)temp->data;
    if(!strncmp(name, tempData->userName, len) && IP==tempData->IP) {
      found = tempData;
      break;
    }
    temp=temp->next;
  }
  pthread_mutex_unlock(mutex);
  return found;
}

/**
  * Looks through a list of WrongCount*'s to return the one thats found 
  *
  */
WrongCounts* findWrongCount(List *list, char *name, pthread_mutex_t *mutex) {
  WrongCounts* found = NULL;
  int len = strlen(name);

  pthread_mutex_lock(mutex);
  Node *temp = list->head;
  while(temp) {
    WrongCounts *tempData = (WrongCounts *)temp->data;
    if(!strncmp(name, tempData->userName, len)) {
      found = tempData;
      break;
    }
    temp=temp->next;
  }
  pthread_mutex_unlock(mutex);
  return found;
}


/* 
 *
 *  Tells us if a particular user exists in the list of users or not.
 *  regardless of if they are online 
 *  
 *  Returns:  The userdata structure if the user is found on the list
 *            NULL otherwise.
 */
UserData* findUser(List *list, char* name, pthread_mutex_t *mutex) {
  UserData* found = NULL;
  int len = strlen(name);

  pthread_mutex_lock(mutex);
  Node *temp = list->head;
  while(temp) {
    UserData *tempData = (UserData *)temp->data;

    //Does this user exist in this list yet?
    if(!strncmp(name, tempData->userName, len-1)) {
      found = tempData;
      break;
    }
    temp=temp->next;
  }
  pthread_mutex_unlock(mutex);
  return found;
}


/** THE FUNCTION BELOW IS ONLY FOR DEBUGGING PURPOSES **/
//Not threadsafe becuase it won't ever get called. Put in for debugging purposes.
void traverse(List *list) {
  Node *temp = list->head;
  while(temp) {
    UserData *data = (UserData *)temp->data;
    printf("Name: %s\n", data->userName); 
    printf("Logged In: %d\n", data->loggedIn);
    temp=temp->next;
  }
}

/* Function used to remove all threads on the linked list of threads */
void cancelThreads(List *list) {
  Node *temp = list->head;
  while(temp) {
    pthread_t *thread = (pthread_t *)temp->data;
    printf("Cancelling thread %d\n", *thread);
    pthread_cancel(*thread);
    temp=temp->next;
  }
}


/***** THE FOLLOWING FUNCTIONS ARE SPECIALIZED FOR THE COMMANDS ISSUED BY A USER
******/

//Fills in the listOfUsers with the list of users online right now, who are not the current user pointed to by ptr
void whoelse(List *list, char *listOfUsers, void *ptr, pthread_mutex_t *mutex) {
  pthread_mutex_lock(mutex);
  Node *temp = list->head;
  while(temp) {
    UserData *data = (UserData *)temp->data;
    if(data->loggedIn && data!=ptr) {  //The user is logged in currently
      strcat(listOfUsers, data->userName);
      strcat(listOfUsers, "\n");
    }
    temp=temp->next;
  }
  pthread_mutex_unlock(mutex);
}

//Fills in the listOfUsers with a list of users, who are not the current user pointed to by ptr
void wholasthr(List *list, char *listOfUsers, void *ptr, int timeBack, pthread_mutex_t *mutex) {
  time_t now;
  pthread_mutex_lock(mutex);
  now = time(NULL);
  Node *temp = list->head;
  while(temp) {
    UserData *data = (UserData *)temp->data;
    /*
        The three conditions when we should return the name are:
          1. Logged in currently
          2. Logged in less than an hour ago. Which means lastLogin is < now-timeBack
          3. Logged out less than an hour ago. Which means lastLogout is <n now-timeBack
      */
    if(data!=ptr && (data->loggedIn || (data->lastLogin > (now-timeBack)) || (data->lastLogout > (now-timeBack)))) {  //The user is logged in currently
      strcat(listOfUsers, data->userName);
      strcat(listOfUsers, "\n");
    }
    temp=temp->next;
  }
  pthread_mutex_unlock(mutex);
}

/* Going to be a little bit complicated.
    We're going to pass in a function pointer that this function will call with the UserData pointer
 */
void broadcastMessage(List *list, char *message, int len, void (*fn)(void *, char *, int), pthread_mutex_t *mutex) {
  pthread_mutex_lock(mutex);
  Node *temp = list->head;
  while(temp) {
    UserData *data = (UserData *)temp->data;
    if(data->loggedIn) {  //The user is logged in currently
      fn(data, message, len);
    }
    temp=temp->next;
  }
  pthread_mutex_unlock(mutex);
}

/* Checks if the user whom we're trying to send to has blocked us or not 
 *
 *  Returns 0 if the user has not blocked us
 *          1 if they have
 */
int userBlocked(UserData *from, UserData* to, pthread_mutex_t *mutex) {
  int blocked=0;
  pthread_mutex_lock(mutex);
  List *list = (List *)to->blockedUsers;
  Node *temp = list->head;
  while(temp) {
    UserData *data = (UserData *)temp->data;
    if(data==from) {  //The user is logged in currently
      blocked=1;
      break;
    }
    temp=temp->next;
  }
  if(blocked) {
    pthread_mutex_unlock(mutex);
    return blocked;
  }

  //Checking if we've blocked them either
  list = (List *)from->blockedUsers;
  temp = list->head;
  while(temp) {
    UserData *data = (UserData *)temp->data;
    if(data==to) {  //The user is logged in currently
      blocked=1;
      break;
    }
    temp=temp->next;
  }
  pthread_mutex_unlock(mutex);
  return blocked;
}

int blockedAlready(UserData *from, UserData *to, pthread_mutex_t *mutex) {
  int possible = 0;
  pthread_mutex_lock(mutex);
  List *list = (List *)from->blockedUsers;
  Node *temp = list->head;
  while(temp) {
    UserData *data = (UserData *)temp->data;
    if(data==to){
      possible = 1;
      break;
    }
    temp=temp->next;
  }
  pthread_mutex_unlock(mutex);
  return possible;
}


//Don't need to make these thread safe because assuming it will only be used
//in the main thread
void *popFront(List *list) { 
  if (!list->head) 
    return NULL; 
  struct Node *oldHead = list->head; 
  list->head = oldHead->next; 
  void *data = oldHead->data; 
  free(oldHead); 
  return data; 
}

/* Remove all nodes in the list
   This function is done a little hackily because in here I get rid of all the memory
   that is allocated to the UserData structures in the server.c file
 */
void deleteList(List *list) {
  while (list->head) {
    void *temp = popFront(list);
    free(temp);
  }
}


/*
 *  Delete the entire list of users while sending a message to each of them 
 *
 */
void deleteListWithMessage(List *list, void (*fn)(int, char *)) {
  while (list->head) {
    UserData* temp = (UserData *)popFront(list);
    if(temp->loggedIn) {
      fn(temp->sockNum, "Logging off.");
    }
    free(temp);
  }
}


/*
 *  Delete a list without freeing any of the user data 
 */
void deleteListNoFree(List *list) {
  while (list->head)
    popFront(list);
}

/*
 *  Delete the list of blocked users held by each user
 *
 */
void deleteBlockList(List *list) {
  Node *temp = list->head;
  while(temp) {
    UserData *data = (UserData *)temp->data;
    List *blockList = (List *)data->blockedUsers;
    deleteListNoFree(blockList);
    free(blockList);
    temp=temp->next;
  }
}


