#include "comm.h"

int 
dhkerequest(int new_fd, char *reqbufptr){
	//TODO
	return 0;
}

int 
sendAOK(int new_fd, gnutls_session_t session){
	char ok = 0;
	int numofbytes;

	if((numofbytes = send(new_fd, &ok, sizeof ok, 0)) == -1){
		fprintf(stderr, "Error on send for OK: %s\n", strerror(errno));
	}
	printf("Number of bytes sent: %d\n", numofbytes);
	
	return numofbytes;
}

int 
sendNopes(int retval, int new_fd, gnutls_session_t session){
	int numofbytes;

	if((numofbytes = send(new_fd, &retval, sizeof retval, 0)) == -1){
		fprintf(stderr, "Error on send for retval: %s\n", strerror(errno));
	}
	printf("Number of bytes sent: %d\n", numofbytes);
	
	return numofbytes;
}

int 
numberofcars(int new_fd, char *reqbufptr){
	char numofcars = 7;
	int numbytes;
	printf("Function: numberofcars\n");
	if(0 != strncmp("CARS", reqbufptr, 5)){
		fprintf(stderr, "Not CARS\n");
		return -3;
	}
	printf("Sending packet\n");
	if((numbytes = send(new_fd, &numofcars, sizeof numofcars, 0)) == -1){
		fprintf(stderr, "Error on send for cars: %s\n", strerror(errno));
	}
	printf("Number of bytes sent: %d\n", numbytes);
	return 0;
}

int 
movekey(int new_fd, gnutls_session_t session){
	//TODO
	return 0;
}
