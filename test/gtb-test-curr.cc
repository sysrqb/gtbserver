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

#include "gtbcommunication.hpp"
#include "sqlconn.hpp"
#include "gtest/gtest.h"
#include "patron.pb.h"
#include "communication.pb.h"
#include <iostream>

TEST(GETCURRENT, CallsgetCurr)
{
  MySQLConn * aMySQLConn;
  Response * apbRes;

  PatronList * apbPL = apbRes->mutable_plpatronlist();
  apbPL = new PatronList();
  long npladdr = (long)apbPL;
  std::cout << "Address = " << npladdr << std::endl;

  EXPECT_NE(0, npladdr);
  EXPECT_TRUE(apbRes->has_plpatronlist());
  EXPECT_EQ(0, aMySQLConn->getCurr(5, apbPL));
}

TEST(GETCURRENT, GetNewPatron)
{
  Response * apbRes;

  PatronList * apbPL = apbRes->mutable_plpatronlist();
  EXPECT_NE(0, apbPL->patron_size());
  apbPL = new PatronList();
  apbPL->clear_patron();
  EXPECT_EQ(0, apbPL->patron_size());
  PatronInfo * apbPI = apbPL->add_patron();
  EXPECT_NE(0, (long)apbPI);
}

TEST(GETCURRENT, SendResponse)
{
  MySQLConn * aMySQLConn;
  GTBCommunication * aGtBComm;
  Response * apbRes;

  PatronList * apbPL = apbRes->mutable_plpatronlist();
  apbPL = new PatronList();
  EXPECT_EQ(0, aMySQLConn->getCurr(5, apbPL));

  Response apbRes2;
  apbRes2.set_sresvalue("CURR");
  apbRes2.set_nrespid(12345);
  apbRes2.PrintDebugString();
  apbRes2.CheckInitialized();

  apbRes->set_sresvalue("CURR");
  apbRes->set_nrespid(12345);
  apbRes->PrintDebugString();
  apbRes->CheckInitialized();
  std::string spbRes = "";
  EXPECT_TRUE(apbRes->SerializeToString(&spbRes));
}
