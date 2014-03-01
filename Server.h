#ifndef __SERVER_H__
#define __SERVER_H__

#define MAXCHARS 50
#define MAXUSERS 100

typedef struct UserData {
  unsigned long IP;	//What's this nigga's IP number?
  int sockNum;	//What's this nigga's socket number?
  time_t lastLogin;	//When did this nigga last log in?
  time_t lastLogout;	//When did this nigga last log out?
  char userName[MAXCHARS];	//Whats this niggas name?
  int loggedIn;	//Is this nigga logged in?
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
