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


#include "gtbserver.hpp"
#include "gtbcommunication.hpp"
#include "patron.pb.h"

using namespace std;

int 
main(int argc, char *arv[])
{
//FileDescriptors: sockfd (listen), new_fd (new connections)
  int sockfd, fdAccepted;
  GTBCommunication aComm;
  Request aPBReq;

  sockfd = aComm.getSocket();
  cout << "Establish Incoming Connections" << endl;;
  initIncomingCon (&sockfd);
  cout << "Continuing..." << endl;

  //Initialize gnutls
  aComm.initGNUTLS();

  cout << "server: waiting for connection" << endl;
  fdAccepted = aComm.listeningForClient (sockfd);
  aComm.handleConnection(fdAccepted, sockfd);
  //cout << "\nHandle Request" << endl;
  try
  {
    aComm.receiveRequest(&aPBReq);
  } catch (PatronException &e)
  {
    try
    {
      Request * apPBReq = new Request();
    }
    catch (PatronException &ex)
    {
      close(fdAccepted);
      aComm.initGNUTLS();
    }
  }
  int handledreqerr = 0;
  if((handledreqerr = aComm.dealWithReq(aPBReq)))
  {
    cerr << "ERROR: dealWithReq returned with value: " << handledreqerr
      << endl;
  }
  close(fdAccepted);
  aComm.initGNUTLS();
}
