/* 
 * Troy Pandhumsoporn
 * client_test1.c
 * COEN233 Programming Assignment 1
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>						
#include <errno.h>
#include <arpa/inet.h>

#define MAXCHUNKSIZE 10
#define MAXBUFSIZE 4096																	

void error(const char *message) {
	perror(message);
	exit(EXIT_FAILURE); 
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
	
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {

	int listen_sock_fd, rtrv, bytes_rd, bytes_sd, c;
	struct addrinfo hints, *servinfo, *p;
	char cbuffer[MAXBUFSIZE];														// holds output filename to be sent to server 																												
	char clibuf[MAXCHUNKSIZE];														// buffer to hold chunks to be read
	char sck[INET6_ADDRSTRLEN];													
		
	if (argc != 5) {																// client takes in four arguments
		fprintf(stderr,"usage: client ip_address port filename1 filename2 \n");
		exit(EXIT_FAILURE);
	}
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;

	if ((rtrv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rtrv));
		exit(EXIT_FAILURE);
	}

	// loop through all the results and connect to the first socket one can get (removing this loop produces a segmentation fault)
	for(p = servinfo; p != NULL; p = p->ai_next) {

		listen_sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		
			if(listen_sock_fd == -1) {
				perror("client: socket failed");
				continue;
			} else {
				printf("{CLIENT}: Procured Socket Descriptor Successfully.\n");
			}
		
		c = connect(listen_sock_fd, p->ai_addr, p->ai_addrlen);
		
			if(c  == -1) {
				close(listen_sock_fd);
				perror("client: connect failed");
				continue;
			} else {
				printf("{CLIENT}: Connect SUCCESS\n");
			}
			break;
	}
	
	if(p == NULL) {
		fprintf(stderr, "ERROR, client failed to connect\n");
		exit(EXIT_FAILURE);
	}
	
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)p->ai_addr),sck, sizeof(sck));		//Convert binary version of IP address to text.
	printf("{CLIENT}: connecting to %s\n", sck);
	
	freeaddrinfo(servinfo); 																	// servinfo structure not needed anymore

	/*Copy input filename specified by argument into a buffer to be sent to server*/

	memset(cbuffer, 0, MAXBUFSIZE);	
	
	strcpy(cbuffer, argv[4]);

	/*Test code to send the name of file <output> to server <output>*/
	send(listen_sock_fd, cbuffer, MAXCHUNKSIZE, 0);
	
	/////////////////////////////////////////////////////////////
	
	/*Reading <input> file*/

	char* fr_name = argv[3];
	FILE *fp = fopen(fr_name, "rb");
	if(fp == NULL) {
			printf("File %s is not found\n", fr_name);
	} else {
		printf("Preparing to read file %s\n", fr_name);		
		memset(clibuf, 0, sizeof(clibuf));    					/* make buf empty before using it*/
		bytes_rd = 0;
		bytes_sd = 0;
		printf("bytes contained %d\n", sizeof(clibuf));
			
		while (((bytes_rd = (fread(clibuf, sizeof(char), MAXCHUNKSIZE, fp))) > 0)) {					// As long as there are bytes to be read...
			printf("File Chunk Size Block Read is %d \n", bytes_rd);									// Print read bytes in chunks of 10
				if((bytes_sd = (send(listen_sock_fd, clibuf, bytes_rd, 0))) < 0) {						// If the number of bytes are less than 0
					error("Send Error\n");
				} else {
					printf("File Chunk Size Block Sent is %d \n", bytes_sd);							// Print number of sent bytes
				}
				memset(clibuf, 0, sizeof clibuf); 
		}
		fclose(fp);
		printf("File %s read finished at %s\n", fr_name, argv[1]);
	}
	close(c);				//close connection
	close(listen_sock_fd);	//close socket
	return 0;
}
	/////////////////////////////////////////////////////////////
