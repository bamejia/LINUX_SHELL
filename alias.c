#include "alias.h"

//adds some predifined aliases
struct aliaselement *make_alias(){
  struct aliaselement *tmp, *aliaslist = NULL;

  tmp = calloc(1, sizeof(struct aliaselement));
  aliaslist = tmp;

  tmp->alias = malloc((strlen("ls")+1)*sizeof(char));
  strncpy(tmp->alias, "ls", strlen("ls"));
  tmp->command = malloc((strlen("list")+1)*sizeof(char));
  strncpy(tmp->command, "list", strlen("list"));

  tmp->next = calloc(1, sizeof(struct aliaselement));
  tmp = tmp->next;

  tmp->alias = malloc((strlen("q")+1)*sizeof(char));
  strncpy(tmp->alias, "q", strlen("q"));
  tmp->command = malloc((strlen("exit")+1)*sizeof(char));
  strncpy(tmp->command, "exit", strlen("exit"));

  tmp->next = calloc(1, sizeof(struct aliaselement));
  tmp = tmp->next;

  tmp->alias = malloc((strlen("prenv")+1)*sizeof(char));
  strncpy(tmp->alias, "prenv", strlen("prenv"));
  tmp->command = malloc((strlen("printenv")+1)*sizeof(char));
  strncpy(tmp->command, "printenv", strlen("printenv"));

  tmp->next = calloc(1, sizeof(struct aliaselement));
  tmp = tmp->next;

  tmp->alias = malloc((strlen("hs")+1)*sizeof(char));
  strncpy(tmp->alias, "hs", strlen("hs"));
  tmp->command = malloc((strlen("history")+1)*sizeof(char));
  strncpy(tmp->command, "history", strlen("history"));

  tmp->next = NULL;
  return aliaslist;
}

//creates a new alias
void add_alias(char *new_alias, char **args, int argsize, struct aliaselement *aliaslist){
  char cmdArgs[MAX_CANON];
  struct aliaselement *tmp;
  tmp = aliaslist;
  while(tmp->next != NULL){
    tmp = tmp->next;
  }
  tmp->next = calloc(1, sizeof(struct aliaselement));
  tmp = tmp->next;
  tmp->alias = malloc((strlen(new_alias)+1)*sizeof(char));
  strncpy(tmp->alias, new_alias, strlen(new_alias));
  // tmp->alias[strlen(new_alias)] = '\0';
  sprintf(cmdArgs, "%s", args[0]);
  for(int i = 1; i < argsize; i++){
    sprintf(cmdArgs, "%s %s", cmdArgs, args[i]);
  }
  tmp->command = malloc((strlen(cmdArgs)+1)*sizeof(char));
  strncpy(tmp->command, cmdArgs, strlen(cmdArgs));
  // tmp->command[strlen(cmdArgs)] = '\0';
  printf("Alias: %s | Command: %s\n", tmp->alias, tmp->command);
  tmp->next = NULL;
}

//prints all known aliases
void view_aliaslist(struct aliaselement *aliaslist){
  struct aliaselement *tmp = aliaslist;

  printf("___________Known aliases___________\n");
  while(tmp != NULL){
    printf("Alias: %s | Command: %s\n", tmp->alias, tmp->command);
    tmp = tmp->next;
  }
  printf("__________________________________\n");
}

//returns 1 if alias is already in list, 0 otherwise
int isInAliasList(char *new_alias, struct aliaselement *aliaslist){
  struct aliaselement *tmp = aliaslist;

  while(tmp != NULL){
    if(strcmp(new_alias, tmp->alias) == 0){
      return 1;
    }
    tmp = tmp->next;
  }
  return 0;
}

//deletes entire alias list, freeing up space
void delete_alias(struct aliaselement *aliaslist){
  struct aliaselement *tmp = aliaslist, *tmpPrev;

  while(tmp != NULL){
    tmpPrev = tmp;
    tmp = tmp->next;
    free(tmpPrev->alias);
    free(tmpPrev->command);
    free(tmpPrev);
  }
}
