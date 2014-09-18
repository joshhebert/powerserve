/*
 * =====================================================================================
 *
 *       Filename:  client_core.c
 *
 *    Description:  Handles initialization of the http client
 *
 *        Version:  1.0
 *        Created:  09/05/2014 08:24:22 AM
 *       Revision:  none
 *       Compiler:  gcc
 *     C Standard:  gnu99
 *
 *         Author:  Josh Hebert (jahebert@wpi.edu) 
 *   Organization:  WPI
 *
 * =====================================================================================
 */

#include "client_core.h"


/*
 * Acquires a socket connected to the specified server
 */
int get_socket(int *socket_file_desc, char *host, char *port){
	// Temp holder for the socket
	int tmp_file_desc;
	
	// Configure connection info
	struct addrinfo hints;
		memset( &hints, 0, sizeof hints );
		hints.ai_family		=	AF_UNSPEC;
		hints.ai_socktype	=	SOCK_STREAM;
	
	// Figure out where we're going
	struct addrinfo *res;
	getaddrinfo( host, port, &hints, &res );

	// Create the web socket
	tmp_file_desc = socket( res->ai_family, res->ai_socktype, res->ai_protocol );

	// Connect using the socket
	if( connect( tmp_file_desc, res->ai_addr, res->ai_addrlen ) == -1 ){
		printf( "Connection failed!\n");
		// *dies*
		return -1;
	}
	freeaddrinfo( res );
		
	// Pass the socket up, now that we know it's good
	*socket_file_desc = tmp_file_desc;

	return 0;
}

/*
 * Sends a GET request for the specified resource to the server connected on the given socket
 */
int http_get( int socket_file_desc, char *host, char *file ){
	// Send HTTP GET request
	// 25 chars + host + filename + 1
	char *get_request = malloc( sizeof( char ) * ( 25 + strlen( host ) + strlen( file ) ) + 1 );
	sprintf( get_request, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", file, host);
	int  get_request_size = strlen( get_request );
	int bytes_sent = send( socket_file_desc, get_request, get_request_size, 0 );

	// Sanity check
	if( bytes_sent != get_request_size ){
		printf( "ERROR: Expected to send %d bytes, but only sent %d\n", get_request_size, bytes_sent ); 
		return -1;
	} 
	
	free( get_request );

	// Return happy
	return 0;
}

/*
 * Receives the data coming back from the server
 */
int http_receive( int socket_file_desc, char **receive_buffer ){
	*receive_buffer = NULL;

	// Step 1: Get the HTTP header and figure out what we're dealing with
		// Buffer for one character at a time
		char *socket_read_buffer = malloc( 2 );
		char *http_header = NULL;
		// Get the header and figure out what we're dealing with
		for( ;; ){
			//Zero out the receive buffer
			memset( socket_read_buffer, 0, 2 );
			
			//Receive new bytes
			int bytes_received = recv( socket_file_desc, socket_read_buffer, 1, 0 );

			// Make sure we actually got something
			if( bytes_received > 0 ){
				// Deal with a potentially NULL receive_buffer
				int http_header_size;
				if( http_header == NULL ){
					http_header_size = 0;
				}else{
					http_header_size = strlen( http_header );
				}
				
				// No room in receive_buffer for the new bytes?
				// MAKE IT FIT
				char *tmp = realloc( http_header, http_header_size + bytes_received + 1);
				if( http_header == NULL ){
					memset( tmp, ( int ) '\0', http_header_size + bytes_received );
				}
				if( tmp != NULL ){
					// Realloc successful
					http_header = tmp;
				}else{
					// We probably ran out of memmory
					printf( "Out of memory (-2)\n" );
					// Clean up
					free( socket_read_buffer );
					free( http_header );
					return -2;
				}

				// Add them in
				strcat( http_header, socket_read_buffer );
				
				// Check for end of header
				char *http_header_term = strstr( http_header, "\r\n\r\n" );
				if( http_header_term != NULL ){
					// Awesome, we found it. No more byte-by-byte reading
					break;
				}
			}else{
				// *Connection dies*
				printf( "Server gave up before it transmitted a complete HTTP header; don't blame me\n" );				
				// Clean up
				free( socket_read_buffer );
				free( http_header );
				// We're done here
				return -1;
			}
		}
		free( socket_read_buffer );

	// We now have the HTTP header
	// Step 2: Parse it out and receive appropriately

		// Figure out how long the body is
		char *content_length_field = strstr( http_header, "Content-Length: " );
		
		// If we can't figure it out, the server's probably using chunked encoding
		// *sigh*
		if( content_length_field == NULL ){
			char *encoding_field = strstr( http_header, "Transfer-Encoding: ");
			encoding_field = strtok( encoding_field + 19, "\r" );

			// Make *sure* that we're using chunked encoding
			if( strcmp( encoding_field, "chunked" ) == 0 ){
				char *http_body = NULL;	
				char *chunk_buffer = NULL;
				int get_chunk_code;

				// Iterate until we've parsed all the chunks
				for( ;; ){

					// Get the next chunk
					get_chunk_code = get_next_chunk( socket_file_desc, &chunk_buffer );
					if( http_body == NULL ){
						http_body = realloc( http_body, strlen( chunk_buffer ) + 1 );
						memset( http_body, 0, strlen( chunk_buffer ) + 1 );
					}else{
						http_body = realloc( http_body, strlen( http_body ) + strlen( chunk_buffer ) + 1 );
					}
					strcat( http_body, chunk_buffer );
					if( get_chunk_code == 1 ){
						break;
					}

				}
				// Pass the HTML body back up
				free( chunk_buffer );
				*receive_buffer = http_body;
					
			}else{
				return -2;
			}
			// Clean up
			free( http_header );
			
			return 0;

		// Otherwise, receive as per-usual
		}else{

			// Seeing as we know how big the content is
			content_length_field = strtok( content_length_field + 16, "\r" );
			int content_length = atoi( content_length_field );	
			
			// Make a buffer that big
			char *http_body = malloc( content_length + 1 );
			memset( http_body, ( int ) '\0', content_length + 1);
			
			// Receive to it until we've received everything
			int bytes_received = 0; 
			for( ;; ){
				socket_read_buffer = malloc( content_length + 1);
				memset( socket_read_buffer, ( int ) '\0', content_length + 1);
				bytes_received += recv( socket_file_desc, socket_read_buffer, content_length, 0 );
				strcat( http_body, socket_read_buffer );	
				if( bytes_received >= content_length ){
					break;
				}
				free( socket_read_buffer );
			}

			// Pass the HTML body back up
			*receive_buffer = http_body;
		}

	// Clean up
	free( socket_read_buffer );
	free( http_header );

	// Return happy
	return 0;
}


int get_next_chunk( int socket_file_desc, char **receive_buffer ){
	// Deal with chunked encoding 

	// First we need to figure out how big this chunk is
	int chunk_size;
	char chunk_line[100] = {0};
	
	// Read in that first line, which has a hex on it to indicate the 
	// chunk size
	for( ;; ){
		char *tmp = malloc( 2 );
		memset( tmp, 0, 2 );
		recv( socket_file_desc, tmp, 1, 0 );
		strcat( &( chunk_line[ 0 ] ), tmp );
		free( tmp );

		if( strstr( &( chunk_line[ 0 ] ), "\r" ) != NULL ){
			break;
		}
		
	}

	//Extract the chunk size
	chunk_size = strtol( &( chunk_line[ 0 ] ), NULL, 16 ); 
	// If this is a terminating chunk, let the calling function know
	if( chunk_size == 0 ){
		return 1;
	}

	// Add two because /r/n
	chunk_size += 2; 
	
	// Create a buffer to fill with this chunk
	char *chunk_buffer = malloc( chunk_size + 1 );
	memset( chunk_buffer, ( int ) '\0', chunk_size + 1 );
	
	// Fill the buffer with exactly chunk_size bytes
	int bytes_received = 0;
	int read_buffer_size = BUFSIZ;
	for( ;; ){
		char *tmp = malloc( chunk_size + 1 );
		memset( tmp, ( int ) '\0', chunk_size + 1 );

		int bytes_remaining = chunk_size - bytes_received;
		if( bytes_remaining < read_buffer_size ){
			read_buffer_size = bytes_remaining;
		}
		bytes_received += recv( socket_file_desc, tmp, read_buffer_size, 0 );
		
		strcat( chunk_buffer, tmp );
		free( tmp );
		
		if( bytes_received == chunk_size ){
			break;
		}
		
	}

	// Pass the chunk back up
	*receive_buffer = chunk_buffer;

	// Return happy
	return 0;
}

int parse_url( char* url, char **host, char **resource){
	*host = strtok( url, "/");
	*resource = malloc( strlen( url ) + 2 );
	strcpy( *resource, "/" );
	char *resource_tmp = strtok( NULL, " ");
	if( resource_tmp != NULL ){
		strcat( *resource, resource_tmp );
	}
	return 0;
}

int main( int argc, char *argv[ ] ){
	char *host;
	char *port;
	char *target;
	char *url;
	int calc_rtt = 0;

	if( argc == 3 ){
		url = argv[1];
		port = argv[2];
	}else if( argc == 4 ){
		if( strcmp( argv[1], "-p" ) != 0 ){
			printf( "Invalid arguments\n");
		}else{
			calc_rtt = 1;
			url = argv[2];
			port = argv[3];
		}
	}else{
		printf( "Invalid arguments\n" );
	}

	parse_url( url, &host, &target);
	printf( "Connecting to server %s for resource %s on port %s\n", host, target, port );
	
	
	int socket_file_desc;
	
	// Acquire a socket
	if( get_socket( &socket_file_desc, host, port ) ){
		printf( "Error getting socket\n" );
		return 1;
	}
	
	// Send a GET request for the specified resource on the target server
	if( http_get( socket_file_desc, host, target ) ){
		printf( "Error sending GET request\n" );
		return 1;
	}

	// Receive the server's response and print dump the body into recv_buffer
	char *recv_buffer = NULL;
	if( http_receive( socket_file_desc, &recv_buffer ) ){
		printf( "Error receiving from the server\n" );
		return 1;
	}

	// Print the HTML stuff we received
	printf( "Begin HTML Content:\n\n%s\n", recv_buffer );
	
	// Clean up our buffers
	free( recv_buffer );
	free( target );
	// Close out socket
	shutdown( socket_file_desc, 2 );
	close( socket_file_desc );

	// Return happy
	return 0;
}


