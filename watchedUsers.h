#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>

#define PROMPTMAX 32
#define MAXARGS 10

pthread_t user_thread;
pthread_mutex_t user_mutex;

struct watcheduserList *make_watched_users();
void add_user(char *new_user, struct watcheduserList *watchedUserList);
int isInUserWatchList(char *new_user, struct watcheduserList *watchedUserList);
int remove_user(char *rem_user, struct watcheduserList *watchedUserList);
void delete_watched_users(struct watcheduserList *watchedUserList);

struct watcheduser
{
  char *user;			/* watched user*/
  struct watcheduser *next;		/* pointer to next watched user node */
};

struct watcheduserList{
  struct watcheduser *first;
  struct watcheduser *last;
};
