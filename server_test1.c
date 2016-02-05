/* 
 * Troy Pandhumsoporn
 * server_test1.c
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
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define INQUEUE 1 													// how many pending connections queue will hold, since we want to exit
#define MAXDIVSIZE 5
#define MAXCHUNKSIZE 10
#define MAXBUFSIZE 4096																					

void error(const char *message)										// /Simple Error message handling function
{
    perror(message);
    exit(EXIT_FAILURE);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {

	int listen_sock_fd, client_newsock_fd, rtrv, bytes, b, s;
	int on = 1;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage local_addr;
	socklen_t sin_size;
	char sck[INET6_ADDRSTRLEN];
	char buffer[MAXBUFSIZE];													// Receive buffer for filename sent by client					
	char buf[MAXBUFSIZE];														// Buffer for reading/writing data (bytes transferred) through socket
	
	if (argc != 2) {									
		fprintf(stderr,"usage: server PORTNO \n");
		exit(EXIT_FAILURE);
	}

	memset(&hints, 0, sizeof(hints));											//Replaces bzero() as a zero reset on struct parameters 
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; 												// use my IP
	hints.ai_protocol = 0;
	
	if ((rtrv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rtrv));
		return(1);
	}
	
	// loop through all the results and bind to the first socket one can get (removing this loop produces a segmentation fault)
    for(p = servinfo; p != NULL; p = p->ai_next) {
        
		listen_sock_fd = socket(p->ai_family, p->ai_socktype,p->ai_protocol);
		
			if (listen_sock_fd == -1) {
				perror("server: socket");
				continue;
			} else {
				printf("{SERVER}: Procured Socket Descriptor Successfully.\n");
		}

		s = setsockopt(listen_sock_fd, SOL_SOCKET, SO_REUSEADDR, (char* ) &on, sizeof(on));
		
			if (s == -1) {
            	perror("setsockopt");
            	exit(EXIT_FAILURE);
			} else {
				printf("{SERVER}: Reused IP address Successfully\n");
		}

		b = bind(listen_sock_fd, p->ai_addr, p->ai_addrlen);
		
        	if (b == -1) {
            	close(listen_sock_fd);
            	perror("server: bind ERROR");
				continue;
        	} else {
				printf("{SERVER}: Binded to TCP port on localhost successfully.\n");
		}
		break;
	}

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "{SERVER}: could not bind\n");
        exit(EXIT_FAILURE);
    }

    if (listen(listen_sock_fd, INQUEUE) == -1) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
	
	printf("{SERVER}: connections in queue...\n");
		
    while(1) {  // primary accept() loop

        sin_size = sizeof(local_addr);
        client_newsock_fd = accept(listen_sock_fd, (struct sockaddr *)&local_addr, &sin_size);

        if (client_newsock_fd == -1) {
            perror("accept");
            continue;
        } else {
			printf("{SERVER}: Connection acccepted successfully\n");
		}
		
	//Convert binary version of IP address to text.
	inet_ntop(local_addr.ss_family, get_in_addr((struct sockaddr *)&local_addr),sck, sizeof(sck));
    printf("{SERVER}: got connection from %s\n", sck);

	/* Receive filename <input> from client */
	recv(client_newsock_fd, buffer, MAXBUFSIZE, 0);
	printf("%s\n", buffer);

	/* Open the received input file from client */
	FILE *fp = fopen(buffer, "wb");
		if(fp == NULL) {
			printf("File %s cannot be opened on server\n", buffer);
		} else {
			printf("Reading file from buffer %s\n", buffer);
			memset(buf, 0, sizeof MAXBUFSIZE);
			bytes = 0;
			
			while ((bytes = recv(client_newsock_fd, buf, MAXDIVSIZE, 0))) {
				if(bytes < 0) {
					error("Receive error!\n");
				} else {
					int write_div = fwrite(buf, sizeof(char), bytes, fp);				// write data in chunks of 5 bytes
					printf("File Block Write Length is %d\n", write_div);
				}
				memset(buf, 0, sizeof buf);					
			}
			fclose(fp);
            printf("Write file finished.\n");
		}
		/*close connection*/
		close(client_newsock_fd);
	}
	/* close socket */
	close(listen_sock_fd);
}
