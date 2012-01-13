#ifndef gtbcommunication_h
#define gtbcommunication_h
#include "mysqlconn.h"
#include <gnutls/gnutls.h>
#include "patron.pb-c.h"
#include <string>

#include "gtbconnection.hh"

//Sets the priority string for an acceptable TLS handshake
//#define GNUTLS_PRIORITY "NONE:+VERS-TLS-1.2:+DHE_RSA:+AES-128-CBC:"\
//                           "+SHA256:SIGN-RSA-256:%SAFE_RENEGOTIATION"
#define GNUTLS_PRIORITY "NORMAL"

//Number of bits to be used for the DHE
#define DH_BITS 1024

//CA, CERT, and CRL files
#define KEYFILE "pem/key.pem"
#define CAFILE "pem/cacrt.pem"
#define CRLFILE "pem/cacrl.pem"
#define CERTFILE "pem/crt.pem"

//Needed because of mutual inclusion
class GTBConnection;

class GTBCommunication {
    gnutls_priority_t * m_pPriorityCache;
    gnutls_certificate_credentials_t * m_pX509Cred;
    gnutls_dh_params_t * m_pDHParams;
    gnutls_session_t m_aSession;

  public:
    friend class GTBConnection;

    /*Constructors*/
    GTBCommunication();

    GTBCommunication(
    gnutls_priority_t * pPriorityCache,
    gnutls_certificate_credentials_t * pX509Cred) : 
        m_pPriorityCache (pPriorityCache),
	m_pX509Cred (pX509Cred) {}

    /*GNUTLS related methods*/
    void initTLSSession ();
    int generateDHParams ();
    void loadCertFiles ();

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
    int authRequest (
        std::string i_sReqBuf, 
	int i_fdSock, 
	GTBConnection * i_pConn);
};
#endif
