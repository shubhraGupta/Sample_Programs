#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <pthread.h> 
#include <unistd.h>


#define	MAX(x, y)	((x) > (y) ? (x) : (y))

pthread_mutex_t mutex_th;

int count = 0;
int tcount = 0;
int ucount = 0;
int purchased = 0;

struct dvd_type{
	int id;
	char title[100];
	int quantity;
};

struct dvd_type dvd[] = {{101,"MI4      ",10},
			{201,"Inception",20},
			{301,"Batman   ",25},
			{401,"Troy     ",2},
			{501,"Gandhi   ",5}
	};
	
/*structure to be sent as argument to udp thread*/
struct udp_type{
	int usock; /*for containing udp socket*/
	struct sockaddr_in client_addr; /*for containing client address*/
	int client_size; /*for containing size of client address*/
	char recvBuff[1025]; /*for containing client request*/
};

/*This method breaks the request string in 3 parts on the basis of delimiter ',' and stores the separated strings in tokens */
int return_tokens(char *str, char s[], char **tokens){
   char *tok;

   /* get the first token */
   tok = strtok(str, s);
   if(tok == NULL)
        return -1;

   tokens[0] = tok;

   int i = 1;

   while(i < 3) 
   {
      tok = strtok(NULL, s);

      if(tok == NULL)
        return -1;

      tokens[i] = tok;
      i++;
   }

   return 1;
}

/*This method concatenating the data of all Dvds in one string that can be sent to client*/
void get_concat_str(char *s){
	strcpy(s,"Item Number   Title        Quantity\n");
	int i = 0;

	for(i= 0;i<5;i++){
		char temp_str[15];
		sprintf(temp_str, "%d", dvd[i].id);
		strcat(s,temp_str);
		strcat(s,"           ");
		strcat(s,dvd[i].title);
		strcat(s,"       ");
		sprintf(temp_str, "%d", dvd[i].quantity);
		strcat(s,temp_str);
		strcat(s,"\n");		
	}
}

/*This method checks whether the request made by client is proper handle request or not. 
The method calls return_tokens to split client request in 3 substrings and checks whether proper values are given for order request.
If request is not order or proper id and quantity is not sent by client, error messaged is sent to client. Else requested quantity is deducted
from specified id and success response is sent*/
char* handleOrderRequest(char buf[]){
	char s[2] = ",";
   	char *tokens[3];
   	int c=0,quan=0,i=0;
	
	if(return_tokens(buf, s, tokens) == -1)
		return "Request parameters invalid";

	c = atoi(tokens[1]);
	quan = atoi(tokens[2]);

	if(strcmp(tokens[0],"order") || !(c>0) ||!(quan>0)){
		return "Request parameters invalid";
	}

	while((i<5) && dvd[i].id!=c){
		i++;
	}
	
	if(i==5)
		return "Provided item number not found";	
	
	if(dvd[i].quantity<quan){
			return("Order can't be done");
	}
	else{ 
		pthread_mutex_lock (&mutex_th);
			dvd[i].quantity = dvd[i].quantity - quan;
			purchased = purchased + quan;
			printf ("Books purchased till now : %d\n",purchased);
		pthread_mutex_unlock (&mutex_th);
		return "Order successful";

	}
}

/*The function process_request serves request of each tcp client. It creates its local buffer to receive client data and send data.
It checks the request from client:- if list sends the dvd list obtined from function get_concat_str. or calls handleOrderRequest
to process request for ordering books. It also keeps track of number of connections made*/
void *processTcpRequest(void *arg){
	int conn_fd;
	char sendBuff[1025];
	char recvBuff[1025];
	int n = 0; 

	conn_fd = (int)arg;

	pthread_mutex_lock (&mutex_th);
   		count++;  tcount++;
   		printf("Total client requests :-%d\n",count);
		printf ("Tcp connection :-%d\n",tcount);
    pthread_mutex_unlock (&mutex_th);

	memset(sendBuff, '0', sizeof(sendBuff)); 
	memset(recvBuff, '0', sizeof(recvBuff)); 

	n=read(conn_fd,recvBuff,sizeof(recvBuff));
		recvBuff[n] = '\0';

	if(n){
			printf("Request from Tcp Client :- %s\n",recvBuff);
			fflush(stdout);
		}

	if(strcmp((recvBuff),"list")==0){
		char s[1024];
		get_concat_str(s);
		snprintf(sendBuff, sizeof(sendBuff), "%s",s);
		if(write(conn_fd,sendBuff,strlen(sendBuff))){
			printf("Response sent\n");
		}
				
	}else{
		char *result = handleOrderRequest(recvBuff);
		printf("response for client request :- %s\n",result);
		snprintf(sendBuff, sizeof(sendBuff), "%s",result);
		if (write(conn_fd,sendBuff,strlen(sendBuff)))
			printf("Response sent\n");		
	}

		close(conn_fd);

}

/*This function is called when a new udp client request comes. For new udp client request, a thread is created
and this function is called. The function checks each request and process accordingly for 'list' and 'order'
request.*/
void *processUdpRequest(void *arg){
	struct udp_type *tu;
	char sendBuff[1025];
	tu = (struct udp_type *)arg;

	pthread_mutex_lock (&mutex_th);
   		count++;  ucount++;
   		printf("Total client requests :-%d\n",count);
		printf ("Udp client :-%d\n",ucount);
    pthread_mutex_unlock (&mutex_th);

    memset(sendBuff, '0', sizeof(sendBuff));

    if(strcmp(tu->recvBuff,"list")==0){
				char s[1024];
				get_concat_str(s);
				snprintf(sendBuff, sizeof(sendBuff), "%s",s);
				if( sendto(tu->usock,sendBuff,strlen(sendBuff),0,(struct sockaddr *)&(tu->client_addr),sizeof(tu->client_addr))){
					printf("Response sent\n");
				}
					
	}else{
			char *result = handleOrderRequest(tu->recvBuff);
			printf("Response for client request:- %s\n",result);

			snprintf(sendBuff, sizeof(sendBuff), "%s",result);
			if( sendto(tu->usock,sendBuff,strlen(sendBuff),0,(struct sockaddr *)&(tu->client_addr),sizeof(tu->client_addr)))
					printf("Response sent\n");			
	}



}

int main(int argc,char **argv){
	int tsock=0, conn_fd = 0, usock=0;
	struct sockaddr_in my_addr, client_addr;
	int client_size; int port_number;
	int n = 0; 
	int optValue = 1;
	int	nfds;
	fd_set	rfds;

	if(argc<3){
		printf("Give proper address of server\n");
		return -1;
	}

	/*creating socket tsock for serving tcp clients*/;
	tsock = socket(AF_INET, SOCK_STREAM, 0);

	/*creating socket usock for serving udp clients*/;
	usock = socket(AF_INET, SOCK_DGRAM, 0);
	
	memset(&my_addr, '0', sizeof(my_addr));

	port_number = atoi(argv[2]);

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port_number);
	
	if(inet_pton(AF_INET, argv[1], &my_addr.sin_addr)== -1){
		printf("error in reading server addrss\n");
		return -1;
	}

	/*binding address to tsock tcp scoket */
	setsockopt(tsock,SOL_SOCKET,SO_REUSEADDR,&optValue,sizeof(optValue));	
	if(bind(tsock,(struct sockaddr*)&my_addr, sizeof(my_addr))==-1){
		printf("Error in binding address to tcp socket\n");
		return -1;	
	}

	/*Calling listen() for tsock so that it can listen to client requests with buffer for 5 clients */
	listen(tsock,5);

	/*binding socket usock for udp server*/
	setsockopt(usock,SOL_SOCKET,SO_REUSEADDR,&optValue,sizeof(optValue));
	if(bind(usock,(struct sockaddr*)&my_addr, sizeof(my_addr))==-1){
			printf("Error in binding address to udp socket\n");
			return -1;	
	}

	printf("server opened for requests\n");

	pthread_mutex_init(&mutex_th, NULL);

	/*Setting arguments for select system call()*/
	nfds = MAX(tsock, usock) + 1;	/* bit number of max fd	*/
	FD_ZERO(&rfds);

	while(1){

		FD_SET(tsock, &rfds);
		FD_SET(usock, &rfds);

		if (select(nfds, &rfds, (fd_set *)0, (fd_set *)0,(struct timeval *)0) < 0){
			printf("Error while calling select() system call");
			return -1;
		}
		
		/*checking whether request came from tcp client. If request is from tcp client, a thread is created to handle to each client connection.*/	
		if (FD_ISSET(tsock, &rfds)) {
		
			if((conn_fd = accept(tsock, (struct sockaddr*)&client_addr,&client_size))==-1){
				printf("Error in connecting to client\n");
				continue;	
			} 

			printf("\nTcp Server address :- %s : %d\n",inet_ntoa(my_addr.sin_addr),ntohs(my_addr.sin_port));
			printf("Tcp Client address :- %s : %d\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));

			/*creating thread to process each client request*/
			pthread_t th;
			pthread_create(&th, NULL, processTcpRequest, (void *)conn_fd);

		}

		/*Checking whether there is read on udp socket and then processing request from udp client*/
		if (FD_ISSET(usock, &rfds)) {
			
			struct udp_type u;
			u.usock = usock;	
			u.client_size = sizeof(struct sockaddr_in);
			memset(u.recvBuff, '0', sizeof(u.recvBuff)); 

			if ((n=recvfrom(u.usock,u.recvBuff, sizeof(u.recvBuff)-1, 0,(struct sockaddr *)&u.client_addr, &u.client_size)) >0){
				u.recvBuff[n] ='\0';
			}
			printf("\nUdp server address    :- %s : %d\n",inet_ntoa(my_addr.sin_addr),ntohs(my_addr.sin_port));
			printf("Udp Client address    :- %s : %d\n",inet_ntoa(u.client_addr.sin_addr),ntohs(u.client_addr.sin_port));	
			printf("Request from udp client :- %s\n",u.recvBuff);

			pthread_t th;
			pthread_create(&th, NULL, processUdpRequest, &u);
		
		}

		sleep(1);
	}
	pthread_mutex_destroy(&mutex_th);
	pthread_exit(NULL);
	close(tsock);
	close(usock);
	return 0;
}

