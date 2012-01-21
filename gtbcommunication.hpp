#ifndef gtbcommunication_h
#define gtbcommunication_h

#include "sqlconn.hpp"
#include <errno.h>

#ifndef gnutls_h
#define gnutls_h
#include <gnutls/gnutls.h>
#endif

//Sets the priority string for an acceptable TLS handshake
//#define GNUTLS_PRIORITY "NONE:+VERS-TLS1.2:+VERS-TLS1.1:+VERS-TLS-ALL:+DHE-DSS:"\
    "+DHE-RSA:+KX-ALL:+AES-256-CBC:+AES-128-CBC:CIPHER-ALL:"\
    "+SHA256:+SHA1:+MAC-ALL::%SAFE_RENEGOTIATION"
#define GNUTLS_PRIORITY "NORMAL"

//Number of bits to be used for the DHE
#define DH_BITS 1024
#define DH_PK_ALGO "RSA"

//CA, CERT, and CRL files
#define KEYFILE "pem/key.pem"
#define CAFILE "pem/cacrt.pem"
#define CRLFILE "pem/cacrl.pem"
#define CERTFILE "pem/ecrt.pem"

///Sets port number for server to listen on
#define PORT "4680"

//Sets number of connections to allow in queue
#define BACKLOG 1

//Sets number of bytes each request should be, incomming from client
#define REQSIZE 5

//Sets number of bytes each login request should be from a client
#define LOGINSIZE 18

//Sets number of bytes each authentication request should be from a client
#define AUTHSIZE 32


class GTBCommunication {
    gnutls_priority_t * m_pPriorityCache;
    gnutls_certificate_credentials_t * m_pX509Cred;
    gnutls_dh_params_t * m_pDHParams;
    gnutls_session_t m_aSession;
    char m_vIPAddr[INET6_ADDRSTRLEN];
    std::string m_sHash;
    MySQLConn * m_MySQLConn;

  public:
    //Constructors
    GTBCommunication() {
      m_pPriorityCache = (gnutls_priority_t *) 
          operator new (sizeof (gnutls_priority_t));
      m_pX509Cred = (gnutls_certificate_credentials_t * ) 
          operator new (sizeof (gnutls_certificate_credentials_t));
      m_MySQLConn = new MySQLConn();
    }

    /*GNUTLS related methods*/
    void initTLSSession ();
    int generateDHParams ();
    void loadCertFiles ();

    /*Misc mutator and accessor methods*/
    void getClientInfo (int i_sockfd);
    int getSocket ();
    void *getInAddr (struct sockaddr *i_sa);
    std::string getClientAddr(){ return std::string(m_vIPAddr); } 

    /*GNUTLS variables accessor methods */
    gnutls_priority_t * getPriorityCache() { return m_pPriorityCache; }
    gnutls_certificate_credentials_t * getCertCred() { return m_pX509Cred; }
    gnutls_dh_params_t * getDHParams() { return m_pDHParams; }
    gnutls_session_t * getSession() { return &m_aSession; }

    /*Communication with client*/
    int sendAOK();
    int sendNopes(int i_nRetVal);
    int DHKERequest(std::string i_sReqBuf);
    int sendNumberOfCars(std::string i_sReqBuf);
    int moveKey();
    int authRequest (std::string i_sReqBuf, int i_fdSock);
    int dealWithReq (std::string i_sReqBuf, int i_fdSock);
    int listeningForClient (int i_sockfd);
};
#endif
