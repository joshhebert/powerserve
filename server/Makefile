# Options
CC 			= gcc
CSTANDARD 	= gnu99
CFLAGS 		= -Wall -Wextra -lpthread

all: server_core.c
	$(CC) -std=$(CSTANDARD) $(CFLAGS) server_core.c -o ./bin/server

debug:
	$(CC) -std=$(CSTANDARD) $(CFLAGS) -g server_core.c -o ./bin/server_debug

run: all
	./server

run-debug: debug
	valgrind --tool=memcheck --leak-check=yes --track-fds=yes --track-origins=yes ./bin/server_debug

clean: 
	
