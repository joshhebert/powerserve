# Options
CC 			= gcc
CSTANDARD 	= gnu99
CFLAGS 		= -Wall -Wextra -lpthread

all: server_core.c
	mkdir -p bin
	$(CC) -std=$(CSTANDARD) $(CFLAGS) server_core.c -o ./bin/server

debug:
	mkdir -p bin
	$(CC) -std=$(CSTANDARD) $(CFLAGS) -g server_core.c -o ./bin/server_debug

clean: 
	rm -r bin/*
	rmdir bin	
