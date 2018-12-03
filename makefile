CC = gcc
CFLAGS = -D_REENTRANT
LDFLAGS = -lpthread

all: server_main client_main

server_main: server_routine.o server_main.c
	${CC} -g -o server_app server_main.c server_routine.o  ${LDFLAGS}

client_main: client_main.c
	${CC} -g -o client_app client_main.c 

server_routine.o: server_routine.c
	${CC} -g -o server_routine.o -c server_routine.c ${LDFLAGS}

clean:
	rm -Rf server_app client_app server_routine.o *.log

cleanlog:
	rm -Rf *.log

