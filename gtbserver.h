#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "mysqlconn.h"

#define PORT "4680"
#define BACKLOG 1
#define REQSIZE 5
#define LOGINSIZE 18
#define AUTHSIZE 32

void sigchld_handler(int s);
void *get_in_addr(struct sockaddr *sa);
int authrequest(int sockfd, char *reqbufptr);
void getclientinfo(int sockfd, char *hash);
int checkhash(char hash);
int numberofcars(int new_fd, char *reqbufptr);

/*Return Values:
-4: Error MySQL query prep
-3: reqbuf did not equal the value it was supposed to
-2: the hash query returned more than one result (should not be possible) - invalid login
-1: No hash was povided by client
*/
