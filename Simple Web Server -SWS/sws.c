/*
 * CSC361 P1
 * Zihan Ye
 * V00793984
 * 2015.10.12
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<fcntl.h> 


#define MAXBUFLEN 1024
int sockfd, portno;
int numbytes;
socklen_t cli_len;
char buffer[MAXBUFLEN];	//data buffer
struct sockaddr_in serv_addr, cli_addr;	//we need these structures to store socket info

/*soeckt()*/
int TryToSock(){
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1){ 
		perror("sws: error on socket()");
	  	return -1;
     	}
}

/*set the address from client*/
void setAddress(char *argv[]){
	bzero((char *) &serv_addr, sizeof(serv_addr));
     	portno = atoi(argv[1]);  //read the port number value from stdin
     	serv_addr.sin_family = AF_INET;	//Address family, for us always: AF_INET
     	serv_addr.sin_addr.s_addr = INADDR_ANY; //INADDR_ANY; //Listen on any ip address I have
     	serv_addr.sin_port = htons(portno);  //byte order again 
}

/*bind the address between sockfd and address*/
int TryToBind(){
	if (bind(sockfd, (struct sockaddr *) &serv_addr,
        	sizeof(serv_addr)) < 0){
        	close(sockfd);
		perror("sws: error on binding!");
	  	return -1;
     	}
}

/*check if the command if legal or not*/
int checkCommand(char *command){
	if(strncmp(command, "GET /", 5)==0){
		return 200;
	}else if(strncmp(command, "get /", 5)==0){
		return 200;
	}else{
		return 400;
	}
}

/*print current time on the command*/
int printTime(){
	time_t rawtime;
    	struct tm * timeinfo;

    	time ( &rawtime );
    	timeinfo = localtime ( &rawtime );
	
    	printf("\nOct %d %d:%d:%d",timeinfo->tm_mday , timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}

/*if a message will be sent to client, call this method*/
int sendTo(){
	if ((numbytes = sendto(sockfd, buffer, strlen(buffer), 0,
             (struct sockaddr *)&cli_addr, cli_len)) == -1) {
            perror("sws: error in sendto()");
            return -1;
    	}
}

/*try to find if the path exists*/
int find(char *path, int numbytes, char *filename){
   	//FILE *fpw;
	FILE *fpr;
	int fd;
	
	//printf("%s\n", path);
	char * line = NULL;
        size_t len = 0;
        ssize_t read;
	
	if ((fd=open(path, O_RDONLY))==-1||(strcmp(filename, "/./")==0||strcmp(filename, "/../")==0)){
		//printf("404 Not Found\n");
		return 404;
	}
	else{
	   	fpr = fopen(path,"r"); // read mode
	   	if( fpr == NULL ){
	      		perror("Error while opening the file.\n");
	      		exit(EXIT_FAILURE);
	   	}
		if (fpr!=NULL){
			//fpw = fopen("index.html","w");
			strcpy(buffer, "HTTP/1.0 200 OK\n");
			sendTo();
			buffer[0] = '\0';
			while ((read = getline(&line, &len, fpr)) != -1) {
				strncpy(buffer, line, MAXBUFLEN);
				sendTo();
				buffer[0] = '\0';
		       	}
			strcpy(buffer, "\n");
			sendTo();
			buffer[0] = '\0';
			//fclose (fpw);
		}
	   	fclose(fpr);
		return 200;
	}
}

/*receive information from client*/
int receiveFrom(char *argv[], int numbytes, char *path){
	cli_len = sizeof(cli_addr);
	char command[5];
	int result1;
	int result2;
	int n = strlen(path);
		
   	if ((numbytes = recvfrom(sockfd, buffer, MAXBUFLEN-1 , 0,
		(struct sockaddr *)&cli_addr, &cli_len)) == -1) {
		perror("sws: error on recvfrom()!");
		return -1;
     	}
	buffer[numbytes] = '\0';
	//printf("%s\n", buffer);
	//printf("%i", numbytes);
	

	printTime();
	printf(" %s:%d ", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
  	
	strncpy(command, buffer, 5);
	result1=checkCommand(command);

	char * pch;
	char storage[4][1024];
	int i=0;
	
	pch = strtok (buffer," ");
	while (pch != NULL)
	{
		strcpy(storage[i], pch);
	 	pch = strtok (NULL, " ");
		i++;
	}
	//printf("%s\n", storage[0]);
	//printf("%s\n", storage[1]);
	//printf("%s\n", storage[2]);
	if(strlen(storage[i-1])>8){
			storage[i-1][8] = '\0';
	}
	if(result1==400||(strncmp(storage[i-1], "HTTP/1.0",8)!=0&&strncmp(storage[i-1], "http/1.0", 8)!=0)){
		//storage[i-1][8] = '\0';
		//printf("%s\n", path);
		printf("%s %s %s; %s 400 Bad Request; %s\n", storage[0], storage[1], storage[2], storage[2], path);
		strcpy(buffer, "HTTP/1.0 400 Bad Request\n");
		sendTo();
		buffer[0] = '\0';
	}
	else if(result1==200){
		if(strcmp(storage[1],"/")==0){
			strcpy(storage[1], "/index.html");
		}
		strcat(path, storage[1]);
		//printf("%s\n", path);
		result2 = find(path, numbytes, storage[1]);
		
		
		if(result2==200){
			printf("%s %s %s; %s 200 OK; %s\n", storage[0], storage[1], storage[2], storage[2], path);	
			
			
		}else if(result2 == 404){     
			strcpy(buffer, "HTTP/1.0 404 Not Found\n");
			sendTo();
			buffer[0] = '\0';
			printf("%s %s %s; %s 404 Not Found; %s\n", storage[0], storage[1],storage[2], storage[2], path);
		}
		path[n]= '\0';
		
		
	}
	printf("sws is running on UDP port %s and serving %s\n", argv[1], argv[2]);
	printf("press 'q' to quit ...\n");
}

int main(int argc, char *argv[]){
	char argPath[1024];
     	if (argc < 2) {
        	printf( "Usage: %s <port> <directory>\n", argv[0] );
        	fprintf(stderr,"ERROR, no port/directory provided\n");
        	return -1;
    	}
	if (argc < 3) {
        	printf( "Usage: %s <port> <directory>\n", argv[0] );
        	fprintf(stderr,"ERROR, no directory provided\n");
        	return -1;
    	}
	strcpy(argPath, argv[2]);
	//argPath[strlen(argv[2])-1] = '\0';
	//strncpy(argPath, argPath, strlen(argPath)-1);
	//printf("%s\n",argPath); 
	
     	TryToSock();
     	setAddress(argv);
     	TryToBind();	
	
	while(1){
		receiveFrom(argv, numbytes, argPath);
	}	
	
    	close(sockfd);
    	//freeing memory!
    	return 0;
}
