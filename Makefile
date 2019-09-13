# choose your compiler
CC=gcc
#CC=gcc -Wall

mysh: sh.o get_path.o history.o alias.o watchedUsers.o watchedFiles.o main.c
	$(CC) -g main.c sh.o get_path.o history.o alias.o watchedUsers.o watchedFiles.o -o mysh -lpthread
#	$(CC) -g main.c sh.o get_path.o bash_getcwd.o -o mysh

sh.o: sh.c sh.h
	$(CC) -g -c sh.c

get_path.o: get_path.c get_path.h
	$(CC) -g -c get_path.c

history.o: history.c history.h
	$(CC) -g -c history.c

alias.o: alias.c alias.h
	$(CC) -g -c alias.c

watchedUsers.o: watchedUsers.c watchedUsers.h
	$(CC) -g -c watchedUsers.c

watchedFiles.o: watchedFiles.c watchedFiles.h
	$(CC) -g -c watchedFiles.c

clean:
	rm -rf sh.o get_path.o history.o alias.o watchedUsers.o watchedFiles.o mysh
