/* CSC361 p2
 * Zihan Ye
 * V00793984
 * 2015.11.25
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#define MAXBUFLEN 10240

char event_type = 'r';
char *package_type = "SYN";
int seqno = 0;
int ackno = 0;
int length = 0;
int payload_l = 900;
int window = 10240;
struct sockaddr_in receiver_addr, sender_addr;
socklen_t sender_len;
int sockfd;
char temp[13];
char seq[11];
char ack[11];
int storage[9];
int RSTcount=0;

char curTime[30];
struct timeval tv;
time_t curtime;
void printTime(){
    gettimeofday(&tv, NULL);
    curtime=tv.tv_sec;
    strftime(curTime,30,"%T.",localtime(&curtime));
    printf("%s%ld ",curTime,tv.tv_usec);
}

void printStat(){
    	 printf("total data bytes received: %i\n", storage[0]+storage[1]);
	 printf("unique data bytes received: %i\n", storage[1]);
	 printf("total data packets received: %i\n", storage[2]+storage[3]);
	 printf("unique data packets received: %i\n", storage[3]);
	 printf("SYN packets received: %i\n", storage[4]);
	 printf("FIN packets received: %i\n", storage[5]);
	 printf("RST packets received: %i\n", storage[6]);
	 printf("ACK packets sent: %i\n", storage[7]);
	 printf("RST packets sent: %i\n", storage[8]);
}

void sendBack(char *str){
    sendto(sockfd, str, strlen(str), 0, (struct sockaddr *)&sender_addr, sender_len);
}

int main(int argc, char *argv[]){

    int receiver_port;    
    fd_set readfds, testfds;
    char header2[1024];
    char *fileName;
    char send_addr[16];
    char send_port[16];
    FILE *file;
    
    int numbytes;
    if (argc < 4) {
        fprintf(stderr,"ERROR, not enough information provide\n");
        return -1;
        
    }
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        perror("sws: error on socket()");
        return -1;
    }
    bzero((char *) &receiver_addr, sizeof(receiver_addr));
    receiver_port = atoi(argv[2]);
    fileName = argv[3];
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_addr.s_addr = inet_addr(argv[1]);
    receiver_addr.sin_port = htons(receiver_port);
    if (bind(sockfd, (struct sockaddr *) &receiver_addr,
    sizeof(receiver_addr)) < 0){
        close(sockfd);
        perror("sws: error on binding!");
        return -1;
    }

    file = fopen(fileName, "w");
    
    int result;
    struct timeval timeout;
    
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);

    clock_t begin = clock(), diff;


    while(1){
	timeout.tv_sec = 0;
        timeout.tv_usec = 300000;
        sender_len = sizeof(sender_addr);
        testfds = readfds;
        result = select(FD_SETSIZE, &testfds, (fd_set *)0,(fd_set *)0,& timeout);

        switch (result) {
            case -1:
                printf( "Error\n" );
            	break;
            case 0:
		//keep waiting
		break;
            default:
                numbytes = recvfrom(sockfd, header2, sizeof(header2)+1, 0, (struct sockaddr *)&sender_addr,&sender_len);
		header2[numbytes] = '\0';
		memcpy(seq, &header2[32], sizeof(seq));
		seqno = atoi(seq);
		if(header2[60]=='1'){
			storage[6]++;
			header2[60] = '0';
		}
		if(header2[58]=='1'){   //SYN == YES
			
			storage[4]++;
			memcpy (send_addr, &header2[0], 15);
			strtok(send_addr, " ");
			memcpy (send_port, &header2[16], 15);
			strtok(send_port, " ");
			
			printTime();
			printf("%c %s:%s %s:%i %s %i/%i  %i/%i\n",event_type,
	    			send_addr, send_port, argv[1], receiver_port,
	    			package_type, seqno, ackno, length, window);
		
			sprintf(temp, "%d", payload_l);
			memcpy (&header2[64], temp, strlen(temp)); //payload
			memset(temp, 0, sizeof(char));

			sprintf(temp, "%d", window);
			memcpy (&header2[71], temp, strlen(temp)); //windowsize;

			

			header2[57] = '1';  //ACK == YES
			package_type = "ACK";
			storage[7]++;
			event_type = 's';
			printTime();
			printf("%c %s:%s %s:%i %s %i/%i  %i/%i\n",event_type,
	    			send_addr, send_port, argv[1], receiver_port,
	    			package_type, seqno, ackno, length, window);
			
			sendBack(header2);
			
			break;
		}
		else if(header2[56]=='1'){  //DAT ==YES
			event_type = 'r';
			package_type = "DAT";
			printTime();
			length = payload_l;
			printf("%c %s:%s %s:%i %s %i/%i  %i/%i\n",event_type,
	    			send_addr, send_port, argv[1], receiver_port,
	    			package_type, seqno, ackno, length, window);
			length=0;
			if(seqno-ackno==900){
				ackno+=payload_l;
				sprintf(ack, "%d", ackno);
				memcpy(&header2[44], ack, strlen(ack));
				char payload[payload_l];
				memcpy (payload, &header2[100], sizeof(payload)+1); //get context from header
				//printf("%s", payload);

				if(strlen(payload)>900){
					fwrite (payload , sizeof(char), sizeof(payload), file);
					storage[1]+=sizeof(payload);
				}else{
					//printf("%s", payload);
					fwrite (payload , sizeof(char), strlen(payload), file);
					storage[1]+=strlen(payload);
				}
				storage[3]++;
				header2[57] = '1';  //ACK == YES
				event_type = 's';	
				package_type = "ACK";
				storage[7]++;
				printTime();
				printf("%c %s:%s %s:%i %s %i/%i  %i/%i\n",event_type,
		    			send_addr, send_port, argv[1], receiver_port,
		    			package_type, seqno, ackno, length, window);

				sendBack(header2);
			
				break;
				//printf("%s\n" , header2);
			}else if(seqno==ackno){
				sprintf(ack, "%d", ackno);
				memcpy(&header2[44], ack, strlen(ack));
				package_type = "ACK";
				storage[7]++;
				event_type = 'S';
				printTime();
				storage[0]+=payload_l;
				storage[2]++;
				RSTcount++;
				if(RSTcount==3){
					storage[8]++;
					header2[60]='1';
					RSTcount = 0;
				}
				//printf("%i", ackno);
				//printf("%s", &header2[44]);
				printf("%c %s:%s %s:%i %s %i/%i  %i/%i\n",event_type,
		    			send_addr, send_port, argv[1], receiver_port,
		    			package_type, seqno, ackno, length, window);

				sendBack(header2);
				break;
			}
			
			//printf("%i\n", seqno);
		}
		else if(header2[59]=='1'){
			storage[5]++;
			event_type = 'r';
			package_type = "FIN";
			printTime();
			printf("%c %s:%s %s:%i %s %i/%i  %i/%i\n",event_type,
	    			send_addr, send_port, argv[1], receiver_port,
	    			package_type, seqno, ackno, length, window);

			header2[57]='1';
			event_type = 's';	
			package_type = "ACK";
			storage[7]++;
			printTime();
			printf("%c %s:%s %s:%i %s %i/%i  %i/%i\n",event_type,
		    		send_addr, send_port, argv[1], receiver_port,
		    		package_type, seqno, ackno, length, window);
			sendBack(header2);
			fclose(file);
			printStat();
			diff = clock() - begin;
			int msec = diff * 1000 / CLOCKS_PER_SEC;
			printf("total time duration (second): %d.%d\n", msec/1000, msec%1000);
			return;
		}
		break;
        }
    }
    
    close(sockfd);
    return 0;
}
