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


#include "threading.hpp"

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

/** \brief Wrapper used to allow pthread_create call watchdog 
 *
 *
 */
void cpp_forWatchDog(void * instance)
{
  GTBCommunication * aComm = (GTBCommunication *) instance;
  aComm->launchWatchDog();
}

/** \brief Wrapper used to allow pthread_create call watchdog 
 *
 */
extern "C"
void *(for_watchdog)(void * instance)
{
  cpp_forWatchDog(instance);
  return NULL;
}


extern pthread_t createAcceptThread(GTBCommunication * aComm,
                                    pthread_attr_t * attr)
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

extern pthread_t createCommThread(GTBCommunication * aComm, 
                                         pthread_attr_t * attr)
{
  int nRetVal(0);
  pthread_t thread_id(0);

  nRetVal = pthread_create(&thread_id, attr, &for_communication, (void *) aComm);
  if(nRetVal != 0)
  {
    cerr << "Failed to Create pthread!" << std::endl;
    throw new exception();
  }

  return thread_id;
}

extern pthread_t createWDogThread(GTBCommunication * aComm, pthread_attr_t * attr)
{
  int nRetVal(0);
  pthread_t thread_id(0);

  nRetVal = pthread_create(&thread_id, attr, &for_watchdog, (void *) aComm);
  if(nRetVal != 0)
  {
    cerr << "Failed to Create pthread!" << endl;
    throw new exception();
  }

  return thread_id;
}
