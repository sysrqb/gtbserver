/*
 * GUARD the Bridge
 * Copyright (C) 2012  Matthew Finkel <Matthew.Finkel@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gtbserver.h"

#if THREADS
#include <pthread.h>
#endif

#define MAX_SESSION_ID_SIZE 32
#define MAX_SESSION_DATA_SIZE 512
#define TLS_SESSION_CACHE 10


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

void *initGNUTLS( void * ptr)
{
  GTBCommunication aComm = *(GTBCommunication *)ptr;
  printf("Initialize gnutls\n");
  if( gnutls_global_init() ) printf("gnutls_global_init: Failed to intialize\n");
  aComm.loadCertFiles();
  return NULL;
}

/* Functions and other stuff needed for session resuming.
 * This is done using a very simple list which holds session ids
 * and session data.
 *
 * Stolen from GNUTLS documentation
 */
#if __cplusplus
extern "C" {
#endif
typedef struct
{
  char session_id[MAX_SESSION_ID_SIZE];
  size_t session_id_size;

  char session_data[MAX_SESSION_DATA_SIZE];
  size_t session_data_size;
} CACHE;

static CACHE *cache_db;
static int cache_db_ptr = 0;

static void
wrap_db_init (void)
{

  /* allocate cache_db */
  cache_db = (CACHE *)calloc (1, TLS_SESSION_CACHE * sizeof (CACHE));
}

static void
wrap_db_deinit (void)
{
  free (cache_db);
  cache_db = NULL;
  return;
}

static int
wrap_db_store (void *dbf, gnutls_datum_t key, gnutls_datum_t data)
{

  if (cache_db == NULL)
    return -1;

  if (key.size > MAX_SESSION_ID_SIZE)
    return -1;
  if (data.size > MAX_SESSION_DATA_SIZE)
    return -1;

  memcpy (cache_db[cache_db_ptr].session_id, key.data, key.size);
  cache_db[cache_db_ptr].session_id_size = key.size;

  memcpy (cache_db[cache_db_ptr].session_data, data.data, data.size);
  cache_db[cache_db_ptr].session_data_size = data.size;

  cache_db_ptr++;
  cache_db_ptr %= TLS_SESSION_CACHE;

  return 0;
}

static gnutls_datum_t
wrap_db_fetch (void *dbf, gnutls_datum_t key)
{
  gnutls_datum_t res = { NULL, 0 };
  int i;

  if (cache_db == NULL)
    return res;

  for (i = 0; i < TLS_SESSION_CACHE; i++)
    {
      if (key.size == cache_db[i].session_id_size &&
	  memcmp (key.data, cache_db[i].session_id, key.size) == 0)
	{


	  res.size = cache_db[i].session_data_size;

	  res.data = (unsigned char *) gnutls_malloc (res.size);
	  if (res.data == NULL)
	    return res;

	  memcpy (res.data, cache_db[i].session_data, res.size);

	  return res;
	}
    }
  return res;
}

static int
wrap_db_delete (void *dbf, gnutls_datum_t key)
{
  int i;

  if (cache_db == NULL)
    return -1;

  for (i = 0; i < TLS_SESSION_CACHE; i++)
    {
      if (key.size == cache_db[i].session_id_size &&
	  memcmp (key.data, cache_db[i].session_id, key.size) == 0)
	{

	  cache_db[i].session_id_size = 0;
	  cache_db[i].session_data_size = 0;

	  return 0;
	}
    }

  return -1;

}

#if __cplusplus
}
#endif

int 
main(int argc, char *arv[])
{
//FileDescriptors: sockfd (listen), new_fd (new connections)
  int sockfd;
//
//sigaction: sa (used to make sys call to change action taken on receipt of a certain sig)
  struct sigaction sa;

#if THREADS
  pthread_t * pIncomingConThread, * pGNUTLSThread;
#endif

  GTBCommunication aComm;

  sockfd = aComm.getSocket();

  printf("Establish Incoming Connections\n");
#if THREADS
  printf("Spawning PassiveSocket Thread\n");
  if (
  pthread_create(
      pIncomingConThread, 
      NULL, 
      initIncomingCon, 
      &sockfd ) )
    fprintf(stderr, "Thread Create Failed on initIncomingCon\n" );
#else 
  initIncomingCon (&sockfd);
#endif
  printf("Continuing...\n");

  //Initialize gnutls
#if THREADS
  printf("Spawning GNUTLS Thread\n");
  if (
  pthread_create(
      pGNUTLSThread,
      NULL,
      initGNUTLS,
      (void *)&aComm) )
    fprintf(stderr, "Thread Create Failed on initGNUTLS\n" );
#else
  initGNUTLS (&aComm);
#endif

#if THREADS
  void * retval;
  pthread_join(*pGNUTLSThread, &retval);
  pthread_join(*pIncomingConThread, &retval);
#endif

  printf("server: waiting for connection\n");
  return aComm.listeningForClient (sockfd);
  
}
