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


#define FALSE 0
#define TRUE 1
#define NOT_FOUND 404
#define BAD_METHOD 4040
#define BAD_URI 4041
#define BAD_HTTP_VERSION 4042
#define NUM_OF_FILE_TYPES 4


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
  char extensions[5][512];
  char encodings [5][512];
  const char *png[2];
  const char *gif[2];

};

/* Function Declarations */
void send_response(int client, int status_code, struct HTTP_RequestParams *params, char *full_path);
int handle_file_serving(char *path, char *body, struct TextfileData *config_data, int *result_status);
void client_handler(int client, struct TextfileData *config_data);
int interpret_request(struct HTTP_RequestParams *params, int *decision);
void extract_request_path(char *response, struct HTTP_RequestParams *params);
void removeSubstring(char *s,const char *toremove);
int setup_socket (int port_number, int max_clients);
void setup_server(struct TextfileData *config_data);
void construct_file_response(char *full_path, int client);



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
  //printf("HTTP Encoding Details: %s, %s\n", system_config_data.extensions[0], system_config_data.encodings[0]);
  //printf("Text Encoding Details: %s, %s\n", system_config_data.extensions[1], system_config_data.encodings[1]);
  //printf("PNG Encoding Details: %s, %s\n", system_config_data.extensions[2], system_config_data.encodings[2]);
  //printf("GIF Encoding Details: %s, %s\n", system_config_data.extensions[3], system_config_data.encodings[3]);
  main_socket = setup_socket(system_config_data.port_number, MAX_CLIENTS);

  while (1)  {

    if ( (cli = accept(main_socket, (struct sockaddr *)&client, &sockaddr_len)) < 0) {
      perror("ERROR on accept");
      exit(1);
    }

    //printf("New Client connected from port number %d and IP %s\n", ntohs(client.sin_port), inet_ntoa(client.sin_addr));

    pid = fork();
    if (pid < 0){
      perror("ERROR on fork");
      exit(1);
    }

    if (pid == 0) {
      close(main_socket);
      client_handler(cli, &system_config_data);
      exit(0);
    }

    if (pid > 0) {
      //printf("Disconnecting client\n");
      close(cli);
    }
  }
}

void client_handler(int client, struct TextfileData *config_data) {

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
  int data_len;
  int decision;
  char data[MAX_DATA];
  int status_code;
  char absolute_file_path[MAX_DATA];
  char file_path[MAX_PATH_LENGTH];

  data_len = recv(client, data, MAX_DATA, 0);
  if (data_len) {
    //printf("Request received of size: %zd\n", data_len);
    //printf("Request: \n%s\n", data);
    extract_request_path((char *)&data, &request_params);
    printf("Results of parsing the request body: METHOD: %s  URI: %s  VERSION: %s\n", request_params.method, request_params.URI, request_params.httpversion);
    if (interpret_request(&request_params, &status_code))
      send_response(client, status_code, &request_params, (char *)&absolute_file_path);
    else {
      handle_file_serving( (request_params.URI), (char *)&absolute_file_path, config_data, &status_code);
      //printf("Returned status code is %d\n", status_code);
      send_response(client, status_code, &request_params, (char *)&absolute_file_path);
    }
    //printf("Checking to see if the file path is being transferred. This is what gets stored in absolute_file_path: %s\n", absolute_file_path);
    //send_response(client, 200, &request_params);
  }
}
/*--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 * send_response - this function will be the function that actually sends a message to the client. This message will contain the proper headers and the respective body content
 *--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 */
void send_response(int client, int status_code, struct HTTP_RequestParams *params, char *full_path) {
 // printf("Beginning of: send_response()\n");

  char invalid_version[] = "HTTP/1.1 400 Bad Request: Invalid Version\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n\r\n";

  char invalid_uri[] = "HTTP/1.1 400 Bad Request: Invalid URI\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n\r\n";

  char invalid_method[] = "HTTP/1.1 400 Bad Request: Invalid Method\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n\r\n";

  char not_found[] = "HTTP/1.1 404 Not Found:%s\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n\r\n"
    "<!DOCTYPE html><html><head><title>404 Not Found</title>"
    "<body><h1>404 Not Found:</h1></body></html>\r\n";

  char response[] = "HTTP/1.1 200 OK:\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n\r\n"
    "<!DOCTYPE html><html><head><title>404 Not Found</title>"
    "<body><h1>GOODDDDDDDDD</h1></body></html>\r\n";

  char not_implemented[] = "HTTP/1.1 501 Not Implemented: \r\n"
    "Content-Type: text/html; charset=UTF-8\r\n\r\n"
    "<!DOCTYPE html><html><head><title>501 Not Implemented</title>"
    "<body><h1>501 Not Implemented</h1></body></html>\r\n";

  //sprintf((char *)&actual_response, not_found, params->URI);


  switch (status_code)
  {
    case 501:
      printf("Printing not implemented page\n");
      write(client, not_implemented, sizeof(not_implemented) -1);
      break;
    case 404:
      printf("Printing error page\n");
      //removeSubstring(params->URI, "/");
      //sprintf(final_response, not_found, params->URI);
      //printf("The final response should be %s\n", final_response);
      //write(client, final_response, sizeof(final_response) -1);
      write(client, not_found, sizeof(not_found) -1);
      break;
    case 200:
      construct_file_response(full_path, client);
      break;
    case 4001:
      printf("This is a bad http method\n");
      write(client,invalid_method, sizeof(invalid_method) -1);
      break;
    case 4002:
      printf("This is a bad http version\n");
      write(client,invalid_version, sizeof(invalid_version) -1);
      break;
    case BAD_HTTP_VERSION:
      break;
  }

  //printf("This is what the body is returning: %s\n", body);
  //write(client, body, sizeof(body) -1);
  //printf("Leaving: send_response()\n\n\n");
}

void construct_file_response(char *full_path, int client) {
  printf("Beginning of: construct_file_response\n\n");

  char response[] = "HTTP/1.1 200 OK:\r\n" "Content-Type: text/html; charset=UTF-8\r\n\r\n";
  char buffer[32];
  long file_size;
  FILE *requested_file;
  size_t read_bytes;

  requested_file = fopen(full_path, "r");

  fseek(requested_file, 0, SEEK_END);
  file_size = ftell(requested_file);
  printf("File size: %lu\n", file_size);
  fseek(requested_file, 0, SEEK_SET);

 
  read_bytes = fread(buffer, 1, 32, requested_file);
  printf("%s\n", buffer);
  printf("Bytes Read %zu\n", read_bytes);
  fclose(requested_file);

      //sprintf(final_response, not_found, params->URI);

      send(client, response, sizeof(response)-1, 0);
      send(client, buffer, read_bytes, 0);

}




/*--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 * handle_file_serving - this function will take in a file path, and either construct the correct response body to serve up that file or it will return false if the file does not exist
 *--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 */
int handle_file_serving(char *path, char *body, struct TextfileData *config_data, int *result_status) {
 // printf("Beginning of: handle_file_serving()\n");

  int file_supported, i;
  file_supported = FALSE;
  char user_request_file_path[PATH_MAX + 1];
  char *user_request_extension;
  struct stat buffer;

  /* append the user requested path to the document root to obtain the absolute path for files that are being requested */
  strcpy(user_request_file_path, config_data->document_root);
  strcat(user_request_file_path, path);
  user_request_extension = strchr(user_request_file_path, '.');

  //printf("This is the path that the user officially requested: %s\n", user_request_file_path);
  //printf("This is the extension that the user specified: %s\n", strchr(user_request_file_path, '.'));

  /* check if user is requesting the root page */
  if ( ((strcmp(path, "/index")) == 0) || ((strcmp(path,"/")) ==0) || ((strcmp(path,"/index.html")) ==0)  ) {
    //printf("User has requested the main site, time to serve up index.html");
    *result_status = 200;
    strcat(user_request_file_path, "index.html");
    strcpy(body, user_request_file_path);
    return 0;
  }

  /* check to see if file requested is of a supported file type */
  for (i = 0; i < NUM_OF_FILE_TYPES; i++) {
    if ( (strcmp(user_request_extension, config_data->extensions[i])) == 0)
      file_supported = TRUE;
  }
  if (!file_supported) {
    printf("File type is not supported, time to print 501\n");
    *result_status = 501;
    strcpy(body, user_request_file_path);
    return(0);
  }

  /* at this point the path requested is neither the home page, nor is it an invalid file type. This means that it must either 
   * not exist in the directory (return 404) or it must exist and we must serve it up */
  if ( (stat (user_request_file_path, &buffer) == 0)) {
    printf("YAY FILE EXISTS\n");
    printf("Time to construct the file and serve it up...");
    *result_status = 200;
    strcpy(body, user_request_file_path);
    return 0;
  }
  else {
    printf("File not found\n");
    *result_status = 404;
    strcpy(body, user_request_file_path);
    return 0;
  }
//printf("Leaving: handle_file_serving()\n");
}



/*-------------------------------------------------------------------------------------------------------------------------------------------
 * interpret_request - this function will be take the users path and decide what to do based on the result
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
int interpret_request(struct HTTP_RequestParams *params, int *decision) {

  //printf("Beginning of: interpret_request()\n");

  if ( (strcmp(params->method, "GET")) == 0)
  {
    //printf("Valid Method!!\n");
  }
  else {
    printf("Invalid Method: %s\n", params->method);
    *decision = 4001;
    return 1;
  }

  if (((strncmp(params->httpversion, "HTTP/1.1",8 )) == 0)  || ((strcmp(params->httpversion, "HTTP/1.0")) == 0)) {
    //printf("Valid Version\n");
  }
  else {
    printf("Uh oh method version not HTTP/1.1 or HTTP/1.0\n");
    *decision = 4002;
    return 1;
  }
  //printf("Leaving: interpret_request()\n");
  return 0;
}
/*-------------------------------------------------------------------------------------------------------------------------------------------
 * extract_request_path - this function will be mainly responsible for parsing and extracting the path from the HTTP request from the client
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void extract_request_path(char *response, struct HTTP_RequestParams *params) {
 // printf("Beginning of: extract_request_path()\n");
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
  //printf("Beginning of: setup_server()\n");

  char *current_path;
  char buff[PATH_MAX + 1];
  char conf_file_path[PATH_MAX + 1];
  char read_line[200];
  char *saveptr;
  char *current_line;
  current_path = getcwd(buff, PATH_MAX + 1);

  strcpy(conf_file_path, current_path);
  strcat(conf_file_path, "/ws.conf");


  FILE *config_file;
  config_file = fopen(conf_file_path, "r");
  if (config_file == NULL)
    printf("File was unable to be opened\n");
  //else
    //printf("File succesfully opened!!\n");

  int counter = 1;
  /* Hard coded the extraction of attributes from the conf file by referencing the line numebers that I expect each property to be on
   * THIS WILL BREAK IF THE CONF FILE ISN'T IN THE SAME FORMAT */
  while (fgets (read_line,200, config_file) != NULL)
  {
    if (counter == 2) {
      current_line = strtok_r(read_line, " ", &saveptr);
      config_data->port_number = atoi(saveptr);
    }
    if (counter == 4) {
      current_line = strtok_r(read_line, " ", &saveptr);
      removeSubstring(saveptr, "\"");
      removeSubstring(saveptr, "\n");
      strcpy(config_data->document_root, saveptr);
    }
    if (counter == 6) {
      current_line = strtok_r(read_line, " ", &saveptr);
      removeSubstring(saveptr, "\n");
      strcpy(config_data->default_web_page, saveptr);
    }
    if (counter == 8) {
      current_line = strtok_r(read_line, " ", &saveptr);
      strcpy(config_data->extensions[0], current_line);
      removeSubstring(saveptr, "\n");
      strcpy(config_data->encodings[0], saveptr);
    }
    if (counter == 9) {
      current_line = strtok_r(read_line, " ", &saveptr);
      strcpy(config_data->extensions[1], current_line);
      removeSubstring(saveptr, "\n");
      strcpy(config_data->encodings[1], saveptr);
    }
    if (counter == 10) {
      current_line = strtok_r(read_line, " ", &saveptr);
      strcpy(config_data->extensions[2], current_line);
      removeSubstring(saveptr, "\n");
      strcpy(config_data->encodings[2], saveptr);
    }
    if (counter == 11) {
      current_line = strtok_r(read_line, " ", &saveptr);
      strcpy(config_data->extensions[3], current_line);
      removeSubstring(saveptr, "\n");
      strcpy(config_data->encodings[3], saveptr);
    }
    counter++;
  }


  //printf("Leaving: setup_server()\n\n");
}

/*----------------------------------------------------------------------------------------------
 * setup_socket - allocate and bind a server socket using TCP, then have it listen on the port
 *----------------------------------------------------------------------------------------------
 */
int setup_socket(int port_number, int max_clients)
{
  //printf("Beginning of: setup_socket()\n");

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
  //printf("Leaving: setup_socket()\n\n");
  return sock;
}
