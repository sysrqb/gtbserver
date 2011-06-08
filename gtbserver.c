#include "gtbserver.h"

/*function sigchld_handler
 *SIGCHLD handler: wait for all dead (zombie) processes
*/

void sigchld_handler(int s){
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

/*function *get_in_addr
 *get_in_addr:
 *Get Internet Address returns the IP address of the client
*/

void *get_in_addr(struct sockaddr *sa){
	if(sa->sa_family == AF_INET){
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int authrequest(int sockfs, char authbuf, int AUTHSIZE){
	

	return 0;
}

int main(int argc, char *arv[]){
//FileDescriptors: sockfd (listen), new_fd (new connections)
	int sockfd, new_fd;

//STRUCTS
//addrinfo: hints (used to retrieve addr), servinfo (used to store retrieved addr), p (iterator)
	struct addrinfo hints, *servinfo, *p;
//sockaddr_storage: their_addr (IP Address of the requestor)
	struct sockaddr_storage their_addr;
//sigaction: sa (used to make sys call to change action taken on receipt of a certain sig)
	struct sigaction sa;

//socklen_t: sin_size (set size of socket)
	socklen_t sin_size;
//int rv: return value; i: iterator; yes: optval set in setsockopt
	int rv, i = 0, yes = 1, numbytes;
//char buf: buffer to hold incoming connection requests
	char reqbuf[REQSIZE];
	char loginbuf[LOGINSIZE];
//	char buf[MAXSIZE]

	memset(&hints, 0, sizeof hints);

//struct set to allow for binding and connections
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; //uses extern local IP

//Retrieve addr and socket
	if((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

//Iterate through servinfo and bind to the first available
	for(p = servinfo; p != NULL; p = p->ai_next){
		i++;
		if ((sockfd = socket(p->ai_family, p->ai_socktype, //If socket is unseccessful in creating an
		                p->ai_protocol)) == -1) { //endpoint, e.g. filedescriptor, then it returns -1,
			perror("server: socket"); //so continue to the next addr
			continue;
		}

		if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, //If unsuccessful in setting socket options
				sizeof(int)) == -1){ //exit on error
			perror("setsockopt");
			error(1);
		}

		if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){ //Assign a name to an addr
			close(sockfd);	//If unsuccessful, print error and check next
			perror("server: bind");
			continue;
		}

		break; //if successful, exit loop
	}

	if(p == NULL){
		fprintf(stderr, "server: Failed to bind for unknown reason\n Iterated through loop %d times\n", i);
		return 2;
	}

	freeaddrinfo(servinfo); //no longer need this struct

	if (listen(sockfd, BACKLOG) == -1){ //marks socket as passive, so it accepts incoming connections
		perror("listen: failed to mark as passive");
		exit(1);
	}

	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if(sigaction(SIGCHLD, &sa, NULL) == -1){
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connection\n");

	while(1){
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1){
			perror("accept");
			continue;
		}

		if((numbytes = recv(sockfd, reqbuf, REQSIZE-1, 0)) == -1){
			perror("reqrecv");
			exit(1);
		}

		reqbuf[numbytes] = '\0';

		if(!strncmp(reqbuf, "AUTH", REQSIZE)){
			authrequest(sockfs, authbuf, AUTHSIZE);
			break;
		} else 
		if(1){
		}

		close(new_fd);
	}

	return 0;
}
