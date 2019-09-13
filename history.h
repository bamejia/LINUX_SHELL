#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define PROMPTMAX 32
#define MAXARGS 10

/* function prototype.  It returns a pointer to a linked list for the path
   elements. */
struct historyelementList *make_history();

void add_history(char *new_alias, struct historyelementList *historylist);

void view_history(struct historyelementList *historylist, int numCmds);

void delete_history(struct historyelementList *historylist);

char *getLast(struct historyelementList *historylist);

// int isInList(char *new_alias, struct aliaselement *aliaslist);

struct historyelement
{
  char *command;			/* an past command*/
  struct historyelement *next;		/* pointer to next alias node */
  struct historyelement *prev;
};

struct historyelementList
{
  struct historyelement *first;
  struct historyelement *last;
  int size;
};
