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
#include <errno.h>

#define Timeout_ON

void error_func(int childsocket_no, char message[32])
{
	int errsv = errno;
	printf("Child Sock: %d. %s. Refer to error number: %d of the errno function\n", childsocket_no, message, errsv);
	_exit(1);
}

// remove file/folder if it already exists
void remove_old_item(char* item_path) 
{
	if(access(item_path, F_OK) != -1)
		{
			remove(item_path);
		}
}

//To check if an item (file/folder) is present or not. 0 not present. 1 present
int check_item_presence(char* item_path)
{
	printf("Checking item %s. ", item_path);
	if(access(item_path, F_OK) < 0)
		{
			printf("Doesn't exist\n");
			//Item not present
			// error_func(item_name);
			return 0; 
		}
	else
	{
		printf("Exists\n");
		return 1;
	}
}

void CheckBlockedAddresses(int childsocket_no, char *ipaddr, char hostname[64])
{
	size_t filesize;
	char *linebuffer = NULL;


  FILE *fp = fopen("blocked_sites", "rb");

  fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

  while((getline(&linebuffer, &filesize, fp)) != -1)
	{
		if(strcmp(ipaddr, strtok(linebuffer, "\n")) == 0)
		{
			printf("Child Sock: %d. Can't service the blocked IP address. Exiting the process....\n", childsocket_no);
			_exit(1);
		}
		else if(strcmp(hostname, strtok(linebuffer, "\n")) == 0)
		{
			printf("Child Sock: %d. Can't service the blocked hostname address. Exiting the process....\n", childsocket_no);
			_exit(1);
		}
		else
			printf("Child Sock: %d. Address not blocked.\n", childsocket_no);
	}
	fclose(fp);
}

char* charPointmemoryAllocator(int pointSize)
{

	char *pointName = (char *)malloc(pointSize*sizeof(char));
	memset(pointName, '\0', pointSize*sizeof(char));
	return pointName;
}

void createSubDirectories(char *subpage)
{
	int len;
	char *tmp;
	char buf[50];
	char buf1[50];
	int count=0;

	// subpage = (char *)malloc(512*sizeof(char));
	//printf("subpage:%s",subpage);
	printf("%lu", strlen(subpage));
	for(int i=0;i<strlen(subpage);i++)
	{
		if(*(subpage+i) == '/')
		{
			count++;
		}
	}
	// printf("count:%d\n",count);
	bzero(buf, sizeof(buf));
	tmp = strtok(subpage, "/");
	while(count != 0)
	{
		bzero(buf1, sizeof(buf1));
		strcat(buf, tmp);
		strcat(buf, "/");
		sprintf(buf1, "mkdir %s", buf);
		// printf("buf1: %s\n", buf1);
		tmp = strtok(NULL, "/");
		if (check_item_presence(buf) == 0)
			system(buf1);
		count--;
	}
}

int listner(int portNum)
{
	struct sockaddr_in client, server;
	struct in_addr **addr_list;
	struct hostent *he;
	int client_sock, socket_desc, server_sock_desc, server_sock;
	int portSer = 80, n;
	int read_size, childsocket_no = 0;
	char dummyVar[64], client_message[2048], client_message_cpy[2048], client_command[16], webpage[256], webpage_cpy[256], ver[10];
	char hostname[64], hostfolder[64], junk[64], buf[512], buffer[1024];
	char foldername[64], filename[128], subpage_path[1024];
	char *subpage=NULL, *subpage_path_cpy = NULL, *tmp = NULL, *linebuffer = NULL, *ipaddr = NULL;
	size_t c, filesize;
	pid_t PID;

	subpage = charPointmemoryAllocator(512);
	if(subpage_path_cpy == NULL)
		printf("1Malloc Failed");
	subpage_path_cpy = charPointmemoryAllocator(1024);
	if(subpage_path_cpy == NULL)
		printf("2Malloc Failed");
	tmp = charPointmemoryAllocator(256);
	if(subpage_path_cpy == NULL)
		printf("3Malloc Failed");
	linebuffer = charPointmemoryAllocator(256);
	if(subpage_path_cpy == NULL)
		printf("4Malloc Failed");
	ipaddr = charPointmemoryAllocator(128);
	if(subpage_path_cpy == NULL)
		printf("5Malloc Failed");


	printf("----Client Side----\n");
	//Create socket for proxy to client
	socket_desc = socket(AF_INET , SOCK_STREAM , IPPROTO_TCP);
	if (socket_desc == -1)
	{
		printf("Could not create socket for client");
	}
	printf("Socket created at port: %d\n", portNum);

	//For reusing the port
	if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
    	perror("setsockopt(SO_REUSEADDR) failed");

	//Prepare the sockaddr_in structure to connect to client
	client.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &client.sin_addr.s_addr);
	client.sin_port = htons(portNum);

	//Bind
	if( bind(socket_desc,(struct sockaddr *)&client , sizeof(client)) < 0)
	{
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
		int subpage_check_flag = 2;


		//accept connection from an incoming client. Wait till received
		client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
		if (client_sock < 0)
		{
			perror("accept failed\n");
			return 1;
		}
		
		printf("\nConnection accepted\n");
		childsocket_no++;

		PID = fork();
		if(PID == 0) 
		{
			// close(socket_desc);
			printf("forked on child sock %d!\n", childsocket_no);
			if((read_size = read(client_sock , client_message , sizeof(client_message))) > 0 ) {
				// printf("Full client Message : \n%s\n", client_message);        //Received message from the web page
			}

			strcpy(client_message_cpy, client_message); //One for hostname and the other for subpage

			// printf("%s\n", client_message);
			sscanf(client_message, "%s %s %s",client_command,webpage,ver);
			// printf("client_command: %s\nwebpage: %s\nver: %s\n", client_command, webpage, ver);

			if((!strncmp("GET", client_command, 3))&&((!strncmp("HTTP/1.1", ver, 8))||(!strncmp("HTTP/1.0", ver, 8)))&&(!strncmp("http://", webpage, 7))) 
			{
				//printf("Child Sock: %d. Client Command:%s\n",childsocket_no, client_command);
				//printf("Child Sock: %d. Webpage requested: %s\n",childsocket_no, webpage);
			
				strcpy(webpage_cpy, webpage); //Creating copy to access subpage
				
				//removing the http for getting the IP address
				bzero(dummyVar, sizeof(dummyVar));
				strcpy(dummyVar, webpage);

				strcpy(junk, strtok(dummyVar, "//"));			//Extracted http://
				strcpy(hostname, strtok(NULL, "//"));			//Extracted hostname

				//Checking if the cache of "hostname" is present. 
				sprintf(hostfolder, "cache/%s", hostname);
				if(check_item_presence(hostfolder) == 0)				//0 not present.
				{
					printf("Child Sock: %d. Host not present in Cache. Creating host directory at %s.\n", childsocket_no, hostfolder);

					//Creating folder for "hostname"
					sprintf(foldername, "mkdir %s", hostfolder);
					system(foldername);

					//Finding the IP addresses
			    printf("Child Sock: %d. Host = %s\n",childsocket_no, hostname);
			    if ((he = gethostbyname(hostname)) < 0) // get the host info
			    	error_func(childsocket_no, "gethostbyname error");

			    // print information about this host:
			    //printf("Child Sock: %d. Official name is: %s\n",childsocket_no, he->h_name);
			    //printf("Child Sock: %d. IP addresses: ",childsocket_no);
			    addr_list = (struct in_addr **)he->h_addr_list;
			    for(int i = 0; addr_list[i] != NULL; i++) 
			    {
			        printf("%s ", inet_ntoa(*addr_list[i]));
			    }
			    printf("\n");

			    ipaddr = inet_ntoa(*addr_list[0]);

			    //Checking amongst blocked IP address/hostnames
			    CheckBlockedAddresses(childsocket_no, ipaddr, hostname);

					//Creating hostname conf file.
					sprintf(filename, "%s/%s.conf", hostfolder, hostname);

					printf("Child Sock: %d. writing to file\n", childsocket_no);
					FILE *fp = fopen(filename,"w");
					if (fp == NULL)
						error_func(childsocket_no, "Conf file not opened");
					bzero(buffer, sizeof(buffer));
					sprintf(buffer, "%s %s\n",hostname, inet_ntoa(*addr_list[0]));					
					if(fwrite(buffer, 1, sizeof(buffer), fp) ==0)
						error_func(childsocket_no, "conf file packet not written");
					else
						printf("Child Sock: %d. hostname IP_addr written in conf file\n", childsocket_no);
					fclose(fp);
				}

				//1 cache "hostname" present.
				else
				{
					printf("Child Sock: %d. Host present in Cache. Accessing IP address from there.\n", childsocket_no);
					sprintf(filename, "%s/%s.conf", hostfolder, hostname);
					FILE *fp = fopen(filename,"rb");
					if (fp == NULL)
					{
						error_func(childsocket_no, "Conf file not opened");
					}
					printf("Child Sock: %d. Opened conf file\n", childsocket_no);

					fseek(fp, 0, SEEK_END);
					filesize = ftell(fp);
					fseek(fp, 0, SEEK_SET);

					if(getline(&linebuffer, &filesize, fp) != -1)
					{
						if(strcmp(hostname, strtok(linebuffer, " ")) == 0)
						{
							ipaddr = strtok(NULL, "\n");
							printf("Child Sock: %d. IP address from conf file: %s\n",childsocket_no, ipaddr);
						}
						else
							printf("Improper data in conf file\n");
						printf("ipaddr: %s\n", ipaddr);
					}
					printf("HI\n");
					fclose(fp);
				}

				//Creating the subpage file path
				bzero(junk, sizeof(junk));
				strcat(webpage_cpy, "^]");
				tmp = strtok(webpage_cpy, "//");    //http:
				tmp = strtok(NULL, "/");						//host name
				// printf("webpage_cpy: %s\n", webpage_cpy);
				// printf("tmp%s\n", tmp);
				// printf("tmp: %s\n", tmp);
				if(tmp!=NULL) //If true, valid hostname. Sending&receiving subpage accordingly.
				{
					subpage = strtok(NULL, "^]");
					// printf("subpage: %s\n", subpage);


					if(subpage == NULL)
						sprintf(subpage_path, "%s/index.html", hostfolder);
					else
						sprintf(subpage_path, "%s/%s", hostfolder, subpage);

					//Checking if the file is present in the cache or not
					if(check_item_presence(subpage_path) != 0) //Subpage file Present in cache.
					{
						
						printf("Child Sock: %d. Accessing Subpage '%s' from cache.\n", childsocket_no, subpage_path);
						FILE *fp = fopen(subpage_path, "rb");
						if (fp == NULL)
							error_func(childsocket_no, "data file not opened");
						else
						{
							printf("starting the send process..\n");
							do
							{
								// printf("%d\n", n);
								int d;
								bzero((char*)buf,500);
								n = fread(buf,1, 500,fp);
								d = send(client_sock,buf,n,0);
								// printf("n: %d  d: %d\n",n,d);
							}while(n>0);
							fclose(fp);
						}
					}
					else //Subpage file not present
					{
						printf("Child Sock: %d. Subpage not present. Collecting data from server\n", childsocket_no);

						//Connecting to the server
						printf("Child Sock: %d. ----Server side----\n", childsocket_no);
						printf("Child Sock: %d. Webpage path: %s on port: %d\n", childsocket_no, subpage, portSer);

						// Server side connection socket configuration
						bzero((char*)&server, sizeof(server));
						server.sin_port = htons(portSer);
						server.sin_family = AF_INET;
						server.sin_addr.s_addr = inet_addr(ipaddr);
						// bcopy((char*)he->h_addr,(char*)&server.sin_addr.s_addr,he->h_length);			

						server_sock_desc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
						server_sock = connect(server_sock_desc, (struct sockaddr*)&server, sizeof(struct sockaddr));
						if(server_sock < 0)
							printf("Child sock: %d. Error connecting to server\n", childsocket_no);
						
						bzero((char*)buf,sizeof(buf));
						if(subpage!=NULL)
							sprintf(buf,"GET /%s %s\r\nHost: %s\r\nConnection: close\r\n\r\n",subpage, ver, hostname);
						else
							sprintf(buf,"GET / %s\r\nHost: %s\r\nConnection: close\r\n\r\n", ver, hostname);
						//printf("Child Sock: %d. Sending the following data to Server:\n%s", childsocket_no, buf);
						n = send(server_sock_desc, buf, strlen(buf), 0);
						if(n <0)
							printf("Child sock: %d. Error writing to server socket", childsocket_no);
						
						printf("send done\n");
						// printf("subpage_path: %s, subpage_path_cpy: %s", subpage_path, subpage_path_cpy);
						memcpy(subpage_path_cpy, subpage_path, strlen(subpage_path));
						printf("here!\n");
						createSubDirectories(subpage_path_cpy); // if required it will create

						//Writing data to client and saving the same if not present.
						FILE *fp = fopen(subpage_path, "w");
						if (fp == NULL)
							error_func(childsocket_no, "data file not opened for writing");
						else
						{
							printf("writing to file and sending\n");
							do
							{
								bzero((char*)buf,500);
								n=recv(server_sock_desc,buf,500,0);
								if(!(n<=0))
								{
									int d;
									// printf("%d\n", n);
									d = send(client_sock,buf,n,0);
									// printf("d:%d\n",d);
									if(fwrite(buf, 1, n, fp) ==0)
										error_func(childsocket_no,"data file packet not written");
								}
							}while(n>0);
							fclose(fp);
						}
					}
				}
			}
			else
			{
				send(client_sock,"400 : BAD REQUEST\nONLY HTTP REQUESTS ALLOWED",18,0);
				printf("Child Sock: %d. %s 400 : BAD REQUEST\n", childsocket_no, client_command);
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
	printf("Sup!\n");

	if(argc != 2)
	{
		printf("Incorrect command typed.\nFormat is './proxyserver <portnumber>'\nExiting...\n");
		exit(0);
	}

	inputCmd = argv[1];
	portNum = atoi(strtok(inputCmd, "&"));

	//Function to listen and get data from client
	listner(portNum);
}