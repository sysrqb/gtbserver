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
#include "gtbexceptions.hpp"

#include <gnutls/gnutls.h>
#include <gnutls/gnutlsxx.h>
#include <gnutls/x509.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <libtasn1.h>
#include <pthread.h>

#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <cerrno>
#include <cstring>

#include "../test/gtbcommunicationtest.hpp"
#include "../test/externalfunctions.hpp"

using namespace std;

static int
cert_callback (gnutls_session_t session,
               const gnutls_datum_t * req_ca_rdn, int nreqs,
               const gnutls_pk_algorithm_t * sign_algos,
               int sign_algos_length, gnutls_retr2_st * st);

TEST(CommunicationTest, HandlingConnectionNoThrow)
{
  int nRetVal(0);
  bool bserverthrows(0);
  pthread_attr_t attr;
  pthread_t thread_id(0);
  nRetVal = pthread_attr_init(&attr);
  if(nRetVal)
  {
    cerr << "Failed to Initialize pthread_attr_t!" << endl;
    FAIL();
  }
  nRetVal = pthread_create(&thread_id, &attr, &startserver, (void *)&bserverthrows);
  if(nRetVal)
  {
    cerr << "Failed to Create pthread!" << endl;
    FAIL();
  }
  nRetVal = pthread_attr_destroy(&attr);
  if(nRetVal)
  {
     cerr << "Failed to destroy pthread_attr_t!" << endl;
    FAIL();
  }

  int client_sockfd(0);
  unsigned int sleep(15000);
  Request request;
  //std::fstream cert("/home/mfinkel/repos/gtbserver/pem/certs/gtbt1crt.pem", std::fstream::in);
  std::fstream cert("/home/mfinkel/repos/gtbserver/pem/keys/gtbt1key.pem", 
    std::fstream::in);
  ASSERT_TRUE(cert.is_open());
  cert.close();
  cert.open("/home/mfinkel/repos/gtbserver/pem/certs/gtbt1crt.pem", 
    std::fstream::in);
  ASSERT_TRUE(cert.is_open());
  cert.close();
  usleep(sleep);
  gnutls_certificate_credentials_t xcred;
  gnutls_session_t session;
  //gnutls_global_set_log_function(gnutls_log_fun);
  //gnutls_global_set_log_level(6);
  gnutls_global_init();
  gnutls_certificate_allocate_credentials (&xcred);
  //ASSERT_EQ(gnutls_certificate_set_verify_function (xcred, _verify_certificate_callback), 0);
  ASSERT_EQ(gnutls_certificate_set_x509_trust_file (xcred, 
      CAFILE, 
      GNUTLS_X509_FMT_PEM), 
      1);
  gnutls_certificate_set_retrieve_function( xcred, &cert_callback);
  ASSERT_EQ(gnutls_certificate_set_x509_key_file (xcred, 
      "/home/mfinkel/repos/gtbserver/pem/tmpl/vdev/gtbvdevcrt.pem", 
      "/home/mfinkel/repos/gtbserver/pem/tmpl/vdev/gtbvdevkey.pem", 
	GNUTLS_X509_FMT_PEM), 0);
  ASSERT_EQ(gnutls_init (&session, GNUTLS_CLIENT), 0);
  ASSERT_EQ(gnutls_priority_set_direct (session, "NORMAL", NULL), 0);
  ASSERT_EQ(gnutls_credentials_set (session, GNUTLS_CRD_CERTIFICATE, xcred), 
      0);
  client_sockfd = establishTCPConnection();
  gnutls_transport_set_ptr (session, (gnutls_transport_ptr_t) client_sockfd); 
   //int nRetVal = 0;
  do
  {
    nRetVal = gnutls_handshake(session);
    //std::cout << "Client Return Value: " << nRetVal << std::endl;
    usleep(sleep);
  } while (gnutls_error_is_fatal (nRetVal) != GNUTLS_E_SUCCESS);
  /* Uncomment when we can successfully establish authenicated connection */
  // ASSERT_EQ(gnutls_certificate_client_get_request_status(session), 1);
  EXPECT_EQ(nRetVal, 0);
  request.set_nreqid(1);
  request.set_sreqtype("test");
  string srequest;
  request.SerializeToString(&srequest);
  int reqsize = request.ByteSize();
  gnutls_record_send(session, &reqsize, sizeof(request.ByteSize()));
  gnutls_record_send(session, srequest.c_str(), request.ByteSize());
  gnutls_bye (session, GNUTLS_SHUT_RDWR);
  close(client_sockfd);
  gnutls_deinit (session);
  gnutls_certificate_free_credentials (xcred);
}

TEST(CommunicationTest, HandlingConnectionThrowBadConnectionExceptionFromInvalidCert)
{
  int nRetVal(0);
  bool bserverthrows(1);
  pthread_attr_t attr;
  pthread_t thread_id(0);
  nRetVal = pthread_attr_init(&attr);
  if(nRetVal)
  {
    cerr << "Failed to Initialize pthread_attr_t!" << endl;
    FAIL();
  }
  nRetVal = pthread_create(&thread_id, &attr, &startserver, (void *)&bserverthrows);
  if(nRetVal)
  {
    cerr << "Failed to Create pthread!" << endl;
    FAIL();
  }
  nRetVal = pthread_attr_destroy(&attr);
  if(nRetVal)
  {
     cerr << "Failed to destroy pthread_attr_t!" << endl;
    FAIL();
  }

  int client_sockfd(0);
  Request request;
  unsigned int sleep(15000);
  gnutls_certificate_credentials_t xcred;
  gnutls_session_t session;
  /*gnutls_global_set_log_function(gnutls_log_fun);
  gnutls_global_set_log_level(9);*/
  gnutls_certificate_allocate_credentials (&xcred);
  gnutls_certificate_set_x509_trust_file (xcred, "pem/oldcerts/cacrt.pem", 
      GNUTLS_X509_FMT_PEM);
  gnutls_certificate_set_x509_key_file (xcred, "pem/oldcert/gtbscrt.pem", 
      "pem/keys/gtbskey.pem", GNUTLS_X509_FMT_PEM);
  gnutls_init (&session, GNUTLS_CLIENT);
  gnutls_priority_set_direct (session, "NORMAL", NULL);
  gnutls_credentials_set (session, GNUTLS_CRD_CERTIFICATE, xcred);
  usleep(sleep);
  client_sockfd = establishTCPConnection();
  gnutls_transport_set_ptr (session, (gnutls_transport_ptr_t) client_sockfd); 

  EXPECT_LT(gnutls_handshake(session), 0);
  request.set_nreqid(1);
  request.set_sreqtype("test");
  string srequest;
  request.SerializeToString(&srequest);
  int reqsize = request.ByteSize();
  gnutls_record_send(session, &reqsize, sizeof(request.ByteSize()));
  gnutls_record_send(session, srequest.c_str(), request.ByteSize());
  close(client_sockfd);
  gnutls_deinit (session);
  gnutls_certificate_free_credentials (xcred);
}

TEST(CommunicationTest, ReceiveRequestValidRequest)
{
  int nRetVal(0);
  bool bserverthrows(0);
  pthread_attr_t attr;
  pthread_t thread_id(0);
  nRetVal = pthread_attr_init(&attr);
  if(nRetVal)
  {
    cerr << "Failed to Initialize pthread_attr_t!" << endl;
    FAIL();
  }
  nRetVal = pthread_create(&thread_id, &attr, &startserver, (void *)&bserverthrows);
  if(nRetVal)
  {
    cerr << "Failed to Create pthread!" << endl;
    FAIL();
  }
  nRetVal = pthread_attr_destroy(&attr);
  if(nRetVal)
  {
     cerr << "Failed to destroy pthread_attr_t!" << endl;
    FAIL();
  }

  int client_sockfd(0);
  Request aPBReq;
  std::string sPBReq("");
  unsigned int sleep(15000);
  gnutls_certificate_credentials_t xcred;
  gnutls_session_t session;
  gnutls_certificate_allocate_credentials (&xcred);
  gnutls_certificate_set_x509_trust_file (xcred, "pem/oldcerts/cacrt.pem", 
      GNUTLS_X509_FMT_PEM);
  gnutls_certificate_set_x509_key_file (xcred, "pem/oldcert/gtbscrt.pem", 
      "pem/keys/gtbskey.pem", GNUTLS_X509_FMT_PEM);
  gnutls_init (&session, GNUTLS_CLIENT);
  gnutls_priority_set_direct (session, "NORMAL", NULL);
  gnutls_credentials_set (session, GNUTLS_CRD_CERTIFICATE, xcred);
  usleep(sleep);
  client_sockfd = establishTCPConnection();
  gnutls_transport_set_ptr (session, (gnutls_transport_ptr_t) client_sockfd); 

  ASSERT_EQ(gnutls_handshake(session), 0);

  aPBReq.set_sreqtype("CURR");
  aPBReq.set_nreqid(1);
  aPBReq.SerializeToString(&sPBReq);
  int reqsize = aPBReq.ByteSize();
  if(gnutls_record_send(session, &reqsize, aPBReq.ByteSize()) == -1)
  {
    std::cerr << "ERROR: Error on send: " << strerror(errno) << std::endl;
  }
  if(gnutls_record_send(session, sPBReq.c_str(), aPBReq.ByteSize()) == -1)
  {
    std::cerr << "ERROR: Error on send: " << strerror(errno) << std::endl;
  }
  close(client_sockfd);
  gnutls_deinit (session);
  gnutls_certificate_free_credentials (xcred);
}

TEST(CommunicationTest, ReceiveRequestNULLRequest)
{
  int nRetVal(0);
  bool bserverthrows(0);
  pthread_attr_t attr;
  pthread_t thread_id(0);
  nRetVal = pthread_attr_init(&attr);
  if(nRetVal)
  {
    cerr << "Failed to Initialize pthread_attr_t!" << endl;
    FAIL();
  }
  nRetVal = pthread_create(&thread_id, &attr, &startserver, (void *)&bserverthrows);
  if(nRetVal)
  {
    cerr << "Failed to Create pthread!" << endl;
    FAIL();
  }
  nRetVal = pthread_attr_destroy(&attr);
  if(nRetVal)
  {
     cerr << "Failed to destroy pthread_attr_t!" << endl;
    FAIL();
  }

  int client_sockfd(0);
  Request aPBReq;
  std::string sPBReq("");
  unsigned int sleep(15000);
  gnutls_certificate_credentials_t xcred;
  gnutls_session_t session;
  gnutls_certificate_allocate_credentials (&xcred);
  gnutls_certificate_set_x509_trust_file (xcred, "pem/oldcerts/cacrt.pem", 
      GNUTLS_X509_FMT_PEM);
  gnutls_certificate_set_x509_key_file (xcred, "pem/oldcert/gtbscrt.pem", 
      "pem/keys/gtbskey.pem", GNUTLS_X509_FMT_PEM);
  gnutls_init (&session, GNUTLS_CLIENT);
  gnutls_priority_set_direct (session, "NORMAL", NULL);
  gnutls_credentials_set (session, GNUTLS_CRD_CERTIFICATE, xcred);
  usleep(sleep);
  client_sockfd = establishTCPConnection();
  gnutls_transport_set_ptr (session, (gnutls_transport_ptr_t) client_sockfd); 

  ASSERT_EQ(gnutls_handshake(session), 0);

  aPBReq.set_sreqtype("CURR");
  aPBReq.set_nreqid(1);
  aPBReq.SerializeToString(&sPBReq);
  int reqsize = aPBReq.ByteSize();
  if(gnutls_record_send(session, &reqsize, aPBReq.ByteSize()) == -1)
  {
    std::cerr << "ERROR: Error on send: " << strerror(errno) << std::endl;
  }
  if(gnutls_record_send(session, sPBReq.c_str(), aPBReq.ByteSize()) == -1)
  {
    std::cerr << "ERROR: C: Error on send for OK: " << strerror(errno) << std::endl;
  }
  close(client_sockfd);
  gnutls_deinit (session);
  gnutls_certificate_free_credentials (xcred);
}

static int
cert_callback (gnutls_session_t session,
               const gnutls_datum_t * req_ca_rdn, int nreqs,
               const gnutls_pk_algorithm_t * sign_algos,
               int sign_algos_length, gnutls_retr2_st * st)
{
  long filelen = 0;
  gnutls_datum_t certdata;
  gnutls_datum_t keydata;
  gnutls_x509_crt_t crt;
  gnutls_x509_privkey_t key;
  char * file;

  EXPECT_EQ(gnutls_x509_crt_init (&crt), 0);
  gnutls_x509_privkey_init (&key);

  std::fstream cert(
      "/home/mfinkel/repos/gtbserver/pem/tmpl/vdev/gtbvdevcrt.pem", 
      std::fstream::in);
  if(cert.is_open())
  {
    cert.seekg(0, std::ios::end);
    filelen = cert.tellg();
    cert.seekg(0, std::ios::beg);
    file = (char *)operator new (sizeof(filelen));
    cert.read(file, filelen);
    certdata.data = (unsigned char *)file;
    certdata.size = filelen;
    EXPECT_EQ(gnutls_x509_crt_import (crt, &certdata, GNUTLS_X509_FMT_PEM), 
        0);
    st->cert.x509 = &crt;
    cert.close();
  }
  cert.open("/home/mfinkel/repos/gtbserver/pem/tmpl/vdev/gtbvdevkey.pem", 
      std::fstream::in);
  if(cert.is_open())
  {
    cert.seekg(0, std::ios::end);
    filelen = cert.tellg();
    cert.seekg(0, std::ios::beg);
    file = (char *)operator new (sizeof(filelen));
    cert.read(file, filelen);
    keydata.data = (unsigned char *)file;
    keydata.size = filelen;
    EXPECT_EQ(gnutls_x509_privkey_import (key, &keydata, GNUTLS_X509_FMT_PEM), 
        0);
    st->key.x509 = key;
    st->key_type = GNUTLS_PRIVKEY_X509;
    cert.close();
  } 
  
  st->deinit_all = 0;
  st->cert_type = GNUTLS_CRT_X509;
  st->ncerts = 1;
  return 0;
}

