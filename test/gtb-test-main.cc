
#include <pthread.h>
#include <csignal>
#include <unistd.h>
#include <vector>
#include <cerrno>
#include <iostream>

#include <gtest/gtest.h>

using namespace std;

/** \brief Wrapper used to allow pthread_create call method 
 *
 *
 * \sa listenForClient
 */
void cpp_forCommunicationMainTest(void * in)
{
  pthread_t threadId;
  int nRetVal = 0;
  
  threadId = (pthread_t &) *in;

  sleep(2);
  cout << "Sending SIGCHLD to " << threadId << endl;
  nRetVal = pthread_kill(threadId, SIGCHLD);
  sleep(2);
  cout << "Sending SIGHUP" << endl;
  nRetVal = pthread_kill(threadId, SIGHUP);
  sleep(2);
  cout << "Sending SIGURS1" << endl;
  nRetVal = pthread_kill(threadId, SIGUSR1);
  sleep(2);
  cout << "Sending SIGIO" << endl;
  nRetVal = pthread_kill(threadId, SIGIO);
  if(nRetVal != 0)
  {
    if(nRetVal == EINVAL)
    {
      cerr << "SIGIO Does Not Exist On This System!";
      cerr << " We Need To Use A Different Signal. Exiting." << endl;
    }
    else if(nRetVal == ESRCH)
    {
      cerr << "The Main Thread No Longer Exists??"<< 
      cerr << "We Can't Send SIGIO To It! Exiting." << endl;
    }
  }
}


/** \brief Wrapper used to allow pthread_create call method 
 *
 * \sa cpp_listenForClient
 */
extern "C"
void *(for_communication_main_test)(void * instance)
{
  cpp_forCommunicationMainTest(instance);
  return NULL;
}


TEST(TreadingTest, ActionTakenWhenSignalRecieved)
{
  /* This code is copied, and modified slightly, from that in
   * main. The test shows the validity of this event-driven
   * server design...hopefully.
   */

  pthread_attr_t attr;
  int nRetVal(0);
  pthread_t thread_id(0), thread_id2(0);
  int signum(0);
  sigset_t set;
  vector<pthread_t> thread_ids;

  sigemptyset(&set);
  sigaddset(&set, SIGIO);
  sigaddset(&set, SIGCHLD);
  sigaddset(&set, SIGHUP);
  sigaddset(&set, SIGUSR1);
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
  thread_ids.push_back(thread_id);
  nRetVal = pthread_create(&thread_id2, &attr, &for_communication_main_test, (void *) &thread_id);
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
  thread_ids.push_back(thread_id2);

  /*sigemptyset(&set);
  sigaddset(&set, SIGIO);
  */
  cout << "Thread 1: " << thread_ids.at(0) << ", Thread 2: " 
      << thread_ids.at(1) << endl;
  for(;;)
  {
    if(sigwait(&set, &signum) != 0)
    {
      cerr << "Error while waiting for signal!" << endl;
      continue;
    }
    if(signum == SIGIO)
    {
      cout << "Received SIGIO Signal!" << endl;
      break;
    }
    else
    {
      cout << "Received Signal " << signum << ", Expected " << SIGIO << endl;
    }
  }
}
