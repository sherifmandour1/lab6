#include "http_server.h"
#include "log.h"
#include <assert.h>
#include <dirent.h>
#include <signal.h>
#include <sys/stat.h>
#include <time.h>

#define STRINGS_MATCH 0
#define SERVER_LISTENNING_ERROR -15
#define LISTENED 0
#define BINEDED 0
#define SERVER_CREATION_ERROR -30
#define TIME_OUT_COUNTER_MAX 500
#define SET_BAD_REQUEST_INVALID "invalid"
#define LINE_READ_ERROR -1
#define GET_LINE_ERROR -2
#define ZERO_VALUE 0
#define SPACE_CHAR ' '
#define ARRAY_INIT_NUM 1000
#define SIZEOF_LONGEST_ACTION 10
#define LAB1_BUFFER_SIZE 1024
#define MAX_ACCEPTABLE_BUFFER_CAPACITY_PERCENTAGE 0.7
#define TWO_VALUE 2
#define ONE_VALUE 1
#define NULL_TERMINATOR '\0'
#define READ_FROM_STDIN '-'
#define LENGTH_OF_LONGEST_ACTION 10
#define HANDLE_RESPONSE_ERROR -13
#define UPPERCASE_CODE 0x1
#define LOWERCASE_CODE 0x2
#define TITLE_CASE_CODE 0x4
#define REVERSE_CODE 0x8
#define SHUFFLE_CODE 0x10
#define DYNAMIC_BUFFER_ERROR -33
#define RECEIVED_CHAR_ERROR -34
#define RESPONSE_ERROR -35
#define SENDING_ERROR -36
#define CONNECT_ERROR -37
#define SENT_COMPLETE 0
#define ZERO_RESET_INIT_VALUE 0


//this function is used with the user doesn't use the correct format
void printUsage() {
    printf("Usage: http_server [--help] [-v] [-p PORT] [-f FOLDER]\n\n");
    printf("Options:\n");
    printf("--help\n");
    printf("-v, --verbose\n");
    printf("--port PORT, -p PORT\n");
    printf("--folder FOLDER, -f FOLDER\n");
}


// Parses the options given to the program. It will return a Config struct with
// the necessary information filled in. argc and argv are provided by main. If
// an error occurs in processing the arguments and options (such as an invalid
// option), this function will print the correct message and then exit.
Config http_server_parse_arguments(int argc, char *argv[]) {
        log_set_quiet(true);
    log_trace("Parsing arguments..");
    Config myConfig;
    myConfig.port = "invalid";
    myConfig.relative_path = "invalid";
    bool stillParsing = true;
    int selectedOption = 0;
    int optionIndex = 0;
    bool portReceived = false;
    bool pathReceived = false;
    struct option long_opts[] = {{"help", no_argument, 0, 0},
                                 {"verbose", no_argument, 0, 'v'},
                                 {"port", required_argument, 0, 'p'},
                                 {"folder", required_argument, 0, 'f'},
                                 {0, 0, 0, 0}};

    while ((selectedOption = getopt_long(argc, argv, ":hvp:f:", long_opts, &optionIndex)) != -1) {

        while (stillParsing) {
            switch (selectedOption) {
                break;
            case 'h':
                log_trace("Providing help information");
                myConfig.port = "invalid";
                myConfig.relative_path = "invalid";
                return myConfig;
                break;
            case 'v':
                log_set_quiet(false);
                log_trace("Verbose flag activated\n");
                stillParsing = false;
                break;
            case 'p':
                log_trace("Port option chosen and now parsing parameter passed in");
                stillParsing = false;
                myConfig.port = optarg;
                portReceived = true;
                break;
            case 'f':
                myConfig.relative_path = optarg;
                log_trace("Folder option was chosen\n");
                pathReceived = true;
                stillParsing = false;
                DIR *dir = opendir(myConfig.relative_path);
                if (!dir || errno == ENOTDIR) {
                    log_error("invlaid strating folder");
                    myConfig.port = "invalid";
                    myConfig.relative_path = "invalid";
                    return myConfig;
                }
                closedir(dir);
                break;
                case ':':
            printf("Option argument not provided\n");
            printUsage();
                    myConfig.port = "invalid";
        myConfig.relative_path = "invalid";
            return myConfig;
                break;
            case '?':
            printf("Unkown option provided\n");
            printUsage();
                    myConfig.port = "invalid";
        myConfig.relative_path = "invalid";
            return myConfig;
                break;
            default:
                printf("Option not passed in or extra unknown option provided\n");
                printUsage();
                        myConfig.port = "invalid";
        myConfig.relative_path = "invalid";
                return myConfig;
            }
        }
    }




    // subtracting the options from the arguments to be left with th arguments
    // passed in
    argc -= optind;
    argv += optind;
    //extra checking for extra arguments
    if(argc > 0)
    {
        printf("Unkown argument provided\n");
        printUsage();
        myConfig.port = "invalid";
        myConfig.relative_path = "invalid";
        return myConfig;
    }
    // Checking if port received or I need to assign the default ports
    if (!portReceived) {
        myConfig.port = HTTP_SERVER_DEFAULT_PORT;
    }
    //checking if the path was provided or assign the default path 
    if (!pathReceived) {
        myConfig.relative_path = HTTP_SERVER_DEFAULT_RELATIVE_PATH;
    }
    
    return myConfig;
}

////////////////////////////////////////////////////
///////////// SOCKET RELATED FUNCTIONS /////////////
////////////////////////////////////////////////////

void sigchld_handler()
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}



// Create and bind to a server socket using the provided configuration. A socket
// file descriptor should be returned. If something fails, a -1 must be
// returned.
int http_server_create(Config config) {
    log_trace("Creating the server\n");
    // Socket var
int sockfd; 
    struct addrinfo hints, *servinfo, *p;
    struct sigaction sa;
    int yes=1;
    int rv;

    memset(&hints, ZERO_RESET_INIT_VALUE, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, config.port, &hints, &servinfo)) != 0) {
        log_debug("addr info");
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
                    log_debug("looping in setting socket");
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
                    log_debug("in setting socket");
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            log_debug("Closing after bind");
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

        if (p == NULL)  {
        log_debug("null");
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, 10) == -1) {
        log_debug("Listening");
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        log_debug("sigaction");
        perror("sigaction");
        exit(1);
    }

    log_trace("server: waiting for connections...\n");

    return sockfd;
}

// Listen on the provided server socket for incoming clients. When a client
// connects, return the client socket file descriptor. This is a blocking
// call. If an error occurs, return a -1.
int http_server_accept(int socket) {
    log_trace("Server is now Accepting connection on socket %d", socket);
    // var for the socket
    int finalSockfd;
    // Variable to recognize client
    struct sockaddr_in cli;
    // Now server is ready to listen and verification
    socklen_t len = sizeof(cli);
    // Accept the data packet from client and verification
    finalSockfd = accept(socket, (struct sockaddr *)&cli, &len);

    if (finalSockfd < 0) {
        log_error("server acccept failed...\n");
        return -1;
    } else
        log_info("server acccepted the client...\n");

    // server should be running and connected to a client on theis socket
    return finalSockfd; // FIX ME
}

// Read data from the provided client socket, parse the data, and return a
// Request struct. This function will allocate the necessary buffers to fill
// in the Request struct. The buffers contained in the Request struct must be
// freed using http_server_client_cleanup. If an error occurs, return an empty
// request and this function will free any allocated resources.
Request http_server_receive_request(int socket) {
    log_trace("Receiving request");

    uint32_t timeOutCounter = ZERO_RESET_INIT_VALUE;
    // initalizing the request struct
    Request newRequest;
    newRequest.headers = NULL;
    newRequest.method = NULL;
    newRequest.num_headers = ZERO_RESET_INIT_VALUE;
    newRequest.path = NULL;
    // sentinal values used for checking for complete requests
    char *sentinal = "\r\n\r\n\0";
    //sie from lab 1
    size_t startingSize = LAB1_BUFFER_SIZE;
    //allocated buffer saving the request
    char *dynamicBuffer = malloc(sizeof(char) * startingSize);
    //total received bytes
    size_t receivedAll = ZERO_RESET_INIT_VALUE;

    while (1) {
        //reallocate a bigger buffer if the current one is almost filled up
        if (receivedAll >= (MAX_ACCEPTABLE_BUFFER_CAPACITY_PERCENTAGE * startingSize)) {
            startingSize *= 2;
            dynamicBuffer = realloc(dynamicBuffer, sizeof(char) * startingSize);
            ///error checking for buffer validity
            if (dynamicBuffer == NULL) {
                free(dynamicBuffer);
                log_error("Buffer failed while receiving..");
                newRequest.headers = NULL;
                newRequest.method = NULL;
                newRequest.num_headers = ZERO_RESET_INIT_VALUE;
                newRequest.path = NULL;
                return newRequest;
            }
        }

        //receive from client
        int charsReceived = recv(socket, (dynamicBuffer + receivedAll),
                                 (startingSize - receivedAll), ZERO_RESET_INIT_VALUE);

        //error checking for recv function return
        if (charsReceived == -1) {
            log_error("Read function failed in receiving..");
            free(dynamicBuffer);
            newRequest.headers = NULL;
            newRequest.method = NULL;
            newRequest.num_headers = ZERO_RESET_INIT_VALUE;
            newRequest.path = NULL;
            return newRequest;
        }
        //accumelate the received bytes
        receivedAll += charsReceived;
        //null terminate it for printing
        dynamicBuffer[receivedAll] = NULL_TERMINATOR;

        //check if strings match
        if (strcmp(&dynamicBuffer[receivedAll - 4], sentinal) == STRINGS_MATCH) {
            log_trace("Received complete request");
            break;
        } else if (timeOutCounter >= 100000 &&
                   charsReceived == ZERO_RESET_INIT_VALUE) { // did not receive
                                                             // anything for a while
            log_error("Connection timeout, now disconncing");
            newRequest.headers = NULL;
            newRequest.method = NULL;
            newRequest.num_headers = ZERO_RESET_INIT_VALUE;
            newRequest.path = NULL;
            return newRequest;
        } else {
            timeOutCounter++;
        }
    }

    //return the parsed request that was just received
    return http_server_parse_request(dynamicBuffer);
}


//I used this function from Dr. Lundrigan when he helped me

//this function takes in a socket, buffer/data and the length of ther buffer/bytes to send
int send_all(int socket, char *data, int length) {
    // Send all of the data
    int total_bytes_sent = 0;
    int bytes_sent = 0;
    //keep sending until the bytes sent are not less than the total bytes
    while (total_bytes_sent < length) {
        bytes_sent = send(socket, data + total_bytes_sent, length - total_bytes_sent, 0);

        if (bytes_sent == -1) {
            log_error("Send all failed");
            return -1;
        }

        total_bytes_sent += bytes_sent;
        //log_debug("Bytes sent: %d (%d/%d)", bytes_sent, total_bytes_sent, length);
    }
    log_debug("Done sending...");

    return 0;
}


// Sends the provided Response struct on the provided client socket.
int http_server_send_response(int socket, Response response) {
log_trace("Server sending back the response..");
    //getting the headerlength for sending the header
    int headerLength =
        snprintf(NULL, ZERO_RESET_INIT_VALUE, "%s\r\n%s: %s\r\n\r\n", response.status,
                 response.headers[ZERO_RESET_INIT_VALUE]->name, response.headers[ZERO_RESET_INIT_VALUE]->value);
    //allocated memory for storing header
    char *myHeader = malloc(headerLength + 1);
    //formatting the string
    snprintf(myHeader, headerLength + 1, "%s\r\n%s: %s\r\n\r\n", response.status,
             response.headers[0]->name, response.headers[0]->value);


    log_info("Combined header before sending is %s", myHeader);

    send_all(socket, myHeader, headerLength);

    //null terminating it for printing
    char responseMem[HTTP_SERVER_FILE_CHUNK];
    //size of response
    size_t responseSize = ZERO_RESET_INIT_VALUE;
    //return value from the send function
    int sendStatus = ZERO_RESET_INIT_VALUE;
    int totalSent = 0;
    //do while loop for sending at least one chunk before chekcing the condition
    do {
        //reading from the file into the response memory and breaking down the file chuncks into 1024 so can be sent one at a time
        responseSize = fread(responseMem, sizeof(char), HTTP_SERVER_FILE_CHUNK, response.file);
        //sending the chunk
        sendStatus = send_all(socket, responseMem, responseSize);
        totalSent += 1024;
        log_trace("total sent is %d \n", totalSent);
    } while (responseSize == HTTP_SERVER_FILE_CHUNK);

    return EXIT_SUCCESS;
}

// Closes the provided client socket and cleans up allocated resources.
void http_server_client_cleanup(int socket, Request request, Response response) {
    //closing socket and keeping the status
    int closed = close(socket);
    //checking the status of the closed socket
    if (closed) {
        log_error("Client cleanup could not close rthe socket properly");
    }
    log_trace("Client cleanup closed socket successfully");
    //freeing request method if it's allocated
    if (request.method != NULL) {

        free(request.method);
        log_trace("Request method freed successfully");
    }
    //freeing request path if it's allocated
    if (request.path != NULL) {

        free(request.path);
        log_trace("Request path freed successfully");
    }
    //freeing request headers if it's allocated
    if (request.headers != NULL) {
        for (int i = 0; i < request.num_headers; i++) {
            free(request.headers[i]->name);
            free(request.headers[i]->value);
            free(request.headers[i]);
        }
        free(request.headers);
        log_trace("Request headers freed succesfully");
    }
    //freeing response status if it's allocated
    if (response.status != NULL) {
        free(response.status);
        log_trace("Response status freed successfuly");
    }
    //freeing response file if it's allocated
    if (response.file != NULL) 
    {
        fclose(response.file);
        log_trace("Allocated memory for file freed succesfully");
    }
    //freeing response headers if it's allocated
    if (response.headers != NULL) {
        for (int i = 0; i < response.num_headers; i++) {
            free(response.headers[i]->name);
            free(response.headers[i]->value);
            free(response.headers[i]);
        }
        free(response.headers);
        log_trace("Response headers freed successfully");
    }
    log_trace("Client cleanup done and exiting succesfully");
}

// Closes provided server socket
void http_server_cleanup(int socket) {
    log_trace("Closing socket connection and cleaning up");
    //close the socket and keeping status update
    int status = close(socket);
    //cheking the status and reporting if any error happened
    if (status != ZERO_RESET_INIT_VALUE) {
        log_error("Server socket did not close successfully");
    }
    log_trace("Closed socket successfully");
}

////////////////////////////////////////////////////
//////////// PROTOCOL RELATED FUNCTIONS ////////////
////////////////////////////////////////////////////

// A helper function to be used inside of http_server_receive_request. This
// should not be used directly in main.c.
Request http_server_parse_request(char *buf) {
    log_trace("Parsing the request...");
    //preloading the request to be empty
    Request newRequest;
    newRequest.headers = NULL;
    newRequest.method = NULL;
    newRequest.num_headers = -400;
    newRequest.path = NULL;
    char *context = NULL;
    char *header;
    int headerCount = 10;
    //allocate memory for the request header
    newRequest.headers = malloc(sizeof(Header *) * headerCount);
    //finding the \r\n
    strtok_r(buf, "\r\n", &context);
    //finding the methof by locating the space
    char *localMethod = strtok(buf, " ");

    //if error happened return request
    if (localMethod == NULL) {
        log_error("The header was invalid");
        newRequest.headers = NULL;
        newRequest.method = NULL;
        newRequest.num_headers = -400;
        newRequest.path = NULL;
        return newRequest;
    }

    //allocate memory for the request method
    newRequest.method = malloc(HTTP_SERVER_MAX_HEADER_SIZE);
    //copying the data into the allocated memory
    memcpy(newRequest.method, localMethod, strlen(localMethod) + 1);
    //finding the local path by finding the next space
    char *localPath = strtok(NULL, " ");
    //error checking
    if (localMethod == NULL) {
        log_error("Path was invalid");
        newRequest.headers = NULL;
        newRequest.method = NULL;
        newRequest.num_headers = -400;
        newRequest.path = NULL;
        return newRequest;
    }
    //allocating memory for the request path
    newRequest.path = malloc(HTTP_SERVER_MAX_HEADER_SIZE);
    //copying the data into the memory
    memcpy(newRequest.path, localPath, strlen(localPath) + 1);
    //finding the http version
    char *currentHTTPVersion = strtok(NULL, " ");
    //error checking if not found
    if (currentHTTPVersion == NULL) {
        log_error("The current http version is invalid");
        newRequest.headers = NULL;
        newRequest.method = NULL;
        newRequest.num_headers = -400;
        newRequest.path = NULL;
        return newRequest;
    }

    newRequest.num_headers = 1;

    while (1) {
        //saving the header
        header = strtok_r(NULL, "\r\n", &context);
        //error checking
        if (header == NULL) {
            newRequest.num_headers--;
            log_info("Parsing done");
            return newRequest;
        }
        //reallocate the memory for the headers if it's almost filled up
        if (newRequest.num_headers == (headerCount - 2)) {
            headerCount *= 2;
            newRequest.headers = realloc(newRequest.headers, sizeof(Header *) * headerCount);
        }
        //place holder/index for headers
        int placeHolder = newRequest.num_headers - ONE_VALUE;
        //allocating memory for the index
        newRequest.headers[placeHolder] = malloc(sizeof(Header));

        char *name = strtok(header, ":");

        if (name == NULL) {
            log_error("header error");
            newRequest.headers = NULL;
            newRequest.method = NULL;
            newRequest.num_headers = -400;
            newRequest.path = NULL;
            return newRequest;
        }
        //allocate the haeder for the index name 
        newRequest.headers[placeHolder]->name = malloc(HTTP_SERVER_MAX_HEADER_SIZE);
        //copy data in the memory
        memcpy(newRequest.headers[placeHolder]->name, name, strlen(name) + ONE_VALUE);

        char *value = strtok(NULL, "\r\n");
        if (value == NULL) {
            log_error("invalid header");
            newRequest.headers = NULL;
            newRequest.method = NULL;
            newRequest.num_headers = -400;
            newRequest.path = NULL;
            return newRequest;
        }

        newRequest.headers[placeHolder]->value = malloc(HTTP_SERVER_MAX_HEADER_SIZE);
         //allocate the haeder for the index name 
        memcpy(newRequest.headers[placeHolder]->value, &value[1], strlen(value));
         //copy data in the memory
        newRequest.num_headers++;
    }
}

// Convert a Request struct into a Response struct. Use relative_path to
// determine the path of the file being requested. This function will allocate
// the necessary buffers to fill in the Response struct. The buffers contained
// in the Resposne struct must be freeded using http_server_client_cleanup. If
// an error occurs, an empty Response will be returned and this function will
// free any allocated resources.
Response http_server_process_request(Request request, char *relative_path) {
    log_trace("Processing client request");
    //creating and preloading empty reponse
    Response newResponse;
    newResponse.file = NULL;
    newResponse.headers = NULL;
    newResponse.num_headers = ZERO_RESET_INIT_VALUE;
    newResponse.status = NULL;
    char *fPath;

    log_trace("Checking request method..");

    //if the request method exists
    if (request.method != NULL) {
        //getting the length of the file path string
        size_t lengthOfFilePath = strlen(relative_path) + strlen(request.path);
        //allocate meory for the file path
        fPath = malloc(sizeof(char) * (lengthOfFilePath + 1));
        //copy the data in memory
        memcpy(fPath, relative_path, strlen(relative_path));
        //copy the data in th request path struct
        memcpy(&fPath[strlen(relative_path)], request.path, strlen(request.path));
        //terminate the string for printing
        fPath[lengthOfFilePath] = NULL_TERMINATOR;
        log_info("The file path is %s", fPath);
        struct stat myStat;

        //checking the nature of the path
        if (stat(fPath, &myStat) == ZERO_RESET_INIT_VALUE) {
            //if directory
            if (myStat.st_mode & S_IFDIR) {
                log_error("This is a directeory not a file");
                newResponse.status = malloc(23);
                memcpy(newResponse.status, "HTTP/1.1 403 Forbidden", 22);
                newResponse.status[22] = NULL_TERMINATOR;

                newResponse.file = fopen("www/403.html", "r");
            } else if (myStat.st_mode & S_IFREG) { //if it is a file
                newResponse.file = fopen(fPath, "r");
                if (newResponse.file == NULL) {
                    log_trace("Couldn't open file");
                    newResponse.status = malloc(23);
                    memcpy(newResponse.status, "HTTP/1.1 404 Not Found", 22);
                    newResponse.status[22] = NULL_TERMINATOR;

                    newResponse.file = fopen("www/404.html", "r");
                } else if (strcmp(request.method, "GET") == STRINGS_MATCH) { //if its a get request for a file
                    log_trace("everything ran ok");
                    newResponse.status = malloc(16);
                    memcpy(newResponse.status, "HTTP/1.1 200 OK", 15);
                    newResponse.status[15] = NULL_TERMINATOR;
                    log_debug("reponse message is %s", newResponse.status);
                } else { //otherwise it's unauthorized
                    log_trace("405 unauthorized");
                    newResponse.status = malloc(32);
                    memcpy(newResponse.status, "HTTP/1.1 405 Method Not Allowed", 31);
                    newResponse.status[31] = NULL_TERMINATOR;

                    newResponse.file = fopen("www/405.html", "r");
                }
            }
        } else { //error checking for the path/request check failure
            log_error("something failed %s", strerror(errno));

            newResponse.status = malloc(23);
            memcpy(newResponse.status, "HTTP/1.1 404 Not Found", 22);
            newResponse.status[22] = NULL_TERMINATOR;

            newResponse.file = fopen("www/404.html", "r");
        }
    }

    size_t fLen = ZERO_RESET_INIT_VALUE;
    //checking the validity of the file
    if (newResponse.file != NULL) {
        log_trace("Seeking in file");
        //seek/look until the end of the file
        fseek(newResponse.file, 0L, SEEK_END);
        //getting the length of the file
        fLen = ftell(newResponse.file);
        //setting the pointer to the beginning of the file again
        rewind(newResponse.file);
    }

    log_info("the flen is %d", fLen);

    newResponse.num_headers = 1;
    //allocating mem for he number of header present
    newResponse.headers = malloc(sizeof(Header *) * newResponse.num_headers);
    //allocate memory for the first spot of the headers array
    newResponse.headers[0] = malloc(sizeof(Header));
    //alocate memory for the data name 
    newResponse.headers[ZERO_RESET_INIT_VALUE]->name = malloc(sizeof(char) * 15);
    //filling the memory with the hard coded name
    memcpy(newResponse.headers[ZERO_RESET_INIT_VALUE]->name, "Content-Length", 14);
    //null termination for printing
    newResponse.headers[ZERO_RESET_INIT_VALUE]->name[14] = NULL_TERMINATOR;
    //getting the length of the value
    size_t valLen = snprintf(NULL, 0, "%ld", fLen);
    //allocating memory for the value of the data spot
    newResponse.headers[0]->value = malloc(sizeof(char) * (valLen + 1));
    //formatting the string memory
    snprintf(newResponse.headers[0]->value, valLen + 1, "%ld", fLen);
    return newResponse;
}

