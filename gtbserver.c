#include "gtbserver.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>

#include "gtbcommunication.hpp"


/*  struct GNUTLSArgs {
    gnutls_certificate_credentials_t * x509_cred;
    gnutls_priority_t * priority_cache;
    gnutls_dh_params_t dh_params;
  } * pGNUTLSArgs ,aGNUTLSArgs;
*/

/*
 * function sigchld_handler
 *
 *SIGCHLD handler: wait for all dead (zombie) processes
*/
extern "C" {
void 
sigchld_handler (int s)
{
  while(waitpid(-1, NULL, WNOHANG) > 0);
}


void *initIncomingCon(void * i_pfdSock)
{
  int fdSock, * pfdSock;
  struct sigaction sa, * pSa;

  pfdSock = (int *)i_pfdSock;
  fdSock = *pfdSock;


   if (listen(fdSock, BACKLOG) == -1)
  { //marks socket as passive, so it accepts incoming connections
    perror("listen: failed to mark as passive\n");
    exit(1);
  }
  printf("Socket marked as passive...listening for connections\n");

  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if(sigaction(SIGCHLD, &sa, NULL) == -1)
  {
    perror("sigaction");
    exit(1);
  }
  return NULL;
} 

}

void *initGNUTLS( void * ptr)
{
  GTBCommunication aComm = *(GTBCommunication *)ptr;
  printf("Initialize gnutls\n");
  if( gnutls_global_init() ) printf("gnutls_global_init: Failed to intialize\n");
  aComm.loadCertFiles();
  return NULL;
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

  //Thread structures
  //pthread_t * pIncomingConThread, * pGNUTLSThread;

  GTBCommunication aComm;

  sockfd = aComm.getSocket();

  printf("Establish Incoming Connections\n");
  /*printf("Spawning PassiveSocket Thread\n");
  if (
  pthread_create(
      pIncomingConThread, 
      NULL, 
      initIncomingCon, 
      &sockfd ) )
    fprintf(stderr, "Thread Create Failed on initIncomingCon\n" );
  */
  initIncomingCon (&sockfd);
  //printf("Continuing...\n");

  //Initialize gnutls
  /*printf("Spawning GNUTLS Thread\n");
  pthread_create(
      pGNUTLSThread,
      NULL,
      initGNUTLS,
      NULL);
  */
  initGNUTLS (&aComm);

  //void * retval;
  //pthread_join(*pGNUTLSThread, &retval);
  //pthread_join(*pIncomingConThread, &retval);

  printf("server: waiting for connection\n");
  return aComm.listeningForClient (sockfd);
  
}
