#include "gtbserver.h"

/*
 *
 *
 * function sigchld_handler
 *
 *SIGCHLD handler: wait for all dead (zombie) processes
 *
 *
*/

void sigchld_handler(int s){
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

/*
 *
 *
 * function *get_in_addr
 *
 *get_in_addr:
 * Get Internet Address returns the IP address of the client
 *
 *
*/

void *get_in_addr(struct sockaddr *sa){
	if(sa->sa_family == AF_INET){
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
/******
 *
 *
 * int authrequest()
 *
 *
*/ 


int authrequest(int sockfd, char *reqbufptr){
	char hash[AUTHSIZE];
	int retval;
	
	if(0 != strncmp("AUTH", reqbufptr, 5)){
		fprintf(stderr, "Not AUTH\n");
		return -3;
	}
	getclientinfo(sockfd, hash);
	/*if((retval = checkhash(*hash)) < 1){
		fprintf(stderr, "Authenication Failed\n");
		return retval;
	}*/
	

	return 0;
}
/******
 *
 *
 * void getclientinfo()
 *
 *
*/ 


void getclientinfo(int sockfd, char *hash){
	int numbytes;
	printf("Get hash\n");
	if((numbytes = recv(sockfd, hash, AUTHSIZE-1, 0)) == -1){
		perror("reqrecv");
		exit(1);
	}

	hash[numbytes] = '\0';
	printf("Received Hash: %s\n", hash);
}
/******
 *
 *
 * int getsocket()
 *
 *
*/ 


int get_socket(){
//FileDescriptors: sockfd (listen), new_fd (new connections)
	int sockfd, new_fd;

//STRUCTS
//addrinfo: hints (used to retrieve addr), servinfo (used to store retrieved addr), p (iterator)
	struct addrinfo hints, *servinfo, *p;

//int rv: return value; i: iterator; yes: optval set in setsockopt
	int rv, i = 0, yes = 1;

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
			exit(1);
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
		freeaddrinfo(servinfo); //no longer need this struct
		return 2;
	}

	if(servinfo)
		freeaddrinfo(servinfo); //no longer need this struct
	return sockfd;
}
/******
 *
 *
 * int main()
 *
 *
*/ 

int main(int argc, char *arv[]){
//FileDescriptors: sockfd (listen), new_fd (new connections)
	int sockfd, new_fd;
//STRUCTS
//sigaction: sa (used to make sys call to change action taken on receipt of a certain sig)
	struct sigaction sa;
//sockaddr_storage: their_addr (IP Address of the requestor)
	struct sockaddr_storage their_addr;
//socklen_t: sin_size (set size of socket)
	socklen_t sin_size;
//	char buf[MAXSIZE]
//char: s (stores the IP Addr of the incoming connection so it can be diplayed)
	char s[INET6_ADDRSTRLEN];
//char: reqbuf (request key)
	char reqbuf[REQSIZE];
//int: numbytes (received)
	int numbytes;
	
	sockfd = get_socket();

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

	gnutls_global_init(); //Initialize gnutls

	printf("server: waiting for connection\n");

	while(1){
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1){
			perror("accept");
			continue;
		}
		
		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);

		printf("Accepting Connection from: %s\n", s);

		if((numbytes = recv(new_fd, &reqbuf, REQSIZE-1, 0)) == -1){
			fprintf(stderr, "Code reqrecv %s\n", strerror(errno));
			perror("");
			continue;
		}

		reqbuf[numbytes] = '\0';
		printf("Received Transmission: %s\n", reqbuf);
		if(!strncmp(reqbuf, "CARS", REQSIZE)){
			printf("Type CAR, forking....\n");
			if(!fork()){
				printf("Forked, retreiving number of cars\n");
				if(numberofcars(new_fd, reqbuf)){
					numberofcars(new_fd, reqbuf);
				}
				printf("Done....returning to Listening state\n\n");
				break;
			}
		}else

		if(!strncmp(reqbuf, "AUTH", REQSIZE)){
			printf("Type AUTH, forking...\n");
			if(!fork()){
				int authret;
				printf("Forked, processing request\n");
				if(!(authret = authrequest(new_fd, reqbuf))){
					sendAOK(new_fd);
					movekey(new_fd);
				}
				else{
					sendNopes(authret, new_fd);
				}
				break;
			}
		} else 

		if(!strncmp(reqbuf, "DHKE", REQSIZE)){
			printf("Type DHKE, forking...\n");
			if(!fork()){
				int authret;
				printf("Forked, processing request\n");
				if(!(authret = dhkerequest(new_fd, reqbuf))){
					sendAOK(new_fd);
				}
				else{
					sendNopes(authret, new_fd);
				}
				break;
			}
		} else
	
		if(1){
		}

		close(new_fd);
	}

	return 0;
}
