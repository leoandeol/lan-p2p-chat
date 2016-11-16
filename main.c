
#include <stdio.h> //printf
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <pthread.h>
#include <string.h> //memset
#include <unistd.h> //close
#include <time.h> // sleep

/**********
 * Main thread is the client
 * Secondary is the server, there will be one server thread per other machine contacted
 **************/

#define BUF_LEN 512

#define PORT 21487

#define CLR "\x1B[0m"
#define RED "\x1B[31m"
#define GRN "\x1B[32m"
#define BLU "\x1B[34m"
#define YEL "\x1B[33m"

// structs

/*typedef struct server {
  pthread_t thread;
  struct server* next;
  struct server* previous;
} server;*/

// prototypes

void add_server(void);
void remove_server(server*);
void *server_thread();
void *client_thread();
void clr(void);

// globals

struct sockaddr_in socket_address;
char nick[20];
FILE* log; //todo

// functions

void clr(void)
{
  printf("\e[2J\e[H");
}

void add_server(void)
{
  printf("Adding new server\n");
  server tmp;
  pthread_create(&tmp.thread, NULL, server_thread, NULL);
  tmp.next = &servers;
  servers.previous = &tmp;
  servers = tmp;
  nb_servers++;
}

void remove_server(server* serv)
{
  printf("Removing server\n");
  if(serv->previous != NULL && serv->next != NULL)
    {
      serv->previous->next = serv->next;
      serv->next->previous = serv->previous;
    }
  else if(serv->previous != NULL)
    {
      serv->previous->next = NULL;
    }
  else if(serv->next != NULL)
    {
      serv->next->previous = NULL;
    }
}

void *server_thread()
{
  static u_short port_incr = 1;
  printf("Starting server number %d\n",port_incr);
  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  int broadcastEnable=1;
  int ret=setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
	
  if(ret==-1)
    {
      perror("setsockopt");
      pthread_exit(NULL);
    }
  
  int err = bind(sock, (struct sockaddr*)&socket_address, sizeof(struct sockaddr));
  if(err == -1)
    {
      perror("Server bind");
      pthread_exit(NULL);
    }
  
  struct sockaddr_in src;
  socklen_t len = sizeof(src);
  memset(&src, 0, len);
  
  char buff[BUF_LEN];
  memset(&buff, 0, BUF_LEN);
  printf("Waiting for message ...\n");
  u_int length = sizeof(src);
  while(1)
  {
  recvfrom(sock, buff, BUF_LEN, 0, (struct sockaddr *)&src, &length);
  printf("Message received :\n");
  printf("%s\n",buff);
  }
  printf("Closing server socket ...\n");
  close(sock);
  pthread_exit(NULL);
}

void *client_thread()
{
  printf("Starting client\n");
  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  int broadcastEnable=1;
  int ret=setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
  if(ret==-1)
    {
      perror("setsockopt");
      pthread_exit(NULL);
    }

  char msg[BUF_LEN];
  
  while(1)
    {
      printf("Please enter a message\n");
      fgets(msg,BUF_LEN,stdin);
      printf("Sending message ...\n");
      sendto(sock,msg,BUF_LEN,0,(struct sockaddr*)&socket_address, sizeof(socket_address));
      sleep(1);
    }
  printf("Message sent, closing client socket ...\n");
  close(sock);
  
  pthread_exit(NULL);
}

int main(void)
{
  clr();
  printf("Starting ...\n");
  //TODO welcome screen

  printf("Choose a nickname\n");
  scanf("%s",nick);

  
  memset(&socket_address, 0, sizeof(struct sockaddr_in));
  socket_address.sin_family = AF_INET;
  socket_address.sin_port = htons(STARTING_PORT);
  socket_address.sin_addr.s_addr = htonl(INADDR_BROADCAST);

  pthread_t client,server;

  nb_servers = 1;
	
  pthread_create(&server, NULL, server_thread, NULL);
  pthread_create(&client, NULL, client_thread, NULL);

  pthread_join(client, NULL);
  pthread_join(server,NULL); // temporary
	
  return 0;
}
