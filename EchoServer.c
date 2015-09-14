#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<errno.h>
#include<unistd.h>
#include<arpa/inet.h>
#define ERROR -1
#define MAX_CLIENTS 32
#define MAX_DATA 1024
#define MAX_PATH_LENGTH 512

/* TODO */
/*
 *
 *  have extraction function also extract the HTTP method and the HTTP version 
 *
 *
 *
 * >>> Setup Up Config File Reading/Extacting
 * 1.) Open file in existing filepath/directory (ws.conf), read it line by line
 * 2.) After reading lines from file, extract fields that we need (regex? string search?)
 * 3.) Populate our TextfileData struct with the fields that we extact from the conf file
 * 4.) Using the data in our struct, set the variables for the server accordingly
 *
 * >>> Find out how to serve file content to client
 *1.) Read/learn how the response body/header needs to be structured for different kinds of file types
 *2.) Learn how to navigate directory structures (does file exist, get this file, etc) in C
 *3.) Start with trying to serve up static html page
 *3.) 

 * >>> Find out / create function that will parse the request URL and:
 *   - decide if it is a valid URL
 *   - parse out the method type
 *   - parse out the version of HTML
 *   - parse out the file that is requested
 *
 * >>> Design logic for when to display 404 and when to display static index page based on URL

*/
void interpret_request(char *path);
void removeSubstring(char *s,const char *toremove);
void extract_request_path(char *response, char *file_path);
int setup_socket (int port_number, int max_clients);
void setup_server ();

struct TextfileData {
  int port_number;
  char *document_root;
  char *default_web_page;
};

int main(int argc, char ** argv)
{
  printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
  printf("| Welcome to this wonderful C server! |\n");
  printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");

  char response[] = "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n\r\n"
    "<!DOCTYPE html><html><head><title>Bye-bye baby bye-bye</title>"
    "<style>body { background-color: #111 }"
    "h1 { font-size:4cm; text-align: center; color: black;"
    " text-shadow: 0 0 2mm blue}</style></head>"
    "<body><h1>Hello World</h1></body></html>\r\n";
  int main_socket;
  struct TextfileData system_config_data;
  struct sockaddr_in client;
  int cli;
  unsigned int sockaddr_len = sizeof(struct sockaddr_in);
  int data_len;
  char data[MAX_DATA];
  char file_path[MAX_PATH_LENGTH];

  setup_server(&system_config_data); 
  main_socket = setup_socket(system_config_data.port_number, MAX_CLIENTS);


  while (1)
  {
    if ((cli = accept(main_socket, (struct sockaddr *)&client, &sockaddr_len)) == ERROR) {
      perror("accept");
      exit(-1);
    }
    printf("New Client connected from port number %d and IP %s\n\n", ntohs(client.sin_port), inet_ntoa(client.sin_addr));

    data_len = recv(cli, data, MAX_DATA, 0);
    if (data_len) {
      extract_request_path((char *)&data, (char *)&file_path);
      interpret_request((char *)&file_path);
      write(cli, response, sizeof(response) -1);
      printf("Client disconnected\n");
      close(cli);
    }
  }
}


/*-------------------------------------------------------------------------------------------------------------------------------------------
 * interpret_request - this function will be take the users path and decide what to do based on the result
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void interpret_request(char *path) {

  printf("Beginning of: interpret_request()\n");
  printf("here is the passed in path:%s\n", path);

  if ( (strcmp(path, "\r")) == 0)
    printf("Empty Path!!");
  else
    printf("We have a path!!");
  


  printf("Leaving: interpret_request()\n");

}



/*-------------------------------------------------------------------------------------------------------------------------------------------
 * extract_request_path - this function will be mainly responsible for parsing and extracting the path from the HTTP request from the client
 *-------------------------------------------------------------------------------------------------------------------------------------------
 */
void extract_request_path(char *response, char *return_path) {


  printf("Beginning of: extract_request_path()\n");
  char *saveptr;
  char *the_path;


  // after this returns, return_path contains the first token while saveptr is a reference to the remaining tokens 
  the_path = strtok_r(response, "\n", &saveptr);
  removeSubstring(the_path, "HTTP/1.1");
  //printf( "First Token after seperating response by newline: \n%s\n", the_path);
  //printf( "Remaining Tokens after seperating response by newline: \n%s\n", saveptr);

  the_path = strtok_r(the_path, "/", &saveptr);
  //printf( "First token after seperation by slash character: \n%s\n", the_path );
  //printf( "Remaining tokens after seperation by slash character: \n%s\n", saveptr );

  the_path = strtok_r(NULL, " ", &saveptr);
  //printf( "Second token after seperation by slash character: %s\n", the_path );
  //printf( "Remaining tokens after seperation by slash character: \n%s\n", saveptr );
  printf("Leaving: extract_request_path()\n");
  strcpy(return_path, the_path);

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
  config_data->port_number = 10000;
  config_data->document_root = "doc root";
  config_data->default_web_page = "web page";

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
