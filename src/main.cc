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

/** \brief Wrapper used to allow pthread_create call gtbAccept method
 *
 */
void gtbAccept_cpp(void * instance)
{
  GTBCommunication * aComm = (GTBCommunication *) instance;
  aComm->gtbAccept();
}

/** \brief Wrapper used to allow pthread_create call method 
 *
 */
extern "C"
void *(gtbAccept_c)(void * instance)
{
  gtbAccept_cpp(instance);
  return NULL;
}


/** \brief Wrapper used to allow pthread_create call method 
 *
 *
 */
void cpp_forCommunication(void * instance)
{
  GTBCommunication * aComm = (GTBCommunication *) instance;
  aComm->gtb_wrapperForCommunication();
}

/** \brief Wrapper used to allow pthread_create call method 
 *
 */
extern "C"
void *(for_communication)(void * instance)
{
  cpp_forCommunication(instance);
  return NULL;
}

inline pthread_t createAcceptThread(GTBCommunication * aComm, pthread_attr_t * attr)
{
  int nRetVal(0);
  pthread_t thread_id(0);
     
  nRetVal = pthread_create(&thread_id, attr, &gtbAccept_c, (void *) aComm);
  if(nRetVal != 0)
  {
    cerr << "Failed to Create pthread!" << endl;
    throw new exception();
  }

  return thread_id;
}

inline pthread_t createCommThread(GTBCommunication * aComm, pthread_attr_t * attr)
{
  int nRetVal(0);
  pthread_t thread_id(0);

  nRetVal = pthread_create(&thread_id, attr, &for_communication, (void *) aComm);
  if(nRetVal != 0)
  {
    cerr << "Failed to Create pthread!" << endl;
    throw new exception();
  }

  return thread_id;
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

  /* Prepare to create threads */
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

  /* Create worker threads */
  thread_id = pthread_self();
  aComm.threadid_push_back(thread_id);

  thread_id = createAcceptThread(&aComm, &attr);
  aComm.threadid_push_back(thread_id);

  thread_id = createCommThread(&aComm, &attr);
  aComm.threadid_push_back(thread_id);

  /* Get rid of what we don't need */
  nRetVal = pthread_attr_destroy(&attr);
  if(nRetVal != 0)
  {
     cerr << "Failed to destroy pthread_attr_t!" << endl;
     throw new exception();
  }

  if(debug & 18)
  {
    cout << "Number of threads spawned: " << aComm.threadid_getSize() << endl;
    int i;;
    for(i = 0; i< aComm.threadid_getSize(); ++i)
    {
      switch(i)
      {
        case MAINTHREAD:
          cout << "Main threadID: " << aComm.getThreadIDAt(i) << endl;
          break;
        case ACPTTHREAD:
          cout << "Accept threadID: " << aComm.getThreadIDAt(i) << endl;
          break;
        case CONNTHREAD:
          cout << "Communication threadID: " << aComm.getThreadIDAt(i) << endl;
          break;
        default:
          break;
      }
    }
  }
	  
  /* Wait for requests to process */
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
