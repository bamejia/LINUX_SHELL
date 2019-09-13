#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/stat.h>

#define PROMPTMAX 32
#define MAXARGS 10

pthread_mutex_t file_mutex;

struct watchedfileList *make_watched_files();
void add_file(char *new_file, struct watchedfileList *watchedFileList);
int isInFileWatchList(char *check_file, struct watchedfileList *watchedFileList);
int remove_file(char *rem_file, struct watchedfileList *watchedFileList);
void delete_watched_files(struct watchedfileList *watchedFileList);
void *mailThread(void *file_in);

struct watchedfile
{
  char *file;			/* watched user*/
  pthread_t thread;
  struct watchedfile *next;		/* pointer to next watched user node */
};

struct watchedfileList{
  struct watchedfile *first;
  struct watchedfile *last;
};
