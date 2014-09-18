/*
 * =====================================================================================
 *
 *       Filename:  server_core.c
 *
 *    Description:  Simple, multithreaded HTTP server 
 *
 *        Version:  1.0
 *        Created:  09/07/2014 01:37:05 PM
 *       Revision:  none
 *       Compiler:  gcc
 *     C Standard:  gnu99
 
 *         Author:  Josh Hebert (jahebert@wpi.edu)
 *   Organization:  WPI
 *
 * =====================================================================================
 */

#include "server_core.h"

/* 
 * What to do on SIGINT
 */
void SIGINT_handler( ){
	abrt = 1;
	shutdown( *listen_file_desc, 2 );
}

/* 
 * Bind up signals
 */
int bind_signals( ){
	signal( SIGINT, SIGINT_handler );
	return 0;
}

/*
 * How many threads are in use?
 */
int count_thread_sockets( ){
	int accum = 0;
	for( int i = 0; i < MAX_CLIENTS; ++i ){
		if( threads[ i ] != 0 ){
			++accum;
		}
	}
	return accum;
}

/*
 * Get a socket to listen with, using the requested port
 */
int get_socket( int *listen_file_desc, int port ){
	// Safety first
		if( listen_file_desc == NULL ){
			listen_file_desc = malloc( sizeof( int ) );
		}
	//Configure hints
		struct addrinfo *hints = malloc( sizeof( struct addrinfo ) );
			memset ( hints, 0, sizeof( struct addrinfo ) );
			hints->ai_family = AF_INET;
			hints->ai_socktype = SOCK_STREAM;
	
	// Socket that shit up
		*listen_file_desc = socket( hints->ai_family, hints->ai_socktype, 0 );
		free( hints );
		if( *listen_file_desc == -1 ){
			// Failed to get a socket
		}

			
	// Bind
		int bind_status = -1;
		struct sockaddr_in address;
			address.sin_family = AF_INET;
			address.sin_addr.s_addr = INADDR_ANY;
			address.sin_port = htons( port );

		bind_status = bind( *listen_file_desc, ( struct sockaddr * ) &address, sizeof( address ) );
		if( bind_status != 0 || bind_status == -1 ){
			// Bind failed
			printf( "\n[MAIN THREAD] Bind failed! Is something else running on that port?\n" );
			return -1;	
		}

	// Start listening with our file descriptor
		int listen_status = listen( *listen_file_desc, 100 );
		if( listen_status != 0 ){
			// We couldn't set up listening
		}
	
	// Return happy
		return 0;

}

/*
 * Parses incoming HTTP requests to figure out what the client wants
 */
int http_receive_request( int client_file_desc, char **request ){
	// Set up some buffers
	char *receive_buff = NULL;
	char *sock_buff = malloc( BUFSIZ );
	memset( sock_buff, ( int ) '\0', BUFSIZ );
	int total_bytes_received = 0;
	
	// Receive until we have everything
	for( ;; ){
		// Read _one_at_a_time_, because we don't want to overshoot
		int bytes_received = recv( client_file_desc, sock_buff, BUFSIZ, 0 );
		total_bytes_received += bytes_received;
		if( !( bytes_received > 0 ) ){
			break;
		}
		char *tmp = malloc( sizeof( receive_buff ) + bytes_received + 1 );
		if( tmp  != NULL ){
			memset( tmp, 0, sizeof( receive_buff ) + bytes_received + 1 );
			if( receive_buff != NULL ){
				strcat( tmp, receive_buff );
			}
			strcat( tmp, sock_buff );
			
			free( receive_buff );

			receive_buff = tmp;
		}else{
			return -1;
		}

		// Check for the end of the HTTP header
		if( strstr( receive_buff, "\r\n\r\n" ) != NULL ){
			// We're done here
			break;
		}
	}
	
	// Actually pass the value up
	*request = receive_buff;

	// Free the soscket buffer
	free( sock_buff );

	// Return happy
	return total_bytes_received;
}

/*
 * Send the client what they were looking for, if possible
 */
int http_respond( int client_file_desc, char *requested_resource ){
	// HTTP codes we can use
		char *OK200 = "HTTP/1.1 200 OK\n";
		// 404 and default body
		char *ERR404 = "HTTP/1.1 404 Not Found\n";
		char *ERR404_body = "<html>\n<head>\n<title>404 - Not Found</title>\n</head>\n<body>\n<h1>404 - Not Found</h1>\n</body>\n</html>\n";

	char *connection = "Connection: close\n";
	char *content_type = "Content-Type: text/html\n";

	// 25 Bytes hould be enough *fingers crossed*
	char *content_length = malloc( 25 );
	char *resource;
	int resource_file_desc; 	
	// Use 200 OK unless we can't find the requested resource
	char *response_header = OK200;

	// None of that now
	if( strstr( requested_resource, ".." ) != NULL ){
		// Oh, you wanted directory traversal? Have index.html instead
		resource = "/imdex.html";
		// Get a file descriptor. because those are SO much more useful
		resource_file_desc = open( resource + 1, O_RDONLY );
	}else{
		// UGH. They didn't specify a resource. Jerks.
		if( requested_resource == NULL ){
			resource = "/index.html";
		}else{
			resource = malloc( strlen(requested_resource ) + 1 );
			strcpy( resource, requested_resource );
			
			// No, you cannot have a directory
			if( requested_resource[ strlen( requested_resource ) - 1 ] == '/' ){
				
				// Here, have the index.html that's (probably) in there
				resource = realloc( resource, strlen( requested_resource ) + 12 );
				strcpy( resource, requested_resource );
				strcat( resource, "index.html" );
			}	
		}
		// Get a file descriptor. because those are SO much more useful
		resource_file_desc = open( resource + 1, O_RDONLY );
		// We don't need the actual name of the resource now that we've found its descriptor
		free( resource );
	}



	char *read_buffer;
	int file_size;

	// Check if we need to 404 the client
	if( resource_file_desc < 0 ){
		//Send 404
		response_header = ERR404;
		read_buffer = ERR404_body;
		file_size = strlen( ERR404_body );
	}else{
		// Figure out how big this is
		struct stat st;
		fstat( resource_file_desc, &st );
		file_size = st.st_size;
		
		// Read the file to a buffer
		read_buffer = mmap( 0, file_size, PROT_READ, MAP_PRIVATE, resource_file_desc, 0 );
	}

	//Put the size of file_size in our header (could be the length of the 404 message)
	snprintf( content_length, 25, "Content-Length: %d", file_size );
	
	if( read_buffer != ( void * ) - 1 ){
		// Send everything on its way
		send( client_file_desc, response_header, strlen( response_header ), 0 ); 
		send( client_file_desc, connection, strlen( connection ), 0 ); 
		send( client_file_desc, content_type, strlen( content_type ), 0 ); 
		send( client_file_desc, content_length, strlen( content_length ), 0 ); 
		send( client_file_desc, "\r\n\r\n", 4, 0 ); 
		send( client_file_desc, read_buffer, file_size, 0 ); 
		

		// Free the mapped file (if we didn't 404)
		if( !( resource_file_desc < 0 ) ) {
			munmap( read_buffer, file_size );
		}
	}
	//free( read_buffer );
	free( content_length );
	close( resource_file_desc );
		
	return 0;
}

/*
 * Remove the specified thread from the array of threads
 */
int close_thread( pthread_t tid ){
	if( tid != 0 ){
		for ( int i = 0; i < MAX_CLIENTS; ++i ){
			if( threads[ i ] == tid ){
				threads[ i ] = 0;
				return 0;
			}
		}
	}else{
		return 1;
	}
	return -1;
}

/*
 * Handles all the steps needed for and HTTP exchange
 */
void *handle_client( void *args ){
	int client_file_desc = *( ( int *) args );

	// What's the client saying?
	char *client_request;
	int bytes_received =  http_receive_request( client_file_desc, &client_request );
	if( !( bytes_received > 0 ) ){
		printf( "[THREAD %lu] Got less than 1 byte from http_receive_request()\n", ( unsigned long ) pthread_self( ) );
	}
	
	//Parse out the request
	HTTPRequest *request = malloc( sizeof ( HTTPRequest ) );
	memset( request, 0, sizeof( HTTPRequest ) );

	// I'm putting this on the stack
	char sc[10];
	// because
	snprintf( sc, 10, "%lu", ( unsigned long ) pthread_self( ) );
	// shut up
	char *strtok_context = &sc[0];

	// No ragrets
	char *http_request_line = strtok_r( client_request, "\r\n", &strtok_context );
		request->command  = strtok_r( http_request_line, " ", &strtok_context );
		request->resource = strtok_r( NULL, " ", &strtok_context );
		request->version  = strtok_r( NULL, " ", &strtok_context );
	// Oh look! I don't need to free strtok_context.
	// yay

	// Respond to the client	
	if( strcmp( request->command, "GET") == 0 ){
		http_respond( client_file_desc, request->resource );
	}else{
		// Unknown command, throw an error to the client
	}

	// Clean up memory and terminate the thread
	// Don't need these anymore
	free( request );
	free( client_request );
	
	// Deallocate the thread slot and close the file descriptor for the socket
	close_thread( pthread_self( ) );
	printf( "[THREAD %lu] Thread socket unlocked\n", ( unsigned long ) pthread_self( ) );
	printf( "[MAIN THREAD] %d/%d clients connected\n", count_thread_sockets( ), MAX_CLIENTS );
	shutdown( client_file_desc, 2 );
	close( client_file_desc );

	// Terminate the thread
	pthread_exit( NULL );
}

/*
 * What's the next open thread slot?
 */
int get_next_available_thread( ){
	for( int i = 0; i < MAX_CLIENTS; ++i ){
		if( threads[ i ] == 0 ){
			return i;
		}
	}
	return -1;
}

/*
 * Main
 */
int main( int argc, char *argv[ ] ){
	int port;
	if( argc == 2 ){
		port = atoi( argv[ 1 ] );
	}else{
		printf( "You must specify a port to run on!\nNo other options available\n");
	}

	bind_signals( );
	listen_file_desc = malloc( sizeof( int ) );
	
	memset( &threads, 0, sizeof( threads ) );

	// Get the listen socket
	printf( "[MAIN THREAD] Getting socket...." );
	int get_socket_result = get_socket( listen_file_desc, port );
	if( get_socket_result == 0 ){
		printf( "done\n" );
		struct sockaddr_in *clientaddr = malloc( sizeof( struct sockaddr_in ) );
		socklen_t *addrlen = malloc( sizeof( socklen_t ) );
		memset( clientaddr, 0, sizeof( struct sockaddr_in ) );
		memset( addrlen, 0, sizeof( socklen_t ) );
		
		// Loop forever until SIGINT
		for( ;; ){
			if( abrt == 1 ){
					printf( "[MAIN THREAD] Caught SIGINT, not accepting any more connections\n" );
					printf( "[MAIN THREAD] Please wait for existing connections to close\n" );
					break;
			}else{
				if( count_thread_sockets( ) < MAX_CLIENTS ){
					int client_file_desc = accept( *listen_file_desc, ( struct sockaddr * ) clientaddr, addrlen );
					if( client_file_desc != -1 ){
						int available_thread_socket = get_next_available_thread();
					
						if( pthread_create( &( threads[ available_thread_socket ] ), NULL, handle_client, ( void * ) &client_file_desc ) != 0 ){
							printf( "[MAIN THREAD] Failed to create a child thread for connection!\n" );
						}
						printf( "[MAIN THREAD] Locking thread socket %d for thread %lu\n", available_thread_socket, ( unsigned long ) threads[ available_thread_socket ] );
						printf( "[MAIN THREAD] %d/%d clients connected\n", count_thread_sockets( ), MAX_CLIENTS );
					}
				}
			}

		}

		// Shut down
		close( *listen_file_desc );
		printf("[MAIN THREAD] Waiting for child threads to terminate...");
		for( int i = 0; i < MAX_CLIENTS; ++i ){
			if( threads[ i ] != 0 ){
				printf(".");
				pthread_join( threads [ i ], NULL );
				close_thread( threads [ i ] );
			}
		}
		printf("done\n");
		free( addrlen );
		free( clientaddr );
	}
	free( listen_file_desc );

}
