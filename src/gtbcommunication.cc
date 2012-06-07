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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "patron.pb.h"
#include <iostream>
#include <string>

#include "gtbcommunication.hpp"
#include "gtbexceptions.hpp"
#include "sqlconn.hpp"

#define MAX_SESSION_ID_SIZE 32
#define MAX_SESSION_DATA_SIZE 512
#define TLS_SESSION_CACHE 10

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
  ssize_t nretval = gnutls_record_send(m_aSession, sResp.c_str(), sizeof(sResp));
  cout << "C: Bytes sent: " << sizeof(sResp) << " ?= " << nretval << endl;
  return nretval;
}

int 
GTBCommunication::sendResponse(
    int i_nRetVal, 
    Request * i_pbReq,
    Response * i_pbRes,
    PatronList * i_pbPL)
{
  int nNumBytes (0);
  Response aPBRes;
  cout << "c: Replying with RetVal: " << i_nRetVal << endl;
  if (i_pbReq == NULL && i_pbRes == NULL && i_pbPL == NULL)
  {


    if (i_nRetVal)
    {
      aPBRes.set_nrespid(i_nRetVal);
      aPBRes.set_sresvalue("Error");
    }
    else
    {
      aPBRes.set_nrespid(0);
      aPBRes.set_sresvalue("Successful");
    }

    cout << "C: Buffer Contents:" << endl;
    aPBRes.PrintDebugString();

    string sResp = "";
    aPBRes.SerializeToString(&sResp);
    if((nNumBytes = gnutls_record_send (m_aSession, sResp.c_str(), aPBRes.ByteSize())) == -1)
    {
      cerr << "ERROR: C: Error on send for OK: " << strerror(errno) << endl;
    }
    cout << "C: Number of bytes sent: " << nNumBytes << endl;
	
    return nNumBytes;
  }
  else
    if (i_pbRes)
    {
      cout << "C: Buffer Contents: " << endl;
      i_pbRes->set_nrespid(i_nRetVal);
      i_pbRes->set_sresvalue("CURR");
      i_pbRes->CheckInitialized();
      i_pbRes->PrintDebugString();
      string spbRes = "";
      i_pbRes->SerializeToString(&spbRes);
      int nsize = i_pbRes->ByteSize();

      Response aTmpResp;
      aTmpResp.set_nrespid(nsize);
      aTmpResp.set_sresvalue("SIZE");
      string sTmpResp = "";
      aTmpResp.SerializeToString(&sTmpResp);

      cout << "Size of pre-payload: " << sTmpResp.length() << " content: " << endl;
      for (int i =0; i<sTmpResp.length(); i++)
        cout << (int)sTmpResp.at(i) << " ";
      cout << endl;

      if((nNumBytes = gnutls_record_send (m_aSession, sTmpResp.c_str(), sTmpResp.length())) == -1)
      {
        cerr << "ERROR: C: Error on send for OK: " << strerror(errno) << endl;
      }

      cout << "Size of payload: " << spbRes.length() << " content: " << endl;
      for (int i =0; i<spbRes.length(); i++)
        cout << (int)spbRes.at(i) << " ";
      cout << endl;

      if((nNumBytes = gnutls_record_send (m_aSession, spbRes.c_str(), nsize)) == -1)
      {
        cerr << "ERROR: C: Error on send for OK: " << strerror(errno) << endl;
      }
      cout << "C: Number of bytes sent: " << nNumBytes << endl;
	
      return nNumBytes;
    }
  cerr << "ERROR: C: Failed to send response!" << endl;
  return -1;
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
    cerr << "ERROR: C: Not CARS" << endl;
    return -3;
  }
  cout << "C: Sending packet" << endl;

  Response aPBRes;
  aPBRes.set_nrespid(0); //Successful parse
  aPBRes.set_sresvalue("CARS");
  aPBRes.add_nresadd(nNumOfCars);
  if (!aPBRes.IsInitialized())
    cerr << "ERROR: C: Response not properly formatted!" << endl;


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
    cerr << "ERROR: C: Error on send for cars: " << strerror(errno) << endl;
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
GTBCommunication::authRequest (Request * i_aPBReq)
{
  int retval;
  string sHash;
	
  if(0 != i_aPBReq->sreqtype().compare("AUTH"))
  {
    cerr << "ERROR: C: Not AUTH" << endl;
    return -3;
  }

  if (i_aPBReq->sparams_size() == 3)
  {
    if((retval = m_MySQLConn->checkAuth(
        i_aPBReq->sparams(0), 
	i_aPBReq->sparams(1), 
	i_aPBReq->sparams(2)))){
      cerr << "ERROR: C: Authenication Failed: " << retval << endl;;
      return retval;
    }
  }
  else
  {
    cerr << "ERROR: C: Missing Paramters: Only " << i_aPBReq->sparams_size() 
        << " provided!" << endl;
    return -1;
  }

  //TODO Store IP Address, car num, etc for checking later
	
  return 0;
}

int 
GTBCommunication::currRequest (Request * i_aPBReq, Response * i_pbRes)
{
  int nRetVal;

  /*string sRequest = "";
  i_aPBReq->SerializeToString(&sRequest);
  for (int i = 0; i<sRequest.length(); i++)
    cout << (int)sRequest.at(i) << " ";
  cout << endl;
*/
  if(0 != i_aPBReq->sreqtype().compare("CURR"))
  {
    cerr << "ERROR: C: Not CURR: " << i_aPBReq->sreqtype()  << endl;
    i_pbRes->set_nrespid(-1);
    i_pbRes->set_sresvalue("Not CURR");
    return -2;
  }
  int i = 0;
  //int * vrides = new int[i_aPBReq->nparams_size()];
  vector<int> vrides;
  for(; i < i_aPBReq->nparams_size(); i++)
    vrides.insert(vrides.end(), i_aPBReq->nparams(i));

  cout << "C: Getting Current Rides" << endl;
  nRetVal = m_MySQLConn->getCurr(i_aPBReq->ncarid(), i_pbRes->mutable_plpatronlist(), vrides);
  if (nRetVal == -1)
    i_pbRes->clear_plpatronlist();
  //delete[] vrides;
  return 0;
}


int 
GTBCommunication::updtRequest (Request * i_aPBReq, Response * i_pbRes)
{
  map<int, string> vRetVal;
  map<int, string>::iterator vrvIt;
  //int i = 0;

  /*string sRequest = "";
  i_aPBReq->SerializeToString(&sRequest);
  for (int i = 0; i<sRequest.length(); i++)
    cout << (int)sRequest.at(i) << " ";
  cout << endl;
*/
  if(0 != i_aPBReq->sreqtype().compare("UPDT"))
  {
    cerr << "ERROR: C: Not UPDT: " << i_aPBReq->sreqtype()  << endl;
    i_pbRes->set_nrespid(-1);
    i_pbRes->set_sresvalue("Not UPDT");
    return -2;
  }
  cout << "C: Setting Ride Updates" << endl;
  vRetVal = m_MySQLConn->setUpdt(i_aPBReq->ncarid(), i_pbRes->mutable_plpatronlist(), i_aPBReq);
  
  for(vrvIt = vRetVal.begin(); vrvIt != vRetVal.end(); vrvIt++)
  {
    i_pbRes->add_nresadd(vrvIt->first);
    i_pbRes->add_sresadd(vrvIt->second);
  }
  return 0;
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

/*static void
wrap_db_deinit (void)
{
  free (cache_db);
  cache_db = NULL;
  return;
}*/

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


void
GTBCommunication::initTLSSession ()
{

  int retval;

 //Initialize session
  if ((retval = gnutls_init (&m_aSession, GNUTLS_SERVER)))
  {
    cerr << "ERROR: initTLSSession: gnutls_init error code: %d" << retval << endl;
    exit(1);
  }
  //Sets priority of session, i.e. specifies which crypto algos to use
  if ((retval = gnutls_priority_set (m_aSession, *m_pPriorityCache))) 
  {
    cerr << "ERROR: initTLSSession: gnutls_priority_set error code: %d" << retval << endl;
    exit(1);
  }
 //Sets the needed credentials for the specified type; an x509 cert, in this case
  if ((retval = gnutls_credentials_set (
      m_aSession, 
      GNUTLS_CRD_CERTIFICATE,
      *m_pX509Cred))) 
  {
    cerr << "ERROR: initTLSSession: gnutls_credentials_set error code:  %d" 
        << retval << endl;
    exit (1);
  }
//Request client cert
  /*gnutls_certificate_server_set_request (m_aSession, GNUTLS_CERT_REQUEST);
  if (TLS_SESSION_CACHE != 0)
  {
    gnutls_db_set_retrieve_function (m_aSession, wrap_db_fetch);
    gnutls_db_set_remove_function (m_aSession, wrap_db_delete);
    gnutls_db_set_store_function (m_aSession, wrap_db_store);
    gnutls_db_set_ptr (m_aSession, NULL);
  }

  gnutls_certificate_server_set_request (m_aSession, 
              GNUTLS_CERT_REQUIRE); //Require client to provide cert
  gnutls_certificate_send_x509_rdn_sequence  (
              m_aSession, 
              1); //REMOVE IN ORDER TO COMPLETE CERT EXCHANGE
*/
}



void GTBCommunication::initGNUTLS()
{
  //cout << "Initialize gnutls" << endl;
  if( gnutls_global_init() ) cout << "gnutls_global_init: Failed to intialize" << endl;
  loadCertFiles();
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
  //const char ** err_pos; //store returned code (error or successful)
  int retval;

  //cout << "loadCertFiles: allocate creds" << endl;
  if ((retval = gnutls_certificate_allocate_credentials (m_pX509Cred)))
  {
    //TODO
    //cout <<"loadCertFiles: gnutls_certificate_allocate_credentials: false" 
        //<< endl;
  }
  //cout << "loadCertFiles: load cert trust file" << endl;
  if ((retval = gnutls_certificate_set_x509_trust_file (
      *m_pX509Cred, 
      CAFILE, 
      GNUTLS_X509_FMT_PEM)))
  {
    //TODO
    if ( retval <= 0 )
      cerr << "ERROR: loadCertFiles: gnutls_certificate_set_x509_trust_file " << 
          "error code: " << strerror(retval) << endl;
    /*else
      cout << "loadCertFiles: gnutls_certificate_set_x509_trust_file: " << 
          "certs loaded: " << retval << endl;
    */
  }
  //cout << "loadCertFiles: load CSL" << endl;
  if((retval = gnutls_certificate_set_x509_crl_file (*m_pX509Cred, CRLFILE, 
                                        GNUTLS_X509_FMT_PEM)) < 1)
  {
    //TODO
    cerr << "ERROR: loadCertFiles: gnutls_certificate_set_x509_crl_file error code: %d"
        << retval << endl;
  }
    
  //cout << "loadCertFiles: load key" << endl;
  if (( retval = gnutls_certificate_set_x509_key_file (
                   *m_pX509Cred,
		   CERTFILE,
		   SKEYFILE,
		   GNUTLS_X509_FMT_PEM)))
      cerr << "ERROR: loadCertFiles: gnutls_certificate_set_x509_key_file " << 
          "error code: " << strerror(retval) << endl;
  

  //cout << "loadCertFiles: gen DH params" << endl;
  generateDHParams ();
  //cout << "loadCertFiles: priority init" << endl;
  //Set gnutls priority string
  
  const char * cErrLoc;
  if((retval = gnutls_priority_init (m_pPriorityCache, GNUTLS_PRIORITY, &cErrLoc)))
  {
    //TODO
    cerr << "ERROR: loadCertFiles: gnutls_priority_init error code: &s" << *cErrLoc << endl;
  }

  //cout << "Set priority: " << GNUTLS_PRIORITY << endl;

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
    cerr << "ERROR: getaddrinfo: " << gai_strerror(nRV) << endl;
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
      cerr << "ERROR: server: socket" << endl; //so continue to the next addr
      continue;
    }

    if(setsockopt(fdSock, 
                  SOL_SOCKET, 
		  SO_REUSEADDR, 
		  &nYes, //If unsuccessful in setting socket options
		  sizeof nYes) == -1)
    { //exit on error
      cerr << "ERROR: setsockopt" << endl;
      exit(1);
    }

    if(bind(fdSock, pIterator->ai_addr, pIterator->ai_addrlen) == -1)
    { //Assign a name to an addr
      close(fdSock);	//If unsuccessful, print error and check next
      cerr << "ERROR: server: bind" << endl;
      continue;
    }

    break; //if successful, exit loop
  }

  if(pIterator == NULL)
  {
    cerr << "ERROR: server: Failed to bind for unknown reason" << "\n" <<
        " Iterated through loop" << i << " times" << endl;
    freeaddrinfo(pServinfo); //no longer need this struct
    return 2;
  }

  if(pServinfo)
    freeaddrinfo(pServinfo); //no longer need this struct
  return fdSock;
}

int GTBCommunication::dealWithReq (Request i_aPBReq) 
{
  /*
   * Case 1: CURR
   * Case 2: AUTH
   * Case 3: CARS
   * Case 4: UPDT
   */

  Response apbRes;
  switch (i_aPBReq.nreqid())
  {
    case 1: //CURRent Rides
      int nCurrRet;
      cout << "Type CURR" << endl;
      cout << "C: Forked, processing request" << endl;
      if(!(nCurrRet = currRequest (&i_aPBReq, &apbRes)))
      {
        cout << "C: Sending Response for CURR" << endl;
        sendResponse(0, NULL, &apbRes, NULL);
      }
      else
      {
        cerr << "ERROR: C: Sending ERROR Response for CURR" << endl;
        sendResponse(nCurrRet, NULL, &apbRes, NULL);
      }
      break;
    case 2: //AUTH
      int nAuthRet;
      cout << "Type AUTH" << endl;
      cout << "C: Forked, processing request" << endl;
      if(!(nAuthRet = authRequest (&i_aPBReq)))
      {
        sendResponse(0, NULL, NULL, NULL);
        moveKey();
      }
      else
      {
        sendResponse(nAuthRet, NULL, NULL, NULL);
      }
      break;
    case 3: //CARS
      cout << "C: Type CARS, forking...." << endl;
      if(sendNumberOfCars (&i_aPBReq) < 0)
        return sendFailureResponse(2);
      cout << "Done....exiting parent\n" << endl;
      break;
    case 4:  // UPDT
      int nUpdtRet;
      cout << "Type UPDT" << endl;
      cout << "C: Forked, processing request" << endl;
      if(!(nUpdtRet = updtRequest (&i_aPBReq, &apbRes)))
      {
        cout << "C: Sending Response for UPDT" << endl;
        sendResponse(0, NULL, &apbRes, NULL);
      }
      else
      {
        cerr << "ERROR: C: Sending ERROR Response for UPDT" << endl;
        sendResponse(nUpdtRet, NULL, &apbRes, NULL);
      }
      break;
    default:
      break;
  }
  cout << "Child PID: " << getpid();
  cout << " Parent PID: " << getppid() << endl;
  return 0;
}

void GTBCommunication::receiveRequest(Request * aPBReq)
{
  int nNumBytes = 0;
  char vReqBuf[AUTHSIZE] = "";
  //cout << "Receiving request" << endl;
  if(aPBReq == NULL)
    throw new PatronException("NULL Pointer at receiveRequest\n");
  int nsize = 0;
  if((nNumBytes = gnutls_record_recv (m_aSession, &nsize, REQSIZE)) < 0)
  {
    cerr << "ERROR: Code reqrecv " << strerror(errno) << endl;
    throw BadConnectionException(strerror(errno));
  }

  //cout << "Incoming size: " << nsize << endl;
      
  if((nNumBytes = gnutls_record_recv (m_aSession, &vReqBuf, nsize)) < 0)
  {
    cerr << "ERROR: Code reqrecv " << strerror(errno) << endl;
    throw BadConnectionException(strerror(errno));
  }

  //cout << "Received Transmission Size: " << nNumBytes << endl;

  aPBReq->ParseFromString(vReqBuf);

  /*cout << "Request: " << endl;
  for (int i = 0; i<nsize; i++)
      cout << (int)vReqBuf[i] << " ";
  cout << endl;
  */
  //cout << "Print Debug String: " << endl;
  //aPBReq.PrintDebugString();
}

int GTBCommunication::handleConnection(int fdAccepted, int sockfd)
{
  struct stat fdstatus;
  int error;
  if((error = fstat(fdAccepted, &fdstatus)) || fdstatus.st_rdev)
  {
    throw BadConnectionException("Bad Accepted Connection");
  }
  if((error = fstat(sockfd, &fdstatus)) || fdstatus.st_rdev)
  {
    throw BadConnectionException("Bad Incoming Connection");
  }
    
  //cout << "Start TLS Session" << endl;
  gnutls_transport_set_ptr (m_aSession, (gnutls_transport_ptr_t) fdAccepted);
  //cout << "Performing handshake.." << endl;
  //char vReqBuf[AUTHSIZE] = "";
  //int nNumBytes=0;
  int nRetVal, i=0, lastret = 0;;
  do
  {
    i++;
    nRetVal = gnutls_handshake (m_aSession);
    //cout << "First Return value: " << nRetVal << endl;
    /*if (nRetVal == lastret)
    if ( nRetVal == GNUTLS_E_INTERNAL_ERROR ||
	 nRetVal == GNUTLS_E_INVALID_SESSION )
    {
      close(fdAccepted);
      throw BadConnectionException("Failed to Handshake");
    }*/
    lastret = nRetVal;
  } while (gnutls_error_is_fatal (nRetVal) != GNUTLS_E_SUCCESS);
  //cout << "Last Return Value: " << nRetVal << endl;
  
  if ( nRetVal < 0)
  {
    cerr << "ERROR " << i << ": Failed to perform handshake, error code : ";
    cerr << gnutls_strerror(nRetVal) << endl;;
    cerr << "Closing connection..." << endl;
    sendFailureResponse(3);
    gnutls_deinit(m_aSession);
    close(fdAccepted);
    throw BadConnectionException("Handshake Failed");
  }

  gnutls_cipher_algorithm_t aUsingCipher =
      gnutls_cipher_get (m_aSession);
  const char * sCipherName;
  sCipherName = gnutls_cipher_get_name(aUsingCipher);
  //cout << "Using cipher: " << sCipherName << endl;

  unsigned int nstatus;

  /* Need to uncomment when we can successfully establish authenicated 
   * connection 
   */
  /*if( gnutls_certificate_verify_peers2 (m_aSession, &nstatus) )
  {
    cerr << "ERROR: Failed to verify client certificate, error code ";
    //TODO: Send Plaintext message
    cerr << "Closing connection..." << endl;
    close(fdAccepted);
    gnutls_deinit(m_aSession);
    throw BadConnectionException("Client Verification Failed");
  }

  if (nstatus)
  {
    cerr << "ERROR: Failed to verify client certificate, error code ";
    cerr << "Closing connection..." << endl;
    close(fdAccepted);
    gnutls_deinit(m_aSession);
    throw BadConnectionException("Client Verification Failed. Invalid Certificate.");
  }*/

  //TODO
  //cout << "Storing connection information" << endl;
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
  //char vReqBuf[AUTHSIZE] = "";
  //char * ntopRetVal;
//int: numbytes (received)
  //int nNumBytes=0;

  nSinSize = sizeof aClientAddr;

  if (TLS_SESSION_CACHE != 0)
  {
    wrap_db_init ();
  }

 // while(1)
  //{
    //cout << endl;
    //cout << "Initialize TLS Session" << endl;
    initTLSSession();

    //cout << "Accepting Connection" << endl;
    fdAccepted = accept(i_fdSock, (struct sockaddr *)&aClientAddr, &nSinSize);
    //cout << "accept fdAccepted:" << fdAccepted << endl;
    if (fdAccepted == -1)
    {
      cerr << "ERROR: Error at accept!" << endl;
      fdAccepted = 0;
      //continue;
    }

    inet_ntop(aClientAddr.ss_family, 
              getInAddr((struct sockaddr *)&aClientAddr), 
	      vAddr, sizeof vAddr);

    if ( vAddr == NULL ) 
    {
      cerr << "ERROR: Failed to convert the client IP Address!";
      cerr << " Error: " << strerror(errno) << endl;
      cerr << "ERROR: Closing connection..." << endl;
      close(fdAccepted);
      //continue;
    }

    //cout << "Accepting Connection from: " << vAddr << endl;
    //They're already char*, might as well just compare without wasting
    //the time to convert them
    strncpy(m_vIPAddr, vAddr, INET6_ADDRSTRLEN);
    return fdAccepted;

    /*int childpid = 0;
    if (!(childpid = fork()))
    {
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
        cerr << "ERROR " << i << ": Failed to perform handshake, error code : ";
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
         cerr << "ERROR: Failed to verify client certificate, error code ";
        //TODO: Send Plaintext message
        cerr << "Closing connection..." << endl;
        close(fdAccepted);
        gnutls_deinit(m_aSession);
        continue;
      }

      if (nstatus)
      {
        cerr << "ERROR: Failed to verify client certificate, error code ";
        cerr << "Closing connection..." << endl;
        close(fdAccepted);
        gnutls_deinit(m_aSession);
        continue;
      }

      //TODO
      cout << "Storing connection information" << endl;

      cout << "Receiving request" << endl;

      int nsize = 0;
      if((nNumBytes = gnutls_record_recv (m_aSession, &nsize, REQSIZE)) < 0){
        cerr << "ERROR: Code reqrecv " << strerror(errno) << endl;
        continue;
      }

      cout << "Incoming size: " << nsize << endl;
      
      if((nNumBytes = gnutls_record_recv (m_aSession, &vReqBuf, nsize)) < 0)
      {
        cerr << "ERROR: Code reqrecv " << strerror(errno) << endl;
        continue;
      }

      cout << "Received Transmission Size: " << nNumBytes << endl;

      Request aPBReq;
      aPBReq.ParseFromString(vReqBuf);

      cout << "Request: " << endl;
      for (int i = 0; i<nsize; i++)
        cout << (int)vReqBuf[i] << " ";
      cout << endl;

      cout << "Print Debug String: " << endl;
      aPBReq.PrintDebugString();

      cout << "\nHandle Request" << endl;
      int handledreqerr = 0;
      if((handledreqerr = dealWithReq(aPBReq)))
      {
        cerr << "ERROR: dealWithReq returned with value: " << handledreqerr
	    << endl;
      }
      close(fdAccepted);
      gnutls_deinit(m_aSession);
      break;
    }
    else
    {
      cout << "Child pid: " << childpid << endl;
      continue;
    }
  }
  exit(0);
*/
}
