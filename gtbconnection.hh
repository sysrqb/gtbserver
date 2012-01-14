#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h> //exxit()
#include "gtbconnection.hh"

using namespace std;
namespace gtb {

void GTBConnection::getClientInfo(int i_fdSock)
{
  int numbytes;
  char vHash[AUTHSIZE];
  cout<<"Get hash"<<endl;
  if((numbytes = recv(i_fdSock, vHash, AUTHSIZE-1, 0)) == -1)
  {
    std::cerr<<"getClientInfo: reqrecv"<<endl;
    exit(1);
  }

  vHash[numbytes] = '\0';
  m_sHash = string(vHash);
  cout<<"Received Hash: " << m_sHash << endl;
}

/******
 *
 *
 * int getsocket()
 *
 *
*******/ 


int 
GTBConnection::getSocket()
{
//FileDescriptors: sockfd (listen), new_fd (new connections)
  int fdSock;

//STRUCTS
//addrinfo: aHints (used to retrieve addr), 
//pServinfo (used to store retrieved addr), pPIterator (iterator)
  struct addrinfo aHints, aServinfo, *pServinfo, *pIterator;

//int rv: return value; i: iterator; yes: optval set in setsockopt
  int nRV, i = 0, nYes = 1;

  memset(&aHints, 0, sizeof aHints);

//struct set to allow for binding and connections
  aHints.ai_family = AF_UNSPEC;
  aHints.ai_socktype = SOCK_STREAM;
  aHints.ai_flags = AI_PASSIVE; //uses extern local IP

  pServinfo = &aServinfo;

//Retrieve addr and socket
  if((nRV = getaddrinfo(NULL, PORT, &aHints, &pServinfo)) != 0)
  {
    cerr << "getaddrinfo: " << gai_strerror(nRV) << endl;
    return 1;
  }

//Iterate through servinfo and bind to the first available
  for(pIterator = pServinfo; pIterator != NULL; pIterator = pIterator->ai_next)
  {
    i++;
    //If socket is unseccessful in creating an
    if ((fdSock= socket(pIterator->ai_family, pIterator->ai_socktype, 
      pIterator->ai_protocol)) == -1) 
    { //endpoint, e.g. filedescriptor, then it returns -1,
      cerr << "server: socket" << endl; //so continue to the next addr
      continue;
    }

    if(setsockopt(fdSock, 
                  SOL_SOCKET, 
		  SO_REUSEADDR, 
		  &nYes, //If unsuccessful in setting socket options
		  sizeof nYes) == -1)
    { //exit on error
      cerr << "setsockopt" << endl;
      exit(1);
    }

    if(bind(fdSock, pIterator->ai_addr, pIterator->ai_addrlen) == -1)
    { //Assign a name to an addr
      close(fdSock);	//If unsuccessful, print error and check next
      cerr << "server: bind" << endl;
      continue;
    }

    break; //if successful, exit loop
  }

  if(pIterator == NULL)
  {
    cerr << "server: Failed to bind for unknown reason" << "\n" <<
        " Iterated through loop" << i << " times" << endl;
    freeaddrinfo(pServinfo); //no longer need this struct
    return 2;
  }

  if(pServinfo)
    freeaddrinfo(pServinfo); //no longer need this struct
  return fdSock;
}

int 
GTBConnection::dealWithReq (string i_sReqBuf, int i_fdSock, GTBCommunication * i_pComm)
{
  if(!i_sReqBuf.compare("CARS"))
  {
    cout << "Type CAR, forking...." << endl;
    if(!fork())
    {
      cout << "Forked, retreiving number of cars" << endl;
      if(i_pComm->sendNumberOfCars (i_sReqBuf))
      {
        i_pComm->sendNumberOfCars (i_sReqBuf);
      }
      cout << "Done....returning to Listening state\n" << endl;
      return 0;
    }
  }
  else
  {
    if(!i_sReqBuf.compare ("AUTH"))
    {
      cout << "Type AUTH, forking..." << endl;
      if(!fork ())
      {
        int nAuthRet;
        cout << "Forked, processing request" << endl;
        if(!(nAuthRet = i_pComm->authRequest (i_sReqBuf, i_fdSock, this)))
	{
          i_pComm->sendAOK ();
          i_pComm->moveKey();
        }
        else
        {
          i_pComm->sendNopes(nAuthRet);
        }
        return 0;
      }
    } 
    else
    {
      if(!i_sReqBuf.compare ("DHKE"))
      {
        cout << "Type DHKE, forking..." << endl;
        if(!fork())
	{
          int nAuthRet;
          cout << "Forked, processing request" << endl;
          if(!(nAuthRet = i_pComm->DHKERequest (i_sReqBuf)))
	  {
            i_pComm->sendAOK();
          }
          else
	  {
            i_pComm->sendNopes(nAuthRet);
          }
          return 0;
        }
      }
    }
  }
  return 0;
}


int 
GTBConnection::listeningForClient (int i_fdSock)
{
  int fdAccepted;
//sockaddr_storage: their_addr (IP Address of the requestor)
  struct sockaddr_storage aClientAddr;
//socklen_t: sin_size (set size of socket)
  socklen_t nSinSize;
//	char buf[MAXSIZE]
//char: s (stores the IP Addr of the incoming connection so it can be diplayed)
  char vAddr[INET6_ADDRSTRLEN];
//char: reqbuf (request key)
  char vReqBuf[REQSIZE];
//int: numbytes (received)
  int nNumBytes;

  nSinSize = sizeof aClientAddr;
  while(1)
  {
    cout << "Initialize TLS Session" << endl;
    i_pComm->initTLSSession();
    gnutls_certificate_server_set_request (i_pComm->m_aSession, 
                         GNUTLS_CERT_REQUIRE); //Require client to provide cert
    gnutls_certificate_send_x509_rdn_sequence  (
                 i_pComm->m_aSession, 
		 1); //REMOVE IN ORDER TO COMPLETE CERT EXCHANGE

    cout << "Accepting Connection" << endl;
    fdAccepted = accept(i_fdSock, (struct sockaddr *)&aClientAddr, &nSinSize);
    cout << "accept fdAccepted:" << fdAccepted << endl;
    if (fdAccepted == -1)
    {
      cerr << "Error at accept!" << endl;
      continue;
    }
		
    inet_ntop(aClientAddr.ss_family, 
              getInAddr((struct sockaddr *)&aClientAddr), 
	      vAddr, sizeof vAddr);

    cout << "Accepting Connection from: " << vAddr << endl;
    //They're already char*, might as well just compare without wasting
    //the time to convert them
    strncpy(m_vIPAddr, vAddr, INET6_ADDRSTRLEN);

    cout << "Start TLS Session" << endl;
    gnutls_transport_set_ptr (i_pComm->m_aSession, (gnutls_transport_ptr_t) fdAccepted);
    int nRetVal;
    if ((nRetVal = gnutls_handshake (i_pComm->m_aSession)))
    {
      cerr << "Failed to perform handshake, error code : " << gnutls_strerror(nRetVal) << endl;;
      cerr << "Closing connection..." << endl;
      close(fdAccepted);
      gnutls_deinit(i_pComm->m_aSession);
      continue;
    }

    cout << "Receiving request" << endl;

    if((nNumBytes = gnutls_record_recv (i_pComm->m_aSession, &vReqBuf, REQSIZE-1)) < 0)
    //if((numbytes = recv(new_fd, &reqbuf, REQSIZE-1, 0)) == -1)
    {
      cerr << "Code reqrecv " << strerror(errno) << endl;
      continue;
    }

    vReqBuf[nNumBytes] = '\0';
    cout << "Received Transmission: " << vReqBuf << endl;

    if(!dealWithReq(vReqBuf, fdAccepted, i_pComm))
      continue;
		

    close(fdAccepted);
  }
}

}

