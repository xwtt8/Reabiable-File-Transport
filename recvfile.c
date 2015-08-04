#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include "packet.h"
#include "md5.h"

#define SEQ_LENGTH 50000
#define BUFLEN 1024

main(int argc, char **argv)
{
	int i;
	struct sockaddr_in sin;	/* our address */
	struct sockaddr_in remaddr;	/* remote address */
	socklen_t addrlen = sizeof(remaddr);		/* length of addresses */
	int recvlen;			/* # bytes received */
	int fd;				/* our socket */
	unsigned char buf[PCKSIZE];	/* receive buffer */
	char* packet;			/* sending the ack packet */
	unsigned short server_port = atoi(argv[2]);
	char* file_name;
	FILE *fp;			/* pointer to the destination file */
	
	char* content[SEQ_LENGTH];	/* The array to hold the received content buffer */
	int content_length[SEQ_LENGTH];
	int window[SEQ_LENGTH];	/* Receiver window size */
	int remaining_file_size = -1;	/* The number of bits remaining to be received */
	int remaining_packets;		/* The number of packets that have not been received */	
	int last_seq;			/* The index of the last sequence number */
	int ready_flag = 0;		/* Identify whether all the data has been received */
	int first_pingpong = 0;


	FILE* fp_write2;
	char* pck_buf[5000];

	file_name = (char*)malloc(sizeof(char));

	char flag[] = "-p";
	if(strcmp(argv[1],flag) != 0){
		perror("Invalid flag\n");
		return 0;
	}

	/* 
	 * 0 if packet is not received
	 * 1 if packet is received
	 */
	for(i=0;i<SEQ_LENGTH;i++) {
		window[i] = 0;
	}

	for(i=0;i<SEQ_LENGTH;i++) {
		content[i] = (char*)malloc(BUFLEN*sizeof(char));
	}
	
	for(i=0;i<SEQ_LENGTH;i++) {
		content_length[i] = 0;
	}
	
	/* create a UDP socket */
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket\n");
		return 0;
	}

	/* bind the socket to any valid IP address and a specific port */

	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(server_port);

	if (bind(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		perror("bind failed");
		return 0;
	}
 
	/* now loop, receiving data and write the payload into a given file */
	printf("waiting on port %d\n", server_port);
	while(1) {
		printf("remain file size %d\n",remaining_file_size);
		recvlen = recvfrom(fd, buf, PCKSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
		printf("received length is %d\n",recvlen);
		if (recvlen > 0) {
			//buf[recvlen] = 0;
			/* check the correctness of the packet using md5 */
			uint8_t result[16];
			md5((uint8_t *)(buf+16), recvlen-16, result);	
			if (!packetCorrect((uint8_t *)buf, result)) {
				printf("[recv corrupt packet]\n");
				/*
				for(i=0;i<16;i++){
					printf("%x",result[i]);
				}*/
				printf("\n");
				continue;
			}
			short type = (short) ntohs(*(short*)(buf+16));
			/* if it is a ping pong message and it is the first time the receiver receive*/

			if(type==-1) {
				printf("In the check type stage\n");
				remaining_file_size = (int) ntohl(*(int*)(buf+26));
				if(first_pingpong == 0){
					strcpy(file_name,buf+30);				
					strcat(file_name,".recv");
					fp = fopen(file_name, "wb");
					first_pingpong = 1;
				}

				remaining_packets = 1+ (remaining_file_size-1)/BUFLEN;
				last_seq = (remaining_file_size-1)/BUFLEN;
				if (sendto(fd, buf, recvlen, 0, (struct sockaddr *)&remaddr, addrlen) < 0)
					perror("sendto");
				
			} else if(type == 1){ /* else it is a data packet */					
				short seq_num = (short) ntohs(*(short*)(buf+26));
				printf("[Recv data %d] start (%d)\n",seq_num,recvlen-28);		
				/* The packet with this sequence number has not been received */
				if(window[seq_num] != 1 ) {
					int file_buf_size = recvlen-28; /* the received buffer size minus the headers' size */					
					//printf("The size of the buffer is %d\n",file_buf_size);
					remaining_file_size -= file_buf_size;
					remaining_packets--;
					window[seq_num] = 1;
					memcpy(content[seq_num],buf+28,file_buf_size);
					content_length[seq_num] = file_buf_size;
				}
 
				/* sending back acknowledgement*/ 				
				packet = (char*)malloc(26*sizeof(char));

				int tv_sec = (int) ntohl(*(int*)(buf+2));
				int tv_usec = (int) ntohl(*(int*)(buf+6));
				*(short *) (packet+16) = (short) htons(seq_num);
				*(int *)  (packet+18) = (int) htonl(tv_sec);
				*(int *)  (packet+22) = (int) htonl(tv_usec);

				md5((uint8_t *) (packet+16), 10, (uint8_t *) packet);

				printf("[sending ack %d]\n", seq_num); 
				if (sendto(fd, packet, 26, 0, (struct sockaddr *)&remaddr, addrlen) < 0)
					perror("sendto");
			}
		}
		else
			printf("uh oh - something went wrong!\n");	
		
		/* check if all the packet in the window is received */
		int ready_flag=1;
		for(i=0;i<last_seq+1;i++){
			if(window[i] == 0){
				ready_flag = 0;
				printf("Sequence number %d has not been received\n",i);
				break;
			}
		}

		/* write to the file if all packets in one sliding window is received */
		if(ready_flag == 1){
			printf("The last sequence number is %d\n",last_seq);
			for(i=0;i<last_seq+1;i++){
				//printf("strlen is %d  ", strlen(content[i])-1 );
				int write_len;
				if(i==0)
					write_len = fwrite(content[i] , 1 , content_length[i], fp);
				else
					write_len = fwrite(content[i] , 1 , content_length[i] , fp);

				printf("The number of bits being written by sequence %d is %d\n",i,write_len);
				content[i] = (char*)malloc(BUFLEN*sizeof(char));
				window[i] = 0;
			}
			printf("write data to %s\n",file_name);
			remaining_file_size = -1;
			fclose(fp);
			exit(0);
		}
	}
	/* never exits */
}
// ./sendfile -r 128.42.208.6:18000 -f testfiles/30m.bigfile
// netsim --reorder 50 --drop 50 --mangle 50 --duplicate 50