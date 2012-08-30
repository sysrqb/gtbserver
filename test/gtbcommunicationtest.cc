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

#include "../test/gtbcommunicationtest.hpp"
#include "threading.hpp"
#include "gtest/gtest.h"

GTBCommunicationTest * globalCommTest;

 
  void GTBCommunicationTest::gtbAccept()
  {
    int sockfd;
    int nRetVal = 0;
    GTBClient * client;
  
    globalCommTest = this;
    EXPECT_NE(globalCommTest, (GTBCommunicationTest *)NULL);
    sockfd = GTBCommunication::getSocket();
    ASSERT_GT(sockfd, 0);
    if(sockfd == 0)
      std::cout << "Error: " << errno << ", " << strerror(errno) << std::endl;
  
    if(debug & 17)
    {
      std::cout << "Establish Incoming Connections" << std::endl;;
    }

    GTBCommunication::initGNUTLS();
      if(debug & 17)
        std::cout << "Returning to listening state" << std::endl;
      try
      {
        client = GTBCommunication::listeningForClient(sockfd);
      } catch (BadConnectionException &e)
      {
        FAIL();
      }

      ASSERT_GT(client->getFD(), 0);

      int queueSize = acceptedQueue.size();
      acceptedQueue.push(client);
      ASSERT_EQ(acceptedQueue.size(), queueSize + 1);

      pthread_t commthread_id = thread_ids.at(COMMTHREAD);
      std::cout << "Signaling " << commthread_id << std::endl;
      nRetVal = pthread_kill(commthread_id, 0);
      ASSERT_EQ(nRetVal, 0);
      if(nRetVal == 0)
      {
        std::cout << "Comm is alive and well." << std::endl;
        nRetVal = pthread_kill(thread_ids.at(COMMTHREAD), SIGACCEPT);
	ASSERT_EQ(nRetVal, 0);
        if(nRetVal != 0)
        {
          if(nRetVal == EINVAL)
          {
            std::cerr << "SIGACCEPT Does Not Exist On This System!";
            std::cerr << " Why doesn't this work? Exiting." << std::endl;
            exit(-1);
          }
          else if(nRetVal == ESRCH)
          {
            std::cerr << "The Comm Thread No Longer Exists??" << std::endl;
            std::cerr << "Should we try relaunching again?" << std::endl;
          }
        }
      }
      else
      {
        if(nRetVal == ESRCH)
        {
          std::cerr << "The Comm Thread No Longer Exists??"; 
          std::cerr << "We Can't Send SIGACCEPT To It! Perhaps we should relaunch.";
          std::cerr << std::endl;
        }
      }
    close(sockfd);
  }

 

  void GTBCommunicationTest::gtb_wrapperForCommunication(bool throws)
  {
    int nRetVal = 0;
    Request request;
    GTBClient * client;
    sigset_t set;
    int signum;

  sigemptyset(&set);
  sigaddset(&set, SIGACCEPT);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    if(sigwait(&set, &signum) != 0)
    {
      std::cerr << "Error while waiting for signal!" << std::endl;
      FAIL();
    }
    std::cout << "Received a signal" << std::endl;
    if(signum == SIGACCEPT)
    {
      while(!acceptedQueueIsEmpty())
      {
        client = acceptedQueuePop();
        try
        {
          if(throws)
          {
            EXPECT_THROW(handleConnection(client), BadConnectionException);
            close(client->getFD());
            delete client;
	    FAIL();
          }
          else
            ASSERT_NO_THROW(handleConnection(client));
        } catch (BadConnectionException &e)
        {
          close(client->getFD());
          delete client;
          FAIL();
        }
        try
        {
          receiveRequest(&request);
        } catch (BadConnectionException &e)
        {        
          close(client->getFD());
          delete client;
	  FAIL();
        }        
        requestQueue.push(request);
        ASSERT_EQ(requestQueue.size(), 1);
        nRetVal = pthread_kill(thread_ids.front(), SIGIO);
        if(nRetVal != 0)
        {
          if(nRetVal == EINVAL)
          {
            std::cerr << "SIGIO Does Not Exist On This System!";
            std::cerr << " We Need To Use A Different Signal. Exiting." << std::endl;
          }
          else if(nRetVal == ESRCH)
          {
            std::cerr << "The Main Thread No Longer Exists??"<< 
            std::cerr << "We Can't Send SIGIO To It! Exiting." << std::endl;
          }
        }
        sleep(1);
      }
    }
  }


