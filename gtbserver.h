#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "mysqlconn.h"
#include "comm.h"

#include "gtbconnection.hh"
#include "gtbcommunication.hh"

#ifdef __cplusplus
extern "C" {
#endif
//Sets port number for server to listen on
#define PORT "4680"

//Sets number of connections to allow in queue
#define BACKLOG 1

//Sets number of bytes each request should be, incomming from client
#define REQSIZE 5

//Sets number of bytes each login request should be from a client
#define LOGINSIZE 18

//Sets number of bytes each authentication request should be from a client
#define AUTHSIZE 32

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

//Insert session into temp DB
#define TEMPSESHSTMT "INSERT INTO tempconnection"\
    "(ipaddr, sessionkey, sessiondata) values (?, ?, ?)"

//Handlers and Wrappers
void sigchld_handler (int s);
int stdprintf (char * s);

//GNUTLS related functions
static gnutls_session_t init_tls_session (
           gnutls_priority_t * priority_cache,
           gnutls_certificate_credentials_t * x509_cred);
static int gen_dh_params (gnutls_dh_params_t * dh_params);
void load_cert_files (
                 gnutls_certificate_credentials_t * x509_cred,
                 gnutls_priority_t * priority_cache,
		 gnutls_dh_params_t * dh_params);
static int generate_dh_params (gnutls_dh_params_t * dh_params);

//Client related functions
void *get_in_addr (struct sockaddr *sa);
int authrequest (int sockfd, char *reqbufptr);
void getclientinfo (int sockfd, char *hash);
int get_socket ();
int dealwithreq (char * reqbuf, int new_fd, gnutls_session_t session);
int listening_for_client (int sockfd, 
       gnutls_priority_t * priority_cache,
       gnutls_certificate_credentials_t * x509_cred,
       gnutls_session_t session);

/*Return Values:
-4: Error MySQL query prep
-3: reqbuf did not equal the value it was supposed to
-2: the hash query returned more than one result (should not be possible) - invalid login
-1: No hash was povided by client
 0: completed successfully
*/
#ifdef __cplusplus
}
#endif
