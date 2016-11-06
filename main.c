#include <stdio.h> //printf
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <pthread.h>
#include <string.h> //memset
#include <unistd.h> //close

/**********
 * Main thread is the client
 * Secondary is the server, there will be one server thread per other machine contacted
 **************/

typedef struct server {
	pthread_t thread;
	struct server* next;
	struct server* previous;
} server;

#define STARTING_PORT 21487

server servers;
u_short nb_servers;

// prototypes
void add_server(void);
void remove_server(server*);
void *server_thread();
void *client_thread();

// functions

void add_server(void)
{
	server tmp;
	pthread_create(&tmp.thread, NULL, server_thread, NULL);
	tmp.next = &servers;
	servers.previous = &tmp;
	servers = tmp;
	nb_servers++;
}

void remove_server(server* serv)
{
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
	
	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	struct sockaddr_in socket_address;
	memset(&socket_address, 0, sizeof(struct sockaddr_in));
	socket_address.sin_family = AF_INET;
	socket_address.sin_port = htons(STARTING_PORT + port_incr++);
	socket_address.sin_addr.s_addr = INADDR_ANY;

	int err = bind(sock, (struct sockaddr*)&socket_address, sizeof(struct sockaddr));
	if(err == -1)
	{
		perror("Server bind");
		pthread_exit(NULL);
	}

	/*err = listen(sock, 0);
	if(err == -1)
	{
		perror("Server listen");
		pthread_exit(NULL);
	}
		
	struct sockaddr_in client_address;
	memset(&client_address, 0, sizeof(struct sockaddr_in));

	socklen_t client_address_len = sizeof(struct sockaddr);
	int sock_client = accept(sock, (struct sockaddr*)&client_address, &client_address_len);
	if(sock_client == -1)
	{
		perror("Server accept");
		pthread_exit(NULL);
	}*/

	struct sockaddr_in src;
	socklen_t len = sizeof(src);
	memset(&src, 0, len);
		
	char buff[150];
	memset(&buff, 0, sizeof(buff));
	recv(sock_client, &buff, sizeof(buff), 0, &src, len);
	printf("yo\n%s\n",buff);

	close(sock);
	pthread_exit(NULL);
}

void *client_thread()
{
	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	struct sockaddr_in socket_address;
	memset(&socket_address, 0, sizeof(struct sockaddr_in));
	socket_address.sin_family = AF_INET;
	socket_address.sin_port = htons(STARTING_PORT);
	socket_address.sin_addr.s_addr = INADDR_BROADCAST;

	int err = connect(sock, (struct sockaddr*)&socket_address, sizeof(struct sockaddr));
	if(err == -1)
	{
		perror("Client connection");
		pthread_exit(NULL);
	}
		
	char* msg = "message";
	send(sock,&msg,sizeof(msg),0);

	close(sock);

	pthread_exit(NULL);
}

int main(void)
{
	pthread_t client;

	nb_servers = 1;
	
	pthread_create(&client, NULL, client_thread, NULL);
	pthread_create(&servers.thread, NULL, server_thread, NULL);
	
	pthread_join(client, NULL);
	pthread_join(servers.thread,NULL); // temporary
	
	return 0;
}
