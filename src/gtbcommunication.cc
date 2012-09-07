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
#include <sys/wait.h>
#include <unistd.h>
#include <csignal>
#include <unistd.h>
#include <json/json.h>
#include <curl/curl.h>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include <gnutls/x509.h>

#include "gtbcommunication.hpp"
#include "gtbexceptions.hpp"
#include "patron.pb.h"
#include "sqlconn.hpp"
#include "threading.hpp"

#include <json/json.h>

#define MAX_SESSION_ID_SIZE 32
#define MAX_SESSION_DATA_SIZE 512
#define TLS_SESSION_CACHE 10
GTBCommunication * globalComm;

using namespace std;
extern "C"
{
  int saveSessionForResume_C(void * ptr, gnutls_datum_t session_id,
                             gnutls_datum_t session_data);
  gnutls_datum_t retrieveSessionForResume_C(void * ptr,
                                            gnutls_datum_t session_id);
  int removeSessionForResume_C(void * ptr, gnutls_datum_t session_id);
}

extern inline pthread_t createCommThread(GTBCommunication * aComm, 
                                         pthread_attr_t * attr);
/** \brief Output log messages to stdout
*/
void gnutls_log_fun(int level, const char * loginfo)
{
  std::cout << "GnuTLS LOG " << level << ": " << loginfo << std::endl;
}
 

/************************** 
* Thread Related Methods *
**************************/

void GTBCommunication::launchWatchDog()
{
  time_t now;
  pthread_attr_t attr;
  int nRetVal;
  void * joinret;
  sigset_t set;
  int sleepfor = WDOGKILLAFTER;
  pthread_t tid = thread_ids[COMMTHREAD];
  for(;;)
  {
    sleep(sleepfor);
    now = time(NULL);
    if((lostcontrolat != 0) && ((lostcontrolat + WDOGKILLAFTER) < now))
    {
      cerr << "Cancelling Comm Thread!" << endl;
      pthread_cancel(tid);
      nRetVal = pthread_join(tid, &joinret);
      if(nRetVal != 0)
      {
        cerr << "Join Failed! " << strerror(errno) << endl;
	continue;
      }
      cout << "Joining: " << " " << nRetVal << endl;
      usleep(15000);
      while(joinret != PTHREAD_CANCELED)
      {
        sleep(2);
	if(debug & 1)
	{
	  cout << "Sleep Not Cancelled" << endl;
	}
      }
      if(nRetVal == 0 && joinret == PTHREAD_CANCELED)
      {
        if(debug & 1)
	  cout << "Successfully Cancelled Comm Thread" << endl;
        sigemptyset(&set);
        sigaddset(&set, SIGIO);
        nRetVal = pthread_sigmask(SIG_BLOCK, &set, NULL);
        if(nRetVal != 0)
        {
          cerr << "Failed to sigio mask!" << endl;
          throw exception();
        }
    
        nRetVal = pthread_attr_init(&attr);
        if(nRetVal != 0)
        {
          cerr << "Failed to Initialize pthread_attr_t!" << endl;
          throw exception();
        }

	cout << "Respawning Now" << endl;
        tid = createCommThread(this, &attr);
        thread_ids[COMMTHREAD] = tid;

        nRetVal = pthread_attr_destroy(&attr);
        if(nRetVal != 0)
        {
           cerr << "Failed to destroy pthread_attr_t!" << endl;
           throw exception();
        }
	cout << "Good To Go Again" << endl;
	lostcontrolat = 0;
      }
    }
    else
    {
      if(debug & 1)
        cout << "Comm looks good from here!" << endl;
    }
  }
}


void GTBCommunication::gtbAccept()
{
  int sockfd;
  int nRetVal = 0;
  GTBClient * client;
  sigset_t set;
  int signum;
  
  sigemptyset(&set);
  sigaddset(&set, SIGACCEPT);
  pthread_sigmask(SIG_BLOCK, &set, NULL);

  globalComm = this;
  sockfd = getSocket();
  
  if(debug & 1)
  {
    cout << "Establish Incoming Connections" << endl;;
  }

  initGNUTLS();

  if(debug & 1)
    cout << "Waiting until Comm thread is ready" << endl;
  sigwait(&set, &signum);

  for(;;)
  {
    if(debug & 1)
      cout << "Returning to listening state" << endl;
    try
    {
      client = listeningForClient(sockfd);
    } catch (BadConnectionException &e)
    {
      continue;
    }

    acceptedQueue.push(client);

    pthread_t commthread_id = thread_ids.at(COMMTHREAD);
    nRetVal = pthread_kill(commthread_id, 0);
    if(nRetVal == 0)
    {
      cout << "Comm is alive and well." << endl;
      nRetVal = pthread_kill(thread_ids.at(COMMTHREAD), SIGACCEPT);
      if(nRetVal != 0)
      {
        if(nRetVal == EINVAL)
        {
          cerr << "SIGACCEPT Does Not Exist On This System!";
          cerr << " Why doesn't this work? Exiting." << endl;
	  exit(-1);
        }
        else if(nRetVal == ESRCH)
        {
          cerr << "The Comm Thread No Longer Exists??" << endl;
	  cerr << "Should we try relaunching again?" << endl;
        }
      }
    }
    else
    {
      if(nRetVal == ESRCH)
      {
        cerr << "The Comm Thread No Longer Exists??"; 
        cerr << "We Can't Send SIGACCEPT To It! Perhaps we should relaunch.";
	cerr << endl;
      }
    }
  }
  close(sockfd);
}

void GTBCommunication::gtb_wrapperForCommunication()
{
  Request request;
  GTBClient * client;
  sigset_t set;
  int nClient;
  int nRetVal;
  int signum;

  sigemptyset(&set);
  sigaddset(&set, SIGACCEPT);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  pthread_sigmask(SIG_BLOCK, &set, NULL);
      
  if(debug & 1)
    cout << "Signal Accept thread that Comm is ready" << endl;
  pthread_kill(thread_ids.at(ACPTTHREAD), SIGACCEPT);

  for(;;)
  {
    if(acceptedQueueIsEmpty())
    {
      if(sigwait(&set, &signum) != 0)
      {
        cerr << "Error while waiting for signal!" << endl;
        continue;
      }
      cout << "Received a signal" << endl;
    }
    else
      signum = SIGACCEPT;
    if(signum == SIGACCEPT)
    {
      while(!acceptedQueueIsEmpty())
      {
        client = acceptedQueuePop();
	if(debug * 4)
	  cout << acceptedQueue.size() << " Clients Remaining" << endl;
        try
        {
          nClient = handleConnection(client);
        } catch (BadConnectionException &e)
        {
	  /* Reset timer */
          lostcontrolat = 0;
          close(client->getFD());
          delete client;
          continue;
        }
        try
        {
          receiveRequest(&request);
        } catch (BadConnectionException &e)
        {
	  /* Reset timer */
          lostcontrolat = 0;
          close(client->getFD());
          delete client;
          continue;
        }
        request.set_nclient(nClient);
        requestQueue.push(request);
        nRetVal = pthread_kill(thread_ids.front(), SIGIO);
        if(nRetVal != 0)
        {
          if(nRetVal == EINVAL)
          {
            cerr << "SIGIO Does Not Exist On This System!";
            cerr << " We Need To Use A Different Signal. Exiting." << endl;
          }
          else if(nRetVal == ESRCH)
          {
            cerr << "The Main Thread No Longer Exists??" << endl;
            cerr << "We Can't Send SIGIO To It! Exiting." << endl;
          }
        }
        /* Extremely important. Client fails otherwise.
         * Well, at the least the emulator does, so sleep a little before
         * we continue
         */
        sleep(1);
      }
    }
  }
}




/**************************
* GNUTLS related methods *
**************************/

int saveSessionForResume_CPP(void * ptr, gnutls_datum_t session_id,
		     gnutls_datum_t session_data)
{
  return globalComm->saveSessionForResume(ptr, session_id, session_data);
}
gnutls_datum_t retrieveSessionForResume_CPP(void * ptr,
                                            gnutls_datum_t session_id)
{
  return globalComm->retrieveSessionForResume(ptr, session_id);
}
int removeSessionForResume_CPP(void * ptr, gnutls_datum_t session_id)
{
  return globalComm->removeSessionForResume(ptr, session_id);
}
extern "C"
{
  int save_session_for_resume(void * ptr, gnutls_datum_t session_id,
                             gnutls_datum_t session_data)
  {
    return saveSessionForResume_CPP(ptr, session_id, session_data);
  }
  gnutls_datum_t retrieve_session_for_resume(void * ptr,
                                            gnutls_datum_t session_id)
  {
    return retrieveSessionForResume_CPP(ptr, session_id);
  }
  int remove_session_for_resume(void * ptr, gnutls_datum_t session_id)
  {
    return removeSessionForResume_CPP(ptr, session_id);
  }

  void set_session_management_functions(gnutls_session_t * session)
  {
    if(*session == NULL)
      return;
    gnutls_db_set_remove_function(*session, &remove_session_for_resume);
    gnutls_db_set_retrieve_function(*session, &retrieve_session_for_resume);
    gnutls_db_set_store_function(*session, &save_session_for_resume);
  }
}
    

void GTBCommunication::initGNUTLS()
{
  if(debug & 1)
  {
    cout << "Initialize gnutls" << endl;
  }
  if( gnutls_global_init() ) cout << "gnutls_global_init: Failed to intialize" 
      << endl;
  loadCertFiles();
  set_session_management_functions(&m_aSession);
  gnutls_global_set_log_function(gnutls_log_fun);
  if(debug)
  {
    int loglevel = debug >> 4;
    if(loglevel)
      cout << "Enableing GnuTLS Debug Level: " << loglevel << endl;
    gnutls_global_set_log_level(loglevel);
  }
}

void GTBCommunication::deinitGNUTLS()
{
  gnutls_deinit(m_aSession);
}


void
GTBCommunication::initTLSSession()
{

  int retval;

  // Initialize session
  if ((retval = gnutls_init (&m_aSession, GNUTLS_SERVER)))
  {
    cerr << "ERROR: initTLSSession: gnutls_init error code: " << retval;
    cerr << ": " << strerror(retval)<< endl;
    exit(1);
  }

  // Sets priority of session, i.e. specifies which crypto algos to use
  if ((retval = gnutls_priority_set (m_aSession, *m_pPriorityCache))) 
  {
    cerr << "ERROR: initTLSSession: gnutls_priority_set error code: " << retval;
    cerr << ": " << strerror(retval)<< endl;
    exit(1);
  }

 // Sets the needed credentials for the specified type; an x509 cert, in this case
  if ((retval = gnutls_credentials_set (
      m_aSession, 
      GNUTLS_CRD_CERTIFICATE,
      *m_pX509Cred))) 
  {
    cerr << "ERROR: initTLSSession: gnutls_credentials_set error code:  " 
        << retval;
    cerr << ": " << strerror(retval) << endl;
    exit (1);
  }

  // Request client cert
  //gnutls_certificate_server_set_request (m_aSession, GNUTLS_CERT_REQUEST);
  /*if (TLS_SESSION_CACHE != 0)
  {
    gnutls_db_set_retrieve_function (m_aSession, wrap_db_fetch);
    gnutls_db_set_remove_function (m_aSession, wrap_db_delete);
    gnutls_db_set_store_function (m_aSession, wrap_db_store);
    gnutls_db_set_ptr (m_aSession, NULL);
  }*/

  gnutls_certificate_server_set_request (m_aSession, 
              GNUTLS_CERT_REQUIRE); //Require client to provide cert
  /*gnutls_certificate_send_x509_rdn_sequence  (
              m_aSession, 
              1); //REMOVE IN ORDER TO COMPLETE CERT EXCHANGE*/
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
  int retval;

  if(debug & 1)
  {
    cout << "loadCertFiles: allocate creds" << endl;
  }
  if ((retval = gnutls_certificate_allocate_credentials (m_pX509Cred)))
  {
    //TODO
    if(debug & 1)
    {
      cout << "loadCertFiles: gnutls_certificate_allocate_credentials: false" 
          << endl;
    }
  }
  if(debug & 1)
  {
    cout << "loadCertFiles: load cert trust file" << endl;
  }
  if ((retval = gnutls_certificate_set_x509_trust_file (
      *m_pX509Cred, 
      CAFILE, 
      GNUTLS_X509_FMT_PEM)))
  {
    //TODO
    if ( retval <= 0 )
      cerr << "ERROR: loadCertFiles: gnutls_certificate_set_x509_trust_file " << 
          "error code: " << strerror(retval) << endl;
    if(debug & 4)
    {
      cout << "loadCertFiles: gnutls_certificate_set_x509_trust_file: " << 
          "certs loaded: " << retval << endl;
    }
  }
  if(debug & 1)
  {
    cout << "loadCertFiles: load CSL" << endl;
  }
  if((retval = gnutls_certificate_set_x509_crl_file (*m_pX509Cred, CRLFILE, 
                                        GNUTLS_X509_FMT_PEM)) < 1)
  {
    //TODO
    cerr << "ERROR: loadCertFiles: gnutls_certificate_set_x509_crl_file error code: %d"
        << retval << endl;
  }
    
  if(debug & 1)
  {
    cout << "loadCertFiles: load key" << endl;
  }
  if (( retval = gnutls_certificate_set_x509_key_file (
                   *m_pX509Cred,
		   CERTFILE,
		   SKEYFILE,
		   GNUTLS_X509_FMT_PEM)))
      cerr << "ERROR: loadCertFiles: gnutls_certificate_set_x509_key_file " << 
          "error code: " << strerror(retval) << endl;
  

  if(debug & 1)
  {
    cout << "loadCertFiles: gen DH params" << endl;
  }
  generateDHParams ();
  if(debug & 1)
  {
    cout << "loadCertFiles: priority init" << endl;
  }
  
  // Set gnutls priority string
  const char * cErrLoc;
  if((retval = gnutls_priority_init (m_pPriorityCache,
                                     GNUTLS_PRIORITY, &cErrLoc)))
  {
    //TODO
    cerr << "ERROR: loadCertFiles: gnutls_priority_init error code: " <<
            retval << ": " << gnutls_strerror(retval) << " at " << 
	    cErrLoc << endl;
  }

  if(debug & 4)
  {
    cout << "Set priority: " << GNUTLS_PRIORITY << endl;
  }

  gnutls_certificate_set_dh_params (*m_pX509Cred, *m_pDHParams);
  
}


int 
GTBCommunication::moveKey() 
{
  //TODO
  return 0;
}



int GTBCommunication::saveSessionForResume(void * ptr, 
                                           gnutls_datum_t session_id,
                                           gnutls_datum_t session_data)
{
  if(clientSessions == NULL)
  {
    clientSessions = new ClientSession();
    if(clientSessions == NULL)
    {
      if(debug & 1)
        cerr << "Failed to save session" << endl;
      return GNUTLS_E_DB_ERROR;
    }
    clientSessions->session_id = session_id;
    clientSessions->session_data = session_data;
    if(debug & 4)
      cout << "Successfully saved session" << endl;
  }
  else
  {
    clientSessions->tail = new ClientSession();
    if(clientSessions->tail == NULL)
    {
      if(debug & 1)
        cerr << "Failed to save session" << endl;
      return GNUTLS_E_DB_ERROR;
    }
    clientSessions->tail->session_id = session_id;
    clientSessions->tail->session_data = session_data;
  }
    
  if(debug & 1)
    cout << "Successfully saved session" << endl;
  return 0;
}


gnutls_datum_t GTBCommunication::retrieveSessionForResume(void * ptr, 
                                        gnutls_datum_t session_id)
{
  if((clientSessions->session_id.size == session_id.size) &&
     !sess_strncmp(clientSessions->session_id.data, session_id.data,
                  session_id.size))
  {
    if(debug & 1)
      cout << "Successfully retrieved session" << endl;
    return clientSessions->session_data;
  }
  else
    return retrieveSessionRecurse(ptr, session_id, clientSessions->next);
}


gnutls_datum_t GTBCommunication::retrieveSessionRecurse(void * ptr, 
                                             gnutls_datum_t session_id,
                                             ClientSession * client)
{
  if((client->session_id.size == session_id.size) &&
     !sess_strncmp(client->session_id.data, session_id.data,
                  session_id.size))
  {
    if(debug & 1)
      cout << "Successfully retrieved session" << endl;
    return client->session_data;
  }
  else if(client->next == NULL)
  {
    /* Return an empty struct. Code checks that session-> != NULL. In
     * this case it will and GNUTLS will handle correctly.
     */
    gnutls_datum_t session;
    if(debug & 1)
      cerr << "Failed to retrieve session" << endl;
    return session;
  }
  else
    return retrieveSessionRecurse(ptr, session_id, client->next);
}


int GTBCommunication::removeSessionForResume(void * ptr, gnutls_datum_t session_id)
{
  if((clientSessions->session_id.size == session_id.size) &&
     !sess_strncmp(clientSessions->session_id.data, session_id.data,
                  session_id.size))
  {
    ClientSession * newHead = clientSessions->next;
    delete clientSessions;
    clientSessions = newHead;
    if(debug & 1)
      cout << "Successfully removed session" << endl;
    return GNUTLS_E_DB_ERROR;
  }
  else
    return removeSessionRecurse(ptr, session_id, clientSessions);
}


int GTBCommunication::removeSessionRecurse(void * ptr, 
                                            gnutls_datum_t session_id,
                                            ClientSession * client)
{
  if((client->next->session_id.size == session_id.size) &&
     !sess_strncmp(client->next->session_id.data, session_id.data,
                  session_id.size))
  {
    ClientSession * next = client->next->next;
    delete client->next;
    client->next = next;
    if(debug & 1)
      cout << "Successfully removed session" << endl;
    return 0;
  }
  else if (client->next == NULL)
  {
    if(debug & 1)
      cout << "Failed to remove session" << endl;
    return GNUTLS_E_DB_ERROR;
  }
  else
    return removeSessionRecurse(ptr, session_id, client->next);
}

int GTBCommunication::sess_strncmp(unsigned char * s1,
                                    unsigned char * s2,
                                    size_t n)
{
  int i;
  for(i = 0; i < n; ++i)
    if (s1[i] != s2[i])
      return -1;
  return 0;
}

/**********************
* Networking Related *
**********************/

int 
GTBCommunication::getSocket()
{
  // FD the server will listen on
  int fdSock;

  /* STRUCTS
   * addrinfo: aHints (used to retrieve addr), 
   * pServinfo (used to store retrieved addr), pPIterator (iterator)
   */
  struct addrinfo aHints, aServinfo, *pServinfo, *pIterator;

  // int rv: return value; i: iterator; yes: optval set in setsockopt
  int nRV, i = 0, nYes = 1;

  memset(&aHints, 0, sizeof aHints);

  // struct set to allow for binding and connections
  aHints.ai_family = AF_UNSPEC;
  aHints.ai_socktype = SOCK_STREAM;
  aHints.ai_flags = AI_PASSIVE; //uses extern local IP

  pServinfo = &aServinfo;

  //Retrieve addr and socket
  if((nRV = getaddrinfo(NULL, PORT, &aHints, &pServinfo)) != 0)
  {
    cerr << "ERROR: getaddrinfo: " << gai_strerror(nRV) << endl;
    return nRV;
  }

  //Iterate through servinfo and bind to the first available
  for(pIterator = pServinfo; pIterator != NULL; pIterator = pIterator->ai_next)
  {
    i++;
    /* If socket is unseccessful in creating an
     * endpoint, e.g. filedescriptor, then it returns -1,
     */
    if ((fdSock= socket(pIterator->ai_family, pIterator->ai_socktype, 
      pIterator->ai_protocol)) == -1) 
    {
      cerr << "ERROR: server: socket" << endl; //so continue to the next addr
      continue;
    }

    if(setsockopt(fdSock, 
                  SOL_SOCKET, 
		  SO_REUSEADDR, 
		  &nYes, //If unsuccessful in setting socket options
		  sizeof nYes) == -1)
    {
      // exit on error
      cerr << "ERROR: setsockopt" << endl;
      exit(1);
    }

    if(bind(fdSock, pIterator->ai_addr, pIterator->ai_addrlen) == -1)
    { 
      cerr << "ERROR: server: bind: " << fdSock << endl;
      cerr << "\t" << pIterator->ai_addr->sa_data << " " << pIterator->ai_addrlen << endl;
      close(fdSock);
      continue;
    }

    // if successful, exit loop, yup, yup +)
    break;
  }

  if(pIterator == NULL)
  {
    cerr << "ERROR: server: Failed to bind for unknown reason" << "\n" <<
        "\tIterated through loop " << i << " times" << endl;

    // We no longer need this struct
    freeaddrinfo(pServinfo);
    exit(-2);
    return -2;
  }

  if(pServinfo)
    // We no longer need this struct
    freeaddrinfo(pServinfo);

  // Marks socket as passive, so it accepts incoming connections
  if (listen(fdSock, BACKLOG) == -1)
  { 
    cerr << "listen: failed to mark as passive" << endl;
    exit(1);
  }
  return fdSock;
}

void * GTBCommunication::getInAddr (struct sockaddr *i_sa)
{

  if(i_sa->sa_family == AF_INET)
  {
    return &(((struct sockaddr_in*)i_sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)i_sa)->sin6_addr);
}


int GTBCommunication::handleConnection(GTBClient * client)
{
  struct stat fdstatus;
  int error, idx;
  if((error = fstat(client->getFD(), &fdstatus)) || fdstatus.st_rdev)
  {
    throw BadConnectionException("Bad Accepted Connection");
  }

  if(debug & 1)
  {
    cout << "Start TLS Session" << endl;
  }

  gnutls_transport_set_ptr (m_aSession, 
                           (gnutls_transport_ptr_t) client->getFD());
  if(debug & 1)
  {
    cout << "Performing handshake.." << endl;
  }

  lostcontrolat = time(NULL);
  int nRetVal, i(0);
  do
  {
    i++;
    nRetVal = gnutls_handshake (m_aSession);
    if(debug & 4)
    {
      cout << "First Return value: " << nRetVal << ": " << 
              gnutls_strerror(nRetVal) << endl;
    }
    if ( i > 10 )
    {
      close(client->getFD());
      throw BadConnectionException("Failed to Handshake");
    }
  } while (gnutls_error_is_fatal (nRetVal) != GNUTLS_E_SUCCESS);
  if(debug & 4)
  {
    cout << "Last Return Value: " << nRetVal << endl;
  }
  lostcontrolat = 0;
  
  if ( nRetVal < 0)
  {
    cerr << "ERROR " << i << ": Failed to perform handshake, error code : ";
    cerr << gnutls_strerror(nRetVal) << endl;;
    cerr << "Closing connection..." << endl;
    sendFailureResponse(3);
    gnutls_deinit(m_aSession);
    close(client->getFD());
    throw BadConnectionException("Handshake Failed");
  }

  gnutls_cipher_algorithm_t aUsingCipher =
      gnutls_cipher_get (m_aSession);
  const char * sCipherName;
  sCipherName = gnutls_cipher_get_name(aUsingCipher);
  if(debug & 4)
  {
    cout << "Using cipher: " << sCipherName << endl;
  }

/* Uncomment when we request certs
 * TODO*/
  const gnutls_datum_t * certList;
  gnutls_x509_crt_t cert = NULL;
  unsigned int certLength;
  certList = gnutls_certificate_get_peers(m_aSession, &certLength);

  if((nRetVal = gnutls_x509_crt_init(&cert)))
  {
    if(debug & 4)
      cerr << "Failed to initialize cert: " <<
               gnutls_strerror(nRetVal) << endl;
      
    throw BadConnectionException("Bad Client Certs");
  }

  if(certList)
  {
    if(debug & 4)
    {
      cout << "Client sent a certificate chain with " << certLength;
      cout << " certificates." << endl;
      cout << "Certificate is " << certList[0].size << " bytes" << endl;
    }

    if((nRetVal = gnutls_x509_crt_import(cert, &(certList[0]),
                                         GNUTLS_X509_FMT_DER)))
    {
      if(debug & 4)
        cerr << "Could not parse certificate(s): " <<
	        gnutls_strerror(nRetVal) << endl;
      
      throw BadConnectionException("Bad Client Certs");
    }

    // We only want the clients certs
    client->setCertificate(*certList);

    // This may cause problems.
    gnutls_x509_crt_deinit(cert);
  }
  else
    throw BadConnectionException("No Client Certs");

    /**/

  unsigned int nstatus;

  /* Need to uncomment when we can successfully establish authenicated 
   * connection 
   */
  if( gnutls_certificate_verify_peers2 (m_aSession, &nstatus) )
  {
    cerr << "ERROR: Failed to verify client certificate, error code ";
    //TODO: Send Plaintext message
    cerr << "Closing connection..." << endl;
    close(client->getFD());
    gnutls_deinit(m_aSession);
    throw BadConnectionException("Client Verification Failed");
  }

  if (nstatus)
  {
    cerr << "ERROR: Failed to verify client certificate, error code ";
    cerr << "Closing connection..." << endl;
    close(client->getFD());
    gnutls_deinit(m_aSession);
    throw BadConnectionException("Client Verification Failed. Invalid Certificate.");
  }

  //TODO
  //cout << "Storing connection information" << endl;
  /* Add client to clientsList here and, because we passed the above checks
   * mark client as verified
   *
   * Actually, not verified until after Auth
   */

  //client->setVerified(true);
  idx = addIfNewClient(client);

  return idx;
}
  

GTBClient * 
GTBCommunication::listeningForClient (int i_fdSock)
{
  int fdAccepted = 0;
  // sockaddr_storage: aClientAddr (IP Address of the requestor)
  struct sockaddr_storage aClientAddr;
  // socklen_t: nSinSize (set size of socket)
  socklen_t nSinSize;
  // char: vAddr (stores the IP Addr of the incoming connection so it can be diplayed)
  char vAddr[INET6_ADDRSTRLEN] = "";

  GTBClient * client; 

  nSinSize = sizeof aClientAddr;

  /*
  if (TLS_SESSION_CACHE != 0)
  {
    wrap_db_init ();
  }
  */

  if(debug & 1)
  {
    cout << endl;
    cout << "Initialize TLS Session" << endl;
  }
  initTLSSession();

  if(debug & 1)
  {
    cout << "Accepting Connection" << endl;
  }
  fdAccepted = accept(i_fdSock, (struct sockaddr *)&aClientAddr, &nSinSize);
  
  if(debug & 1)
  {
    cout << "accept fdAccepted:" << fdAccepted << endl;
  }
  if (fdAccepted == -1)
  {
    cerr << "ERROR: Error at accept!" << endl;
    fdAccepted = 0;
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
  }
  if(debug & 1)
  {
    cout << "Accepting Connection from: " << vAddr << endl;
  }
    
  //They're already char*, might as well just compare without wasting
  //the time to convert them
  strncpy(m_vIPAddr, vAddr, INET6_ADDRSTRLEN);

  client = new GTBClient(fdAccepted);
  if(debug & 1)
    cout << "client: fdAccepted: " << client->getFD() << endl;
  client->setIPAddr(vAddr);
  gnutls_datum_t empty;
  empty.data = NULL;
  empty.size = 0;
  client->setCertificate(empty);
  return client;
}


void GTBCommunication::receiveRequest(Request * aPBReq)
{
  int nNumBytes = 0;
  /* This causes a segfault, obviously, why did this work before? 
   * char vReqBuf[AUTHSIZE] = "";
   */
  if(debug & 1)
  {
    cout << "Receiving request" << endl;
  }
  if(aPBReq == NULL)
    throw PatronException("NULL Pointer at receiveRequest\n");
  int nsize = 0;
  lostcontrolat = time(NULL);
  if((nNumBytes = gnutls_record_recv (m_aSession, &nsize, REQSIZE)) < 0)
  {
    cerr << "ERROR: Code reqrecv " << strerror(errno) << endl;
    lostcontrolat = 0;
    throw BadConnectionException(strerror(errno));
  }
  lostcontrolat = 0;

  if(debug & 4)
  {
    cout << "Incoming size: " << nsize << endl;
  }
      
  ++nsize;
  char vReqBuf[nsize];
  lostcontrolat = time(NULL);
  if((nNumBytes = gnutls_record_recv (m_aSession, &vReqBuf, nsize)) < 0)
  {
    cerr << "ERROR: Code reqrecv " << strerror(errno) << endl;
    lostcontrolat = 0;
    throw BadConnectionException(strerror(errno));
  }
  lostcontrolat = 0;

  if(debug & 4)
  {
    cout << "Received Transmission Size: " << nNumBytes << endl;
  }

  /* Hack or solution? */
  vReqBuf[nsize - 1] = '\0';
  string sReqBuf(vReqBuf);

  aPBReq->ParseFromString(sReqBuf);

  if(debug & 4)
  {
    cout << "Request: " << sReqBuf << ", size:  " << sReqBuf.size() << endl;
    for (int i = 0; i<nsize; ++i)
        cout << (unsigned int)vReqBuf[i] << " ";
    cout << endl;

    cout << "Type: " << (vReqBuf + 4) << endl;
  
    cout << "Print Debug String: " << endl;
    aPBReq->PrintDebugString();
  }
}



/*****************************
* Communication with client *
*****************************/

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
GTBCommunication::sendNopes(int i_nRetVal)
{
  int nNumBytes;

  if ((nNumBytes = gnutls_record_send (m_aSession, &i_nRetVal, sizeof i_nRetVal)) == -1){
    cerr << "Error on send for retval: " << strerror(errno) << endl;
  }
  cout << "Number of bytes sent: " << nNumBytes << endl;
	
  return nNumBytes;
}







/******************************
* Related to Client Requests *
******************************/

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
  string sResponse;
  aPBRes.SerializeToString(&sResponse);

  if ((nNumBytes = gnutls_record_send (
      m_aSession, 
      sResponse.c_str(), 
      npbsize)) < 0)
  {
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
      i_pbRes->CheckInitialized();
      i_pbRes->PrintDebugString();
      cout << "nRespID: " << i_pbRes->nrespid() << endl;
      cout << "nResValue: " << i_pbRes->sresvalue() << endl;
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

size_t
GTBCommunication::authRequest_callback(void *buffer, size_t size,
                                       size_t nmemb, void *userp)
{
  /* If valid, add authed users to DB for subsequent validation
   * Rename checkAuth to something more appropriate
   */
  GTBCommunication agtbc;
  MySQLConn aMySQLConn;
  string gpgfilename("authres.json.gpg");
  fstream fs;
  fs.open(gpgfilename.c_str(), ios::out | ios::binary);
  fs.write((char *)buffer, (size*nmemb));
  fs.close();
  string filename;
  try
  {
    filename = agtbc.getDecryptedPackage(gpgfilename);
  } catch(CryptoException &e)
  {
    return 0;
  }
  char * stringbuf;
  stringstream strbuf;
  fs.open(filename.c_str(), ios::in);
  if(fs.is_open()){
    fs.seekp(0, ios_base::end);
    int length = fs.tellp();
    fs.seekp(0, ios_base::beg);
    stringbuf = (char *) malloc(sizeof(char) * length);
    fs.read(stringbuf, length);
    strbuf << stringbuf;
    if(!strbuf){
      cerr << "Failed to input buffer into stream" << endl;
      return 0;
    }
  } else{
    cerr << "Failed to open plaintext json file" << endl;
    return 0;
  }
  fs.close();
  Json::Value root;
  Json::Reader reader;
  string retval("0");
  bool parsingSuccessful = reader.parse(filename, root);
  parsingSuccessful = reader.parse(strbuf, root);
  if(parsingSuccessful)
  { 
    retval = root.get("response", "0").asString();
    if(!retval.compare("1"))
    {
      Request * aPBReq = (Request *)userp;
      try
      {
        aMySQLConn.storeAuth(aPBReq->sparams(0), aPBReq->sparams(1),
                               aPBReq->sparams(2), aPBReq->sparams(3));
      } catch (exception &e)
      {
        return 0;
      }
      return (size * nmemb);
    }
  }
  cerr << reader.getFormattedErrorMessages() << endl;
  return 0;
}


string GTBCommunication::getDecryptedPackage(string gpgfilename)
{
  int pid, res;
  string jsonout;

  if(!(pid = fork()))
  {
    const char * const argv[] = {"/usr/bin/gpg", "--no-tty", "--batch",
                                 "--passphrase", PASSPHRASE, "-d", "-o",
                                 "authres.json", "authres.json.gpg"};
    for(int i = 0; i < 7; ++i)
      cout << argv[i] << " ";
    cout << endl;
    res = execv(argv[0], (char * const *) argv);
    cerr << "GPG EXEC returned: " << res << ": " << strerror(res) << endl;
    exit(-1);
  }
  else
  {
    res = waitpid(pid, NULL, 0);
    if(res != pid)
      throw CryptoException("Unsuccessful Return Code");
    
    string filename(gpgfilename);
    filename.replace(filename.size() - 4, filename.size() - 1, "");
    fstream fs;
    fs.open(filename.c_str(), ios::in);
    if(fs.is_open())
    {
      fs.close();
      unlink(gpgfilename.c_str());
      return filename;
    }
    else
    {
      unlink(gpgfilename.c_str());
      throw CryptoException("File Does Not Exist");
    }
  }
  return NULL;
}

string GTBCommunication::getEncryptedPackage(Request * aPBReq)
{
  int pid, res;
  const char * filename = "authreq.json";
  string encjsonout;

  Json::ValueType type = Json::objectValue;
  Json::Value root(type);
  root["AUTH"] = aPBReq->sreqtype(); 
  root["user1"] = aPBReq->sparams(0); 
  root["user2"] = aPBReq->sparams(1); 
  root["NH"] = aPBReq->sparams(2);
  root["HASH"] = aPBReq->sparams(3);
  Json::StyledWriter writer;
  string jsonout = writer.write(root);
 
  if(!(pid = fork()))
  {
    string gpgfilename(filename);
    gpgfilename.append(".gpg");
    fstream fs;
    fs.open(gpgfilename.c_str(), ios::in);
    if(fs.is_open()){
      fs.close();
      unlink(gpgfilename.c_str());
    }
    fs.open(filename, ios::out);
    fs.write(jsonout.c_str(), jsonout.size());
    fs.close();
    const char * const argv[] = {"/usr/bin/gpg", "--no-tty", "--batch",
                                 "--passphrase", PASSPHRASE, "-r",
				 PUBKEYID, "-se", "authreq.json"};
    res = execv(argv[0], (char * const *) argv);
    cerr << "GPG EXEC returned: " << res << ": " << strerror(res) << endl;
    exit(-1);
  }
  else
  {
    res = waitpid(pid, NULL, 0);
    if(res != pid)
      throw CryptoException("Unsuccessful Return Code");
    
    string gpgfilename(filename);
    gpgfilename.append(".gpg");
    fstream fs;
    fs.open(gpgfilename.c_str(), ios::in);
    if(fs.is_open())
    {
      fs.close();
      unlink(filename);
      return gpgfilename;
    }
    else
    {
      unlink(filename);
      throw CryptoException("File Does Not Exist");
    }
  }
  unlink(filename);
  return NULL;
}

int
GTBCommunication::authRequest (Request * i_aPBReq)
{
  string filename;
  string sHash;
	
  if(0 != i_aPBReq->sreqtype().compare("AUTH"))
  {
    cerr << "ERROR: C: Not AUTH" << endl;
    return -3;
  }

  if (i_aPBReq->sparams_size() == 4)
  {
    stringstream errstream;
    string errmsg;
    fstream fs;
    filename = getEncryptedPackage(i_aPBReq);
    fs.open(filename.c_str(), ios::in | ios::binary);
    if(!fs.is_open())
      throw CryptoException("File Does Not Exist");
    fs.close();

    CURL * curl;
    CURLcode res;
    struct curl_httppost * formpost = NULL;
    struct curl_httppost * lastptr = NULL;

    curl = curl_easy_init();
    if(curl == NULL)
      throw GTBException("Could not get cURL handle");

    res = curl_easy_setopt(curl, CURLOPT_URL,
                      "http://guarddogs.uconn.edu/index.php?id=42");
    if(res != CURLE_OK){
      cerr << "URLOPT_URL failed: " <<  curl_easy_strerror(res) << endl;
      errstream << "URLOPT_URL failed: " <<  curl_easy_strerror(res) << endl;
      curl_easy_cleanup(curl);
      curl_formfree(formpost);
      errstream >> errmsg;
      throw GTBException(errmsg);
    }

    res = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0);
    if(res != CURLE_OK){
      cerr << "CURLOPT_FOLLOWLOCATION failed: " <<
              curl_easy_strerror(res) << endl;
      errstream << "CURLOPT_FOLLOWLOCATION failed: " <<
              curl_easy_strerror(res) << endl;
      curl_easy_cleanup(curl);
      curl_formfree(formpost);
      errstream >> errmsg;
      throw GTBException(errmsg);
    }

    res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, authRequest_callback);
    if(res != CURLE_OK){
      cerr << "CURLOPT_WRITEFUNCTION failed: " <<
              curl_easy_strerror(res) << endl;
      errstream << "CURLOPT_WRITEFUNCTION failed: " <<
              curl_easy_strerror(res) << endl;
      curl_easy_cleanup(curl);
      curl_formfree(formpost);
      errstream >> errmsg;
      throw GTBException(errmsg);
    }
    res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, i_aPBReq);
    if(res != CURLE_OK){
      cerr << "CURLOPT_WRITEDATA failed: " <<
              curl_easy_strerror(res) << endl;
      errstream << "CURLOPT_WRITEDATA failed: " <<
              curl_easy_strerror(res) << endl;
      curl_easy_cleanup(curl);
      curl_formfree(formpost);
      errstream >> errmsg;
      throw GTBException(errmsg);
    }

    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "filename",
                 CURLFORM_FILE, filename.c_str(), CURLFORM_END);

    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

    try
    {
      res = curl_easy_perform(curl);
    } catch (CryptoException &e)
    {
      throw e;
    }
    if(res != CURLE_OK){
      cerr << "curl_easy_perform() failed: " <<
              curl_easy_strerror(res) << endl;
      errstream << "curl_easy_perform() failed: " <<
              curl_easy_strerror(res) << endl;
      curl_easy_cleanup(curl);
      curl_formfree(formpost);
      errstream >> errmsg;
      throw GTBException(errmsg);
    }
    unlink(filename.c_str());
    curl_easy_cleanup(curl);
    curl_formfree(formpost);
  }
  else
  {
    if(debug & 4)
      cerr << "ERROR: C: Missing Paramters: Only " << i_aPBReq->sparams_size()
           << " provided!" << endl;
    throw UserException("All fields were not filled in");
  }

  return 0;
}

int 
GTBCommunication::currRequest (Request * i_aPBReq, Response * i_pbRes)
{
  if(0 != i_aPBReq->sreqtype().compare("CURR"))
  {
    cerr << "ERROR: C: Not CURR: " << i_aPBReq->sreqtype()  << endl;
    i_pbRes->set_nrespid(-1);
    i_pbRes->set_sresvalue("Not CURR");
    return -2;
  }
  int i = 0;
  vector<int> vrides;
  for(; i < i_aPBReq->nparams_size(); i++)
    vrides.insert(vrides.end(), i_aPBReq->nparams(i));

  cout << "C: Getting Current Rides" << endl;
  try
  {
    m_MySQLConn->getCurr(i_aPBReq->ncarid(), i_pbRes->mutable_plpatronlist(), vrides);
  } catch (PatronException &e)
  {
    i_pbRes->clear_plpatronlist();
  }
  return 0;
}


int 
GTBCommunication::updtRequest (Request * i_aPBReq, Response * i_aPBRes)
{
  map<int, string> vRetVal;
  map<int, string>::iterator vrvIt;

  if(0 != i_aPBReq->sreqtype().compare("UPDT"))
  {
    cerr << "ERROR: C: Not UPDT: " << i_aPBReq->sreqtype()  << endl;
    i_aPBRes->set_nrespid(-1);
    i_aPBRes->set_sresvalue("Not UPDT");
    return -2;
  }
  cout << "C: Setting Ride Updates" << endl;
  vRetVal = m_MySQLConn->setUpdt(i_aPBReq->ncarid(), i_aPBRes->mutable_plpatronlist(), i_aPBReq);
 
  for(vrvIt = vRetVal.begin(); vrvIt != vRetVal.end(); vrvIt++)
  {
    i_aPBRes->add_nresadd(vrvIt->first);
    i_aPBRes->add_sresadd(vrvIt->second);
  }
  return 0;
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
    // CURRent Rides
    case 1:
      int nCurrRet;

      if(debug & 1)
      {
        cout << "Type CURR" << endl;
      }
      apbRes.set_sresvalue("CURR");
      if(!(nCurrRet = currRequest (&i_aPBReq, &apbRes)))
      {
        if(debug & 1)
	{
          cout << "C: Sending Response for CURR" << endl;
	}
        sendResponse(0, NULL, &apbRes, NULL);
      }
      else
      {
        cerr << "ERROR: C: Sending ERROR Response for CURR" << endl;
        sendResponse(nCurrRet, NULL, &apbRes, NULL);
      }
      break;
    // AUTH
    case 2:
      int nAuthRet;
      if(debug & 1)
      {
        cout << "Type AUTH" << endl;
      }
      try
      {
        if(!(nAuthRet = authRequest (&i_aPBReq)))
        {
          int idx = i_aPBReq.nclient();
	  if(idx > clientsList.size())
            cerr << "Client is not in list! Client idx: " << idx <<
                    ", clientList.size() = " << clientsList.size() << endl;
          GTBClient * client = clientsList.at(idx);
          client->setVerified(true);
          client->setCarNum(i_aPBReq.ncarid());
          if(i_aPBReq.sparams_size() > 1)
          {
            client->setNetIDs(i_aPBReq.sparams(0), i_aPBReq.sparams(1));
            client->setCarSize(i_aPBReq.nparams(0));
            sendResponse(0, NULL, NULL, NULL);
            moveKey();
          }
        }
        else
        {
          sendResponse(nAuthRet, NULL, NULL, NULL);
        }
      } catch(CryptoException &e)
      {
        sendResponse(-3, NULL, NULL, NULL);
      } catch (UserException &e)
      {
        sendResponse(-1, NULL, NULL, NULL);
      } catch (BadConnectionException &e)
      {
        sendResponse(-10, NULL, NULL, NULL);
      } catch(GTBException &e)
      {
        sendResponse(-10, NULL, NULL, NULL);
      }
      break;
    // CARS
    case 3:
      if(debug & 1)
      {
        cout << "C: Type CARS" << endl;
      }
      if(sendNumberOfCars(&i_aPBReq) < 0)
        return sendFailureResponse(2);
      break;
    // UPDT
    case 4:
      int nUpdtRet;
      if(debug & 1)
      {
        cout << "Type UPDT" << endl;
      }
      apbRes.set_sresvalue("UPDT");
      if(!(nUpdtRet = updtRequest(&i_aPBReq, &apbRes)))
      {
	if(debug & 1)
	{
          cout << "C: Sending Response for UPDT" << endl;
	}
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
  return 0;
}

#ifdef usedbfunctions
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

} // extern
#endif //usedbfunctions

/******************************
 * Client Information Related *
 ******************************/

int GTBCommunication::addIfNewClient(GTBClient * client)
{
  vector<GTBClient *>::iterator it;
  unsigned char * data;
  size_t size;
  int i = 0;

  if(client->getCertificate().data != 0)
  {
    /* Skip this while we test and not checking client certs */
    data = client->getCertificate().data;
    size = client->getCertificate().size;

    for(it = clientsList.begin(); it < clientsList.end() && ++i; ++it)
      if (!memcmp((*it)->getCertificate().data, data, size))
      {
        (*it)->setIPAddr(client->getIPAddr());
        return i;
      }
  }
  clientsList.push_back(client);
  return i;
}
