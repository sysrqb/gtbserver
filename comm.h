#include "mysqlconn.h"
#include <gnutls/gnutls.h>

int sendAOK(int new_fd);
int sendNopes(int retval, int new_fd);
int dhkerequest(int new_fd, char *reqbufptr);
int numberofcars(int new_fd, char *reqbufptr);
int movekey(int new_fd);
