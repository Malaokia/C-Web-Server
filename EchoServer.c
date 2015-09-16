#include<limits.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<errno.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<sys/stat.h>
#define ERROR -1
#define MAX_CLIENTS 32
#define MAX_DATA 1024
#define MAX_RESPONSE 1024*1024
#define MAX_PATH_LENGTH 512

#define NUM_THREADS 4


#define NOT_FOUND 404
#define BAD_METHOD 4040
#define BAD_URI 4041
#define BAD_HTTP_VERSION 4042


/*--------------/
 * HEADER STUFF
 *--------------*/

/* Struct Definitions */
struct HTTP_RequestParams {
  char *method;
  char *URI;
  char *httpversion;
};

struct TextfileData {
  int port_number;
  char document_root[MAX_PATH_LENGTH];
  char default_web_page[20];
};

/* Function Declarations */
void send_response(int client, int status_code, struct HTTP_RequestParams *params);
int handle_file_serving(char *path, char *body);
void client_handler(int client);
void interpret_request(struct HTTP_RequestParams *params, int *decision);
void extract_request_path(char *response, struct HTTP_RequestParams *params);
void removeSubstring(char *s,const char *toremove);
int setup_socket (int port_number, int max_clients);
void setup_server(struct TextfileData *config_data);



int main(int argc, char ** argv)
{
  printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
  printf("| Welcome to this wonderful C server! |\n");
  printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");


  int main_socket;
  int pid;
  struct TextfileData system_config_data;
  struct sockaddr_in client;
  int cli;
  unsigned int sockaddr_len = sizeof(struct sockaddr_in);


  setup_server(&system_config_data); 
  printf("CURRENT SERVER SETUP: \n");
  printf("Port Number: %d\n", system_config_data.port_number);
  printf("Document Root: %s\n", system_config_data.document_root);
  printf("Default Web Page: %s\n", system_config_data.default_web_page);
  main_socket = setup_socket(system_config_data.port_number, MAX_CLIENTS);

  while (1)  {

    if ( (cli = accept(main_socket, (struct sockaddr *)&client, &sockaddr_len)) < 0) {
      perror("ERROR on accept");
      exit(1);
    }

    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("New Client connected from port number %d and IP %s\n", ntohs(client.sin_port), inet_ntoa(client.sin_addr));

    pid = fork();
    if (pid < 0){
      perror("ERROR on fork");
      exit(1);
    }

    if (pid == 0) {
      close(main_socket);
      client_handler(cli);
      exit(0);
    }

    if (pid > 0) {
      close(cli);
    }
  }
}

void client_handler(int client) {

  printf("Hello from client_handler with process number: %d\n", (int) getpid());

  struct HTTP_RequestParams request_params;
  int what_to_do;

  char response[] = "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n\r\n"
    "<!DOCTYPE html><html><head><title>Bye-bye baby bye-bye</title>"
    "<style>body { background-color: #111 }"
    "h1 { font-size:4cm; text-align: center; color: black;"
    " text-shadow: 0 0 2mm blue}</style></head>"
    "<body><h1>Hello World</h1></body></html>\r\n";
  int cli = client;
  int data_len;
  char data[MAX_DATA];
  char file_response[MAX_DATA];
  char file_path[MAX_PATH_LENGTH];

  data_len = recv(cli, data, MAX_DATA, 0);
  if (data_len) {
    //printf("Request received of size: %zd\n", data_len);
    //printf("Request: \n%s\n", data);
    extract_request_path((char *)&data, &request_params);
    printf("Here are the results of parsing the request body and reading the data into our struct:::\n");
    printf("METHOD: %s\n", request_params.method);
    printf("URI: %s\n", request_params.URI);
    printf("VERSION: %s\n", request_params.httpversion);
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");
    write(cli, response, sizeof(response) -1);
    /*if ((handle_file_serving((char *)&request_params.URI, (char *)&file_response)) == 1) {
      printf("File wasn't found, need to print 404...\n");
      send_response(cli, 404, &request_params);
    }
    */
  }
}
/*--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 * send_response - this function will be the function that actually sends a message to the client. This message will contain the proper headers and the respective body content
 *--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 */
void send_response(int client, int status_code, struct HTTP_RequestParams *params) {
  printf("Beginning of: send_response()\n");
  char actual_response [MAX_RESPONSE];

  char not_found[] = "HTTP/1.1 404 Not Found: %s\r\n Content-Type: text/html; charset=UTF-8\r\n\r\n <!DOCTYPE html><html><head><title>404 Error</title> <body><h1>404 Not Found</h1></body></html>\r\n";
  char invalid_uri[] = "HTTP/1.1 400 Bad Request: Invalid URI: %s\r\n Content-Type: text/html; charset=UTF-8\r\n\r\n <!DOCTYPE html><html><head><title>400 Bad Request</title> <body><h1>400 Bad Request</h1></body></html>\r\n";
  char invalid_method[] = "HTTP/1.1 400 Bad Request: Invalid Method: %s\r\n Content-Type: text/html; charset=UTF-8\r\n\r\n <!DOCTYPE html><html><head><title>400 Bad Request</title> <body><h1>400 Bad Request</h1></body></html>\r\n";
  char invalid_version[] = "HTTP/1.1 400 Bad Request: Invalid HTTP-Version: %s\r\n Content-Type: text/html; charset=UTF-8\r\n\r\n <!DOCTYPE html><html><head><title>400 Bad Request</title> <body><h1>400 Bad Request</h1></body></html>\r\n";

  switch (status_code)
  {
    case NOT_FOUND:
      printf("Printing error page");
      sprintf((char *)&actual_response, not_found, params->URI);
      write(client, actual_response, sizeof(actual_response) -1);
      break;
    case BAD_METHOD:
      break;
    case BAD_URI:
      break;
    case BAD_HTTP_VERSION:
      break;
  }

  //printf("This is what the body is returning: %s\n", body);
  //write(client, body, sizeof(body) -1);
  printf("Leaving: send_response()\n");


}

/*--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 * handle_file_serving - this function will take in a file path, and either construct the correct response body to serve up that file or it will return false if the file does not exist
 *--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 */
int handle_file_serving(char *path, char *body) {
  printf("Beginning of: handle_file_serving()\n");

  char bad_response[] = "HTTP/1.1 404 Not Found\r\n Content-Type: text/html; charset=UTF-8\r\n\r\n <!DOCTYPE html><html><head><title>404 Error</title> <body><h1>404 Error</h1></body></html>\r\n";
  struct stat buffer;
  if ((stat ("www/images/weome.png", &buffer) == 0)) {
    printf("YAY FILE EXISTS\n");
    printf("Time to construct the file and serve it up...");
    return 0;
  }
  else{
    printf("File not found\n");
    strcpy(body, bad_response);
    return 1;
  }
  printf("Leaving: handle_file_serving()\n");

}


/*-------------------------------------------------------------------------------------------------------------------------------------------
 * interpret_request - this function will be take the users path and decide what to do based on the result
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void interpret_request(struct HTTP_RequestParams *params, int *decision) {

  printf("Beginning of: interpret_request()\n");

  if ( (strcmp(params->URI, "/")) == 0)
  {
    printf("Empty Path!!");
    *decision = 0;
  }
  else {
    printf("We have a path!!");
    *decision = 1;
  }
  printf("Leaving: interpret_request()\n");
}
/*-------------------------------------------------------------------------------------------------------------------------------------------
 * extract_request_path - this function will be mainly responsible for parsing and extracting the path from the HTTP request from the client
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
//void extract_request_path(char *response, char *return_path) {
void extract_request_path(char *response, struct HTTP_RequestParams *params) {
  printf("Beginning of: extract_request_path()\n");
  char *saveptr;
  char *the_path;

  // after this returns, return_path contains the first token while saveptr is a reference to the remaining tokens 
  the_path = strtok_r(response, "\n", &saveptr);
  //removeSubstring(the_path, "HTTP/1.1");
  //printf( "First Token after seperating response by newline: \n%s\n", the_path);
  //printf( "Remaining Tokens after seperating response by newline: \n%s\n", saveptr);

  the_path = strtok_r(the_path, " ", &saveptr);
  params->method = the_path;
  //printf( "First token after seperation by space character: \n%s\n", the_path );
  //printf( "Remaining tokens after seperation by space character: \n%s\n", saveptr );

  the_path = strtok_r(NULL, " ", &saveptr);
  //printf( "Second token after seperation by space character: %s\n", the_path );
  params->URI = the_path;
  //printf( "Remaining tokens after second seperation by space character: \n%s\n", saveptr );
  params->httpversion = saveptr;
  //printf("Leaving: extract_request_path()\n");
}

/*-------------------------------------------------------------------------------------------------------------------------------------------
 * removeSubstring - this function is a helper function that is used when extracting the path that the client sends a GET request on
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void removeSubstring(char *s,const char *toremove)
{
  while( (s=strstr(s,toremove)) )
    memmove(s,s+strlen(toremove),1+strlen(s+strlen(toremove)));
}

/*----------------------------------------------------------------------------------------------
 * setup_server - first function called on entry, prints information and reads in config file 
 *----------------------------------------------------------------------------------------------
 */
void setup_server(struct TextfileData *config_data)
{
  printf("Beginning of: setup_server()\n");

  char *current_path;
  char buff[PATH_MAX + 1];
  char conf_file_path[PATH_MAX + 1];
  char read_line[200];
  char *saveptr;
  char *port_number;
  char *current_line;
  current_path = getcwd(buff, PATH_MAX + 1);

  strcpy(conf_file_path, current_path);
  strcat(conf_file_path, "/ws.conf");

  printf("The file path of the conf file is %s\n", conf_file_path);

  FILE *config_file;
  config_file = fopen(conf_file_path, "r");
  if (config_file == NULL)
    printf("File was unable to be opened\n");
  else
    printf("File succesfully opened!!");

  int counter = 1;
  while (fgets (read_line,200, config_file) != NULL)
  {
    if (counter == 2) {
      current_line = strtok_r(read_line, " ", &saveptr);
      printf("This should be the second token: %s\n", saveptr);
      config_data->port_number = atoi(saveptr);
    }
    if (counter == 4) {
      current_line = strtok_r(read_line, " ", &saveptr);
      removeSubstring(saveptr, "\"");
      removeSubstring(saveptr, "\n");
      printf("This should be the second token: %s\n", saveptr);
      strcpy(config_data->document_root, saveptr);
    }

    if (counter == 6) {
      current_line = strtok_r(read_line, " ", &saveptr);
      printf("This should be the second token: %s\n", saveptr);
      strcpy(config_data->default_web_page, saveptr);
    }

    counter++;
  }


  printf("Leaving: setup_server()\n\n");
}

/*----------------------------------------------------------------------------------------------
 * setup_socket - allocate and bind a server socket using TCP, then have it listen on the port
 *----------------------------------------------------------------------------------------------
 */
int setup_socket(int port_number, int max_clients)
{
  printf("Beginning of: setup_socket()\n");

  /* The data structure used to hold the address/port information of the server-side socket */
  struct sockaddr_in server;

  /* This will be the socket descriptor that will be returned from the socket() call */
  int sock;

  /* Socket family is INET (used for Internet sockets) */
  server.sin_family = AF_INET;
  /* Apply the htons command to convert byte ordering of port number into Network Byte Ordering (Big Endian) */
  server.sin_port = htons(port_number);
  /* Allow any IP address within the local configuration of the server to have access */
  server.sin_addr.s_addr = INADDR_ANY;
  /* Zero off remaining sockaddr_in structure so that it is the right size */
  memset(server.sin_zero, '\0', sizeof(server.sin_zero));

  /* Allocate the socket */
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == ERROR)
  {
    perror("server socket: ");
    exit(-1);
  }


  /* Bind it to the right port and IP using the sockaddr_in structuer pointed to by &server */
  if ((bind(sock, (struct sockaddr *)&server, sizeof(struct sockaddr_in))) == ERROR)
  {
    perror("bind : ");
    exit(-1);
  }

  /* Have the socket listen to a max number of max_clients connections on the given port */
  if ((listen(sock, max_clients)) == ERROR)
  {
    perror("Listen");
    exit(-1);
  }
  printf("Leaving: setup_socket()\n\n");
  return sock;
}
