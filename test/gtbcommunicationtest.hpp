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

#ifndef gtbcommunicationtest_hpp
#define gtbcommunicationtest_hpp

#include "gtbcommunication.hpp"

class GTBCommunicationTest: public GTBCommunication
{
public:

  GTBCommunicationTest(int debug) : GTBCommunication(debug)
  {
  /*  if(m_pX509Cred != NULL)
    {
      delete m_pX509Cred;
      m_pX509Cred = (gnutls_certificate_credentials_t * ) 
          operator new (sizeof (gnutls_certificate_credentials_t));
    }
    else
      m_pX509Cred = (gnutls_certificate_credentials_t * ) 
          operator new (sizeof (gnutls_certificate_credentials_t));
    */
  }
  void gtbAccept();
  void gtb_wrapperForCommunication(bool throws);
  int addIfNewClientTest(GTBClient * client) {
    return GTBCommunication::addIfNewClient(client);
  }
  int addClientToList_BypassChecks(GTBClient * client) {
    clientsList.push_back(client);
    return clientsList.size() - 1;
  }
};


#endif //gtbcommunicationtest_hpp
