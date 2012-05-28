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


#include <iostream>
#include <csignal>
#include <cstring>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include "gtbserver.h"
#include "gtbcommunication.hpp"

using namespace std;


/*
 * function sigchld_handler
 *
 *SIGCHLD handler: wait for all dead (zombie) processes
*/
void 
sigchld_handler (int s)
{
/* Wait for all dead processes.
 * We use a non-blocking call to be sure this signal handler will not
 * block if a child was cleaned up in another part of the program.
 */
  while(waitpid(-1, NULL, WNOHANG) > 0);
}


void initIncomingCon(int * pfdSock)
{
  int fdSock;
  struct sigaction sa, * pSa;

  fdSock = *pfdSock;


   if (listen(fdSock, BACKLOG) == -1)
  { //marks socket as passive, so it accepts incoming connections
    cerr << "listen: failed to mark as passive" << endl;
    exit(1);
  }
  cout << "Socket marked as passive...listening for connections" << endl;

  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if(sigaction(SIGCHLD, &sa, NULL) == -1)
  {
    cerr << "sigaction: unable to set syscalls restartable across signals" << endl;
    exit(1);
  }
} 

inline void initGNUTLS(GTBCommunication * aComm)
{
  cout << "Initialize gnutls" << endl;
  if( gnutls_global_init() ) cout << "gnutls_global_init: Failed to intialize" << endl;
  aComm->loadCertFiles();
}

int 
main(int argc, char *arv[])
{
//FileDescriptors: sockfd (listen), new_fd (new connections)
  int sockfd;
//
//sigaction: sa (used to make sys call to change action taken on receipt of a certain sig)
  struct sigaction sa;

  GTBCommunication aComm;

  sockfd = aComm.getSocket();

  cout << "Establish Incoming Connections" << endl;;
  initIncomingCon (&sockfd);
  cout << "Continuing..." << endl;

  //Initialize gnutls
  initGNUTLS (&aComm);

  cout << "server: waiting for connection" << endl;
  return aComm.listeningForClient (sockfd);
}