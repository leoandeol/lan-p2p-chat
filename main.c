
#include <stdio.h> //printf
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <pthread.h>
#include <string.h> //memset
#include <unistd.h> //close
#include <time.h> // sleep
#include <sys/ioctl.h>

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

// prototypes

void get_line(char*,size_t);
void *server_thread();
void *client_thread();
void clr();
void gui();

// globals

struct sockaddr_in socket_address;
char nick[20];
char msg_to_send[BUF_LEN];
char msg_list[150][BUF_LEN];
FILE* f_log; //todo
int update = 0;
int running = 1;
int new_message = 0;

// functions

void get_line(char* m,size_t s)
{
	// BUF_LEN or sizeof(m)?
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
	printf("Message sent, closing client socket ...\n");
	close(sock);
  
	pthread_exit(NULL);
}

void gui()
{
	short running = 1;
	struct winsize win;
	int cols = 0;
	int rows = 0;
	int chat_size = 0;
    int members_size = 20;
	
	while(running)
	{
		//BEFORE
		clr();
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &win);
		cols = win.ws_col;
		rows = win.ws_row;
	    chat_size = cols - 21;
		
		//DURING

		for(int l = 0; l < rows-2; l++)
		{
			//CHAT : filling space
			for(int i = 0; i < chat_size; i++)
			{
				printf(" ");
			}

			//SEPARATION
			printf("#");
			
			//MEMBERS : filling space
			for(int i = 0; i < members_size; i++)
			{
				printf(" ");
			}

			//BACKSLASH
			printf("\n");
		}
		for(int i = 0; i < cols; i++) { printf("#");}
		printf("\n");

		// UPDATE ONLY IF NEW LINE OR UPDATE = 1 TODOTODOTDO
		get_line(msg_to_send,BUF_LEN);
		new_message = 1;

		//IF GETLINE THEN SET msg_to_send to msg and set new_message to 1
		
		//AFTER
		//Give back cpu time, sleep some 20ms
		usleep(20000);
	}
}

int main()
{
	f_log = fopen("log","w+");


	fprintf(f_log,"Starting ...\n");

	//TODO close f_log or it will fucking not write

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
	pthread_join(server,NULL); // temporary
	
	return 0;
}
