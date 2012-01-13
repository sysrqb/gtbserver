#include "gtbserver.h"

//Global
char ipaddr[INET6_ADDRSTRLEN];

/*
 *
 *
 * function sigchld_handler
 *
 *SIGCHLD handler: wait for all dead (zombie) processes
 *
 *
*/

extern "C" 
void 
sigchld_handler (int s)
{
  while(waitpid(-1, NULL, WNOHANG) > 0);
}



/******
 *
 *
 * int main()
 *
 *
*******/ 

int 
main(int argc, char *arv[])
{
//FileDescriptors: sockfd (listen), new_fd (new connections)
  int sockfd;
//STRUCTS
//sigaction: sa (used to make sys call to change action taken on receipt of a certain sig)
  struct sigaction sa;
//GNUTLS settings
  gnutls_priority_t * priority_cache;
  gnutls_certificate_credentials_t * x509_cred;
  gnutls_session_t session;
  static gnutls_dh_params_t dh_params;

  GTBConnection aConn;
  GTBCommunication aComm;

  sockfd = aConn.getSocket();

  printf("Establish Incoming Connections\n");
  if (listen(sockfd, BACKLOG) == -1)
  { //marks socket as passive, so it accepts incoming connections
    perror("listen: failed to mark as passive");
    exit(1);
  }

  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if(sigaction(SIGCHLD, &sa, NULL) == -1)
  {
    perror("sigaction");
    exit(1);
  }
  
  //Initialize gnutls
  printf("Initialize gnutls\n");
  if( gnutls_global_init() ) printf("gnutls_global_init: Failed to intialize\n");


  x509_cred = (gnutls_certificate_credentials_t * )malloc(sizeof x509_cred);
  priority_cache = (gnutls_priority_t *)malloc(sizeof priority_cache);
  aComm.loadCertFiles(x509_cred, priority_cache, &dh_params);

  printf("server: waiting for connection\n");
  return aConn.listeningForClient (sockfd, &aComm);
}
