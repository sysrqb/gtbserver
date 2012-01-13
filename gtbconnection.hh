#ifndef gtbconnection_h
#define gtbconnection_h

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "gtbcommunication.hh"
//
//Sets port number for server to listen on
#define PORT "4680"

//Sets number of connections to allow in queue
#define BACKLOG 1

//Sets number of bytes each request should be, incomming from client
#define REQSIZE 5

//Sets number of bytes each login request should be from a client
#define LOGINSIZE 18

//Sets number of bytes each authentication request should be from a client
#define AUTHSIZE 32

//Needed because of mutual inclusion
class GTBCommunication;

class GTBConnection {

  char m_vIPAddr[INET6_ADDRSTRLEN];
  std::string m_sHash;
  
  public:

  friend class GTBCommunication;
   
/*
 *
 *
 * function *get_in_addr
 *
 *get_in_addr:
 * Get Internet Address returns the IP address of the client
 *
 *
*/
    void *getInAddr (struct sockaddr *i_sa)
    {//Extracts addr from sa

      if(i_sa->sa_family == AF_INET)
      {
        return &(((struct sockaddr_in*)i_sa)->sin_addr);
      }
      return &(((struct sockaddr_in6*)i_sa)->sin6_addr);
    }

    std::string getClientAddr(){ return std::string(m_vIPAddr); } //returns global value
    void getClientInfo (int i_sockfd);
    int getSocket ();
    int dealWithReq (std::string i_sReqBuf, int i_fdSock, GTBCommunication * i_pComm);
    int listeningForClient (
        int i_sockfd, 
	GTBCommunication * i_pComm);
};
#endif
