#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define PROMPTMAX 32
#define MAXARGS 10

/* function prototype.  It returns a pointer to a linked list for the path
   elements. */
struct aliaselement *make_alias();

void add_alias(char *new_alias, char **args, int argsize, struct aliaselement *aliaslist);

void view_aliaslist(struct aliaselement *aliaslist);

int isInAliasList(char *new_alias, struct aliaselement *aliaslist);

void delete_alias(struct aliaselement *aliaslist);

struct aliaselement
{
  char *alias;			/* an alias*/
  char *command;
  struct aliaselement *next;		/* pointer to next alias node */
};
