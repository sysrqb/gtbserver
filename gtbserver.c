#include "gtbserver.h"

/*
 *
 *
 * function sigchld_handler
 *
 *SIGCHLD handler: wait for all dead (zombie) processes
 *
 *
*/

void 
sigchld_handler (int s){
  while(waitpid(-1, NULL, WNOHANG) > 0);
}

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

void *
get_in_addr(struct sockaddr *sa){
  if(sa->sa_family == AF_INET){
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
/******
 *
 *
 * int authrequest()
 *
 *
*/ 


int 
authrequest (int sockfd, char *reqbufptr){
  char hash[AUTHSIZE];
  int retval;
	
  if(0 != strncmp("AUTH", reqbufptr, 5)){
    fprintf(stderr, "Not AUTH\n");
    return -3;
  }
  getclientinfo(sockfd, hash);
	/*if((retval = checkhash(*hash)) < 1){
		fprintf(stderr, "Authenication Failed\n");
		return retval;
	}*/
	

  return 0;
}
/******
 *
 *
 * void getclientinfo()
 *
 *
*/ 


void 
getclientinfo (int sockfd, char *hash){
  int numbytes;
  printf("Get hash\n");
  if((numbytes = recv(sockfd, hash, AUTHSIZE-1, 0)) == -1){
    perror("reqrecv");
    exit(1);
  }

  hash[numbytes] = '\0';
  printf("Received Hash: %s\n", hash);
}
/******
 *
 *
 * int getsocket()
 *
 *
*/ 


int 
get_socket(){
//FileDescriptors: sockfd (listen), new_fd (new connections)
  int sockfd, new_fd;

//STRUCTS
//addrinfo: hints (used to retrieve addr), servinfo (used to store retrieved addr), p (iterator)
  struct addrinfo hints, *servinfo, *p;

//int rv: return value; i: iterator; yes: optval set in setsockopt
  int rv, i = 0, yes = 1;

  memset(&hints, 0, sizeof hints);

//struct set to allow for binding and connections
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; //uses extern local IP

//Retrieve addr and socket
  if((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0){
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

//Iterate through servinfo and bind to the first available
  for(p = servinfo; p != NULL; p = p->ai_next){
    i++;
    if ((sockfd = socket(p->ai_family, p->ai_socktype, //If socket is unseccessful in creating an
      p->ai_protocol)) == -1) { //endpoint, e.g. filedescriptor, then it returns -1,
      perror("server: socket"); //so continue to the next addr
      continue;
    }

    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, //If unsuccessful in setting socket options
      sizeof(int)) == -1){ //exit on error
      perror("setsockopt");
      exit(1);
    }

    if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){ //Assign a name to an addr
    close(sockfd);	//If unsuccessful, print error and check next
    perror("server: bind");
    continue;
    }

    break; //if successful, exit loop
  }

  if(p == NULL){
    fprintf(stderr, "server: Failed to bind for unknown reason\n Iterated through loop %d times\n", i);
    freeaddrinfo(servinfo); //no longer need this struct
    return 2;
  }

  if(servinfo)
    freeaddrinfo(servinfo); //no longer need this struct
  return sockfd;
}

int 
dealwithreq (char * reqbuf, int new_fd){
  if(!strncmp(reqbuf, "CARS", REQSIZE)){
    printf("Type CAR, forking....\n");
    if(!fork()){
    printf("Forked, retreiving number of cars\n");
    if(numberofcars(new_fd, reqbuf)){
      numberofcars(new_fd, reqbuf);
    }
    printf("Done....returning to Listening state\n\n");
    return 0;
    }
  }else

  if(!strncmp(reqbuf, "AUTH", REQSIZE)){
    printf("Type AUTH, forking...\n");
    if(!fork()){
      int authret;
      printf("Forked, processing request\n");
      if(!(authret = authrequest(new_fd, reqbuf))){
        sendAOK(new_fd);
        movekey(new_fd);
      }
      else{
        sendNopes(authret, new_fd);
      }
      return 0;
    }
  } else 

  if(!strncmp(reqbuf, "DHKE", REQSIZE)){
    printf("Type DHKE, forking...\n");
    if(!fork()){
    int authret;
    printf("Forked, processing request\n");
      if(!(authret = dhkerequest(new_fd, reqbuf))){
        sendAOK(new_fd);
      }
      else{
        sendNopes(authret, new_fd);
      }
      return 0;
    }
  }
  return 0;
}

/******
 *
 *
 * init_tls_session()
 *
 *
 8*****/

 static gnutls_session_t
 init_tls_session (gnutls_priority_t * priority_cache,
                   gnutls_certificate_credentials_t * x509_cred){

  gnutls_session_t session;
  int retval;
 //Initialize session
  if ((retval = gnutls_init (&session, GNUTLS_SERVER))){
    fprintf(stderr, "gnutls_init error code: %d", retval);
    exit(1);
  }
 //Sets priority of session, i.e. specifies which crypto algos to use
  if ((retval = gnutls_priority_set (session, *priority_cache))) {
    fprintf(stderr, "gnutls_priority_set error code: %d", retval);
    exit(1);
  }
    
 //Sets the needed credentials for the specified type; an x509 cert, in this case
  if ((retval = gnutls_credentials_set (session, GNUTLS_CRD_CERTIFICATE, *x509_cred))) {
    fprintf(stderr, "gnutls_credentials_set error code:  %d", retval);
    exit (1);
  }
//Request client cert
  gnutls_certificate_server_set_request (session, GNUTLS_CERT_REQUEST);

  return session;
}


/****
 *
 *
 * generate_dh_params()
 *
 *
 *****/

static int
generate_dh_params (gnutls_dh_params_t * dh_params){
  gnutls_dh_params_init(dh_params);
  gnutls_dh_params_generate2(*dh_params, DH_BITS);

  return 0;
}

/*****
 *
 * load_certs_files()
 *
 *
 * ****/

gnutls_certificate_credentials_t *
load_cert_files (gnutls_certificate_credentials_t * x509_cred,
                 gnutls_priority_t * priority_cache, 
		 gnutls_dh_params_t * dh_params)
{
  gnutls_certificate_credentials_t x509_crd;
  const char ** err_pos; //store returned code (error or successful)
  int retval;

  stdprintf("load_cert_files: allocate creds\n");
  if ((retval = gnutls_certificate_allocate_credentials (&x509_crd))){
    //TODO
  }
  stdprintf("load_cert_files: load cert trust file\n");
  if ((retval = gnutls_certificate_set_x509_trust_file (*x509_cred, CAFILE, GNUTLS_X509_FMT_PEM))){
    //TODO
    fprintf(stderr, "gnutls_certificate_set_x509_trust_file error code: %d\n", retval);
  }
  stdprintf("load_cert_files: load CSL\n");
  if((retval = gnutls_certificate_set_x509_crl_file (*x509_cred, CRLFILE, 
                                        GNUTLS_X509_FMT_PEM)) < 1){
    //TODO
    fprintf(stderr, "gnutls_certificate_set_x509_crl_file error code: %d\n", retval);
  }
    
  stdprintf("load_cert_files: gen DH params\n");
  generate_dh_params (dh_params);
  stdprintf("load_cert_files: priority init\n");
  //Set gnutls priority string
  if((retval = gnutls_priority_init (priority_cache, GNUTLS_PRIORITY, NULL))){
    //TODO
    fprintf(stderr, "gnutls_priority_init error code");
  }
  /*if (**err_pos){
    fprintf(stderr, "gnutls_priority_init error code %d", **err_pos);
  }*/

  //gnutls_certificate_set_dh_params (*x509_cred, *dh_params);
  x509_cred = &x509_crd;

  return x509_cred;
}

int
stdprintf(char * s){
  if (DEBUG)
    printf("%s", s);
  return 0;
}

/******
 *
 *
 * int main()
 *
 *
*******/ 

int 
main(int argc, char *arv[]){
//FileDescriptors: sockfd (listen), new_fd (new connections)
  int sockfd, new_fd;
//STRUCTS
//sigaction: sa (used to make sys call to change action taken on receipt of a certain sig)
  struct sigaction sa;
//sockaddr_storage: their_addr (IP Address of the requestor)
  struct sockaddr_storage their_addr;
//socklen_t: sin_size (set size of socket)
  socklen_t sin_size;
//	char buf[MAXSIZE]
//char: s (stores the IP Addr of the incoming connection so it can be diplayed)
  char s[INET6_ADDRSTRLEN];
//char: reqbuf (request key)
  char reqbuf[REQSIZE];
//int: numbytes (received)
  int numbytes;
//GNUTLS settings
  gnutls_priority_t priority_cache;
  gnutls_certificate_credentials_t * x509_cred;
  gnutls_session_t session;
  static gnutls_dh_params_t dh_params;
	
  sockfd = get_socket();

  stdprintf("Establish Incomming Connections\n");
  if (listen(sockfd, BACKLOG) == -1){ //marks socket as passive, so it accepts incoming connections
    perror("listen: failed to mark as passive");
    exit(1);
  }

  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if(sigaction(SIGCHLD, &sa, NULL) == -1){
    perror("sigaction");
    exit(1);
  }
  
  stdprintf("Initialize gnutls\n");
//Initialize gnutls
  gnutls_global_init(); 
  x509_cred = load_cert_files (x509_cred, &priority_cache, &dh_params);
  
  printf("server: waiting for connection\n");

  sin_size = sizeof their_addr;
  while(1){
    stdprintf("Initialize TLS Session\n");
    session = init_tls_session(&priority_cache, x509_cred);

    stdprintf("Accepting Connection\n");
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    printf("accept new_fd: %d\n", new_fd);
    if (new_fd == -1){
      perror("accept");
      continue;
    }
		
    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);

    printf("Accepting Connection from: %s\n", s);

    stdprintf("Start TLS Session\n");
    gnutls_transport_set_ptr (session, (gnutls_transport_ptr_t) new_fd);
    int retval;
    if ((retval = gnutls_handshake (session))){
      fprintf(stderr, "Failed to perform handshake, error code : %s\n", gnutls_strerror(retval));
      fprintf(stderr, "Closing connection...\n");
      close(new_fd);
      gnutls_deinit(session);
      continue;
    }

    if((numbytes = recv(new_fd, &reqbuf, REQSIZE-1, 0)) == -1){
      fprintf(stderr, "Code reqrecv %s\n", strerror(errno));
      perror("");
      continue;
    }

    reqbuf[numbytes] = '\0';
    printf("Received Transmission: %s\n", reqbuf);

    if(!dealwithreq(reqbuf, new_fd))
      continue;
		

    close(new_fd);
  }

  return 0;
}
