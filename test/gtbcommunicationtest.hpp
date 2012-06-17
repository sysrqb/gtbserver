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

class GTBCommunicationTest: public GTBCommunication
{
public:

  GTBCommunicationTest(int debug) : GTBCommunication(debug){}
  
  void gtb_wrapperForCommunication(bool throws)
  {
    int sockfd, fdAccepted;
    int nRetVal = 0;
    Request request;
  
    sockfd = getSocket();
    ASSERT_GT(sockfd, 0);
    if(sockfd == 0)
      std::cout << "Error: " << errno << ", " << strerror(errno) << std::endl;
    std::cout << "Establish Incoming Connections" << std::endl;;
    std::cout << "Continuing..." << std::endl;
    initGNUTLS();
    fdAccepted = listeningForClient(sockfd);
    ASSERT_GT(fdAccepted, 0);

    try
    {
      if(throws)
      {
        EXPECT_THROW(handleConnection(fdAccepted, sockfd), BadConnectionException);
        close(fdAccepted);
        close(sockfd);
	return;
      }
      else
        ASSERT_NO_THROW(handleConnection(fdAccepted, sockfd));
    } catch (BadConnectionException &e)
    {
      close(fdAccepted);
    }
    try
    {
      receiveRequest(&request);
    } catch (BadConnectionException &e)
    {
      close(fdAccepted);
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
    close(fdAccepted);
    close(sockfd);
  }
};


#endif //gtbcommunicationtest_hpp
