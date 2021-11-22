
DEPS = player.h	chlng.h

%.o:	%.c $(DEPS)

all: gwgd

gwgd:	problem10.o	player.o chlng.o
		gcc -Wall -o $@ $^ -levent

problem10.o:	problem10.c
		gcc -c -Wall problem10.c

player.o: player.h	player.c
	gcc -Wall -c player.c

chlng.o:	chlng.h	chlng.c
	gcc -Wall -c chlng.c

deletefile:
	rm -f *.o

deleteexec:
	rm -f gwgd

deleteall:		deletefile	deleteexec