/* CSC361 p2
 * Zihan Ye
 * V00793984
 * 2015.11.25
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <time.h>

#define headerSIZE 1024
char curTime[30];
struct timeval tv;
time_t curtime;
int storage[9];

int sockfd;
int receiverlen;
struct sockaddr_in receiver_addr;

char event_type = 's';
char *package_type = "SYN";
int seqno = 0;
int ackno = 0;
int length = 0;
int window = 0;
int unique = 0;

char seq[11];
char ack[11];
char temp[16];

void error(char *msg) {
    perror(msg);
    exit(0);
}

void printTime(){
    gettimeofday(&tv, NULL); 
    curtime=tv.tv_sec;
    strftime(curTime,30,"%T.",localtime(&curtime));
    printf("%s%ld ",curTime,tv.tv_usec);
}
void printStat(){
    	 printf("total data bytes sent: %i\n", storage[0]+storage[1]);
	 printf("unique data bytes sent: %i\n", storage[1]);
         printf("total data packets sent: %i\n", storage[2]+storage[3]);
         printf("unique data packets sent: %i\n", storage[3]);
         printf("SYN packets sent: %i\n", storage[4]);
         printf("FIN packets sent: %i\n", storage[5]);
         printf("RST packets sent: %i\n", storage[6]);
         printf("ACK packets received: %i\n", storage[7]);
         printf("RST packets received: %i\n", storage[8]);
}

void sendToReceiver(char *header){
	sendto(sockfd, header, strlen(header), 0, (struct sockaddr *)&receiver_addr, receiverlen);
}

int main(int argc, char **argv) {
    int receiver_port;
    
    struct hostent *receiver;
    char *receiver_ip;
    int numbytes;
    char header[headerSIZE];

    char *fileName;
    fd_set readfds, testfds;

    /* check command line arguments */
    if (argc <= 3) {
       fprintf(stderr,"usage: %s <receiver_ip> <port>\n", argv[0]);
       exit(0);
    }
    receiver_ip = argv[3];
    receiver_port = atoi(argv[4]);
    fileName = argv[5];
    
    
    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the receiver's DNS entry */
    receiver = gethostbyname(receiver_ip);
    if (receiver == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", receiver_ip);
        exit(0);
    }

    /* build the receiver's Internet address */
    bzero((char *) &receiver_addr, sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;
    bcopy((char *)receiver->h_addr, 
	  (char *)&receiver_addr.sin_addr.s_addr, receiver->h_length);
    receiver_addr.sin_port = htons(receiver_port);

    bzero(header, headerSIZE);
    receiverlen = sizeof(receiver_addr);
    // int a = 1024;
    //printf("%ld", sizeof(1024));
    memset (header,' ', (size_t)100); 
    memcpy (&header[0], argv[1], strlen(argv[1]));
    memcpy (&header[16], argv[2], strlen(argv[2])); //do not + 1 if dont want to end
    memcpy (&header[32], "0", sizeof(char)); //seqNo
    memcpy (&header[44], "0", sizeof(char)); //ackNo 
    memcpy (&header[56], "0", sizeof(char)); //DAT
    memcpy (&header[57], "0", sizeof(char)); //ACK
    memcpy (&header[58], "1", sizeof(char)); //SYN 
    memcpy (&header[59], "0", sizeof(char)); //FIN
    memcpy (&header[60], "0", sizeof(char)); //RST    

    //printf("%c\n" , header[58]);
    
   //
    //int payload = 1024;
    int payload_size = 0;
    FILE* file = fopen(fileName, "r");
    fseek(file, 0, SEEK_END);
    int fileSize = ftell(file);
    int remain = fileSize;
    

    //printf("%i", remain);
    int result;
    int start = 1;
    int end = 3;
    int RSTcount = 0;
    struct timeval timeout;
    
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);

    clock_t begin = clock(), diff;

    while(1){
	timeout.tv_sec = 0;
	timeout.tv_usec = 300000;
        testfds = readfds;
	if(header[58]=='1'){ // SYN == YES 

		if(storage[4]>0){
			event_type = 'S';
		}
		numbytes = sendto(sockfd, header, strlen(header), 0, (struct sockaddr *)&receiver_addr, receiverlen);
		printTime();
		printf("%c %s:%i %s:%i %s %i/%i  %i/%i\n",event_type, argv[1], atoi(argv[2]), argv[3], atoi(argv[4]), 
            		package_type, seqno, ackno, length, window);
		storage[4]++;
	}

	result = select(FD_SETSIZE, &testfds, (fd_set *)0,(fd_set *)0,& timeout);
	switch (result) {
	    case -1:
		printf( "Error\n" );
		break;
	    case 0:
		if(header[59]=='1'){
			printf("Does not receive FIN acknowledgement, exit after %i s \n", end);
			sleep(1);
			end--;
			if(end==0){
				printStat();
				diff = clock() - begin;
				int msec = diff * 1000 / CLOCKS_PER_SEC;
				printf("total time duration (second): %d.%d\n", msec/1000, msec%1000);
				return;
			}
		}else{
			if(event_type=='s'){
				event_type='S';
			}
			storage[0]+=length;
			storage[2]++;

			if(package_type=="FIN"){
				storage[5]++;
			}
			if(package_type=="DAT"){
				header[60] = '1';
				RSTcount++;
				if(RSTcount==3){
					storage[6]++;
					package_type="RST";
					RSTcount=0;
				}	
			}
			printTime();
			printf("%c %s:%i %s:%i %s %i/%i  %i/%i\n",event_type, argv[1], atoi(argv[2]), argv[3], atoi(argv[4]), 
		    		package_type, seqno, ackno, length, window);
			sendToReceiver(header);
		}
		break;
	    default:
		numbytes = recvfrom(sockfd, header, strlen(header)+1 , 0, (struct sockaddr *)&receiver_addr, &receiverlen);
		header[numbytes] = '\0';
		if(header[57]=='1'){
			package_type="ACK";
			storage[7]++;
			length=0;
		}
		if(header[60]=='1'){
			storage[8]++;
			header[60] = '0';
		}
		if(header[59]=='1' && header[57]=='1'){
			printStat();
			diff = clock() - begin;
			int msec = diff * 1000 / CLOCKS_PER_SEC;
			printf("total time duration (second): %d.%d\n", msec/1000, msec%1000);
			return;
		}
		memcpy(ack, &header[44], sizeof(ack));
			ackno = atoi(ack);

		event_type = 'r';
		
		printTime();
		printf("%c %s:%i %s:%i %s %i/%i  %i/%i\n",event_type, argv[1], atoi(argv[2]), argv[3], atoi(argv[4]), 
            		package_type, seqno, ackno, length, window);


		if(header[58]=='1' && header[57]=='1'){ // SYN == YES && ACK ==YES
			
			header[58]='0';  // set SYN = NO
			header[57]='0';  // && set ACK = NO, so never go back to this if condition and previous SYN condistion

			header[56]='1';  // set DAT = YES

			payload_size = atoi(memcpy (temp, &header[64], sizeof(temp)+1)); //payload_size
			window = atoi(memcpy (temp, &header[71], sizeof(temp)+1)); //payload_size
		}

		header[57]='0';  // set ACK = NO

		char payload[payload_size];
		if(header[56]=='1'){
			package_type = "DAT";
			if(ackno==seqno){
				if(remain>0){  //has context
					fseek(file, seqno, SEEK_SET);

					if(remain > payload_size){
						fread(&header[100], sizeof(char), payload_size, file);
						seqno +=payload_size;
						sprintf(seq, "%d", seqno);
						
						memcpy(&header[32], seq, strlen(seq));
						sendToReceiver(header);
						event_type = 's';
						length = payload_size;
						printTime();
						printf("%c %s:%i %s:%i %s %i/%i  %i/%i\n",event_type, argv[1], 
							atoi(argv[2]), argv[3], atoi(argv[4]), 
					    		package_type, seqno, ackno, length, window);
						storage[1]+=payload_size;
						remain -=payload_size;	
				
					}else{
						fread(&header[100], sizeof(char), remain, file);	
						header[100+remain]='\0';
						seqno +=payload_size;
						sprintf(seq, "%d", seqno);
						
						memcpy(&header[32], seq, strlen(seq));
						storage[1]+=remain;
						sendToReceiver(header);
						event_type = 's';
						length = remain;
						printTime();
						printf("%c %s:%i %s:%i %s %i/%i  %i/%i\n",event_type, argv[1], 
							atoi(argv[2]), argv[3], atoi(argv[4]), 
					    		package_type, seqno, ackno, length, window);
						remain=0;
					}
					storage[3]++;
				}else{  //no context anymore
					header[59] = '1';
					header[56] = '0';
					package_type="FIN";
					sendToReceiver(header);
					sendToReceiver(header);
					sendToReceiver(header);
					event_type = 's';
					length = payload_size;
					printTime();
					printf("%c %s:%i %s:%i %s %i/%i  %i/%i\n",event_type, argv[1], 
						atoi(argv[2]), argv[3], atoi(argv[4]), 
						package_type, seqno, ackno, length, window);
					storage[5]++;
				}
			}
		}
		break;
	}
    }
    return 0;
}
