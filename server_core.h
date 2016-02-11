#ifndef SERVER_CORE_H
#define SERVER_CORE_H
	
	// Libraries and stuff
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <unistd.h>
	#include <sys/socket.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <sys/mman.h>
	#include <fcntl.h>
	#include <netdb.h>
	#include <signal.h>
	#include <pthread.h>

	// Maximum number of concurrent clients
	// Apache uses 20 by default, so I'll use that
	#define MAX_CLIENTS 20

	// Prepare for threading
	pthread_t threads [ MAX_CLIENTS ];
	
	// The file descriptor for the socket our server listens with
	// Needs to be global for clean shutdown reasons
	int *listen_file_desc;
	// Used to abort the program and signal threads to shutdown
	int abrt = 0;

	// Struct for parsing the HTTP header of a request
	typedef struct {
		char* command;
		char* resource;
		char* version;
	} HTTPRequest;

	// Functions
	int get_next_available_thread( );
	void *handle_client( void *args );
	int close_thread( pthread_t tid );
	int http_respond( int client_file_desc, char *requested_resource );
	int http_receive_request( int client_file_desc, char **request );
	int get_socket( int *listen_file_desc, int port );
	int count_thread_sockets( );
	int bind_signals( );
	void SIGINT_handler( );

#endif