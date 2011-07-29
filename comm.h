#include "mysqlconn.h"
#include <gnutls/gnutls.h>
#include "patron.pb-c.h"

int sendAOK(int new_fd, gnutls_session_t session);
int sendNopes(int retval, int new_fd, gnutls_session_t session);
int dhkerequest(int new_fd, char *reqbufptr);
int numberofcars(gnutls_session_t session, int new_fd, char *reqbufptr);
int movekey(int new_fd, gnutls_session_t session);
