/************************ COMPUTER SYSTEMS PROJECT 1 ***********************/
/* Christopher Chapman 760426
 * Computer Systems Project 1
 *
 * A simple server using TCP that implements HTTP 1.0.
 * Can be used to create a server on local machine
 * Implements GET for HTML, JPEG, CSS and JS files.
 * Outputs codes 200 and 404.
 
 * Takes arguments: portnumber  -> The Port number to put the server on
 *                  root path   -> The root dir for finding objects
 * Used client.c from Lab 5 as a baseline code
 */
/***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>

#define HTTP_VERSION "HTTP/1.0"

// HTTP code identifiers
#define OK 200
#define NOT_OK 404

// OUTPUT for code
#define OK_STR "200 OK"
#define NOT_OK_STR "404"

#define BUFFER_MAX 20000

// File endings
#define HTML    "html"
#define JPEG    "jpg"
#define CSS     "css"
#define JS      "js"        // javascript

#define HTML_MIME   "text/html"
#define JPEG_MIME   "image/jpeg"
#define CSS_MIME    "text/css"
#define JS_MIME     "application/javascript"

#define PATH_LEN    200

char path[PATH_LEN];


/***************************** FUNCTION HEADERS *****************************/
void write_test_log(char *out);
void error_out(const char *error_message);
char* create_header(char *http_version, char *http_code);
void send_response(int sockfd);
char* create_get(FILE *file, int sockfd);
void get_path(char* new_path, char* path, char* in);
char* create_mime(char *path);


/***************************************************************************/
// When an error occurs, prints out <error_message> and quit the process
void error_out(const char *error_message) {
    perror(error_message);
    exit(1);
}


/***************************************************************************/
/* Takes input http_version (the version used) and http status code (eg.404)
 * and returns a single string containing the correct header format
 * in order to create an HTTP header in format HTTP(VER) (CODE)\r\n
 */
char * create_header(char *http_version, char *http_code) {
    char *header = malloc(BUFFER_MAX);
    printf("Creating header\n");
    
    // Concatenates the header together
    strcpy(header, http_version);
    strcat(header, " ");
    strcat(header, http_code);
    strcat(header, "\r\n");
    
    printf("create_header - returning: %s", header);
    return header;
}


/***************************************************************************/
// Writes a binary stream from a file (file) to a given socket (sockfd).
char* create_get(FILE *file, int sockfd) {
    char* out;
    out = (char *)malloc(BUFFER_MAX * sizeof(char));
    int filesize;
    
    // Read thorugh the file and store the filesize
    filesize = fread(out, sizeof(char), BUFFER_MAX, file);
    printf("filesize = %d\n", filesize);
    
    // Add a delimiter for strings
    out[filesize] = '\0';
    printf("Output = %s\n", out);
    printf("final char = %c\n", out[filesize-1]);
    
    // Write binary stream out to the socket
    write(sockfd, out, filesize);
    return out;
}


/***************************************************************************/
/* Returns a string containing the mime header for HTTP protocol.
 * Takes the requested path and based on the requested filetype,
 * returns header containing Content-Type: <information type>
 * Mime types can be found: https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/MIME_types
 */
char* create_mime(char *path) {
    printf("Running create_get\n");
    char *temp = malloc(1000);
    char *out = malloc(1000);
    
    strcpy(temp, path); // Uses copy of path so that changes not done in place
    
    // Get the requested fileytype (eg. will output css for URL/PATH.css)
    temp = strtok(temp,".");
    temp = strtok(NULL, ".");
    
    // Create the string in the form Content-Type: <information type>
    strcpy(out, "Content-Type: ");
    
    // Choose the mime type given a filetype using Mime Types from link above.
    if (strcmp(temp, HTML) == 0) {
        strcat(out, HTML_MIME);
    }  else if (strcmp(temp, JPEG) == 0) {
        strcat(out, JPEG_MIME);
    } else if (strcmp(temp, CSS) == 0){
        strcat(out, CSS_MIME);
    } else if (strcmp(temp, JS) == 0) {
        strcat(out, JS_MIME);
    } else {
        error_out("Unknown type");
    }
    
    // Creates space between the mime header and the content
    strcat(out, "\r\n\r\n");
    printf("create get - returning: %s\n", out);
    return out;
}


/***************************************************************************/
/* After receieving a request from client, responds with:
 * HTTP header, Mime-type and file content.
 * Takes sockfd - the socket to send information to
 * Writes out to sockfd, then closes
 */
void send_response(int sockfd) {
    char buffer[BUFFER_MAX];
    char curr_path[PATH_LEN];   // The current path to avoid in place changes
    FILE *fp = NULL;
    int n;
    char out[BUFFER_MAX];
    
    // Zeroing buffer and out
    bzero(buffer, BUFFER_MAX);
    bzero(out, BUFFER_MAX);
    
    // Read characters from the connection and respond to errors
    n = read(sockfd,buffer,sizeof(buffer));
    if (n < 0) { error_out("ERROR reading from socket"); }
    
    // Get the path to the requested directory (curr_path in place)
    get_path(curr_path, path, buffer);
    
    // Open file given by curr_path
    // If the file does not exist, 404 and end
    fp = fopen(curr_path, "r");
    if (fp == NULL) {
        strcpy(out, create_header(HTTP_VERSION, NOT_OK_STR));
        n = write(sockfd, out, 1024);
        return;
    }
    
    // For valid file name - Creating the message output
    strcpy(out, create_header(HTTP_VERSION, OK_STR));
    strcat(out, create_mime(curr_path));
    
    // Write header and mime types to requester
    n = write(sockfd, out, strlen(out));    // change the 1000
    create_get(fp, sockfd);
    
    // write_test_log(out); // uncomment for printing header info to file
    fclose(fp);
}


/***************************************************************************/
/* Takes the root path (path), HTTP request from the buffer (in)
 * and generates the required file path for file io at new path.
 * new_path is path/requested path.
 * Used: https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm
 # for strtok
 */
void get_path(char* new_path, char* path, char* in) {
    char token[100];
    char buffer[BUFFER_MAX];
    
    // Uses strtok to get the given path, which is the third word in the request
    strcpy(new_path, path);
    strcpy(buffer, in);
    
    // Gets third (token) accross
    strcpy(token, strtok(buffer, " "));
    strcpy(token, strtok(NULL, " "));
    
    // Cats root_path/<token>
    strcat(new_path, token);
}


/***************************************************************************/
/* test fn
void write_test_log(char *out) {
    FILE *my_file = fopen("my_test_log.txt", "w");
    fprintf(my_file, "%s", out);
}*/


/***************************************************************************/
int main(int argc, char **argv) {
    int sockfd, newsockfd, portno;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    
    // For when I forget what the inputs are
    if (argc < 3) {
        fprintf(stderr,"ERROR, please provide port and root path\n");
        exit(1);
    }
    
    // Create TCP socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { error_out("ERROR opening socket"); }
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    
    // reading in input from stdio
    portno = atoi(argv[1]);
    strcpy(path,argv[2]);
    
    // Create address we're going to listen on given port number
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);  // Store in machine-neutral format
    
    // Bind address to the socket
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error_out("ERROR on binding");
    }
    
    // Listen on socket - means we're ready to accept connections -
    // incoming connection requests will be queued
    listen(sockfd,5);
    
    clilen = sizeof(cli_addr);
    
    // Accept a connection and stay open
    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) { error_out("ERROR on accept"); }
        
        // Respond to message from newsockfd
        send_response(newsockfd);
        
        // Close the current socket
        close(newsockfd);
    }
    
    // Close socket
    close(sockfd);
    
    return 0;
}

