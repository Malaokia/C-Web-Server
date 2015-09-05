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

int setup_socket (int port_number, int max_clients);
void setup_server ();



struct TextfileData {
  int port_number;
  char *document_root;
  char *default_web_page;
};


int main(int argc, char ** argv)
{

  printf("inside of main\n");

  int main_socket;
  struct TextfileData system_config_data;

  printf("about to call setup_server\n");
  setup_server(&system_config_data); 
  printf("About to call setup_socket!!!\n");
  main_socket = setup_socket(system_config_data.port_number, MAX_CLIENTS);

  char response[] = "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n\r\n"
    "<!DOCTYPE html><html><head><title>Bye-bye baby bye-bye</title>"
    "<style>body { background-color: #111 }"
    "h1 { font-size:4cm; text-align: center; color: black;"
    " text-shadow: 0 0 2mm blue}</style></head>"
    "<body><h1>Hello World</h1></body></html>\r\n";

  //struct sockaddr_in client;
  //int cli;
  //unsigned int sockaddr_len = sizeof(struct sockaddr_in);
  //int data_len;
  //char data[MAX_DATA];



/*
  while (1)
  {

    // we provide the accept command a pointer to an address structure that will
    // contain information about the client
    if ((cli = accept(main_socket, (struct sockaddr *)&client, &sockaddr_len)) == ERROR)
    {
      perror("accept");
      exit(-1);
    }

    printf("New Client connected from port number %d and IP %s\n", ntohs(client.sin_port), inet_ntoa(client.sin_addr));

    data_len = 1;

    while (data_len)
    {
      data_len = recv(cli, data, MAX_DATA, 0);

      if (data_len)
      {
        printf("Recieved Request: \n");
        printf(" %s", data);
        write(cli, response, sizeof(response) );
        //printf("Client disconnected\n");
        //close(cli);
      }
    }
  }
  */
}


/*----------------------------------------------------------------------------------------------
 * setup_server - first function called on entry, prints information and reads in config file 
 *----------------------------------------------------------------------------------------------
 */

void setup_server(struct TextfileData *config_data)
{
  config_data->port_number = 10000;
  config_data->document_root = "doc root";
  config_data->default_web_page = "web page";

  printf("Welcome to setup_server!! This is where we will pull data from the config file and populate our config struct\n");
  printf("leaving setup_server\n");
}

/*----------------------------------------------------------------------------------------------
 * setup_socket - allocate and bind a server socket using TCP, then have it listen on the port
 *----------------------------------------------------------------------------------------------
 */
int setup_socket(int port_number, int max_clients)
{
  printf("At the beginning of setup_socket()");

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
  return 0;
}
