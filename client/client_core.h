#ifndef CLIENT_CORE_H
#define CLIENT_CORE_H
	
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netdb.h>
	#include <unistd.h>

	int get_socket( int *socket_file_desc, char *host, char *port );
	int http_get( int socket_file_desc, char *host, char *file );
	int get_next_chunk( int socket_file_desc, char **receive_buffer );
	int http_receive( int socket_file_desc, char **receive_buffer );

#endif