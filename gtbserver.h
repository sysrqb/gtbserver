#include <sys/wait.h>

#include "mysqlconn.h"

#include "gtbconnection.hh"
#include "gtbcommunication.hh"

//Insert session into temp DB
#define TEMPSESHSTMT "INSERT INTO tempconnection"\
    "(ipaddr, sessionkey, sessiondata) values (?, ?, ?)"

//Handlers and Wrappers
extern "C" void sigchld_handler (int s);

/*Return Values:
-4: Error MySQL query prep
-3: reqbuf did not equal the value it was supposed to
-2: the hash query returned more than one result (should not be possible) - invalid login
-1: No hash was povided by client
 0: completed successfully
*/
