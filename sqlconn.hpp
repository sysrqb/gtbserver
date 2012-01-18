#ifndef SQLINIT
#define SQLINIT

#ifndef gnutls_h
#define gnutls_h
#include <gnutls/gnutls.h>
#endif

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/datatype.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

#include <string.h>
#include "cred.h"
#include <stdlib.h>

#define HASHSTMT  "SELECT car FROM activeusers WHERE nightlyhash = ?"
//#define STMT	"SELECT nightlyhash FROM activeusers"
#define DEBUG 1
#define SETSESSSTMT "INSERT INTO sessionconnection"\
    "(ptr, sessionkey, sessiondata) values (?, ?, ?)"
#define GETSESSSTMT "SELECT sessionkey FROM sessionconnection"\
    " WHERE ptr=? AND sessiondata=?"
#define GETHOST(proto, host, port) proto host ":" port


class MySQLConn {
    sql::Driver *driver;
    sql::Connection *con;

  public:
    MySQLConn() {
      driver = get_driver_instance();
      con = driver->connect( HOST, USER, PASS);
    }

    ~MySQLConn() {
      delete con;
      free(driver); //deconstructor is protected
    }

    int storeConnection (
	void * param, 
	gnutls_datum_t session_id, 
	gnutls_datum_t session_data);
    gnutls_datum_t *getConnection (
        void * ptr, 
        gnutls_datum_t session_id);
    int storeCarInfo(
        std::string ipaddr, 
	int car);
    int checkHash(std::string ptr2hash);
    int execAuth(std::string hash);
};
#endif

