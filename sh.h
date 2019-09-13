#include "watchedUsers.h"
#include "get_path.h"
#include "alias.h"
#include "history.h"
#include "watchedFiles.h"

#define PROMPTMAX 32
#define MAXARGS 10

int sh( int argc, char **argv, char ***envp);
void which(char *command, struct pathelement *pathlist);
void where(char *command, struct pathelement *pathlist);
void list (char **args, int argsize, char ***owd);
void printenv(char **envp);
void builtinCall(char *command, char **args, int argsize, char **prompt, struct pathelement **pathlist, struct aliaselement *aliaslist, struct historyelementList *historylist, char **pwd, char **owd, char **homedir, char **prevdir, char ****envp, int *go, struct watcheduserList *watchedUserList,
  struct watchedfileList *watchedFileList, int *clobber);
void alias(struct aliaselement *aliaslist, char **args, int argsize);
void history(struct historyelementList *historylist, char **args, int argsize);
pid_t createFork(char *path, char **args, int argsize);
void cd(char ***pwd, char ***owd, char ***homedir, char ***prevdir, char **args, int argsize);
void killCmd(char **args, int argsize);
void promptCmd(char ***prompt, char **args, int argsize);
void printenvCmd(char **args, int argsize, char **envp);
void setenvCmd(char **args, int argsize, char *****envp, char ***homedir, struct pathelement ***pathlist);
char *whichHide(char *command, struct pathelement *pathlist);
char *glob_pattern(char *glob);
void watchuser(char **args, int argsize, struct watcheduserList *watchedList);
void *checkUsers(void *watchedList);
char *findRedirection(char **args, int *argsize, int *redirectOption);
int redirectAction(char *redirectFile, int redirectOption, int clobber, int *output, int *save_out, int *save_err);
void redirectToStd(int *output, int *save_out, int *save_err, int redirectOption);
void watchmail(char **args, int argsize, struct watchedfileList *watchedFileList);
