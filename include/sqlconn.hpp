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

#ifndef commpb
#define commnpb
#include "communication.pb.h"
#endif

#define HASHSTMT  "SELECT car FROM activeusers WHERE nightlyhash = ?"
//#define STMT	"SELECT nightlyhash FROM activeusers"
#define DEBUG 1
#define PS_SETSESSSTMT "INSERT INTO sessionconnection"\
    "(ptr, sessionkey, sessiondata) values (?, ?, ?)"
#define PS_GETSESSSTMT "SELECT sessionkey FROM sessionconnection"\
    " WHERE ptr=? AND sessiondata=?"
#define PS_GETCURRRIDES "SELECT * FROM saferide.rides, saferide.ridetimes " \
    "WHERE LEFT(ridetimes.ridecreate, 10) = ? AND status=? AND " \
    " ridetimes.pid = patron.pid AND car=? ORDER BY car ASC"
#define PS_GETPATRONINFO "SELECT * FROM saferide.patron, saferide.riderimes " \
    "WHERE num = ? AND ridetimes.pid = patron.pid"

#define PS_GETLOCATIONID "SELECT lid FROM locations WHERE value = ?" \
    " OR name = ?"

#define PS_SETUPDTRIDES "UPDATE saferide.rides SET name=?, cell=?, riders=?,"  \
    " status=?, pickup=?, dropoff=?, timetaken=?, timedone=? WHERE num=? AND " \
    " car=?"
#define PS_RIDEADDPATRON "INSERT INTO patron (name,cell,riders,pickup,dropoff," \
    "clothes,notes,status) VALUES (?,?,?,?,?,?,?,?)"

#define PS_ADDLOCATION "INSERT INTO locations (name, value) VALUES (?, ?)"

#define PS_RIDEADDTIME "INSERT INTO ridetimes (ridecreated, pid) VALUES (?, ?)"

#define PS_GETLASTINSERTID "SELECT LAST_INSERT_ID()"

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

/** \brief Handle connection to MySQL/MariaDB server
 * 
 * Used to authenicate clients, store, retrieve, and update
 * patron information.
 */
class MySQLConn {
    /** \brief MySQL driver used to connect to server */
    sql::Driver *driver;

    /** \brief MySQL connection used to execute all queries and updates */
    sql::Connection *con;

  public:
    /** \brief Constructor. */
    MySQLConn() {
      driver = get_driver_instance();
      //con = driver->connect( GTBHOST, GTBUSER, GTBPASS);
    }

    /** \brief Deconstructor. */
    ~MySQLConn() {
      //delete con;
      //free(driver); //deconstructor is protected
    }

    /** \brief Store client session info used for session resume.
     *
     * By enabling session resume, subsequent sessions are established 
     * quicker.
     *
     * \param ptr pointer
     * \param session_id gnutls_datum_t holding session id
     * \param session_data gnutls_datum_t holding session data
     */
    int storeConnection (
	void * ptr, 
	gnutls_datum_t session_id, 
	gnutls_datum_t session_data);

    /** \brief Retrieve client session info used for session resume.
     *
     * Using ptr and session_id find and return the session data.
     *
     * \param ptr pointer
     * \param session_id gnutls_datum_t holding session id
     * \return session_data gnutls_datum_t holding session data
     */
    gnutls_datum_t getConnection (
        void * ptr, 
        gnutls_datum_t session_id);

    /** \brief Stores client info that is used for authentication.
     *
     * After the client is authenticated, we store its IP Address
     * and car number in the database. The next time the client 
     * connects, we can verify this client is the same as the previous 
     * one.
     *
     * \todo Implement
     *
     * \param ipaddr The IP Address of the client
     * \param car The car's number
     */
    int storeCarInfo(
        std::string ipaddr, 
	int car);

    /** \brief Authenticate client.
     *
     * Authenticate the client and bind the NetId to the car number.
     * This also ensures that no other car registers as the same 
     * car number.
     *
     * \param i_snetid The NetID of the ride along in the car
     * \param i_sauth The nightly hash
     * \param i_scarnum The car number as a string
     * \return 0 if successful and >0 if not
     * \todo Implement. This only verifies that the three fields 
     * are not empty strings. It must actually query the database
     * and add the information to the DB for later verification.
     */
    int checkAuth(std::string i_snetid, 
        std::string i_sauth, 
	std::string i_scarnum);
    
    /** \brief Unimplemented. I don't remember what this was supposed
     * to do.
     */
    int execAuth(std::string hash);
    
    /** \brief Return all rides currently assigned to the client.
     *
     * Query the database for all cars currently assigned to the client
     * with the given car number. For each car returned, check if the
     * patron id is contained in old which contains all rides the client
     * already knows about. We don't want to send excessive information, so 
     * we skip all patron information that would duplicate that which is 
     * alreadi in the client's database. When these patrons have been 
     * excluded, populate the PatronList with all unique patrons.
     *
     * \param carnum The car number of the client
     * \param i_apbPatl The PatronList that will contain the patron 
     * information
     * \param old The Vector that contains all the patron ids that 
     * the client already has
     * \return If successful, 0, otherwise -1.
     * \todo If pid is in old but fields have been modified after last
     * update, resend patron or only resend modified fields.
     * \todo Check that all rides in old are still assigned to carnum
     * and inform client to remove patron from database if the patron has
     * been reassigned.
     * \todo Handle SQLException better
     */
    int getCurr(int carnum, PatronList * i_apbPatl, std::vector<int>);
    
    /** \brief Update the information for each patron.
     *
     * Update the patron info for each patron contained in i_aPBReq.
     * If an error is occurred while updating a patron, add the patron id
     * and error message to map returned. 
     *
     * \param carnum The car number of the client updating the patrons
     * \param i_apbPatl Unused
     * \param i_aPBReq The request from the client containing the patrons
     * to be updated
     * \return a std::map containing the pid and error returned when the 
     * update of that patron was attempted.
     * \todo Only update fields based on modified fields
     * \todo Handle conflict resolution
     * \todo Handle SQLException better
     */
    std::map<int, std::string> setUpdt(int carnum, PatronList * i_apbPatl, Request * i_aPBReq);

    /** \brief Add a new patron to the database.
     *
     * Insert the patron information for every new patron contained in 
     * i_aPBReq into the database.
     *
     * \param carnum The car number of the client adding new patrons
     * \param i_aPBReq The request from the client containing the 
     * new patrons to be inserted into the database.
     * \return The first pid of the new patrons that were inserted.
     * Actually it is the last pid minus the number of patrons added,
     * which should be the first pid assuming pid is autoincremented.
     * \todo Handle SQLException better
     */
    int addPatron(int carnum, Request * i_aPBReq);

    /** \brief Insert a new location into the database
     *
     * Generates SHA256 hash of location that will be used as the unique value 
     * of the location. The plaintext of the location and first 8 bytes of the
     * hash are then inserted into the database. The lid/unique location id
     * are then returned.
     *
     * \param loc The location as a string
     * \sa getSHA256Hash
     * \sa getLastInsertId
     * \todo Handle SQLException better
     */
    int addLocation(std::string loc);

    /** \brief Returns the information in the database for the patron assigned
     * to the pid/unique id pid.
     *
     * Populate the PatronList i_apbPatl with the information returned by the
     * query to the database. 
     * 
     * \param pid The patron's unique id
     * \param i_apbPatl The PatronList to be populated
     * \todo Handle SQLException better
     * \throw PatronException If patron is not found in database
     */
    int getPatronInfo(int pid, PatronList * i_apbPatl);

    /** \brief Returns the lid of loc.
     *
     * If loc is in the database then the query will return the proper lid.
     * If loc is NOT in the database then loc will be added to the database
     * and that insert id is returned.
     *
     * \param loc Location as a string
     * \return LID / unique location ID of location
     * \sa addLocation
     * \sa getLastInsertID
     * \todo Handle SQLException better
     */
    int getLocationID(std::string loc);

    /** \brief Return SHA256 hash of input string sdata.
     *
     * \param sdata Input string
     * \param size The size of sdata in bytes
     */
    std::string getSHA256Hash(std::string sdata, int size);
    
    /** \brief Returns the last insert ID */
    int getLastInsertId();
};
#endif

