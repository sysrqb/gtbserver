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
#include "gtest/gtest.h"
#include "communication.pb.h"
#include <gnutls/gnutls.h>
#include <gnutls/gnutlsxx.h>
#include <gnutls/x509.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <libtasn1.h>

#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>


int establishTCPConnection()
{
  #define host "192.168.5.29"
  #define port "4680"
  struct addrinfo hints, *res;
  int sockfd;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  getaddrinfo( host, port, &hints, &res); 
  sockfd = socket(res->ai_family,res->ai_socktype, res->ai_protocol);

  EXPECT_NE(connect(sockfd, res->ai_addr, res->ai_addrlen), -1);
  return sockfd;
}

void handleConnectionWrapper(GTBCommunication * aGtbComm, int sockfd, bool throws)
{
  int fdAccepted(0);
  if (throws)
    ASSERT_THROW(aGtbComm->handleConnection(fdAccepted, sockfd), BadConnectionException);
  else
    ASSERT_NO_THROW(aGtbComm->handleConnection(fdAccepted, sockfd));
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

TEST(CommunicationTest, HandlingConnectionNoThrow)
{
  if(fork())
  {
    GTBCommunication aGtbComm;
    int server_sockfd(0), fdAccepted(0);
    server_sockfd = aGtbComm.getSocket();
    initIncomingCon (&server_sockfd);
    aGtbComm.initGNUTLS();
    fdAccepted = aGtbComm.listeningForClient(server_sockfd);
    ASSERT_NO_THROW(aGtbComm.handleConnection(fdAccepted, server_sockfd));
    gnutls_global_deinit();
    close(server_sockfd);
    exit(0);
  }
  else
  {
    int client_sockfd(0);
    unsigned int sleep(50000);
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
    gnutls_global_set_log_function(gnutls_log_fun);
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

    int nRetVal = 0;
    do
    {
      nRetVal = gnutls_handshake(session);
      //std::cout << "Client Return Value: " << nRetVal << std::endl;
      usleep(sleep);
    } while (gnutls_error_is_fatal (nRetVal) != GNUTLS_E_SUCCESS);
    /* Uncomment when we can successfully establish authenicated connection */
    // ASSERT_EQ(gnutls_certificate_client_get_request_status(session), 1);
    EXPECT_EQ(nRetVal, 0);
    gnutls_bye (session, GNUTLS_SHUT_RDWR);
    close(client_sockfd);
    gnutls_deinit (session);
    gnutls_certificate_free_credentials (xcred);
  }
}

TEST(CommunicationTest, HandlingConnectionThrowBadConnectionExceptionFromInvalidFD)
{
  GTBCommunication aGtbComm;
  int sockfd;
  sockfd = aGtbComm.getSocket();
  initIncomingCon (&sockfd);
  aGtbComm.initGNUTLS();

  ASSERT_THROW(aGtbComm.handleConnection(0, 0), BadConnectionException);
  close(sockfd);
  gnutls_global_deinit();
}


TEST(CommunicationTest, HandlingConnectionThrowBadConnectionExceptionFromInvalidCert)
{
  if(fork())
  {
    GTBCommunication aGtbComm;
    int server_sockfd(0);
    server_sockfd = aGtbComm.getSocket();
    initIncomingCon (&server_sockfd);
    aGtbComm.initGNUTLS();
    handleConnectionWrapper(&aGtbComm, server_sockfd, true);
    close(server_sockfd);
  }
  else
  {
    int client_sockfd(0);
    unsigned int sleep(1000);
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

    ASSERT_LT(gnutls_handshake(session), 0);
    close(client_sockfd);
    gnutls_deinit (session);
    gnutls_certificate_free_credentials (xcred);
 }
}