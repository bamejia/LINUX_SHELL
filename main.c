#include "sh.h"
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void sig_handler(int signal);

int main( int argc, char **argv, char **envp )
{
	/* put signal set up stuff here */
	if (signal(SIGINT, sig_handler) == SIG_ERR)
		printf("signal error\n");
	if (signal(SIGCHLD, sig_handler) == SIG_ERR)
		printf("signal error\n");
	if (signal(SIGTSTP, sig_handler) == SIG_ERR)
		printf("signal error\n");
	if (signal(SIGTERM, sig_handler) == SIG_ERR)
		printf("signal error\n");
	sigignore(SIGTSTP);
	sigignore(SIGTERM);

  return sh(argc, argv, &envp);
}//main

void sig_handler(int signo)
{
	int n = 0;
	if (signo == 2){
		if(n){
			exit(0);
		}
	}
	if(signo == SIGCHLD){
		int rc_wait;
		int status;
		rc_wait = waitpid(-1, &status, WNOHANG);
		if(rc_wait != 0 && status != 0){
			// printf("\nChild %d has terminated with exit status: %d\n", rc_wait, status);
			fprintf(stderr, "\nChild has terminated with exit status: %d\n", status);
		}
	}
	// if(signo == USR1){
	// 	int rc_wait;
	// 	int status;
	// 	rc_wait = waitpid(-1, &status, WNOHANG);
	// 	if(status != 0){
	// 		printf("\nChild %d has terminated with exit status: %d\n", rc_wait, status);
	// 	}
	// }
	fflush(stdout);
}//sign_handler
