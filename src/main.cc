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
#include "patron.pb.h"
#include <signal.h>

using namespace std;

/** \brief Wrapper used to allow pthread_create call method 
 *
 *
 * \sa listenForClient
 */
void cpp_forCommunication(void * instance)
{
  GTBCommunication * aComm = (GTBCommunication *) instance;
  aComm->gtb_wrapperForCommunication();
}

/** \brief Wrapper used to allow pthread_create call method 
 *
 * \sa cpp_listenForClient
 */
extern "C"
void *(for_communication)(void * instance)
{
  cpp_forCommunication(instance);
  //return 0;
}

int 
main(int argc, char *argv[])
{
//FileDescriptors: sockfd (listen), new_fd (new connections)
  pthread_attr_t attr;
  int nRetVal(0);
  pthread_t thread_id(0);
  int signum(0);
  int debug = 0;
  sigset_t set;
  Request aPBReq;

  if(argc == 2)
    debug = (int) *argv[1];
  
  cout << "Starting gtbserver...." << endl;
  GTBCommunication aComm(debug);
  sigemptyset(&set);
  sigaddset(&set, SIGIO);
  nRetVal = pthread_sigmask(SIG_BLOCK, &set, NULL);
  if(nRetVal != 0)
  {
    cerr << "Failed to sigio mask!" << endl;
    throw new exception();
  }
    
  nRetVal = pthread_attr_init(&attr);
  if(nRetVal != 0)
  {
    cerr << "Failed to Initialize pthread_attr_t!" << endl;
    throw new exception();
  }
      
  thread_id = pthread_self();
  aComm.threadid_push_back(thread_id);
  nRetVal = pthread_create(&thread_id, &attr, &for_communication, (void *) &aComm);
  if(nRetVal != 0)
  {
    cerr << "Failed to Create pthread!" << endl;
    throw new exception();
  }

  nRetVal = pthread_attr_destroy(&attr);
  if(nRetVal != 0)
  {
     cerr << "Failed to destroy pthread_attr_t!" << endl;
     throw new exception();
  }
  aComm.threadid_push_back(thread_id);

  for(;;)
  {
    if(sigwait(&set, &signum) != 0)
    {
      cerr << "Error while waiting for signal!" << endl;
      continue;
    }
    if(signum == SIGIO)
    {
      while(!aComm.requestQueueIsEmpty())
      {
        aComm.requestQueuePop(&aPBReq);
	aComm.dealWithReq(aPBReq);
      }
    }
  }
}
