#include "history.h"

//sets up the history list
struct historyelementList *make_history(){
  struct historyelementList *historylist;
  historylist = calloc(1, sizeof(struct historyelementList));
  historylist->first = NULL;
  historylist->last = NULL;
  historylist->size = 0;
  return historylist;
}

//adds the last command added to the list
void add_history(char *new_history,struct historyelementList *historylist){
  struct historyelement *tmp;
  tmp = historylist->last;

  if(historylist->first == NULL){
    tmp = calloc(1, sizeof(struct historyelement));
    historylist->first = tmp;
    historylist->last = tmp;
    historylist->size++;
    tmp->prev = NULL;
  }
  else{
    tmp->next = calloc(1, sizeof(struct historyelement));
    tmp = tmp->next;
    tmp->prev = historylist->last;
    historylist->last = tmp;
    historylist->size++;
  }
  tmp->command = malloc((strlen(new_history)+1)*sizeof(char));
  strncpy(tmp->command, new_history, strlen(new_history));
  // tmp->command[strlen(new_history)] = '\0';
  tmp->next = NULL;
}

//shows last 10 commands entered or amount specified
void view_history(struct historyelementList *historylist, int numCmds){
  struct historyelement *tmp = historylist->last->prev;

  printf("___________Last %d commands___________\n", numCmds);
  int i = 1;
  while(i < numCmds+1 && i < historylist->size){
    printf("#%d Command: %s\n", i, tmp->command);
    tmp = tmp->prev;
    i++;
  }
  printf("______________________________________\n");
}

//returns the last command entered
char *getLast(struct historyelementList *historylist){
  return historylist->last->command;
}

//deletes the entire list, freeing up space
void delete_history(struct historyelementList *historylist){
  struct historyelement *tmp = historylist->last;
  while(tmp->prev != NULL){
    tmp = tmp->prev;
    free(tmp->next->command);
    free(tmp->next);
  }
  free(historylist->first->command);
  free(historylist->first);
  free(historylist);
}
