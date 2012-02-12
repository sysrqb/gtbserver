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

#include "sqlconn.hpp"

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
  PatronInfo * apbPI;
  apbPI = i_apbPatl->add_patron();
  //i_apbPatl->add_patron();
  cout << "Getting Current Rides" << endl;
  apbPI->set_name("Test Name 1");
  apbPI->set_passangers(5);
  apbPI->set_status("Waiting");
  apbPI->set_pickup("East");
  apbPI->set_dropoff("Whitney");
  apbPI->set_timetaken("2012-02-9 00:00:00");
  apbPI->set_pid(1);
  apbPI->PrintDebugString();

  
  sql::PreparedStatement *prepStmt;
  sql::ResultSet *res;

  try
  {
    prepStmt = con->prepareStatement(GETCURRRIDES);
    prepStmt->setString(1, "2012-02-11");
    prepStmt->setString(2, "riding");

    cout << "Retrieving Rides" << endl;

    res = prepStmt->executeQuery();
    delete prepStmt;
  
    while ( res->next() ) {
      apbPI = i_apbPatl->add_patron();
      apbPI->set_name(res->getString("name"));
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
    delete prepStmt;
  }
  return 0;
}
