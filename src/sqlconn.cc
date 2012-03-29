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

#include <time.h>
#include "sqlconn.hpp"
#include "gtbexceptions.hpp"
#include <map>

using namespace std;

#if __cplusplus
extern "C" {
#endif
int c_store_connection (
    void * ptr,
    gnutls_datum_t session_id, 
    gnutls_datum_t session_data)
{
  return cpp_store_connection (ptr, session_id, session_data);
}
    
int cpp_store_connection(
    void * ptr,
    gnutls_datum_t session_id, 
    gnutls_datum_t session_data)
{
  MySQLConn mysqlconn;
  return mysqlconn.storeConnection(ptr, session_id, session_data);
}

gnutls_datum_t c_retrieve_connection (
    void * ptr,
    gnutls_datum_t session_id)
{
  return cpp_retrieve_connection (ptr, session_id);
}
    
gnutls_datum_t cpp_retrieve_connection(
    void * ptr,
    gnutls_datum_t session_id)
{
  MySQLConn mysqlconn;
  return mysqlconn.getConnection(ptr, session_id);
}

#if __cplusplus
}
#endif
  

int MySQLConn::storeConnection (
    void * ptr,
    gnutls_datum_t session_id, 
    gnutls_datum_t session_data)
{
 
  sql::PreparedStatement *prepStmt;

  try
  {
    prepStmt = con->prepareStatement(SETSESSSTMT);
    if ( ptr == NULL)
      prepStmt->setNull(1, sql::DataType::UNKNOWN);
    else
      prepStmt->setBlob(1, (basic_istream<char> *)ptr);
    prepStmt->setBlob(2, (basic_istream<char> *)&session_id.data);
    prepStmt->setBlob(3, (basic_istream<char> *)&session_data.data);

    cout << "Storing Session" << endl;

    prepStmt->execute();
  }
  catch (sql::SQLException &e)
  {
    cerr << "ERROR: SQLException in " << __FILE__;
    cerr << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
    cerr << "ERROR: " << e.what();
    cerr << " (MySQL error code: " << e.getErrorCode();
    cerr << ", SQLState: " << e.getSQLState() << " )" << endl;
    delete prepStmt;
    return -1;
  }
  delete prepStmt;
  return 0;
}

gnutls_datum_t MySQLConn::getConnection (
    void * ptr, 
    gnutls_datum_t session_id)
{
  
  sql::PreparedStatement *prepStmt;
  sql::ResultSet *res;
  gnutls_datum_t *retval;
  retval = (gnutls_datum_t *)gnutls_malloc(sizeof(gnutls_datum_t));
  retval->data = NULL; 
  retval->size = 0;

  try
  {
    prepStmt = con->prepareStatement(GETSESSSTMT);
    if (ptr == NULL)
      prepStmt->setNull(1, sql::DataType::UNKNOWN);
    else
      prepStmt->setBlob(1, (basic_istream<char> *)ptr);
    prepStmt->setBlob(2, (basic_istream<char> *)&session_id);

    cout << "Retrieving Session" << endl;

    res = prepStmt->executeQuery();
    delete prepStmt;
  
    while ( res->next() ) {
      retval->data = (unsigned char *)res->getBlob("sessionkey");
      retval->size = sizeof res->getBlob("sessionkey");
    }
    
    delete res;
  }
  catch (sql::SQLException &e)
  {
    cerr << "ERROR: SQLException in " << __FILE__;
    cerr << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
    cerr << "ERROR: " << e.what();
    cerr << " (MySQL error code: " << e.getErrorCode();
    cerr << ", SQLState: " << e.getSQLState() << " )" << endl;
    delete prepStmt;
  }
  return *retval;
}

int MySQLConn::checkAuth(std::string i_snetid, std::string i_sauth, std::string i_scarnum)
{
  int retval (0);
  //TODO
  cout << "NetID: " << i_snetid << endl;
  cout << "AUTH: " << i_sauth << endl;
  cout << "Car Number: " << i_scarnum << endl;

  if (i_snetid.compare("") == 0)
    retval = 1;
  if (i_sauth.compare("") == 0)
    retval = retval | 2;
  if (i_scarnum.compare("") == 0)
    retval = retval | 4;

  return retval;

}

int MySQLConn::getCurr(int carnum, PatronList * i_apbPatl, int old[])
{
  time_t atime;
  struct tm * gtm;
  char date[10];

  atime = time (NULL);
  gtm = gmtime (&atime);
  strftime (date, 11, "%Y-%m-%d", gtm);

  PatronInfo * apbPI;
  cout << "Getting Current Rides" << endl;
  sql::PreparedStatement *prepStmt;
  sql::ResultSet *res;

  try
  {
    prepStmt = con->prepareStatement(GETCURRRIDES);
    prepStmt->setDateTime(1, date);
    prepStmt->setString(2, "waiting");
    prepStmt->setInt(3, carnum);

    cout << "Retrieving Rides" << endl;

    res = prepStmt->executeQuery();
    delete prepStmt;
  
    while ( res->next() ) {
      apbPI = i_apbPatl->add_patron();
      apbPI->set_name(res->getString("name"));
      apbPI->set_phone(res->getString("cell"));
      apbPI->set_passangers(res->getInt("riders"));
      apbPI->set_status(res->getString("status"));
      apbPI->set_pickup(res->getString("pickup"));
      apbPI->set_dropoff(res->getString("dropoff"));
      apbPI->set_timetaken(res->getString("timetaken"));
      apbPI->set_pid(res->getInt("num"));
    }
    
    delete res;
  }
  catch (sql::SQLException &e)
  {
    cerr << "ERROR: SQLException in " << __FILE__;
    cerr << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
    cerr << "ERROR: " << e.what();
    cerr << " (MySQL error code: " << e.getErrorCode();
    cerr << ", SQLState: " << e.getSQLState() << " )" << endl;
    return -1;
  }
  return 0;
}

map<int, string> MySQLConn::setUpdt(int carnum, PatronList * i_apbPatl, Request * i_aPBReq)
{
  PatronInfo * apbPI;
  apbPI = i_apbPatl->add_patron();
  sql::PreparedStatement *prepStmt;
  cout << "Setting Updates" << endl;
  
  map<int, string> patronErrors;
  int i = 0;
  int nRows;

  for (; i<i_aPBReq->plpatronlist().patron_size(); i++)
  {
    try
    {
      prepStmt = con->prepareStatement(SETUPDTRIDES);
      prepStmt->setString(1, i_aPBReq->plpatronlist().patron(i).name());
      prepStmt->setString(2, i_aPBReq->plpatronlist().patron(i).phone());
      prepStmt->setInt(3, i_aPBReq->plpatronlist().patron(i).passangers());
      prepStmt->setString(4, i_aPBReq->plpatronlist().patron(i).status());
      prepStmt->setString(5, i_aPBReq->plpatronlist().patron(i).pickup());
      prepStmt->setString(6, i_aPBReq->plpatronlist().patron(i).dropoff());
      prepStmt->setString(7, i_aPBReq->plpatronlist().patron(i).timetaken());
      prepStmt->setString(8, i_aPBReq->plpatronlist().patron(i).timedone());
      prepStmt->setInt(9, i_aPBReq->plpatronlist().patron(i).pid());
      prepStmt->setInt(10, carnum);
      nRows = prepStmt->executeUpdate();
      if ( nRows == 0 )  // If entry was not updated
      {
	try
	{
          getPatronInfo(i_aPBReq->plpatronlist().patron(i).pid(), i_apbPatl);  // Add current patron info to response buf
	} catch (PatronException &e)
	{
	  patronErrors.insert( pair<int, string> (i_aPBReq->plpatronlist().patron(i).pid(), e.what()) );
	}
      }
    }
    catch (sql::SQLException &e)
    {
      cerr << "ERROR: SQLException in " << __FILE__;
      cerr << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
      cerr << "ERROR: " << e.what();
      cerr << " (MySQL error code: " << e.getErrorCode();
      cerr << ", SQLState: " << e.getSQLState() << " )" << endl;
    }
    cout << "Updated Rides" << endl;
  }
  delete prepStmt;
  return patronErrors;;
}

int MySQLConn::getPatronInfo(int pid, PatronList * i_apbPatl)
{
  PatronInfo * apbPI;
  sql::PreparedStatement *prepStmt;
  sql::ResultSet *res;

  try
  {
    prepStmt = con->prepareStatement(GETPATRONINFO);
    prepStmt->setInt(1, pid);

    cout << "Retrieving Patron" << endl;

    res = prepStmt->executeQuery();
    delete prepStmt;
  
    while ( res->next() ) {
      apbPI = i_apbPatl->add_patron();
      apbPI->set_name(res->getString("name"));
      apbPI->set_phone(res->getString("phone"));
      apbPI->set_passangers(res->getInt("riders"));
      apbPI->set_status(res->getString("status"));
      apbPI->set_pickup(res->getString("pickup"));
      apbPI->set_dropoff(res->getString("dropoff"));
      apbPI->set_timetaken(res->getString("timetaken"));
      apbPI->set_pid(res->getInt("num"));
    }
    if (!(res->rowsCount()))
    {
      throw PatronException("Patron not found!");
    }
    delete res;
  }
  catch (sql::SQLException &e)
  {
    cerr << "ERROR: SQLException in " << __FILE__;
    cerr << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
    cerr << "ERROR: " << e.what();
    cerr << " (MySQL error code: " << e.getErrorCode();
    cerr << ", SQLState: " << e.getSQLState() << " )" << endl;
    throw PatronException();
  }
  return 0;
}
