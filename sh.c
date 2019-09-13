#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>
#include <dirent.h>
#include <glob.h>
#include <utmpx.h>
#include <pthread.h>
#include <fcntl.h>
#include "sh.h"

pid_t cpid;

int sh( int argc, char **argv, char ***envp )
{
  char *prompt = calloc(PROMPTMAX, sizeof(char));
  char *commandline = calloc(MAX_CANON, sizeof(char));
  char *command, *arg, *commandpath, *p, *pwd, *owd;
  char **args = calloc(MAXARGS, sizeof(char*));
  char **aliasargs = calloc(MAXARGS, sizeof(char*));//arguments saved in aliaslist when in use
  char **wildargs = calloc(MAXARGS, sizeof(char*));//wild card args
  int uid, i, j, status, argsct, go = 1;
  struct passwd *password_entry;
  char *homedir = calloc(MAX_CANON, sizeof(char));
  struct pathelement *pathlist;
  //my variables
  struct aliaselement *aliaslist, *atmp;
  struct historyelementList *historylist;
  struct watcheduserList *watchedUserList;
  struct watchedfileList *watchedFileList;
  int cmdFound = 0;//1 when a command the user input is found
  // int bgProcesses = 0;//amount of background processes running
  int argsize = 0;
  int clobber = 0;
  int aliasargsize = 0;
  int wildargsize = 0;
  char *path = NULL;
  char *prevdir;
  pthread_mutex_init(&user_mutex, NULL);
  pthread_mutex_init(&file_mutex, NULL);

  const int BUILTINSIZE = 17;
  const char *BUILTIN[17] = {"exit", "which", "where", "cd", "pwd", "list", "pid", "kill", "prompt", "printenv", "alias", "history", "setenv", "fg", "watchuser", "watchmail", "noclobber"};

  uid = getuid();
  password_entry = getpwuid(uid);               /* get passwd info */
  strcpy(homedir, password_entry->pw_dir);		/* Home directory to start
						  out with*/

  if ( (pwd = getcwd(NULL, PATH_MAX+1)) == NULL )
  {
    perror("getcwd");
    exit(2);
  }
  owd = calloc(strlen(pwd) + 1, sizeof(char));
  memcpy(owd, pwd, strlen(pwd));
  prompt[0] = ' '; prompt[1] = '\0';

  prevdir = calloc(strlen(pwd) + 1, sizeof(char));
  memcpy(prevdir, pwd,strlen(pwd));

  /* Put PATH into a linked list */
  pathlist = get_path();
  //built-in aliases
  aliaslist = make_alias();
  //make historylist
  historylist = make_history();
  //make watched users list
  watchedUserList = make_watched_users();
  //make watched file list
  watchedFileList = make_watched_files();

  while ( go )
  {
      /* print your prompt */
      printf("%s[%s]> ", prompt, owd);
      /* get command line and process */
      // while(fgets(commandline, MAX_CANON-1, stdin) == NULL);
      if (fgets(commandline, MAX_CANON-1, stdin) == NULL){
        printf("\n");
        perror("fgets");
        printf("\n");
        continue;
      }//if
      //if nothing is entered into the commandline, skips to the next while(go) loop
      if (strcmp(commandline, "\n") == 0){
        continue;
      }//if
      if (commandline[strlen(commandline) - 1] == '\n')
  					commandline[strlen(commandline) - 1] = '\0'; /* replace newline with null */
      add_history(commandline, historylist);
      p = strtok(commandline, " "); //skip only spaces
  		command = p;
  		p = strtok(NULL, " ");
  		i = 0;
      while(p != NULL)
          {
              if (i > MAXARGS-1){//if args entered goes over the MAXARGS limit, it is cut of at the MAXARGS limit
                printf("Too many arguments entered, reduced to %d\n", MAXARGS);
                break;
              }
              args[i] = malloc(strlen(p)+1);//printf("args[i]: %s\n", p);
              strcpy(args[i], p);
              argsize++;
              p = strtok(NULL, " ");
              i++;
          }

      /* check for each built in command and implement */
      i = 0;
      cmdFound = 0;
      while(i < BUILTINSIZE){
        if(strcmp(BUILTIN[i], command) == 0){
          cmdFound = 1;
          break;
        }
        i++;
      }

      if(!cmdFound){//checks if input command is an alias
        atmp = aliaslist;
        while(atmp != NULL){
          if(strcmp(atmp->alias, command) == 0){
            cmdFound = 1;
            memset(commandline,0,strlen(commandline));
            strcpy(commandline, atmp->command);
            p = strtok(commandline, " "); //skip only spaces
        		command = p;
        		p = strtok(NULL, " ");
        		i = 0;
            while(p != NULL)//parses the alias into if it has arguments an array
                {
                    if (i > MAXARGS-1){
                      printf("Too many arguments from alias, reduced to %d\n", MAXARGS);
                      break;
                    }
                    aliasargs[i] = malloc(strlen(p)+1);
                    strcpy(aliasargs[i], p);
                    aliasargsize++;
                    p = strtok(NULL, " ");
                    i++;
                }
            i = 0;
            j = aliasargsize;
            while(i < argsize){//copies the main args into the alias args array
              if (j > MAXARGS-1){
                printf("Too many arguments in total, reduced to %d\n", MAXARGS);
                break;
              }
              aliasargs[j] = malloc((strlen(args[i])+1)*sizeof(char));
              strcpy(aliasargs[j], args[i]);
              i++;
              j++;
              aliasargsize++;
            }
            i = 0;
            while(i < aliasargsize){//copies everything back into the main args array
              for(int j = 0; j < argsize; j++){
                free(args[j]);
              }
              args[i] = malloc((strlen(aliasargs[i])+1)*sizeof(char));
              strcpy(args[i], aliasargs[i]);
              i++;
            }
            argsize = aliasargsize;
            break;
          }
          atmp = atmp->next;
        }
      }

      //FOR SEEING INPUT ARGS
      // for(i = 0; i < argsize; i++){
      //   printf("ARGS[%d]: %s\n", i, args[i]);
      // }

      //checks for wildcards
      for(int x = 0; x < argsize; x++){
        for(int y = 0; y < strlen(args[x]); y++){
          if(argsize > MAXARGS){
            break;
          }
          if(args[x][y] == '*' || args[x][y] == '?'){
            sprintf(commandline, "%s %s", command, glob_pattern(args[x]));
            p = strtok(commandline, " "); //skip only spaces
            command = p;
            p = strtok(NULL, " ");
        		i = 0;
            while(p != NULL)//parses the wildcard args if it has arguments into an array
                {
                    if (i > MAXARGS-1){
                      printf("Too many arguments from wildcard, reduced to %d\n", MAXARGS);
                      break;
                    }
                    wildargs[i] = malloc(strlen(p)+1);
                    strcpy(wildargs[i], p);
                    wildargsize++;
                    p = strtok(NULL, " ");
                    i++;
                }
            i = 0;
            j = wildargsize;
            while(i < wildargsize){//copies the main args into the wildcard args array
              if (j > MAXARGS-1){
                printf("Too many arguments in total, reduced to %d\n", MAXARGS);
                break;
              }
              wildargs[j] = malloc((strlen(args[i])+1)*sizeof(char));
              strcpy(wildargs[j], args[i]);
              i++;
              j++;
              wildargsize++;
            }
            i = 0;
            while(i < wildargsize){//copies everything back into the main args array
              for(int j = 0; j < argsize; j++){
                free(args[j]);
              }
              args[i] = malloc((strlen(wildargs[i])+1)*sizeof(char));
              strcpy(args[i], wildargs[i]);
              i++;
            }
            argsize = wildargsize;
          }
        }
      }

      //checks if command is a path and if it can be accessed
      if(!cmdFound){
        if(command[0] == '/' && strlen(command) > 1){
          if (access(command, F_OK|X_OK) == 0) {
            cmdFound = 1;
            path = command;
          }//if
        }else if(command[0] == '.' && strlen(command) > 2){
          char cmd[MAX_CANON+1];
          sprintf(cmd, "%s%s", owd, command+1);
          if (access(cmd, F_OK|X_OK) == 0) {
            char *cmdOut = malloc((strlen(cmd)+1)*sizeof(char));
            strcpy(cmdOut, cmd);
            path = cmdOut;
            cmdFound = 1;
          }//if
        }//else if

        //checks if the command can be found in the pathlist
        if(!cmdFound){
          path = whichHide(command, pathlist);//same as which, but didn't print to console
          if(path != NULL){
            cmdFound = 1;
          }//if
        }//if
        if(cmdFound){//if command was found in a path, it will execute it
          int redirectOption = 0;
          int output, save_err, save_out;
          char *redirectFile = findRedirection(args, &argsize, &redirectOption);
          if(redirectFile != NULL){
            printf("Here\n");
            printf("Output file: %s\n", redirectFile);
            redirectAction(redirectFile, redirectOption, clobber, &output, &save_out, &save_err);
          }//if
          cpid = createFork(path, args, argsize);
          if(redirectFile != NULL){
            redirectToStd(&output, &save_out, &save_err, redirectOption);
            free(redirectFile);
          }//if
        }//if
        else{
          printf("%s: Command not found.\n", command);
        }//else
      } else{//calls buildin functions
        builtinCall(command, args, argsize, &prompt, &pathlist, aliaslist, historylist, &pwd, &owd, &homedir, &prevdir, &envp, &go, watchedUserList, watchedFileList, &clobber);
      }//else

      //checks if any children have terminated and reaps them if they have
      // if(bgProcesses > 0){
      //   int rc_wait = waitpid(-1, &status, WNOHANG);
      //   if(rc_wait != 0 && status != 0){
      //     printf("\nChild %d has terminated with exit status: %d\n", rc_wait, status);
      //     bgProcesses -= 1;
      //   }
      // }

      //clears space no longer in use and resets variables
      memset(commandline,0,strlen(commandline));
      i = 0;
      while(i < argsize){
          if(args[i] != NULL)
            free(args[i]);
          i++;
      }
      argsize = 0;
      i = 0;
      while(i < aliasargsize){
          if(args[i] != NULL)
            free(aliasargs[i]);
          i++;
      }
      aliasargsize = 0;
      i = 0;
      while(i < wildargsize){
          if(args[i] != NULL)
            free(wildargs[i]);
          i++;
      }
      wildargsize = 0;
      if(path != NULL){
        free(path);
        path = NULL;
      }
      if(go == 0){  //on exit
        delete_history(historylist);
        delete_alias(aliaslist);
        delete_path(pathlist);
        delete_watched_users(watchedUserList);
        delete_watched_files(watchedFileList);
        free(prompt);
        free(commandline);
        free(homedir);
        free(owd);
        free(prevdir);
        free(args);
        free(aliasargs);
        free(wildargs);
        free(pwd);
        // pthread_mutex_destroy(&user_mutex);
      }
  }
  return 0;
} /* sh() */

char *whichHide(char *command, struct pathelement *pathlist )
{
   /* loop through pathlist until finding command and return it.  Return
   NULL when not found. */
  char cmd[MAX_CANON-1];
  struct pathelement *p;

  p = pathlist;
  while (p) {
    sprintf(cmd, "%s/%s", p->element, command);
    if (access(cmd, F_OK|X_OK) == 0) {
      char *cmdOut = malloc((strlen(cmd)+1)*sizeof(char));
      strcpy(cmdOut, cmd);
      return cmdOut;
      // break;
    }
    p = p->next;
  }
  return NULL;
} /* which() */

void which(char *command, struct pathelement *pathlist )
{
   /* loop through pathlist until finding command and return it.  Return
   NULL when not found. */
   if(command == NULL){
     printf("which: Not enough arguments.\n");
     return;
   }
  char cmd[MAX_CANON];
  struct pathelement *p;

  p = pathlist;
  while (p) {
    sprintf(cmd, "%s/%s", p->element, command);
    if (access(cmd, F_OK|X_OK) == 0) {
      printf("[%s]\n", cmd);
      return;
    }
    p = p->next;
  }
  printf("which: File \"%s\" not found.\n", command);
  return;
} /* which() */

void where(char *command, struct pathelement *pathlist )
{
  /* similarly loop through finding all locations of command */
  if(command == NULL){
    printf("where: Not enough arguments.\n");
    return;
  }
  char cmd[MAX_CANON+1];
  struct pathelement *p;

  p = pathlist;
  int i = 0;
  while (p) {
    sprintf(cmd, "%s/%s", p->element, command);
    if (access(cmd, X_OK|F_OK) == 0){
      i++;
      if(i == 1){
        printf("___________Paths command \"%s\" was found in___________\n", command);
      }
      printf("[%s]\n", cmd);
    }
    p = p->next;
  }
  if(i > 0){
    // printf("___________________________________________________\n");
    printf("\n");
  }
  else{
    printf("where: File \"%s\" not found.\n", command);
  }
} /* where() */

char *findRedirection(char **args, int *argsize, int *redirectOption){
  char *redirectFile;
  int i;
  for(i = 0; i < *argsize; i++){
    if(strcmp(args[i], ">") == 0) {*redirectOption = 1; break;}
    else if(strcmp(args[i], ">&") == 0) {*redirectOption = 2; break;}
    else if(strcmp(args[i], ">>") == 0) {*redirectOption = 3; break;}
    else if(strcmp(args[i], ">>&") == 0) {*redirectOption = 4; break;}
    else if(strcmp(args[i], "<") == 0) {*redirectOption = 5; break;}
  }//for
  if(*redirectOption){
    if(i+1 < *argsize){
      redirectFile = malloc(strlen(args[i+1])+1);
      strcpy(redirectFile, args[i+1]);
      *argsize--;
      free(args[i+1]);
      return redirectFile;
    }else return NULL;
  }else{
    return NULL;
  }//else
}//findRedirection

int redirectAction(char *redirectFile, int redirectOption, int clobber, int *output, int *save_out, int *save_err){
  if(access(redirectFile, F_OK | R_OK) != -1 && clobber)
    {fprintf(stderr, "%s: File exists.", redirectFile); return 7;}
  if(redirectOption == 1 || redirectOption == 2){
    if(access(redirectFile, F_OK | R_OK) == -1){
      *output = open(redirectFile, O_RDWR|O_CREAT|O_APPEND);
      printf("This\n");
    }else{
      printf("That\n");
      *output = open(redirectFile, O_RDWR|O_TRUNC);
    }//else
    // if (-1 == *output) { perror("opening cout log"); return 7; }
  }//if
  // if(redirectOption == 4){
  //   int err = open("cerr.log", O_RDWR|O_CREAT|O_APPEND, 0600);
  //   if (-1 == err) {perror("opening error log"); return 7;}
  // }//if

  if(redirectOption == 3 || redirectOption == 4){
    *output = open(redirectFile, O_RDWR|O_CREAT|O_APPEND);
    // if (-1 == *output) { perror("opening cout log"); return 7; }
  }//if
  // if(redirectOption == 4){
  //   int err = open("cerr.log", O_RDWR|O_CREAT|O_APPEND, 0600);
  //   if (-1 == err) {perror("opening error log"); return 7;}
  // }//if

  if(redirectOption != 5) {*save_out = dup(fileno(stdout));}
  if(redirectOption == 2 || redirectOption == 4) {*save_err = dup(fileno(stderr));}

  if(redirectOption != 5) {
    if (-1 == dup2(*output, fileno(stdout))) { perror("cannot redirect stdout"); return 7;}
  }//if
  if(redirectOption == 2 || redirectOption == 4){
    if (-1 == dup2(*output, fileno(stderr))) { perror("cannot redirect stderr"); return 7;}
  }//if
  puts("doing an ls or something now");

  return 6;
}

void redirectToStd(int *output, int *save_out, int *save_err, int redirectOption){
    fflush(stdout); //close(*output);
    fflush(stderr); close(*output);
    if(redirectOption != 5) {dup2(*save_out, fileno(stdout));}
    if(redirectOption == 2 || redirectOption == 4) {dup2(*save_err, fileno(stderr));}

    if(redirectOption != 5) {close(*save_out);}
    if(redirectOption == 2 || redirectOption == 4) {close(*save_err);}

    puts("back to normal output");
}

//calls built in commands
void builtinCall(char *command, char **args, int argsize, char **prompt, struct pathelement **pathlist, struct aliaselement *aliaslist, struct historyelementList *historylist, char **pwd, char **owd, char **homedir, char **prevdir, char ****envp, int *go, struct watcheduserList *watchedUserList,
  struct watchedfileList *watchedFileList, int *clobber){
  const char *BUILTIN[15] = {"exit", "which", "where", "cd", "pwd", "list", "pid", "kill", "prompt", "printenv", "alias", "history", "setenv", "fg", "watchuser"};
  if(strcmp(command, "exit") == 0){
    *go = 0; return;
  }else if(strcmp(command, "which") == 0){
    which(args[0], *pathlist);
  }else if(strcmp(command, "where") == 0){
    where(args[0], *pathlist);
  }else if(strcmp(command, "cd") == 0){
    cd(&pwd, &owd, &homedir, &prevdir, args, argsize);
  }else if(strcmp(command, "pwd") == 0){
    printf("pwd: %s\n", *pwd);
  }else if(strcmp(command, "list") == 0){
    list(args, argsize, &owd);
  }else if(strcmp(command, "pid") == 0){
    printf("pid: %d\n", (int) getpid());
  }else if(strcmp(command, "kill") == 0){
    killCmd(args, argsize);
  }else if(strcmp(command, "prompt") == 0){
    promptCmd(&prompt, args, argsize);
  }else if(strcmp(command, "printenv") == 0){
    printenvCmd(args, argsize, **envp);
  }else if(strcmp(command, "alias") == 0){
    alias(aliaslist, args, argsize);
  }else if(strcmp(command, "history") == 0){
    history(historylist, args, argsize);
  }else if(strcmp(command, "setenv") == 0){
    setenvCmd(args, argsize, &envp, &homedir, &pathlist);
  }else if(strcmp(command, "fg") == 0){
    // fg();
  }else if(strcmp(command, "watchuser") == 0){
    watchuser(args, argsize, watchedUserList);
  }else if(strcmp(command, "watchmail") == 0){
    watchmail(args, argsize, watchedFileList);
  }else if(strcmp(command, "noclobber") == 0){
    *clobber = (*clobber + 1)%2;
  }
}//end builtinCall

//shows all aliases or adds a new one
void alias(struct aliaselement *aliaslist, char **args, int argsize){
  if(argsize > 1){//adds alias for a certain command or string of commands
    const char *BUILTIN[13] = {"exit", "which", "w", "cd", "pwd", "list", "pid", "kill", "prompt", "printenv", "alias", "history", "setenv"};
    int i = 0;
    while(i < 13){
      if(isInAliasList(args[0], aliaslist)){
        printf("alias: Alias \"%s\" is already taken\n", args[0]);
        return;
      }else if(strcmp(BUILTIN[i], args[1]) == 0){
        add_alias(args[0], &args[1], argsize-1, aliaslist);
        return;
      }
      i++;
    }
    printf("alias: Cannot add an alias to command \"%s\", it was not found\n", args[1]);
  } else if(argsize == 1){//when the command that the alias should be binded to is not stated
    printf("alias: Please enter command you wish to add the alias \"%s\" to.\n", args[0]);
  } else {//no args shows all known aliases
    view_aliaslist(aliaslist);
  }
}//end alias

//shows last 10 input commands by default or however many specified by user
void history(struct historyelementList *historylist, char **args, int argsize){
  if(argsize > 1){
    printf("history: Too many arguments.");
    return;
  } else if(argsize == 1){//when the user tells how many of the last commands they wish to see
    for(int i = 0; i < strlen(args[0]); i++){
      if(isdigit(args[0][i]) == 0){//makes sure the argument is a number
        printf("history: Argument \"%s\" should only contain a number.", args[0]);
        return;
      }
    }
    char * pEnd;
    int arg1;
    arg1 = strtol (args[0],&pEnd,10);
    view_history(historylist, arg1);
  } else{//shows the default 10 last commands
    view_history(historylist, 10);
  }
}//end history

//deals with wildcards
char *glob_pattern(char *wildcard){
  char *gfilename;
  size_t cnt, length = 0;
  glob_t glob_results;
  char **p;
  glob(wildcard, GLOB_NOCHECK, 0, &glob_results);

  /* Find space needed */
  for (p = glob_results.gl_pathv, cnt = glob_results.gl_pathc;
       cnt; p++, cnt--)
    length += strlen(*p) + 1;

  /* Allocate the space and generate the list.  */
  gfilename = (char *) calloc(length, sizeof(char));
  for (p = glob_results.gl_pathv, cnt = glob_results.gl_pathc;
       cnt; p++, cnt--)
    {
      strcat(gfilename, *p);
      if (cnt > 1)
        strcat(gfilename, " ");
    }

  globfree(&glob_results);
  printf("WILDCARDS: %s\n", gfilename);
  return gfilename;
}//end glob_pattern

//shows all files in current directory or specified directory
void list (char **args, int argsize, char ***owd)
{
  /* see man page for opendir() and readdir() and print out filenames for
  the directory passed */
  DIR *pDir;
  struct dirent *pDirent;
  if(argsize > 0){//shows the files in each directory entered
    struct dirent *pDirent;
    DIR *pDir;

    for(int i = 0; i < argsize; i++){
      pDir = opendir (args[i]);
      if (pDir == NULL) {
          printf("Searched for \"%s\"\n", args[i]);
          perror("opendir");
          continue;
      }
      printf("_____Listing directory for \"%s\"_____\n", args[i]);
      while ((pDirent = readdir(pDir)) != NULL) {
          printf ("[%s]\n", pDirent->d_name);
      }
      closedir (pDir);
      // printf("___________________________________________\n");
      printf("\n");
    }
  }else {//shows what is inside the current directory
    struct dirent *pDirent;
    DIR *pDir;

    pDir = opendir (**owd);
    if (pDir == NULL) {
        perror("opendir");
        return;
    }
    printf("_____Listing current working directory_____\n");
    while ((pDirent = readdir(pDir)) != NULL) {
        printf ("[%s]\n", pDirent->d_name);
    }
    closedir (pDir);
    // printf("___________________________________________\n");
    printf("\n");
  }
} /* list() */

//used when attempting to execute a separate program
pid_t createFork(char *path, char **args, int argsize){
  int status;
  int rc = fork();
  int isBgProcess = 0;
  if(argsize > 0 && strcmp(args[argsize-1], "&") == 0){
    argsize -= 1;
    isBgProcess = 1;
  }
  if (rc < 0) { // fork failed; exit
      fprintf(stderr, "fork failed\n");
      exit(1);
  } else if (rc == 0) { // child (new process)
      char *execArgs[argsize+2];
      execArgs[0] = strdup(path); // program path is entered here
      int i = 1;
      while(i < argsize+1){
        execArgs[i] = strdup(args[i-1]); // arguments to be entered
        i++;
      }
      execArgs[argsize+1] = NULL; // marks end of array
      execv(execArgs[0], execArgs); // runs word count
      fprintf(stderr, "No executable file found at path: %s.\n", path); //prints if path was not found
      exit(1);
  } else if(isBgProcess == 0){ // parent goes down this path (main)
      cpid = waitpid(rc, &status, 0);
      // printf("Pid: %d\n", getpid());
      if(status != 0){
        fprintf(stderr, "\nChild has terminated with exit status: %d\n", status);
      }
  }
}//end createFork

//makes the current working directory the home directory or a specified one
void cd(char ***pwd, char ***owd, char ***homedir, char ***prevdir, char **args, int argsize){
  if(argsize > 1){
    printf("cd: Too many arguments\n");
    return;
  }else if(argsize == 1){//when the user puts the directory they wish to move to
    if(args[0][0] == '-'){//checks to see if prev directory argument was entereed
      if(chdir(**prevdir) != 0){
          perror("chdir");
          return;
      }
      char *tmp = calloc(strlen(**owd)+1, sizeof(char));
      strcpy(tmp,**owd);
      if ( (**pwd = getcwd(NULL, PATH_MAX+1)) == NULL )
      {
        perror("getcwd");
        exit(2);
      }
      memset(**owd,0,strlen(**owd));
      **owd = realloc(**owd, (strlen(**pwd)+1)*sizeof(char));
      memcpy(**owd, **pwd, strlen(**pwd));
      memset(**prevdir,0,strlen(**prevdir));
      **prevdir = realloc(**prevdir, (strlen(tmp)+1)*sizeof(char));
      strcpy(**prevdir, tmp);
      free(tmp);
    }else{//checks what other kind of argument was entered
      if(args[0][0] == '.' && strlen(args[0]) > 2){//checks if argument is a relative path
        char cwd[MAX_CANON];
        sprintf(cwd, "%s/%s", **owd, &args[0][2]);
        printf("%s\n", cwd);
        if(chdir(cwd) != 0){
            perror("chdir");
            return;
        }
        char *tmp = calloc(strlen(**owd)+1, sizeof(char));
        strcpy(tmp,**owd);
        if ( (**pwd = getcwd(NULL, PATH_MAX+1)) == NULL )
        {
          perror("getcwd");
          exit(2);
        }
        memset(**owd,0,strlen(**owd));
        **owd = realloc(**owd, strlen(**pwd)+1*sizeof(char));
        memcpy(**owd, **pwd, strlen(**pwd));
        memset(**prevdir,0,strlen(**prevdir));
        **prevdir = realloc(**prevdir, (strlen(tmp)+1)*sizeof(char));
        strcpy(**prevdir, tmp);
        free(tmp);
      }else if(args[0][0] == '/' && strlen(args[0]) > 1){//checks if path is an absolute path
        if(chdir(args[0]) != 0){
            perror("chdir");
            return;
        }
        char *tmp = calloc(strlen(**owd)+1, sizeof(char));
        strcpy(tmp,**owd);
        if ( (**pwd = getcwd(NULL, PATH_MAX+1)) == NULL )
        {
          perror("getcwd");
          exit(2);
        }
        memset(**owd,0,strlen(**owd));
        **owd = realloc(**owd, strlen(**pwd)+1*sizeof(char));
        memcpy(**owd, **pwd, strlen(**pwd));
        memset(**prevdir,0,strlen(**prevdir));
        **prevdir = realloc(**prevdir, (strlen(tmp)+1)*sizeof(char));
        strcpy(**prevdir, tmp);
        free(tmp);
      }else{//assumes it's a relative path
        char cwd[MAX_CANON];
        sprintf(cwd, "%s/%s", **owd, args[0]);
        if(chdir(cwd) != 0){
            perror("chdir");
            return;
        }
        char *tmp = calloc(strlen(**owd)+1, sizeof(char));
        strcpy(tmp,**owd);
        if ( (**pwd = getcwd(NULL, PATH_MAX+1)) == NULL )
        {
          perror("getcwd");
          exit(2);
        }
        memset(**owd,0,strlen(**owd));
        **owd = realloc(**owd, strlen(**pwd)+1*sizeof(char));
        memcpy(**owd, **pwd, strlen(**pwd));
        memset(**prevdir,0,strlen(**prevdir));
        **prevdir = realloc(**prevdir, (strlen(tmp)+1)*sizeof(char));
        strcpy(**prevdir, tmp);
        free(tmp);
      }
    }
  }else{//when no arguments are provided, moves to the home directory
    if(chdir(**homedir) != 0){
        perror("chdir");
        return;
    }
    char *tmp = calloc(strlen(**owd)+1, sizeof(char));
    strcpy(tmp,**owd);
    if ( (**pwd = getcwd(NULL, PATH_MAX+1)) == NULL )
    {
      perror("getcwd");
      exit(2);
    }
    memset(**owd,0,strlen(**owd));
    **owd = realloc(**owd, strlen(**pwd) + 1*sizeof(char));
    memcpy(**owd, **pwd, strlen(**pwd));
    memset(**prevdir,0,strlen(**prevdir));
    **prevdir = realloc(**prevdir, (strlen(tmp)+1)*sizeof(char));
    strcpy(**prevdir, tmp);
    free(tmp);
  }
}//end cd

//sends signal SIGTERM to the pid provided or a different signal if specified
void killCmd(char **args, int argsize){
  if(argsize > 2){
    printf("kill: Too many arguments\n");
  }else if(argsize == 2){//when the user wants to choose their signal
    if(args[0][0] == '-'){//checks for flag
      for(int i = 1; i < strlen(args[0]); i++){//makes sure all arguments are numbers
        if(isdigit(args[0][i]) == 0){
          printf("kill: Argument \"%s\" should only contain a number.\n", args[0]);
          return;
        }
      }
      for(int i = 0; i < strlen(args[0]); i++){//makes sure all arguments are numbers
        if(isdigit(args[1][i]) == 0){
          printf("kill: Argument \"%s\" should only contain a number.\n", args[1]);
          return;
        }
      }
      char * pEnd;
      int arg1, arg2;
      arg1 = strtol (&args[0][1],&pEnd,10);
      arg2 = strtol (args[1],&pEnd,10);
      if (kill(arg2, arg1) != 0){
        perror("kill");
      }
    }else{//when flag is not correctly input
      printf("kill: Please start argument \"%s\" with symbol \"-\".\n", args[0]);
      return;
    }
  }else if(argsize == 1){//sends SIGTERM signal to specified pid
    for(int i = 1; i < strlen(args[0]); i++){//makes sure arugment is a number
      if(isdigit(args[0][i]) == 0){
        printf("kill: Argument \"%s\" should only contain a number.\n", args[0]);
        return;
      }
    }
    char * pEnd;
    int arg1;
    arg1 = strtol (args[0],&pEnd,10);
    if (kill(arg1, SIGTERM) != 0){
      perror("kill");
    }
  }else{
    printf("kill: Not enough arguments\n");
    return;
  }
}//end killCmd

//changes the prompt before the current working directory to whatever is specified
void promptCmd(char ***prompt, char **args, int argsize){
  if(argsize > 1){
    printf("prompt: Too many arguments\n");
  }else if(argsize == 1){//changes prompt to input argument
    char tmp[PROMPTMAX];
    memset(**prompt,0,strlen(**prompt));
    strncpy(**prompt, args[0], PROMPTMAX-1);
    sprintf(tmp, "%s ", **prompt);
    strncpy(**prompt, tmp, PROMPTMAX-1);
  }else{//asks user what they wish to change the prompt to if not specified
    char tmp[PROMPTMAX];
    char buff[PROMPTMAX];
    printf("Input prompt prefix: ");
    if (fgets(buff, PROMPTMAX-1, stdin) == NULL){
      printf("\n");
      perror("fgets");
      printf("\n");
      return;
    }
    buff[strlen(buff)-1] = '\0';
    memset(**prompt,0,strlen(**prompt));
    strncpy(**prompt, buff, PROMPTMAX);
    sprintf(tmp, "%s ", **prompt);
    strncpy(**prompt, tmp, PROMPTMAX);
  }
}//end promptCmd

//prints all the environment variables or the one specified
void printenvCmd(char **args, int argsize, char **envp){
  if(argsize > 1){
    fprintf( stderr, "printenv: Too many arguments\n");
  }else if(argsize == 1){//when the user chooses a variable they wish to view
    char *tmp;
    if((tmp = getenv(args[0])) == NULL){
      printf("printenv: No match\n");
      return;
    }
    printf("%s: %s\n", args[0], tmp);
  }else{//when to variable is specified, all are shown
    printf("___________All Environment Variables___________\n");
    for (char **env = envp; *env != 0; env++){
      printf("%s\n", *env);
    }
    // printf("_______________________________________________\n");
    printf("\n");
  }
}//end printenvCmd

void setenvCmd(char **args, int argsize, char *****envp, char ***homedir, struct pathelement ***pathlist){//question here about adding new environment variables
  if(argsize > 2){//Too many arguments entered
    fprintf( stderr, "setenv: Too many arguments\n");
  }else if(argsize == 2){//sets first argument equal to second
    if(setenv(args[0], args[1] , 1) != 0){
      perror("setenv");
      return;
    }
    if(strcmp(args[0], "HOME") == 0){//if the home directory is changed, the homedir path is updated
      memset(**homedir,0,strlen(**homedir));
      strcpy(**homedir, args[1]);
    }else if(strcmp(args[0], "PATH") == 0){//if the PATH is changed, the path linked list is updated
      delete_path(**pathlist);
      **pathlist = get_path();
    }
    printf("setenv: \"%s\" has been set to value \"%s\".\n", args[0], args[1]);
  }else if(argsize == 1){//sets argument to a blank value
    if(setenv(args[0], "" , 1) != 0){
      perror("setenv");
      return;
    }
    if(strcmp(args[0], "HOME") == 0){//if the home directory is changed, the homedir path is updated
      memset(**homedir,0,strlen(**homedir));
      strcpy(**homedir, "");
    }else if(strcmp(args[0], "PATH") == 0){//if the PATH is changed, the path linked list is updated
      delete_path(**pathlist);
      **pathlist = get_path();
    }
    printf("setenv: \"%s\" has been set to an empty value.\n", args[0]);
  }else{//prints all environments when no arguments are present
    printf("___________All Environment Variables___________\n");
    for (char **env = ***envp; *env != 0; env++){
      printf("%s\n", *env);
    }
    // printf("_______________________________________________\n");
    printf("\n");

  }
}//end setenvCMD

void watchuser(char **args, int argsize, struct watcheduserList *watchedUserList){
  if(argsize > 2){
    fprintf(stderr, "watchuser: Too many arguments.\n");
  } else if(argsize == 2){//the user wishes to remove user from watch list
    if(strcmp(args[1], "off") != 0){
      fprintf(stderr, "watchuser: Option \"%s\" not found.\n", args[1]);
      return;
    }
    if(watchedUserList->first == NULL){
      fprintf(stderr, "watchuser: No users currently being watched.\n");
      return;
    }
    int lock;
    do{//lock to protect shared watched list
      lock = pthread_mutex_trylock(&user_mutex);
      if(lock){
          // printf("lock failed(%d)..attempting again in 1 sec..\n", lock);
          sleep(1);  //wait time here..
      }else{  //ret =0 is successful lock
          // printf("lock success(%d)..\n", lock);
          break;
      }
    } while(lock);
    if(!remove_user(args[0], watchedUserList)){
      fprintf(stderr, "watchuser: User not in watched list\n");
    }
    lock = pthread_mutex_unlock(&user_mutex);//unlock
    // printf("unlocked(%d)..!\n", lock);

  } else if(argsize == 1){//the user only inputs the user they wish to add to the watch list
    int create = 0;
    if(watchedUserList->first == NULL)
      create = 1;
    int lock;
    do{//locked to protect shared watch list
      lock = pthread_mutex_trylock(&user_mutex);
      if(lock)
          // printf("lock failed(%d)..attempting again in 1 sec..\n", lock);
          sleep(1);  //wait time here..
      else  //ret =0 is successful lock
          // printf("lock success(%d)..\n", lock);
          break;
    } while(lock);
    if(!isInUserWatchList(args[0], watchedUserList))
      add_user(args[0], watchedUserList);
    else
      fprintf(stderr, "watchuser: User already in watched list\n");
    lock = pthread_mutex_unlock(&user_mutex);//unlock
    // printf("unlocked(%d)..!\n", lock);
    if(create){
      if(pthread_create(&user_thread, NULL, checkUsers, watchedUserList) != 0){
        perror("pthread_create");
        return;
      }//if
    }//if
  } else{
    fprintf(stderr, "watchuser: Not enough arguments.\n");
  }//else
}//watchuser

void *checkUsers(void *watchedUserList){
  // struct watcheduserList *watchedList = (struct watcheduserList *) watchedListin;
  int lock, printIntro = 1, printEnd = 0;
  struct utmpx *up;
  struct watcheduser *curr_user = ((struct watcheduserList *)watchedUserList)->first;

  setutxent();			/* start at beginning */
  while(1){
    do{
      lock = pthread_mutex_trylock(&user_mutex);
      if(lock) sleep(1);  //wait time here..
      else break;
    } while(lock);
    if(((struct watcheduserList *)watchedUserList)->first == NULL){//if watched user list is emptiy, exits loop
      lock = pthread_mutex_unlock(&user_mutex);//unlock
      // printf("unlocked(%d)..!\n", lock);
      setutxent();
      printIntro = 1;
      if(printEnd){
        printf("\n");
        printEnd = 0;
      }//if
      sleep(20);
      break;
    }//if
    up = getutxent();/* get an entry */
    while(up != NULL){
      while(strcmp(up->ut_user, curr_user->user) != 0){
        curr_user = curr_user->next;
        if(curr_user == NULL) {
          curr_user = ((struct watcheduserList *)watchedUserList)->first;
          break;
        }//if
      }//while
      if(strcmp(up->ut_user, curr_user->user) == 0) break;
      up = getutxent();
    }//while
    if(up == NULL){
      lock = pthread_mutex_unlock(&user_mutex);//unlock
      setutxent();
      printIntro = 1;
      if(printEnd){
        printf("\n");
        printEnd = 0;
      }
      sleep(20);
      continue;
    }
    if(printIntro){
      printf("\n_________________Watched users who are logged in_________________\n");
      printIntro = 0;
      printEnd = 1;
    }
    if ( up->ut_type == USER_PROCESS)/* only care about users and users in the watched list */
    {
      printf("%s has logged on %s from %s\n", up->ut_user, up->ut_line, up ->ut_host);
    }
    lock = pthread_mutex_unlock(&user_mutex);//unlock
  }//while
  pthread_cancel(user_thread);
}//checkUsers

void watchmail(char **args, int argsize, struct watchedfileList *watchedFileList){
  if(argsize > 2){
    fprintf(stderr, "watchmail: Too many arguments.\n");

  }else if(argsize == 2){//user wishes to remove a file from being watched
    if(strcmp(args[1], "off") != 0){//checks to make sure the second argument is the off option
      fprintf(stderr, "watchmail: Option \"%s\" not found.\n", args[1]);
      return;
    }//if
    if(watchedFileList->first == NULL){//checks to see if there are any files being watched
      fprintf(stderr, "watchmail: No files currently being watched.\n");
      return;
    }//if
    while(pthread_mutex_trylock(&file_mutex)) sleep(1);//loops and sleeps until it can lock
    if(!remove_file(args[0], watchedFileList))//checks to see if a file could be successfully removed
      fprintf(stderr, "watchmail: File not in watched list\n");
    pthread_mutex_unlock(&file_mutex);

  }else if(argsize == 1){//user wants to add a file to be watched
    if(isInFileWatchList(args[0], watchedFileList)){
      fprintf(stderr, "watchmail: File already in watched list\n");
      return;
    }//if
    if(access(args[0], R_OK | F_OK) != 0){
      fprintf(stderr, "watchmail: Cannot find file \"%s\".\n", args[0]);
      return;
    }//if
    while(pthread_mutex_trylock(&file_mutex)) sleep(1);
    add_file(args[0], watchedFileList);
    pthread_mutex_unlock(&file_mutex);

  }else{
    fprintf(stderr, "watchmail: Not enough arguments.\n");
  }//else
}//watchmail
