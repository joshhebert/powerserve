# Options
CC 			= gcc
CSTANDARD 	= gnu99
CFLAGS 		= -Wall -Wextra

all: client_core.c
	$(CC) -std=$(CSTANDARD) $(CFLAGS) client_core.c -o ./bin/client

debug:
	$(CC) -std=$(CSTANDARD) $(CFLAGS) -g client_core.c -o ./bin/client_debug

run: all
	./client

run-debug: debug
	valgrind --tool=memcheck --leak-check=yes --track-origins=yes ./bin/client_debug

clean: 
	
