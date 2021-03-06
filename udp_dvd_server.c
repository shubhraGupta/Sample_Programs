#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h> 

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
		dvd[i].quantity = dvd[i].quantity - quan;
		return "Order successful";

	}
}

int main(int argc,char **argv){
	int listen_fd=0;
	struct sockaddr_in my_addr, client_addr;
	int client_size,port_number;int n = 0;
	char sendBuff[1025];
	char recvBuff[1025];
	int i = 0;

	if(argc<3){
		printf("Give proper address of server\n");
		return -1;
	}

	listen_fd = socket(AF_INET, SOCK_DGRAM, 0);

	//setting address for server
	memset(&my_addr, '0', sizeof(my_addr));
	port_number = atoi(argv[2]);

	my_addr.sin_family = AF_INET;        
        my_addr.sin_port = htons(port_number);
        
	if(inet_pton(AF_INET, argv[1], &my_addr.sin_addr)== -1){
		printf("error in reading server addrss\n");
		return -1;
	}
	
	//Bind socket with address
	int optValue = 1;
	setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&optValue,sizeof(optValue));

	if(bind(listen_fd,(struct sockaddr*)&my_addr, sizeof(my_addr))== -1){
		printf("Error in binding address to socket");
		return -1;
	}
		
	printf("Server ready\n");
	
	client_size = sizeof(struct sockaddr_in);
	memset(recvBuff, '0', sizeof(recvBuff));
	memset(sendBuff, '0',sizeof(sendBuff)); 

	while(1){
		if ((n=recvfrom(listen_fd,recvBuff, sizeof(recvBuff)-1, 0,(struct sockaddr *)&client_addr, &client_size)) >0){
			recvBuff[n] ='\0';
		}

		printf("server address    :- %s : %d\n",inet_ntoa(my_addr.sin_addr),ntohs(my_addr.sin_port));
		printf("Client address    :- %s : %d\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));	
		printf("Request from client :- %s\n",recvBuff);
	
		if(strcmp(recvBuff,"list")==0){
			char s[1024];
			get_concat_str(s);
			snprintf(sendBuff, sizeof(sendBuff), "%s",s);
			if( sendto(listen_fd,sendBuff,strlen(sendBuff),0,(struct sockaddr *)&client_addr,sizeof(client_addr))){
				printf("Response sent\n");
			}
				
		}else{
			char *result = handleOrderRequest(recvBuff);
			printf("Response for client request:- %s\n",result);

			snprintf(sendBuff, sizeof(sendBuff), "%s",result);
			if (sendto(listen_fd,sendBuff,strlen(sendBuff),0,(struct sockaddr *)&client_addr,sizeof(client_addr)))
				printf("Response sent\n");			
		}
	
		sleep(1);
	}
	
	close(listen_fd);
	return 0;
}

