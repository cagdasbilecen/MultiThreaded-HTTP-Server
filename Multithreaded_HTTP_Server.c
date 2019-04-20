
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<pthread.h>
#include<semaphore.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<signal.h>
#include<fcntl.h>
#define ROOT "/home/cme2002/Desktop/opsys"   //this is root path, server will create here...
#define BYTES 1024
#define PORT_NUM 8888


char *error404 = "HTTP/1.0 404 Not Found\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n<!doctype html><html><body><h1> 404 - File Not Found ! </h1></body></html>";

char *error400 = "HTTP/1.0 400 Bad Request\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n<!doctype html><html><body><h1>400 - Bad Request</h1></body></html>";

char *serverBusy = "HTTP/1.0 400 Bad Request\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n<!doctype html><html><body><h1>System is busy right now..</h1></body></html>";

char *notJpegOrHtml= "HTTP/1.0 404 Not Found\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n<!doctype html><html><body><h1> This Server support only jpeg or html pages...  </h1></body></html>";

//sem_t controllOfRequest;
sem_t mutex; 
int counter=0; //initially 0

void *connection_handler(void *socket_desc)
{
	sem_init(&mutex,0,1); //controll the counter
	sem_wait(&mutex);
	counter++;  //one of 10 threads increment counter
	sem_post(&mutex);
	int rcvd;
	char *extension[2]; //to control file extensions html or jpeg
	char mesg[99999] , *reqline[3],*temp[3], data_to_send[1024], path[99999];
	
	int fd, bytes_read;
	int cntrl=0;
    	//Get the socket descriptor
   	int sock = *((int*)socket_desc);  
	memset( (void*)mesg, (int)'\0', 99999 );
	
	rcvd=recv(sock, mesg, 99999, 0);

	
	printf("Thread Counter : %d , Incoming Request : \n %s",counter, mesg); //see request from terminal..


	if (rcvd<0)    // receive error
		fprintf(stderr,("recv() error\n"));
	else if (rcvd==0)    // receive socket closed
		fprintf(stderr,"Client disconnected upexpectedly.\n");
	else    // message received
	{
		
		reqline[0] = strtok (mesg, " \t\n"); //request line 0
		if ( strncmp(reqline[0], "GET\0", 4)==0 ) //if GET request comes up
		{
		reqline[1] = strtok (NULL, " \t");
		reqline[2] = strtok (NULL, " \t\n");
		if(counter<10){ //than give response to client

		if ( strncmp( reqline[2], "HTTP/1.0", 8)!=0 && strncmp( reqline[2], "HTTP/1.1", 8)!=0 )
		{

		write(sock, error400, strlen(error400)); 
		}
		
		
 		else
			{
				if (strncmp(reqline[1], "/\0", 2) == 0)
                   		{
                        	strcat(reqline[1], "index.html"); //this is default
                    		}

				
				strcpy(path, ROOT);
				strcpy(&path[strlen(ROOT)], reqline[1]);
				printf("Request comes to here : %s\n", path);
				
				if ( (fd=open(path, O_RDONLY))!=-1 )    //if file found,then..
				{	
					temp[0] = reqline[1];
					extension[0]=strtok(temp[0],".");
					extension[1]=strtok(NULL,"."); //extension[1] is the extension of file
					//index.html or jpeg will be showed
					if ( strcmp(extension[1],"jpeg") != 0 && strcmp(extension[1], "html") != 0)
					{  
						write(sock, notJpegOrHtml , strlen(notJpegOrHtml));
						//printf("------------------------- : %s " , extension);

					}
					else{
				
					send(sock, "HTTP/1.0 200 OK\n\n", 17, 0);  
					sem_wait(&mutex); //only one thread read the file for safety
					while ( (bytes_read=read(fd, data_to_send, BYTES))>0 )
						write (sock, data_to_send, bytes_read);
					sem_post(&mutex); //
					}
					
				} 
				else    //if file not found
					write(sock,error404,strlen(error404));
					
			}   }  


			} //these are responses when counter<10 state....
			else{ //and this one response when counter>=10 // Server is busy
			
			write(sock,serverBusy,strlen(serverBusy));

			}
	}
	sem_wait(&mutex);
	counter--;  // one thread decrement counter after client side get responsed
	sem_post(&mutex);


	free(socket_desc); //free the socket pointer
	//sleep(10);
	shutdown (sock, SHUT_RDWR); //send and recieve operations are disabled..
	close(sock);
	sock=-1;
	return 0;
}


int main(int argc, char *argv[])
	{
    
    	int socket_desc, new_socket, c, *new_sock;
    	struct sockaddr_in server, client;
    	char *message;
     
    	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    	if (socket_desc == -1)
    	{
        puts("Could not create socket");
        return 1;
    	}
     
    	server.sin_family = AF_INET;
    	server.sin_addr.s_addr = INADDR_ANY;
    	server.sin_port = htons(PORT_NUM);
     
    	if(bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
    	{
        	puts("Binding failed");
        	return 1;
    	}
    
   	
	

     
    	puts("Waiting for incoming connections...");
    	c = sizeof(struct sockaddr_in);
	
	listen(socket_desc, 10); 
	
    	while((new_socket = 
           accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
   	{
	
        puts("Connection accepted ");
	
        pthread_t sniffer_thread;
        new_sock = malloc(1); // a memory allocator
        *new_sock = new_socket;
        

        if(pthread_create(&sniffer_thread, NULL, connection_handler,
                          (void*)new_sock) < 0)
        {
            puts("Could not create thread");
            return 1;
        }
	
        puts("Handler assigned");
    	
	
}

return 0;	

    
}


 
