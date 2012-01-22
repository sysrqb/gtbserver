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

gnutls_datum_t * MySQLConn::getConnection (
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
  return retval;
}
