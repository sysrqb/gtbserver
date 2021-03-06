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
#include <gtest/gtest.h>
#include "patron.pb.h"
#include "communication.pb.h"

TEST(GETCURRENT, CallsGetCurrEmptyVector)
{
  MySQLConn * aMySQLConn;
  aMySQLConn = new MySQLConn();
  Response apbRes;

  PatronList * apbPL = apbRes.mutable_plpatronlist();
  long npladdr = (long)apbPL;
  std::vector<int> old;

  EXPECT_NE(0, npladdr);
  EXPECT_TRUE(apbRes.has_plpatronlist());
  EXPECT_EQ(0, aMySQLConn->getCurr(5, apbPL, old));

}

TEST(GETCURRENT, CallsGetCurrNonEmptyVector)
{
  MySQLConn * aMySQLConn;
  aMySQLConn = new MySQLConn();
  Response apbRes;

  PatronList * apbPL = apbRes.mutable_plpatronlist();
  long npladdr = (long)apbPL;
  std::vector<int> old;

  EXPECT_NE(0, npladdr);
  EXPECT_TRUE(apbRes.has_plpatronlist());
  EXPECT_EQ(0, aMySQLConn->getCurr(5, apbPL, old));
}

TEST(GETCURRENT, GetNewPatron)
{
  Response apbRes;

  PatronList * apbPL = apbRes.mutable_plpatronlist();
  EXPECT_EQ(0, apbPL->patron_size());
  apbPL->clear_patron();
  EXPECT_EQ(0, apbPL->patron_size());
  PatronInfo * apbPI = apbPL->add_patron();
  EXPECT_NE(0, (long)apbPI);
}

TEST(GETCURRENT, SendResponse)
{
  MySQLConn * aMySQLConn;
  aMySQLConn = new MySQLConn();
  Response apbRes;
  std::vector<int> old;

  PatronList * apbPL = apbRes.mutable_plpatronlist();
  EXPECT_EQ(0, aMySQLConn->getCurr(5, apbPL, old));

  Response apbRes2;
  apbRes2.set_sresvalue("CURR");
  apbRes2.set_nrespid(12345);
  apbRes2.PrintDebugString();
  apbRes2.CheckInitialized();

  apbRes.set_sresvalue("CURR");
  apbRes.set_nrespid(12345);
  apbRes.CheckInitialized();
  std::string spbRes = "";
  EXPECT_TRUE(apbRes.SerializeToString(&spbRes));
}
