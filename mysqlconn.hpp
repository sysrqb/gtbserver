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

#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

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
//Insert session into temp DB
#define TEMPSESHSTMT "INSERT INTO tempconnection"\
    "(ipaddr, sessionkey, sessiondata) values (?, ?, ?)"
typedef struct myinit{
  int retval;
  MYSQL *myhandler;
  MYSQL_STMT *mystmthdler;
} header;

int storetempconnections_c (char ipaddr[], void * param, gnutls_datum_t key, gnutls_datum_t data);
int checkhash_c(char * ptr2hash);
int execauth_c(char hash);
int closeall_c(MYSQL_RES * result, header *stmthder);
MYSQL_STMT *mysqlinit_c(int *ret, MYSQL *myhandler, MYSQL_STMT *myssh);
int mysqlheader_c (header *stmthder);
int mysqlbindexec_c(int *ret, MYSQL_STMT *mystmthdler, MYSQL_BIND bind[], MYSQL_RES *myres);

int storetempconnections (char ipaddr[], void * param, gnutls_datum_t key, gnutls_datum_t data);
int checkhash(char * ptr2hash);
int execauth(char hash);
int closeall(MYSQL_RES * result, header *stmthder);
MYSQL_STMT *mysqlinit(int *ret, MYSQL *myhandler, MYSQL_STMT *myssh);
int mysqlheader (header *stmthder);
int mysqlbindexec(int *ret, MYSQL_STMT *mystmthdler, MYSQL_BIND bind[], MYSQL_RES *myres);
#endif
