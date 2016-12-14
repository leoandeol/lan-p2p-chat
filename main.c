#include <stdio.h> //printf
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <pthread.h> //pthread
#include <string.h> //memset, strcpy
#include <unistd.h> //close
#include <time.h> // sleep
#include <sys/ioctl.h>
#include <ncurses.h>

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

#define NICK_LEN 20
#define MAX_CLIENTS 50
#define MAX_LINES 60

// prototypes

void get_line(char*,size_t);
void *server_thread();
void *client_thread();
void clr();
void gui();

// globals

struct sockaddr_in socket_address;
char nick[NICK_LEN];
char msg_to_send[BUF_LEN];
FILE* f_log;
int update = 0;
int running = 1;
int new_message = 0;
char nicks[MAX_CLIENTS][NICK_LEN];


/////////////////////////////
/// Message history manager
/////////////////////////////

char **history;

void hist_init(unsigned short rows)
{
  history = malloc(sizeof(char*)*rows);
  for(int i = 0; i < rows; i++)
    {
      history[i] = malloc(sizeof(char)*BUF_LEN);
      memset(history[i],0,sizeof(char)*BUF_LEN);
    }
  for(int i = 0; i < rows; i++)
    {
      strcpy(history[i],"hello world !\n");
    }
}

void hist_update(unsigned short rows, unsigned short last_rows)
{
  if(last_rows < rows)
    {
      // memory leak here, if terminal reduced then made big again, malloc'd twice
      for(int i = last_rows; i < rows; i++)
	{
	  history[i] = malloc(sizeof(char)*BUF_LEN);
	  memset(history[i],0,sizeof(char)*BUF_LEN);
	}
    }
  else if (last_rows > rows)
    {
      //decaller
      for(int i = last_rows-1; i > (last_rows-rows); i--)
	{
	  history[rows-i] = history[i];
	}
      for(int i = rows; i < last_rows; i++)
	{
	  free(history[i]);
	}
    }
  history = realloc(history,sizeof(char*)*rows);
}
    
void hist_push_msg(char* m)
{
  for(int i =0; i < MAX_LINES-1; i++)
    {
      strcpy(history[i+1],history[i]);
    }
  strcpy(m,history[MAX_LINES-1]);
}

// functions

void get_line(char* m,size_t s)
{
  if(fgets(m,s,stdin)==NULL)
    {
      fprintf(f_log,"Error reading inputs from stdin\n");
    }
}

void clr()
{
  printf("\e[2J\e[H");
}

void *server_thread()
{
  static u_short port_incr = 1;
  fprintf(f_log,"Starting server number %d\n",port_incr);
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
  memset(buff, 0, BUF_LEN);
  fprintf(f_log,"Waiting for message ...\n");
  u_int length = sizeof(src);
  while(running)
    {
      recvfrom(sock, buff, BUF_LEN, 0, (struct sockaddr *)&src, &length);
      update = 1;
      fprintf(f_log,"Message received : %s\n",buff);
      //message list
    }
  printf("Closing server socket ...\n");
  close(sock);
  pthread_exit(NULL);
}

void *client_thread()
{
  fprintf(f_log,"Starting client\n");
  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  int broadcastEnable=1;
  int ret=setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
  if(ret==-1)
    {
      perror("setsockopt");
      pthread_exit(NULL);
    }

  while(running)
    {
      if(new_message)
	{
	  time_t val = time(NULL);
	  struct tm* tm_st = localtime(&val);
	  char msg[BUF_LEN+100];
	  snprintf(msg,BUF_LEN+100,"[%d:%d:%d] %s : %s",tm_st->tm_hour, tm_st->tm_min, tm_st->tm_sec, nick, msg_to_send);
	  fprintf(f_log,"Sending message : %s\n",msg);
	  sendto(sock,msg,BUF_LEN+100,0,(struct sockaddr*)&socket_address, sizeof(socket_address));
	  memset(msg_to_send, 0, BUF_LEN);
	}
    }
  fprintf(f_log,"Message sent, closing client socket ...\n");
  close(sock);
  
  pthread_exit(NULL);
}

void gui()
{
  short running = 1;
  int cols = 0;
  int rows, last_rows;
  int chat_size = 25;
  const int members_size = 20;

  initscr();

  getmaxyx(stdscr,rows,cols);
  last_rows=rows;

  hist_init(rows);
	
  while(running)
    {
      //BEFORE
      //clr(); replaced by ncurses, refresh
      refresh();
		
		
      chat_size = cols - members_size - 1;
      //DURING

      for(int r = 0; r < rows-2; r++)
	{
	  move(r,0);
	  printw("%d",r);
	  //CHAT : filling space
	  for(int i = 0; i < chat_size; i++)
	    {
	      //printf(" ");
	    }
	  printw("%s",history[r]);

	  //SEPARATION
	  mvprintw(r,chat_size,"#");
			
	  //MEMBERS : filling space
		    
	  printw("                    ");
			
	}
      move(rows-2,0);
      for(int i = 0; i < cols; i++) { printw("#");}

      move(rows-1,0);
      printw("type here");
		
      // UPDATE ONLY IF NEW LINE OR UPDATE = 1 TODOTODOTDO
      //get_line(msg_to_send,BUF_LEN);
      int c = getch();
      c = c>2;
      new_message = 1;
      
      //IF GETLINE THEN SET msg_to_send to msg and set new_message to 1

		
      getmaxyx(stdscr,rows,cols);

      if(last_rows!=rows)
	{
	  hist_update(rows,last_rows);
	}
      last_rows = rows;
    }
  endwin();
}

int main()
{
  f_log = fopen("log","w+");


  fprintf(f_log,"Starting ...\n");
  
  printf("************************************************************************\n");
  printf("* LAN P2P Chat | by Léo Andéol | GNU GPL                               *\n");
  printf("* An application written in C, using UDP sockets and broadcasts        *\n");
  printf("* Source : http://github.com/leoandeol/lan-p2p-chat/                   *\n");
  printf("************************************************************************\n");
	
  printf("Choose a nickname : \n");
  get_line(nick,sizeof(nick));

  
  memset(&socket_address, 0, sizeof(struct sockaddr_in));
  socket_address.sin_family = AF_INET;
  socket_address.sin_port = htons(PORT);
  socket_address.sin_addr.s_addr = htonl(INADDR_BROADCAST);

  pthread_t client,server;
	
  pthread_create(&server, NULL, server_thread, NULL);
  pthread_create(&client, NULL, client_thread, NULL);

  gui();
	
  pthread_join(client, NULL);
  pthread_join(server,NULL);

  fclose(f_log);
  
  return 0;
}
