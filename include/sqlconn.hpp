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

#ifndef SQLINIT
#define SQLINIT

//#ifndef gnutls_h
//#define gnutls_h
#include <gnutls/gnutls.h>
//#endif

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

#ifndef patronpb
#define patronpb
#include "patron.pb.h"
#endif

#define HASHSTMT  "SELECT car FROM activeusers WHERE nightlyhash = ?"
//#define STMT	"SELECT nightlyhash FROM activeusers"
#define DEBUG 1
#define SETSESSSTMT "INSERT INTO sessionconnection"\
    "(ptr, sessionkey, sessiondata) values (?, ?, ?)"
#define GETSESSSTMT "SELECT sessionkey FROM sessionconnection"\
    " WHERE ptr=? AND sessiondata=?"
#define GETHOST(proto, host, port) proto host ":" port

#if __cplusplus
extern "C" {
#endif

extern int c_store_connection (
    void * param,
    gnutls_datum_t session_id,
    gnutls_datum_t session_data);

extern int cpp_store_connection (
    void * ptr,
    gnutls_datum_t session_id, 
    gnutls_datum_t session_data);

extern gnutls_datum_t c_retrieve_connection (
    void * , 
    gnutls_datum_t);

extern gnutls_datum_t cpp_retrieve_connection (
    void * ptr,
    gnutls_datum_t session_id);

#if __cplusplus
}
#endif

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
    gnutls_datum_t getConnection (
        void * ptr, 
        gnutls_datum_t session_id);
    int storeCarInfo(
        std::string ipaddr, 
	int car);
    int checkAuth(std::string i_snetid, std::string i_sauth, std::string i_scarnum);
    int execAuth(std::string hash);
    int getCurr(int carnum, PatronList * i_apbpatl, int[]);
};
#endif

