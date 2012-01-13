#ifndef MYINIT
#define MYINIT

#ifndef gnutls_h
#define gnutls_h
#include <gnutls/gnutls.h>
#endif

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <mysql/mysql.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include "cred.h"

#define HASHSTMT  "SELECT car FROM activeusers WHERE nightlyhash = ?"
//#define STMT	"SELECT nightlyhash FROM activeusers"
#define DEBUG 1
#define HASH_LEN  8
typedef struct myinit{
  int retval;
  MYSQL *myhandler;
  MYSQL_STMT *mystmthdler;
} header;

int storetempconnections (char ipaddr[], void * param, gnutls_datum_t key, gnutls_datum_t data);
int checkhash(char * ptr2hash);
int execauth(char hash);
int closeall(MYSQL_RES * result, header *stmthder);
MYSQL_STMT *mysqlinit(int *ret, MYSQL *myhandler, MYSQL_STMT *myssh);
int mysqlheader (header *stmthder);
int mysqlbindexec(int *ret, MYSQL_STMT *mystmthdler, MYSQL_BIND bind[], MYSQL_RES *myres);
#endif
