#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>      
#include <sys/types.h>        
#include <sys/wait.h>         
#include <arpa/inet.h>        
#include <unistd.h>           
#include <dirent.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <netdb.h>

int listner(int portNum)
{
	struct sockaddr_in client, server;
	struct in_addr **addr_list;
	struct hostent *he;
	int client_sock, socket_desc, server_sock_desc, server_sock;
	int portSer = 80, n;
	int read_size, childsocket_no = 0;	
	char dummyVar[64], client_message[2048], client_message_cpy[2048], client_command[16], webpage[256], webpage_cpy[256], ver[10];
	char hostname[64], junk[64], buf[512];
	char* tmp=NULL;
	size_t c;
	pid_t PID;


	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , IPPROTO_TCP);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}

	//For reusing the port
	if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
    	perror("setsockopt(SO_REUSEADDR) failed");

	printf("Socket created at port: %d\n", portNum);

	//Prepare the sockaddr_in structure
	client.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &client.sin_addr.s_addr);
	client.sin_port = htons(portNum);

	//Bind
	if( bind(socket_desc,(struct sockaddr *)&client , sizeof(client)) < 0)
	{
		//print the error message
		perror("bind failed. Error");
		return 1;
	}
	puts("bind done");

	//Listen
	listen(socket_desc , 15);

	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);

	while(1)
	{
		//accept connection from an incoming client. Wait till received
		client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
		if (client_sock < 0)
		{
			perror("accept failed\n");
			return 1;
		}
		
		childsocket_no++;
		printf("\nConnection accepted\n");

		PID = fork();
		if(PID == 0) 
		{
			// close(socket_desc);
			printf("forked on child %d!\n", childsocket_no);
			if((read_size = read(client_sock , client_message , sizeof(client_message))) > 0 ) {
				// printf("Full client Message : \n%s\n", client_message);        //Received message from the web page
			}

			strcpy(client_message_cpy, client_message);

			sscanf(client_message, "%s %s %s",client_command,webpage,ver);

			// strcpy(client_command, strtok(client_message, " "));
			if((!strncmp("GET", client_command, 3))&&((!strncmp("HTTP/1.1",ver,8))||(!strncmp("HTTP/1.0",ver,8)))&&(!strncmp("http://",webpage,7))) 
			{
				printf("Child Sock: %d. Client Command:%s\n",childsocket_no, client_command);
				// strcpy(webpage, strtok(NULL, " "));
				printf("Child Sock: %d. Webpage requested: %s\n",childsocket_no, webpage);
				strcpy(webpage_cpy, webpage);
				
				//removing the http for getting the IP address				
				bzero(dummyVar, sizeof(dummyVar));
				strcpy(dummyVar, webpage);

				bzero(webpage, sizeof(webpage));
				strcpy(junk, strtok(dummyVar, "//"));
				strcpy(hostname, strtok(NULL, "//"));

				//Finding the IP addresses
		    printf("Child Sock: %d. Host = %s\n",childsocket_no, hostname);
		    if ((he = gethostbyname(hostname)) == NULL) {  // get the host info
		        herror("gethostbyname");
		        return 1;
		    }

		    // print information about this host:
		    printf("Child Sock: %d. Official name is: %s\n",childsocket_no, he->h_name);
		    printf("Child Sock: %d. IP addresses: ",childsocket_no);
		    addr_list = (struct in_addr **)he->h_addr_list;
		    for(int i = 0; addr_list[i] != NULL; i++) {
		        printf("%s ", inet_ntoa(*addr_list[i]));
		    }
		    printf("\n");
				// printf("1%s  '%s'\n",webpage_cpy, tmp);
				// printf("2%s  '%s'\n",webpage_cpy, tmp);
				bzero(junk, sizeof(junk));
				strcat(webpage_cpy, "^]");
				tmp = strtok(webpage_cpy, "//");
				tmp = strtok(NULL, "/");
				printf("3%s  '%s'\n",webpage_cpy, tmp);
				if(tmp!=NULL)
				{
					tmp = strtok(NULL, "^]");
				}
				printf("Child Sock: %d. --Server side--\n", childsocket_no);
				printf("Child Sock: %d. Webpage path: %s on port: %d\n", childsocket_no, tmp, portSer);

				// Server side connection socket configuration
				bzero((char*)&server, sizeof(server));
				server.sin_port = htons(portSer);
				server.sin_family = AF_INET;
				bcopy((char*)he->h_addr,(char*)&server.sin_addr.s_addr,he->h_length);			
				// memcpy((char*)he->h_addr, (char*)&server.sin_addr.s_addr, he->h_length);

				server_sock_desc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				server_sock = connect(server_sock_desc, (struct sockaddr*)&server, sizeof(struct sockaddr));
				if(server_sock < 0)
					printf("Child sock: %d. Error connecting to server\n", childsocket_no);
				
				bzero((char*)buf,sizeof(buf));
				if(tmp!=NULL)
					sprintf(buf,"GET /%s %s\r\nHost: %s\r\nConnection: close\r\n\r\n",tmp, ver, hostname);
				else
					sprintf(buf,"GET / %s\r\nHost: %s\r\nConnection: close\r\n\r\n", ver, hostname);
				printf("Child Sock: %d. Sending the following data to Server:\n%s", childsocket_no, buf);

				n = send(server_sock_desc, buf, strlen(buf), 0);
				if(n <0)
					printf("Child sock: %d. Error writing to server socket", childsocket_no);
				else
				{
					do
					{
						bzero((char*)buf,500);
						n=recv(server_sock_desc,buf,500,0);
						// printf("received data: %s\n", buf);
						if(!(n<=0))
							send(client_sock,buf,n,0);
					}while(n>0);
				}
			}
			else
			{
				send(client_sock,"400 : BAD REQUEST\nONLY HTTP REQUESTS ALLOWED",18,0);
			}
			close(server_sock_desc);
			close(client_sock);
			close(socket_desc);
			exit(0);
		}
		else if(PID > 0)
		{
			close(client_sock);
		}
		else
		{
			printf("Fork failed\n");
		}
	}
}

void main(int argc, char * argv[])
{
	char *inputCmd;
	int portNum;
	//Taking in the port number
	printf("Hi \n");

	if(argc != 2)
	{
		printf("Incorrect command typed. Exiting...");
		exit(0);
	}

	inputCmd = argv[1];
	portNum = atoi(strtok(inputCmd, "&"));

	//Function to listen and get data from client
	listner(portNum);

	



}