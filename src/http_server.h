#ifndef HTTP_SERVER_H_
#define HTTP_SERVER_H_

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define HTTP_SERVER_DEFAULT_PORT "8085"
#define HTTP_SERVER_DEFAULT_RELATIVE_PATH "."
#define HTTP_SERVER_BAD_SOCKET -1
#define HTTP_SERVER_BACKLOG 10
#define HTTP_SERVER_HTTP_VERSION "HTTP/1.1"
#define HTTP_SERVER_MAX_HEADER_SIZE 512
#define HTTP_SERVER_FILE_CHUNK 1024

// Contains all of the information needed to create to connect to the server and
// send it a message.
typedef struct Config {
    char *port;
    char *relative_path;
} Config;

typedef struct Header {
    char *name;
    char *value;
} Header;

typedef struct Request {
    char *method;
    char *path;
    int num_headers;
    Header **headers;
} Request;

typedef struct Response {
    char *status;
    FILE *file;
    int num_headers;
    Header **headers;
} Response;

// Parses the options given to the program. It will return a Config struct with the necessary
// information filled in. argc and argv are provided by main. If an error occurs in processing the
// arguments and options (such as an invalid option), this function will print the correct message
// and then exit.
Config http_server_parse_arguments(int argc, char *argv[]);

////////////////////////////////////////////////////
///////////// SOCKET RELATED FUNCTIONS /////////////
////////////////////////////////////////////////////

// Create and bind to a server socket using the provided configuration. A socket file descriptor
// should be returned. If something fails, a -1 must be returned.
int http_server_create(Config config);

// Listen on the provided server socket for incoming clients. When a client connects, return the
// client socket file descriptor. This is a blocking call. If an error occurs, return a -1.
int http_server_accept(int socket);

// Read data from the provided client socket, parse the data, and return a Request struct. This
// function will allocate the necessary buffers to fill in the Request struct. The buffers contained
// in the Request struct must be freed using http_server_client_cleanup. If an
// error occurs, return an empty request and this function will free any allocated resources.
Request http_server_receive_request(int socket);

// Sends the provided Response struct on the provided client socket.
int http_server_send_response(int socket, Response response);

// Closes the provided client socket and cleans up allocated resources.
void http_server_client_cleanup(int socket, Request request, Response response);

// Closes provided server socket
void http_server_cleanup(int socket);

////////////////////////////////////////////////////
//////////// PROTOCOL RELATED FUNCTIONS ////////////
////////////////////////////////////////////////////

// A helper function to be used inside of http_server_receive_request. This should not be used
// directly in main.c.
Request http_server_parse_request(char *buf);

// Convert a Request struct into a Response struct. Use relative_path to determine the path of the
// file being requested. This function will allocate the necessary buffers to fill in the Response
// struct. The buffers contained in the Resposne struct must be freeded using
// http_server_client_cleanup. If an error occurs, an empty Response will be returned and this
// function will free any allocated resources.
Response http_server_process_request(Request request, char *relative_path);

#endif
