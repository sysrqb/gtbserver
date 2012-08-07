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

#include "gtbcommunication.hpp"
#include "gtest/gtest.h"
#include "communication.pb.h"
#include "../test/externalfunctions.hpp"
#include "../test/gtbcommunicationtest.hpp"
#include "gtbexceptions.hpp"
#include <gnutls/gnutls.h>
#include <gnutls/gnutlsxx.h>
#include <gnutls/x509.h>
#include <iostream>
#include <cerrno>
#include <cstring>
#include <fstream>
#include "threading.hpp"

extern "C" void *(for_communicationTest)(void * instance);
void cpp_forCommunicationTest(void * instance);

using namespace std;

void 
*(startserver)(void * expectthrow)
{
//FileDescriptors: sockfd (listen), new_fd (new connections)
  pthread_attr_t attr;
  int nRetVal(0);
  pthread_t thread_id(0);
  int signum(0);
  sigset_t set;
  Request aPBReq;
  bool * throws;
  throws = static_cast<bool *>(expectthrow);

  GTBCommunicationTest aComm(18);
  sigemptyset(&set);
  sigaddset(&set, SIGIO);
  sigaddset(&set, SIGUSR1);
  sigaddset(&set, SIGUSR2);
  nRetVal = pthread_sigmask(SIG_BLOCK, &set, NULL);
  if(nRetVal != 0)
  {
    cerr << "Failed to sigio mask!" << endl;
    throw new exception();
  }
    
  nRetVal = pthread_attr_init(&attr);
  if(nRetVal != 0)
  {
    cerr << "Failed to Initialize pthread_attr_t!" << endl;
    throw new exception();
  }
      
  thread_id = pthread_self();
  aComm.threadid_push_back(thread_id);
  cout << "Main tid: " << thread_id << endl;
  struct gtbasyncthread gtbargs;
  gtbargs.aComm = &aComm;
  gtbargs.throws = *throws;
  nRetVal = pthread_create(&thread_id, &attr, &gtbAccept_c, (void *) &aComm);
  aComm.threadid_push_back(thread_id);
  cout << "Accept tid: " << thread_id << endl;
  nRetVal = pthread_create(&thread_id, &attr, &for_communicationTest, (void *) &gtbargs);
  cout << "Comm tid: " << thread_id << endl;
  aComm.threadid_push_back(thread_id);
  nRetVal = pthread_create(&thread_id, &attr, &for_watchdog, (void *) &aComm);
  cout << "Watchdog tid: " << thread_id << endl;
  aComm.threadid_push_back(thread_id);
  if(nRetVal != 0)
  {
    cerr << "Failed to Create pthread!" << endl;
    throw new exception();
  }

  nRetVal = pthread_attr_destroy(&attr);
  if(nRetVal != 0)
  {
     cerr << "Failed to destroy pthread_attr_t!" << endl;
     throw new exception();
  }

  if(sigwait(&set, &signum) != 0)
  {
    cerr << "Error while waiting for signal!" << endl;
  }
  if(signum == SIGIO)
  {
    aComm.requestQueuePop(&aPBReq);
    aComm.dealWithReq(aPBReq);
  }
  return NULL;
}


int establishTCPConnection()
{
  #define host "192.168.5.29"
  #define port "4680"
  struct addrinfo hints, *res;
  int sockfd, nRetVal = 0;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  getaddrinfo( host, port, &hints, &res); 
  sockfd = socket(res->ai_family,res->ai_socktype, res->ai_protocol);
  cout << "Socket Returned: " << sockfd;
  if(sockfd  == -1)
    cout << ", " << strerror(errno);
  cout << endl;
  EXPECT_GT(sockfd, 0);
  

  nRetVal = connect(sockfd, res->ai_addr, res->ai_addrlen);
  EXPECT_NE(nRetVal, -1);
  if(nRetVal == -1)
    cout << "Error: " << strerror(errno) << endl;
  return sockfd;
}

void handleConnectionWrapper(GTBCommunication * aGtbComm, int sockfd, bool throws)
{
  GTBClient client;
  client.setFD(0);
  if (throws)
    ASSERT_THROW(aGtbComm->handleConnection(&client), BadConnectionException);
  else
    ASSERT_NO_THROW(aGtbComm->handleConnection(&client));
  close(sockfd);
  gnutls_global_deinit();
  exit(0);
}

void gnutls_log_fun(int level, const char * loginfo)
{
  std::cout << "LOG" << level << ": " << loginfo << std::endl;
}

static int
_verify_certificate_callback (gnutls_session_t session)
{
  unsigned int status;
  const gnutls_datum_t *cert_list;
  unsigned int cert_list_size;
  const char *hostname;

  /* read hostname */
  hostname = (char *)gnutls_session_get_ptr (session);

  /* This verification function uses the trusted CAs in the credentials
   * structure. So you must have installed one or more CA certificates.
   */
  EXPECT_GE(gnutls_certificate_verify_peers2 (session, &status), 0);

  EXPECT_FALSE(status & GNUTLS_CERT_INVALID);

  EXPECT_FALSE(status & GNUTLS_CERT_SIGNER_NOT_FOUND);

  EXPECT_FALSE(status & GNUTLS_CERT_REVOKED);

  EXPECT_FALSE(status & GNUTLS_CERT_EXPIRED);

  EXPECT_FALSE(status & GNUTLS_CERT_NOT_ACTIVATED);

  /* Up to here the process is the same for X.509 certificates and
   * OpenPGP keys. From now on X.509 certificates are assumed. This can
   * be easily extended to work with openpgp keys as well.
   */
  EXPECT_EQ(gnutls_certificate_type_get (session), GNUTLS_CRT_X509);

  cert_list = gnutls_certificate_get_peers (session, &cert_list_size);
  EXPECT_NE((long) cert_list, NULL);
  /* notify gnutls to continue handshake normally */
  return 0;
}

/** \brief Wrapper used to allow pthread_create call method 
 *
 *
 */
void cpp_forCommunicationTest(void * instance)
{
  struct gtbasyncthread * gtbargs = (struct gtbasyncthread *) instance;
  gtbargs->aComm->gtb_wrapperForCommunication(gtbargs->throws);
}

/** \brief Wrapper used to allow pthread_create call method 
 *
 */
extern "C"
void *(for_communicationTest)(void * instance)
{
  cpp_forCommunicationTest(instance);
  return NULL;
}
