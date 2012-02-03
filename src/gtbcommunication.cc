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

#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "patron.pb.h"
#include <iostream>
#include <string>

#include "gtbcommunication.hpp"
#include "sqlconn.hpp"

using namespace std;

int
GTBCommunication::sendFailureResponse(int nerr)
{
  Response aPBRes;
  switch (nerr)
  {
    case 1:
    case 2:
      aPBRes.set_nrespid(2);
      aPBRes.set_sresvalue("Invalid Request Type");
      break;
    case 3:
      aPBRes.set_nrespid(3);
      aPBRes.set_sresvalue("Failed to perform handshake");
      //Need to handle differently.
      //TLS session has not been established yet,
      //Can we send a plain-text packet?
      return 0;
    default:
      aPBRes.set_nrespid(-1);
      aPBRes.set_sresvalue("Unknown error");
  }
  string sResp;
  aPBRes.SerializeToString(&sResp);
  cout << "C: Sending Failure Response" << endl;
  ssize_t nretval = gnutls_record_send(m_aSession, &sResp, sizeof(sResp));
  cout << "C: Bytes sent: " << sizeof(sResp) << " ?= " << nretval << endl;
  return nretval;
}

int 
GTBCommunication::sendAOK()
{
  char cOk = 0;
  int nNumBytes;

  if((nNumBytes = gnutls_record_send (m_aSession, &cOk, sizeof cOk)) == -1)
  {
    cerr << "Error on send for OK: " << strerror(errno) << endl;
  }
  cout << "Number of bytes sent: " << nNumBytes << endl;
	
  return nNumBytes;
}

int 
GTBCommunication::sendNopes(int i_nRetVal)
{
  int nNumBytes;

  if ((nNumBytes = gnutls_record_send (m_aSession, &i_nRetVal, sizeof i_nRetVal)) == -1){
    cerr << "Error on send for retval: " << strerror(errno) << endl;
  }
  cout << "Number of bytes sent: " << nNumBytes << endl;
	
  return nNumBytes;
}

int 
GTBCommunication::sendNumberOfCars(Request * i_aPBReq)
{
  char nNumOfCars = 7;
  int nNumBytes;
  const char * pReqBuf;

  pReqBuf = i_aPBReq->sreqtype().c_str();
  cout << "C: Function: numberOfCars: " << i_aPBReq->sreqtype() << endl;
  if(0 != strncmp("CARS", pReqBuf, 5))
  {
    cerr << "C: Not CARS" << endl;
    return -3;
  }
  cout << "C: Sending packet" << endl;

  Response aPBRes;
  aPBRes.set_nrespid(0); //Successful parse
  aPBRes.set_sresvalue("CARS");
  aPBRes.add_nresadd(nNumOfCars);
  if (!aPBRes.IsInitialized())
    cerr << "C: Response not properly formatted!" << endl;


  int npbsize = aPBRes.ByteSize();
/*  if ((nNumBytes = gnutls_record_send(m_aSession, &npbsize, aPBRes.ByteSize())) < 0)
    cerr << "C: Error on sending size of buf: " << strerror(errno) << endl;
  cout << " C: " << aPBRes.ByteSize() << ":" << nNumBytes << " bytes written: Size of Response" << endl;

  char vchars[12] = { 'a', ' ', 'f', 'e', 'w', ' ', 'b', 'y', 't', 'e', 's', '\0'};
  if ((nNumBytes = gnutls_record_send(m_aSession, vchars, sizeof(vchars))) < 0)
    cerr << "C: Error on sending size of buf: " << strerror(errno) << endl;
  cout << " C: " << sizeof(vchars) << ":" << nNumBytes << " bytes written: A few bytes" << endl;

  string vtest = "Test 1 2 3 4";
  if ((nNumBytes = gnutls_record_send(m_aSession, vtest.c_str(), sizeof(vtest))) < 0)
    cerr << "C: Error on sending size of buf: " << strerror(errno) << endl;
  cout << " C: " << sizeof(vtest) << ":" << nNumBytes << " bytes written: Test 1 2 3 4" << endl;
*/
  string sResponse;
  aPBRes.SerializeToString(&sResponse);

  if ((nNumBytes = gnutls_record_send (
      m_aSession, 
      sResponse.c_str(), 
      npbsize)) == -1){
    cerr << "C: Error on send for cars: " << strerror(errno) << endl;
  }
  cout << "C: " << npbsize << ":" << nNumBytes << " Send: ";
  for (int i = 0; i<sResponse.length(); i++)
    cout << (int)sResponse.at(i) << " ";
  cout << endl;
  cout << "C: Number of bytes sent: " << nNumBytes << endl;
  return nNumBytes;
}

int 
GTBCommunication::moveKey() 
{
  //TODO
  return 0;
}

int 
GTBCommunication::authRequest (Request i_aPBReq, int i_fdSock)
{
  int retval;
  string sHash;
	
  if(0 != i_aPBReq.sreqtype().compare("AUTH"))
  {
    cerr << "Not AUTH" << endl;
    return -3;
  }
  getClientInfo(i_fdSock);
	/*if((retval = checkhash(*hash)) < 1){
		fprintf(stderr, "Authenication Failed\n");
		return retval;
	}*/
	

  return 0;
}

void
GTBCommunication::initTLSSession ()
{

  int retval;

 //Initialize session
  if ((retval = gnutls_init (&m_aSession, GNUTLS_SERVER)))
  {
    cerr << "initTLSSession: gnutls_init error code: %d" << retval << endl;
    exit(1);
  }
  //Sets priority of session, i.e. specifies which crypto algos to use
  if ((retval = gnutls_priority_set (m_aSession, *m_pPriorityCache))) 
  {
    cerr << "initTLSSession: gnutls_priority_set error code: %d" << retval << endl;
    exit(1);
  }
 //Sets the needed credentials for the specified type; an x509 cert, in this case
  if ((retval = gnutls_credentials_set (
      m_aSession, 
      GNUTLS_CRD_CERTIFICATE,
      *m_pX509Cred))) 
  {
    cerr << "initTLSSession: gnutls_credentials_set error code:  %d" 
        << retval << endl;
    exit (1);
  }
//Request client cert
  gnutls_certificate_server_set_request (m_aSession, GNUTLS_CERT_REQUEST);
  gnutls_db_set_store_function (m_aSession, c_store_connection);
  gnutls_db_set_retrieve_function (m_aSession, c_retrieve_connection);
}

int
GTBCommunication::generateDHParams ()
{
  /*gnutls_pk_algorithm_t aPKAlgo = gnutls_pk_get_id (DH_PK_ALGO);
  int bits = gnutls_sec_param_to_pk_bits (aPKAlgo, DH_BITS);
  */
  gnutls_dh_params_init(m_pDHParams);
  gnutls_dh_params_generate2(*m_pDHParams, DH_BITS);

  return 0;
}

void
GTBCommunication::loadCertFiles ()
{
  const char ** err_pos; //store returned code (error or successful)
  int retval;

  cout << "loadCertFiles: allocate creds" << endl;
  if ((retval = gnutls_certificate_allocate_credentials (m_pX509Cred)))
  {
    //TODO
    cout <<"loadCertFiles: gnutls_certificate_allocate_credentials: false" 
        << endl;
  }
  cout << "loadCertFiles: load cert trust file" << endl;
  if ((retval = gnutls_certificate_set_x509_trust_file (
      *m_pX509Cred, 
      CAFILE, 
      GNUTLS_X509_FMT_PEM)))
  {
    //TODO
    if ( retval > 0 )
      cout << "loadCertFiles: gnutls_certificate_set_x509_trust_file: " << 
          "certs loaded: " << retval << endl;
    else
      cerr << "loadCertFiles: gnutls_certificate_set_x509_trust_file " << 
          "error code: " << strerror(retval) << endl;
  }
  cout << "loadCertFiles: load CSL" << endl;
  if((retval = gnutls_certificate_set_x509_crl_file (*m_pX509Cred, CRLFILE, 
                                        GNUTLS_X509_FMT_PEM)) < 1)
  {
    //TODO
    cerr << "loadCertFiles: gnutls_certificate_set_x509_crl_file error code: %d"
        << retval << endl;
  }
    
  cout << "loadCertFiles: load key" << endl;
  if (( retval = gnutls_certificate_set_x509_key_file (
                   *m_pX509Cred,
		   CERTFILE,
		   KEYFILE,
		   GNUTLS_X509_FMT_PEM)))
      cerr << "loadCertFiles: gnutls_certificate_set_x509_key_file " << 
          "error code: " << strerror(retval) << endl;
  

  cout << "loadCertFiles: gen DH params" << endl;
  generateDHParams ();
  cout << "loadCertFiles: priority init" << endl;
  //Set gnutls priority string
  
  const char * cErrLoc;
  if((retval = gnutls_priority_init (m_pPriorityCache, GNUTLS_PRIORITY, &cErrLoc)))
  {
    //TODO
    cerr << "loadCertFiles: gnutls_priority_init error code: &s" << *cErrLoc << endl;
  }

  cout << "Set priority: " << GNUTLS_PRIORITY << endl;

  gnutls_certificate_set_dh_params (*m_pX509Cred, *m_pDHParams);
  
}

void * GTBCommunication::getInAddr (struct sockaddr *i_sa)
{

  if(i_sa->sa_family == AF_INET)
  {
    return &(((struct sockaddr_in*)i_sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)i_sa)->sin6_addr);
}


void GTBCommunication::getClientInfo(int i_fdSock)
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

int 
GTBCommunication::getSocket()
{
//FileDescriptors: sockfd (listen), new_fd (new connections)
  int fdSock;

/*
   STRUCTS
   addrinfo: aHints (used to retrieve addr), 
   pServinfo (used to store retrieved addr), pPIterator (iterator)
 */
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
GTBCommunication::dealWithReq (Request i_aPBReq, int i_fdSock)
{
  /*
   * Case 0: Curr
   * Case 1: AUTH
   * Case 2: CARS
   */

  int npid = 1;
  switch (i_aPBReq.nreqid())
  {
    case 1: //AUTH
      cout << "Type AUTH, forking..." << endl;
      if(!(npid = fork ()))
      {
        int nAuthRet;
        cout << "C: Forked, processing request" << endl;
        if(!(nAuthRet = authRequest (i_aPBReq, i_fdSock)))
	{
          sendAOK ();
          moveKey();
        }
        else
        {
          sendNopes(nAuthRet);
        }
        return 0;
      }
      break;
    case 2: //CARS
      cout << "C: Type CAR, forking...." << endl;
      if(!(npid = fork()))
      {
        cout << "C: " << getpid() << " Forked, retrieving number of cars" << endl;
        if(sendNumberOfCars (&i_aPBReq) < 0)
	  return sendFailureResponse(2);
        cout << "Done....exiting parent\n" << endl;
        return 0;
      }
      break;
  }
  cout << "Child PID: " << npid << " " << getpid() << endl;
  return 0;
}

int 
GTBCommunication::listeningForClient (int i_fdSock)
{
  int fdAccepted = 0;
//sockaddr_storage: aClientAddr (IP Address of the requestor)
  struct sockaddr_storage aClientAddr;
//socklen_t: nSinSize (set size of socket)
  socklen_t nSinSize;
//char: vAddr (stores the IP Addr of the incoming connection so it can be diplayed)
  char vAddr[INET6_ADDRSTRLEN] = "";
//char: reqbuf (request key)
  char vReqBuf[REQSIZE] = "";
  char * ntopRetVal;
//int: numbytes (received)
  int nNumBytes=0;

  nSinSize = sizeof aClientAddr;
  while(1)
  {
    cout << endl;
    cout << "Initialize TLS Session" << endl;
    initTLSSession();
    gnutls_certificate_server_set_request (m_aSession, 
                        GNUTLS_CERT_REQUIRE); //Require client to provide cert
    gnutls_certificate_send_x509_rdn_sequence  (
                 m_aSession, 
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

    if ( vAddr == NULL ) 
    {
      cerr << "Failed to convert the client IP Address!";
      cerr << " Error: " << strerror(errno) << endl;
      cerr << "Closing connection..." << endl;
      close(fdAccepted);
      continue;
    }

    cout << "Accepting Connection from: " << vAddr << endl;
    //They're already char*, might as well just compare without wasting
    //the time to convert them
    strncpy(m_vIPAddr, vAddr, INET6_ADDRSTRLEN);

    cout << "Start TLS Session" << endl;
    gnutls_transport_set_ptr (m_aSession, (gnutls_transport_ptr_t) fdAccepted);
    cout << "Performing handshake.." << endl;
    int nRetVal, i=0;
    do
    {
      i++;
      nRetVal = gnutls_handshake (m_aSession);
      cout << "Return value: " << nRetVal << endl;
      if ( nRetVal == -59 || nRetVal == -10)
        break;
    } while (gnutls_error_is_fatal (nRetVal) != GNUTLS_E_SUCCESS);
    
    if ( nRetVal < 0)
    {
      cerr << i << " Failed to perform handshake, error code : ";
      cerr << gnutls_strerror(nRetVal) << endl;;
      cerr << "Closing connection..." << endl;
      sendFailureResponse(3);
      close(fdAccepted);
      gnutls_deinit(m_aSession);
      continue;
    }

    gnutls_cipher_algorithm_t aUsingCipher =
        gnutls_cipher_get (m_aSession);
    const char * sCipherName;
    sCipherName = gnutls_cipher_get_name(aUsingCipher);
    cout << "Using cipher: " << sCipherName << endl;

    unsigned int nstatus;
    if( gnutls_certificate_verify_peers2 (m_aSession, &nstatus) )
    {
      cerr << "Failed to verify client certificate, error code ";
      cerr << "Closing connection..." << endl;
      close(fdAccepted);
      gnutls_deinit(m_aSession);
      continue;
    }

    if (nstatus)
    {
      cerr << "Failed to verify client certificate, error code ";
      cerr << "Closing connection..." << endl;
      close(fdAccepted);
      gnutls_deinit(m_aSession);
      continue;
    }

    cout << "Storing connection information" << endl;

    cout << "Receiving request" << endl;

    if((nNumBytes = gnutls_record_recv (m_aSession, &vReqBuf, REQSIZE-1)) < 0)
    //if((numbytes = recv(new_fd, &reqbuf, REQSIZE-1, 0)) == -1)
    {
      cerr << "Code reqrecv " << strerror(errno) << endl;
      continue;
    }

    vReqBuf[nNumBytes] = '\0';
    cout << "Received Transmission Size: " << nNumBytes << endl;

    Request aPBReq;
    aPBReq.ParseFromString(vReqBuf);
    aPBReq.PrintDebugString();

    if(!dealWithReq(aPBReq, fdAccepted))
      continue;
    else
    {
      close(fdAccepted);
      gnutls_deinit(m_aSession);
      break;
    }
  }
  exit(0);
}
