#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__

#include <pthread.h>
#include "Server.h"

typedef struct Node Node;
typedef struct List List;

struct Node {
  Node *next;
  void *data;
};

struct List {
 Node *head; 
};

void initialize(List *);
void insert(List *, void *, pthread_mutex_t *);
void* removeItem(List *, void *, pthread_mutex_t *);
void* removeThread(List *list, pthread_t data, pthread_mutex_t *mutex);
int find(List *, void *, pthread_mutex_t *);
BlockedUsers* findBlocked(List *, char *, unsigned long, pthread_mutex_t *);
WrongCounts* findWrongCount(List *, char *, pthread_mutex_t *);
UserData* findUser(List *, char *, pthread_mutex_t *);
void traverse(List *);
void cancelThreads(List *);
void whoelse(List *, char *, void *, pthread_mutex_t *);
void wholasthr(List *, char *, void *, int, pthread_mutex_t *);
void broadcastMessage(List *, char *, int,  void (*fn)(void *, char *, int), pthread_mutex_t *);
void deleteList(List *);




#endif
