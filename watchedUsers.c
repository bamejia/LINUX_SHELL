#include "watchedUsers.h"

//makes new watched list
struct watcheduserList *make_watched_users(){
  struct watcheduserList *watchedUserList = calloc(1, sizeof(struct watcheduserList));
  watchedUserList->first = NULL;
  watchedUserList->last = NULL;
  return watchedUserList;
}//make_watched_users

//adds a new user to the watched list
void add_user(char *new_user,struct watcheduserList *watchedUserList){
  struct watcheduser *tmp = watchedUserList->last;

  if(tmp == NULL){
    tmp = calloc(1, sizeof(struct watcheduser));
    watchedUserList->first = tmp;
    watchedUserList->last = tmp;
  }
  else{
    tmp->next = calloc(1, sizeof(struct watcheduser));
    tmp = tmp->next;
    watchedUserList->last = tmp;
  }
  tmp->user = malloc((strlen(new_user)+1)*sizeof(char));
  strncpy(tmp->user, new_user, strlen(new_user));
  tmp->next = NULL;
}//add_user

//checks if a user is in watched list
int isInUserWatchList(char *check_user, struct watcheduserList *watchedUserList){
  struct watcheduser *tmp = watchedUserList->first;

  while(tmp != NULL){
    if(strcmp(tmp->user, check_user) == 0){//returns true if the check_user is already in the watch list
      return 1;
    }
    tmp = tmp->next;
  }
  return 0;
}//isInUserWatchList

int remove_user(char *rem_user, struct watcheduserList *watchedUserList){
  struct watcheduser *tmp = watchedUserList->first, *tmpPrev, *tmpNext;

  if(tmp == NULL) return 0;

  if(strcmp(tmp->user, rem_user) == 0){//returns true if the rem_user is succesffuly removed from watch list
    tmpPrev = tmp;
    tmp = tmp->next;
    free(tmpPrev->user);
    free(tmpPrev);
    watchedUserList->first = tmp;
    if(tmp == NULL) watchedUserList->last = NULL;
    return 1;
  }

  while(tmp->next != NULL){
    if(strcmp(tmp->user, rem_user) == 0){//returns true if the rem_user is succesffuly removed from watch list
      tmpNext = tmp->next->next;
      free(tmp->next->user);
      free(tmp->next);
      if(tmpNext == NULL) watchedUserList->last = tmp;
      return 1;
    }
    tmp = tmp->next;
  }
  return 0;//returns false if no user is found with name rem_user
}//remove_user

//deletes entire watched user list, freeing up space
void delete_watched_users(struct watcheduserList *watchedUserList){
  struct watcheduser *tmp = watchedUserList->first, *tmpPrev;
  if(tmp != NULL) pthread_cancel(user_thread);

  while(tmp != NULL){
    tmpPrev = tmp;
    tmp = tmp->next;
    free(tmpPrev->user);
    free(tmpPrev);
  }
  free(watchedUserList);
}//delete_watched_files
