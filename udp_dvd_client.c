#include<sys/socket.h>
#include<sys/types.h>
#include <netinet/in.h>
#include<string.h>
#include <stdio.h>


int main(int argc, char **argv){
	int conn_fd = 0;
	char sendBuff[1025];
	char recvBuff[1025];
	struct sockaddr_in server_addr,client_addr;
	int i,n = 0,port_number;int client_size;

	if(argc<3){
		printf("Give proper address of server\n");
		return -1;

	}
	
	if((conn_fd=socket(AF_INET, SOCK_DGRAM, 0)) == -1){
		printf("Failed to create socket\n");
		return -1;
	}
	
	memset(&server_addr, '0', sizeof(server_addr));

	port_number = atoi(argv[2]);

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port_number);
	
	if(inet_pton(AF_INET, argv[1], &server_addr.sin_addr)== -1){
		printf("error in reading server addrss\n");
		return -1;
	}
	
	memset(sendBuff, '0',sizeof(sendBuff));
	memset(recvBuff, '0',sizeof(recvBuff));
	client_size = sizeof(struct sockaddr_in);

	if(argc==3){
		printf("No request to send to server\n");
		return -1;
	}
	char *s= argv[3];
	for(i=0;i<strlen(argv[3]);i++)
	     			 s[i] = tolower(s[i]);

    	/*Send message to server*/
	if(argc == 6){
		snprintf(sendBuff, sizeof(sendBuff),"%s,%s,%s",argv[3],argv[4],argv[5]);
		if(n = sendto(conn_fd,sendBuff,strlen(sendBuff),0,(struct sockaddr *)&server_addr,sizeof(server_addr))>0){
					printf("Request send\n");
		}
		/*Receive message from server*/
                if((n=recvfrom(conn_fd,recvBuff,sizeof(recvBuff),0,(struct sockaddr *)&server_addr, &client_size))>0){
     			recvBuff[n] =0;
    			printf("%s\n",recvBuff);
		}
	
	}else if(argc == 4){
		snprintf(sendBuff, sizeof(sendBuff), "%s",argv[3]);
   		if(n = sendto(conn_fd,sendBuff,strlen(sendBuff),0,(struct sockaddr *)&server_addr,sizeof(server_addr))>0){
					printf("Request send\n");
		}

    		/*Receive message from server*/
		if((n=recvfrom(conn_fd,recvBuff,sizeof(recvBuff),0,(struct sockaddr *)&server_addr, &client_size))>0){
     			recvBuff[n] =0;
    			printf("%s\n",recvBuff);
			
		}

	}
	else{
		printf("Request is not proper\n");
	}
	
	close(conn_fd);

	

	
	
	
}
