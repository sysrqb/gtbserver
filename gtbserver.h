#define THREADS 0

extern "C" {

  void sigchld_handler (int s);
  void *initIncomingCon(void * i_pfdSock);
}

void *initGNUTLS( void * ptr);

/*Return Values:
-4: Error MySQL query prep
-3: reqbuf did not equal the value it was supposed to
-2: the hash query returned more than one result (should not be possible) - invalid login
-1: No hash was povided by client
 0: completed successfully
*/
