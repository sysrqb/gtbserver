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

#define PORT "4680"
#define BACKLOG 1
#define REQSIZE 4
#define LOGINSIZE 18

void sigchld_handler(int);
void *get_in_addr(struct sockaddr);
int authrequest(int, char, int);

