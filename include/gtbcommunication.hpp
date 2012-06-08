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

#ifndef gtbcommunication_h
#define gtbcommunication_h

#include <cerrno>
#include "sqlconn.hpp"
#include "communication.pb.h"
#include "patron.pb.h"
#include <gnutls/gnutlsxx.h>
#include "gtbexceptions.hpp"

//Sets the priority string for an acceptable TLS handshake
/*#define GNUTLS_PRIORITY "+VERS-TLS1.2:+VERS-TLS1.1:+DHE-RSA:"\
    "+DHE-DSS:+AES-256-CBC:+AES-128-CBC:"\
    "+SHA256:+SHA128:+SHA1:+COMP-NULL:+NORMAL:%SAFE_RENEGOTIATION"
*/
#define GNUTLS_PRIORITY "NORMAL"

//Number of bits to be used for the DHE
#define DH_BITS 1024
#define DH_PK_ALGO "RSA"

//CA, CERT, and CRL files
#define SKEYFILE "pem/tmpl/vdev/gtbskey.pem"
#define CAFILE "pem/certs/cacrt.pem"
#define CRLFILE "pem/cacrl.pem"
#define CERTFILE "pem/tmpl/vdev/gtbsvdevcrt.pem"

///Sets port number for server to listen on
#define PORT "4680"

//Sets number of connections to allow in queue
#define BACKLOG 7

//Sets number of bytes each request should be, incomming from client
#define REQSIZE 9

//Sets number of bytes each login request should be from a client
#define LOGINSIZE 18

//Sets number of bytes each authentication request should be from a client
#define AUTHSIZE 60


/**
 * Class: GTBCommunication
 * 
 * \brief Handles communication with network clients
 *
 * Facilitates establishing a connection using TLS with client,
 * as well as sending, retrieving, and updating patron information
 * in the selected database
 */
class GTBCommunication {
    gnutls_priority_t * m_pPriorityCache;
    gnutls_certificate_credentials_t * m_pX509Cred;
    gnutls_dh_params_t * m_pDHParams;
    gnutls_session_t m_aSession;
    /** Stores IP address of incoming client */
    char m_vIPAddr[INET6_ADDRSTRLEN];
    std::string m_sHash;
    /** SQL connection handler */
    MySQLConn * m_MySQLConn;
    /** Debug bit mask */
    int debug;

  public:
    /** \brief Constructor with optional debug output 
     * 
     * Constructor defaults to zero debug output.
     * For additional debugging information, increase the value
     * at instantiation.
     * \sa debug
     */
    GTBCommunication(int indebug = 0)
    {
      debug = indebug;
      m_pPriorityCache = (gnutls_priority_t *) 
          operator new (sizeof (gnutls_priority_t));
      m_pX509Cred = (gnutls_certificate_credentials_t * ) 
          operator new (sizeof (gnutls_certificate_credentials_t));
      m_MySQLConn = new MySQLConn();
      m_pDHParams = (gnutls_dh_params_t *) operator 
          new (sizeof (gnutls_dh_params_t));
    }

    /** \brief Deconstructor to clean up dynamic info */
    ~GTBCommunication()
    {
      delete m_pDHParams;
      delete m_MySQLConn;
      delete m_pX509Cred;
      delete m_pPriorityCache;
    }

    /* GNUTLS related methods */
    /** \brief Handle initialization of GNUTLS backend
     *
     * Directly calls gnutls_global_init and subsequently calls for
     * the loading of Certificate files and setting priority values
     */
    void initGNUTLS();

    /** \brief Handle the deinitialization of GNUTLS
     *
     * Calls gnutls_deinit on the current TLS session
     */
    void deinitGNUTLS();

    /** \brief Prepare a new TLS Session
     * 
     * Initialize a new session for the current client.
     * Set certificate to use and connection requirements.
     */
    void initTLSSession ();
 
    /** \brief Prepare Diffie-Hellman Parameters
     *
     * Initialize Diffie-Hellman parameters for new TLS session
     * and generate new keys.
     */
    int generateDHParams ();

    /** \brief Loads server-side certificates and keys */
    void loadCertFiles ();

    /*Misc mutator and accessor methods*/

    /** \brief Unimplemented. May be removed. */
    void getClientInfo (int i_sockfd);

    /** \brief Bind to open port
     *
     * Find free port and bind it it.
     */
    int getSocket ();

    /** \brief Return incoming client as IPv4 or IPv6
     *
     * Checks if client is using IPv4 or IPv6 and sets/initializes 
     * the structs accordingly. Depending on the protocol that is 
     * being used, i_sa will either be struct in_addr or in6_addr.
     * sin_addr or sin6_addr, respectively, is then the return value.
     *
     * \param i_sa struct sockaddr_storage; Address of peer layer
     * as known to the communication layer
     */
    void *getInAddr (struct sockaddr *i_sa);

    /** \brief Accessor: Returns the IP Address of the current client */
    std::string getClientAddr(){ return std::string(m_vIPAddr); } 

    /*GNUTLS variables accessor methods */

    /** \brief Accessor: Returns current priorities set in TLS session */
    gnutls_priority_t * getPriorityCache() { return m_pPriorityCache; }

    /** \brief Accessor: Returns struct holding certificate info */
    gnutls_certificate_credentials_t * getCertCred() { return m_pX509Cred; }

    /** \brief Accessor: Return Diffie-Hellman parameters */
    gnutls_dh_params_t * getDHParams() { return m_pDHParams; }

    /** \brief Accessor: Return current session info */
    gnutls_session_t * getSession() { return &m_aSession; }

    /*Communication with client*/

    /** \brief Return errorcode to client.
     *
     * \param errorcode The errorcode returned from processing the request.
     */
    int sendFailureResponse(int errorcode);

    /** \brief Return 0 = Successful to client. */
    int sendAOK();

    /** \brief Send only sizeof(int) bytes containing i_nRetVal 
     * to client. 
     *
     * \param i_nRetVal The return value from the request.
     */
    int sendNopes(int i_nRetVal);

    /** \brief Return number of running vans to client.
     *
     * Checks that i_aPBReq is a valid request. If so, the database 
     * is queried and the result is returned in a Response packet.
     * \param i_aPBReq The request received from the client.
     */
    int sendNumberOfCars(Request * i_aPBReq);

    /** \brief Unimplemented: May be removed */
    int moveKey();

    /** \brief Request to authenticate client.
     *
     * Checks that i_aPBReq is a valid request. If so, the provided
     * values in the packet are verified.
     *
     * \param i_aPBReq The request received from the client
     * \sa checkAuth
     * \todo Store car number, ip address, cert uid for future 
     * authentication
     */
    int authRequest (Request * i_aPBReq);

    /** \brief Request for current patrons assigned to client.
     *
     * Checks that i_aPBReq is a valid request. If so, the current
     * patrons are returned to the client, excluding those that have
     * have not be modified and the client already has in its DB.
     *
     * \param i_aPBReq The request received from the client
     * \param i_apbRes The response that will be filled with the
     * requested information.
     * \todo Verify car number, ip address, cert uid
     */
    int currRequest (Request * i_aPBReq, Response * i_apbRes);

    /** \brief Request to update a patron's info.
     *
     * Checks that i_aPBReq is a valid request. If so, the patrons
     * information is updated in the database.
     *
     * \param i_aPBReq The request received from the client
     * \param i_apbRes The response that will be filled with the
     * \todo Authenticate client based on car number, ip address, cert uid
     * \todo Conflict resolution if conflicting modified field exist
     */
    int updtRequest (Request * i_aPBReq, Response * i_apbRes);

    /** \brief Handle incoming request.
     *
     * When a request is received, determine the request ID number. Using
     * this number, the request is then handed off to the correct method
     *
     * \param i_sPBReq The request received from the client.
     * \sa authRequest
     * \sa currRequest
     * \sa updtRequest
     */
    int dealWithReq (Request i_sPBReq);

    /** \brief Perform TLS handshake with client
     *
     * After a client connects to the server, perform TLS handshake
     * with client to establish TLS session. Once complete, verify 
     * the client provides a certificate and that it was signed by
     * our CA
     *
     * \todo Figure out why certificate verification segfaults =(
     *
     * \param fdAccepted File descriptor for opened socket connection
     * \param sockfd File descriptor for bound socket
     */
    int handleConnection (int fdAccepted, int sockfd);

    /** \brief Listen for a client connection
     *
     * Initialize TLS session. When a client connects, store the client's
     * IP address.
     *
     * \param i_fdSock File descriptor for bound socket
     * \return File descriptor for opened socket connection
     */
    int listeningForClient (int i_fdSock);

    /** \brief Retrieves all incoming requests from client
     *
     * Once the connection is established, all request/incoming packets
     * are received and returned to plaintext.
     *
     * param aPBReq Where incoming request is stored because it's content
     * will be used throughout the request's lifetime
     */
    void receiveRequest(Request * aPBReq);

  private:
    /** \brief Sends response to client.
     *
     * Once a request has been completed the response is sent to the client.
     *
     * If all input parameters are NULL and i_nRetVal is NOT 0, then return an
     * error message
     * If all input parameters are NULL and i_nRetVal is 0, then return
     * a successful reply
     * If at least the i_pbRes parameter is NOT NULL, then send the response
     * packet.
     * If at least one parameter is NOT NULL, but i_pbRes is NULL then do not
     * send a response.
     *
     * \param i_nRetVal The return value from the request
     * \param i_pbReq The request received from the client
     * \param i_pbRes The response created to be send to the client
     * \param i_pbPL The patron[s] to be sent to the client if created
     * as a result of the request
     */
    int sendResponse(int i_nRetVal, 
            Request * i_pbReq, 
	    Response * i_pbRes, 
	    PatronList * i_pbPL);
};
#endif  // gtbcommunication_h
