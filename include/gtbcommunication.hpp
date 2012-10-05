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
#include <vector>
#include <queue>
#include <pthread.h>
#include <gnutls/gnutlsxx.h>
#include <curl/curl.h>

#include "sqlconn.hpp"
#include "communication.pb.h"
#include "patron.pb.h"
#include "gtbexceptions.hpp"
#include "gtbclient.hpp"


// Sets the priority string for an acceptable TLS handshake
#define GNUTLS_PRIORITY "+VERS-TLS1.2:+VERS-TLS1.1:+RSA:+DHE-RSA:"\
    "+DHE-DSS:+AES-256-CBC:+AES-128-CBC:"\
    "+SHA256:+SHA1:+COMP-NULL:%SAFE_RENEGOTIATION"

//#define GNUTLS_PRIORITY "NORMAL"

//Number of bits to be used for the DHE
#define DH_BITS 1024
#define DH_PK_ALGO "RSA"

// CA, CERT, and CRL files
#define SKEYFILE "pem/tmpl/vdev/gtbskey.pem"
#define CAFILE "pem/certs/cacrt.pem"
#define CRLFILE "pem/cacrl.pem"
#define CERTFILE "pem/tmpl/vdev/gtbsvdevcrt.pem"

/// Sets port number for server to listen on
#define PORT "4680"

// Sets number of connections to allow in queue
#define BACKLOG 7

// Sets number of bytes each request should be, incomming from client
#define REQSIZE 9

// Sets number of bytes each login request should be from a client
#define LOGINSIZE 18

// Sets number of bytes each authentication request should be from a client
#define AUTHSIZE 60

/* Signals that are used internally by the threads */
#define SIGACCEPT SIGUSR1
#define SIGCLIENT SIGUSR2 

#define MAINTHREAD 0
#define ACPTTHREAD 1
#define COMMTHREAD 2
#define WDOGTHREAD 3

/* How long should a thread be blocked for before we kill it? */
#define WDOGKILLAFTER 20

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
    /** Stores IP address of incoming client */
    char m_vIPAddr[INET6_ADDRSTRLEN];
    std::string m_sHash;
    /** SQL connection handler */
    MySQLConn * m_MySQLConn;

    /** Stored Session List
     *
     * Linked-List of stored sessions and their corresponding clients
     */
    struct ClientSession
    {

      /** Session ID */
      gnutls_datum_t session_id;

      /** Session Data */
      gnutls_datum_t session_data;

      /** Client associated with this session */
      GTBClient * client;

      /** Next Session */
      struct ClientSession * next;

      /** End of list - because I'm impatient */
      struct ClientSession * tail;
    } * clientSessions;

    /** Retrieve Session from ClientSession Linked-List
     *
     * Private method
     */
    gnutls_datum_t retrieveSessionRecurse(void * ptr, gnutls_datum_t session_id,
			       ClientSession * client);

    /** Remove Session from ClientSession Linked-List
     *
     * Private method
     */
    int removeSessionRecurse(void * ptr, gnutls_datum_t session_id,
			     ClientSession * client);


    /** Session D String Comparison
     *
     * \return 0 on equality and -1 otherwise
     */
    int sess_strncmp(unsigned char * s1,
                                        unsigned char * s2,
                                        size_t n);

    /** Time at which we entered gnutls_record_receive
     * The watchdog thread checks us to make sure we 
     * haven't been set for longer than WDOGKILLAFTER
     * seconds
     *
     * If 0 then consider unset
     */
    time_t lostcontrolat;

  protected:
    gnutls_priority_t * m_pPriorityCache;
    gnutls_certificate_credentials_t * m_pX509Cred;
    gnutls_dh_params_t * m_pDHParams;
    /** Debug bit mask 
     *
     * 1 = Current Operation
     * 2 = 
     * 4 = Debugging value output (Not in consistant locations)
     * 5 = All
     * 8 = GnuTLS log level 1
     * 9 = GnuTLS log level 1 + Current Operation
     * 12 = GnuTLS log level 1 + Debugging value output
     * 16 = GnuTLS log level 2
     * 17 = GnuTLS log level 2 + Current Operation
     * 20 = GnuTLS log level 2 + Debugging value output
     * 32 = GnuTLS log level 3
     * 33 = GnuTLS log level 3 + Current Operation
     * 36 = GnuTLS log level 3 + Debugging value output
     *
     *          -- debug <       debug
     * Level = |
     *          -- debug >= 8    8*<GnuTLS log level> +
     *                             ((1 for Current Operation) or
     *                              (4 Debugging value output))
     * etc
     * */
    int debug;


    /** \brief Vector containing the thread ids all all spawned threads. */
    std::vector<pthread_t> thread_ids;

    /** \brief Vector containing all remaining requests that stt need to be
     * processed.
     */
    std::queue<Request> requestQueue;

    /** \brief Vector containing all clients who have successfully connected
     * in the past.
     */
    std::vector<GTBClient *> clientsList;

    /** \brief Vector containing all remaining accepted connections that 
     * potentially have pending requests.
     */
    std::queue<GTBClient *> acceptedQueue;

  public:
    /* set_session_management_functions functions fail if private */
    gnutls_session_t m_aSession;
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
      if(debug & 4)
        std::cout << "Entering Debug Level: " << debug << std::endl;

      m_pPriorityCache = (gnutls_priority_t *) 
          operator new (sizeof (gnutls_priority_t));
      m_pX509Cred = (gnutls_certificate_credentials_t * ) 
          operator new (sizeof (gnutls_certificate_credentials_t));
      m_MySQLConn = new MySQLConn();
      m_pDHParams = (gnutls_dh_params_t *) operator 
          new (sizeof (gnutls_dh_params_t));

      if(curl_global_init(CURL_GLOBAL_SSL))
      {
        std::cerr << "Unable to init libcurl!" << std::endl;
	std::cerr << "Exiting." << std::endl;
	exit(-1);
      }
    }

    /** \brief Deconstructor to clean up dynamic info */
    ~GTBCommunication()
    {
      
      int * nRetVal;
      std::vector<pthread_t>::iterator it;
      for(it = thread_ids.begin(); it < thread_ids.end(); it++)
        pthread_join(*it, (void **) &nRetVal);

      delete m_pDHParams;
      delete m_MySQLConn;
      delete m_pX509Cred;
      delete m_pPriorityCache;
      curl_global_cleanup();
    }


    /************************** 
     * Thread Related Methods *
     *                        *
     **************************/

    /** \brief Add thread ID to thread_ids vector */
    void threadid_push_back(pthread_t id)
    {
      thread_ids.push_back(id);
    }

    /** \brief Get the number of threads spawned */
    pthread_t threadid_getSize()
    {
      return thread_ids.size();
    }

    /** \brief Get thread_id at location idx */
    pthread_t getThreadIDAt(int idx)
    {
      return thread_ids.at(idx);
    }

    /** \brief Return if Request Queue is empty
     *
     * Returns if Request queue is empty by calling
     * empty() on requestQueue.
     */
    bool requestQueueIsEmpty()
    {
      return requestQueue.empty();
    }

    /** \brief Return Request at front of queue
     *
     * Fill provided Request address with the first request in the queue.
     * This pops the first request, thus removing it.
     *
     * \param request The request to be filled and fulfilled
     */
    void requestQueuePop(Request * request)
    {
      *request = requestQueue.front();
      requestQueue.pop();
    }

    /** \brief Return if Accepted Queue is empty
     *
     * Returns if Accepted queue is empty by calling
     * empty() on acceptedQueue.
     */
    inline bool acceptedQueueIsEmpty()
    {
      return acceptedQueue.empty();
    }

    /** \brief Return the address of the instance of GTBClient at 
     * front of queue
     *
     * Get and return the first instance in the queue.
     * This pops the first request, thus removing it.
     *
     */
    GTBClient * acceptedQueuePop()
    {
      GTBClient * accepted;
      accepted = acceptedQueue.front();
      acceptedQueue.pop();
      return accepted;
    }

    /** \brief Internal wrapper for accept(). */
    virtual void gtbAccept();

    /** \brief Networking thread handler
     *
     * Internal class method that handles communication with client.
     */
    void gtb_wrapperForCommunication();

    /** \brief Creates watchdog thread
     *
     * We periodically check a variable (or multiple,
     * if necessary) to ensure the thread isn't being blocked by an 
     * unresponsive client.
     *
     * It takes too long to wait for the connection to timeout, so
     * we can easily kill the hung thread and recreate it.
     */
    void launchWatchDog();


    /**************************
     * GNUTLS related methods *
     **************************/

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

    /** \brief Unimplemented: May be removed */
    int moveKey();

    /** Store session info
     *
     * Assuming I interpret the source code correctly, this function will
     * be called with params 
     *  (void *ptr, gnutls_datum_t session_id, gnutls_datum_t session_data) 
     *  where *ptr will be NULL.
     */
    int saveSessionForResume(void * ptr, gnutls_datum_t session_id, 
                         gnutls_datum_t session_data);

    /** Resume session info
     *
     * Assuming I interpret the source code correctly, this function will
     * be called with params (void *ptr, gnutls_datum_t session_id) where
     * *ptr will be NULL. From 
     */
    gnutls_datum_t retrieveSessionForResume(void * ptr, 
                                            gnutls_datum_t session_id);

    /** Remove invalid stored session info
     *
     * Passed parameters (void * ptr, gnutls_datum_t session_id)
     */
    int removeSessionForResume(void * ptr, gnutls_datum_t session_id);


    /**********************
     * Networking Related *
     **********************/

    /** \brief Unimplemented. May be removed. */
    void getClientInfo (int i_sockfd);

    /** \brief Bind to open port
     *
     * Find free port and bind it it.
     */
    virtual int getSocket ();

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
    virtual void *getInAddr (struct sockaddr *i_sa);

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
     * \param client Contains fields describing client
     */
    virtual int handleConnection (GTBClient * client);

    /** \brief Listen for a client connection
     *
     * Initialize TLS session. When a client connects, store the client's
     * IP address.
     *
     * \param i_fdSock File descriptor for bound socket
     * \return new instance of client for this session
     */
    virtual GTBClient * listeningForClient (int i_fdSock);

    /** \brief Retrieves all incoming requests from client
     *
     * Once the connection is established, all request/incoming packets
     * are received and returned to plaintext.
     *
     * param aPBReq Where incoming request is stored because it's content
     * will be used throughout the request's lifetime
     */
    virtual void receiveRequest(Request * aPBReq);




    /*************************************
     * Misc mutator and accessor methods *
     *************************************/

    /** \brief Accessor: Returns the IP Address of the current client */
    virtual std::string getClientAddr(){ return std::string(m_vIPAddr); } 

    /*GNUTLS variables accessor methods */

    /** \brief Accessor: Returns current priorities set in TLS session */
    gnutls_priority_t * getPriorityCache() { return m_pPriorityCache; }

    /** \brief Accessor: Returns struct holding certificate info */
    gnutls_certificate_credentials_t * getCertCred() { return m_pX509Cred; }

    /** \brief Accessor: Return Diffie-Hellman parameters */
    gnutls_dh_params_t * getDHParams() { return m_pDHParams; }

    /** \brief Accessor: Return current session info */
    gnutls_session_t * getSession() { return &m_aSession; }



    /*****************************
     * Communication with client *
     *****************************/

    /** \brief Return errorcode to client.
     *
     * \param errorcode The errorcode returned from processing the request.
     */
    virtual int sendFailureResponse(int errorcode);

    /** \brief Return 0 = Successful to client. */
    int sendAOK();

    /** \brief Send only sizeof(int) bytes containing i_nRetVal 
     * to client. 
     *
     * \param i_nRetVal The return value from the request.
     */
    int sendNopes(int i_nRetVal);




    /******************************
     * Related to Client Requests *
     ******************************/

    /** \brief Return number of running vans to client.
     *
     * Checks that i_aPBReq is a valid request. If so, the database 
     * is queried and the result is returned in a Response packet.
     * \param i_aPBReq The request received from the client.
     */
    virtual int sendNumberOfCars(Request * i_aPBReq);

    /** \brief Called when we receive a response from the server.
     *
     * When we receive a response from the server, handle it correctly.
     * i.e. If the credentials are authenic, then store in the DB that
     * these credentials have been validated. Else, return an error
     * message/code.
     */
    static size_t
    authRequest_callback(void *buffer, size_t size, size_t nmemb, void *userp);

    /** \brief Return a filename corresponding to an encrypted file
     *
     * We generate a json object using the data provided in aPBReq then
     * we store it in a file and pass it into gpg where it is encrypted.
     * We then return the filename of the encrypted file.
     */
    virtual std::string getEncryptedPackage(Request * aPBReq);

    /** \brief Return a filename corresponding to a decrypted file
     *
     * Using the provided filename corresponding to a file with encrypted
     * content, we fork a child process to exec gpg that decrypt it and
     * then we return the filename corresponding to the plaintext file.
     */
    virtual std::string getDecryptedPackage(std::string gpgfilename);

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

  protected:
    /** \brief Add client to internal linked-list
     *
     * We want to be able to track when we receive a connection from
     * clients that have already been verified and we need to make sure
     * unverified clients never gain access privileged operations.
     *
     * All clients with their current status are added to this list for
     * future checking.
     */
    int addIfNewClient(GTBClient * client);
};
#endif  // gtbcommunication_h
