#ifndef __SERVER_H__
#define __SERVER_H__

#define MAXCHARS 50
#define MAXUSERS 100

typedef struct UserData {
  unsigned long IP;	//What's this user's IP number?
  int sockNum;	//What's this user's socket number?
  time_t lastLogin;	//When did this user last log in?
  time_t lastLogout;	//When did this user last log out?
  char userName[MAXCHARS];	//Whats this users name?
  int loggedIn;	//Is this user logged in?
} UserData;


typedef struct Request {
  unsigned long IP;
  int sockNum;
} Request;


typedef struct BlockedUsers {
	unsigned long IP;
	char userName[MAXCHARS];
	time_t until;
} BlockedUsers;


typedef struct WrongCounts {
	char userName[MAXCHARS];
	int wrongCount;
} WrongCounts;


#endif
