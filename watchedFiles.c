#include "watchedFiles.h"

//makes new watched list
struct watchedfileList *make_watched_files(){
  struct watchedfileList *watchedFileList = calloc(1, sizeof(struct watchedfileList));
  watchedFileList->first = NULL;
  watchedFileList->last = NULL;
  return watchedFileList;
}//make_watched_files

//adds a new user to the watched list
void add_file(char *new_file, struct watchedfileList *watchedFileList){
  struct watchedfile *tmp = watchedFileList->last;

  if(tmp == NULL){
    tmp = malloc(sizeof(struct watchedfile));
    watchedFileList->first = tmp;
    watchedFileList->last = tmp;
  }
  else{
    tmp->next = calloc(1, sizeof(struct watchedfile));
    tmp = tmp->next;
    watchedFileList->last = tmp;
  }
  tmp->file = malloc((strlen(new_file)+1)*sizeof(char));
  strncpy(tmp->file, new_file, strlen(new_file));
  tmp->next = NULL;
  pthread_create(&(tmp->thread), NULL, mailThread, tmp->file);
}//add_file

//watches a file
void *mailThread(void *file_in){
  char *file_name = malloc((strlen((char *) file_in)+1)*sizeof(char));
  strcpy(file_name, (char *) file_in);
  struct timeval tv;
  struct stat *file_stats;
  stat(file_name, file_stats);
  int size = file_stats->st_size;
  int lock;
  while(1){
    do{//lock for writing to shared watched file list
      int lock = pthread_mutex_trylock(&file_mutex);
      if(lock){
        if(file_name == NULL){
          printf("YEAH\n");
          free(file_name);
          return file_in;
        }
        sleep(1);  //wait time here..
      }
      else break;
    } while(lock);
    if(file_name != NULL && access(file_name, R_OK | F_OK) == 0){
      stat(file_name, file_stats);
      printf("file: %s\n", file_name);
      if(file_stats->st_size > size){
        printf("WHY\n");
        gettimeofday(&tv, NULL);
        printf("file: %s\n", file_name);
        printf("WHY2\n");
        printf("\aYou've Got Mail in %s at %s", file_name, ctime(&tv.tv_sec));
      }//if
    }//if
    pthread_mutex_unlock(&file_mutex);
    sleep(2);
  }
}//mailThread

//checks if a user is in watched list
int isInFileWatchList(char *check_file, struct watchedfileList *watchedFileList){
  struct watchedfile *tmp = watchedFileList->first;

  while(tmp != NULL){
    if(strcmp(tmp->file, check_file) == 0){//returns true if the check_user is already in the watch list
      return 1;
    }
    tmp = tmp->next;
  }
  return 0;
}//isInFileWatchList

//removes file name from watched file list and stops the thread watching the file
int remove_file(char *rem_file, struct watchedfileList *watchedFileList){
  struct watchedfile *tmp = watchedFileList->first, *tmpNext, *tmpPrev;

  if(tmp == NULL) return 0;

  if(strcmp(tmp->file, rem_file) == 0){//returns true if the rem_user is succesffuly removed from watch list
    pthread_cancel(tmp->thread);
    //
    tmpPrev = tmp;
    tmp = tmp->next;
    free(tmpPrev->file);
    free(tmpPrev);
    watchedFileList->first = tmp;
    if(tmp == NULL) watchedFileList->last = NULL;
    return 1;
  }

  while(tmp->next != NULL){
    if(strcmp(tmp->next->file, rem_file) == 0){//returns true if the rem_user is succesffuly removed from watch list
      pthread_cancel(tmp->next->thread);
      tmpNext = tmp->next->next;
      free(tmp->next->file);
      free(tmp->next);
      tmp->next = tmpNext;
      if(tmpNext == NULL) watchedFileList->last = tmp;
      return 1;
    }
    tmp = tmp->next;
  }
  return 0;//returns false if no user is found with name rem_user
}//remove_file

//deletes entire watched user list, freeing up space
void delete_watched_files(struct watchedfileList *watchedFileList){
  struct watchedfile *tmp = watchedFileList->first, *tmpPrev;

  while(tmp != NULL){
    pthread_cancel(tmp->next->thread);
    tmpPrev = tmp;
    tmp = tmp->next;
    free(tmpPrev->file);
    free(tmpPrev);
  }
  free(watchedFileList);
}//delete_watched_files
