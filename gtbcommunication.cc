#include <iostream>
#include "gtbcommunication.hh"

using namespace std;

int 
GTBCommunication::DHKERequest(string i_sReqBuf)
{
	const char * pReqBuf;

	pReqBuf = i_sReqBuf.c_str();
	//TODO
	return 0;
}

int 
GTBCommunication::sendAOK()
{
	char cOk = 0;
	int nNumBytes;

	if((nNumBytes = gnutls_record_send (m_aSession, &cOk, sizeof cOk)) == -1){
		cerr << "Error on send for OK: " << strerror(errno) << endl;
	}
	cout << "Number of bytes sent: " << nNumBytes << endl;
	
	return nNumBytes;
}

int 
GTBCommunication::sendNopes(int i_nRetVal)
{
	int nNumBytes;

	if ((nNumBytes = gnutls_record_send (m_aSession, &i_nRetVal, sizeof i_nRetVal)) == -1){
		cerr << "Error on send for retval: " << strerror(errno) << endl;
	}
	cout << "Number of bytes sent: " << nNumBytes << endl;
	
	return nNumBytes;
}

int 
GTBCommunication::sendNumberOfCars(string i_sReqBuf)
{
	char nNumOfCars = 7;
	int nNumBytes;
	const char * pReqBuf;

	pReqBuf = i_sReqBuf.c_str();
	cout << "Function: numberOfCars" << endl;
	if(0 != strncmp("CARS", pReqBuf, 5)){
		cerr << "Not CARS" << endl;
		return -3;
	}
	cout << "Sending packet" << endl;
	if ((nNumBytes = gnutls_record_send (m_aSession, &nNumOfCars, sizeof nNumOfCars)) == -1){
		cerr << "Error on send for cars: " << strerror(errno) << endl;
	}
	cout << "Number of bytes sent: " << nNumBytes << endl;
	return nNumBytes;
}

int 
GTBCommunication::moveKey() 
{
	//TODO
	return 0;
}

int 
GTBCommunication::authRequest (string i_sReqBuf, int i_fdSock, GTBConnection * i_pConn)
{
  int retval;
  string sHash;
	
  if(0 != i_sReqBuf.compare("AUTH"))
  {
    cerr << "Not AUTH" << endl;
    return -3;
  }
  i_pConn->getClientInfo(i_fdSock);
	/*if((retval = checkhash(*hash)) < 1){
		fprintf(stderr, "Authenication Failed\n");
		return retval;
	}*/
	

  return 0;
}



/******
 *
 *
 * init_tls_session()
 *
 *
 8*****/

void
GTBCommunication::initTLSSession ()
{

  int retval;
 //Initialize session
  if ((retval = gnutls_init (&m_aSession, GNUTLS_SERVER)))
  {
    fprintf(stderr, "gnutls_init error code: %d", retval);
    exit(1);
  }
 //Sets priority of session, i.e. specifies which crypto algos to use
  if ((retval = gnutls_priority_set (m_aSession, *m_pPriorityCache))) 
  {
    fprintf(stderr, "gnutls_priority_set error code: %d", retval);
    exit(1);
  }
    
 //Sets the needed credentials for the specified type; an x509 cert, in this case
  if ((retval = gnutls_credentials_set (m_aSession, GNUTLS_CRD_CERTIFICATE, *m_pX509Cred))) 
  {
    fprintf(stderr, "gnutls_credentials_set error code:  %d", retval);
    exit (1);
  }
//Request client cert
  gnutls_certificate_server_set_request (m_aSession, GNUTLS_CERT_REQUEST);
}


/****
 *
 *
 * generate_dh_params()
 *
 *
 *****/

static int
generate_dh_params (gnutls_dh_params_t * dh_params)
{
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

void
load_cert_files (gnutls_certificate_credentials_t * x509_cred,
                 gnutls_priority_t * priority_cache, 
		 gnutls_dh_params_t * dh_params)
{
  const char ** err_pos; //store returned code (error or successful)
  int retval;

  cout << "load_cert_files: allocate creds" << endl;
  if ((retval = gnutls_certificate_allocate_credentials (x509_cred)))
  {
    //TODO
    cout <<"load_cert_files: gnutls_certificate_allocate_credentials: false" << endl;
  }
  cout << "load_cert_files: load cert trust file" << endl;
  if ((retval = gnutls_certificate_set_x509_trust_file (*x509_cred, CAFILE, GNUTLS_X509_FMT_PEM)))
  {
    //TODO
    if ( retval > 0 )
     cout << "gnutls_certificate_set_x509_trust_file: certs loaded: " << retval << endl;
    else
      cerr << "gnutls_certificate_set_x509_trust_file error code: " << strerror(retval) << endl;
  }
  cout << "load_cert_files: load CSL" << endl;
  if((retval = gnutls_certificate_set_x509_crl_file (*x509_cred, CRLFILE, 
                                        GNUTLS_X509_FMT_PEM)) < 1)
  {
    //TODO
    fprintf(stderr, "gnutls_certificate_set_x509_crl_file error code: %d\n", retval);
  }
    
  cout << "load_cert_files: gen DH params" << endl;
  generate_dh_params (dh_params);
  cout << "load_cert_files: priority init" << endl;
  //Set gnutls priority string
  if((retval = gnutls_priority_init (priority_cache, GNUTLS_PRIORITY, NULL)))
  {
    //TODO
    cerr << "gnutls_priority_init error code" << endl;
  }
  /*if (**err_pos){
    fprintf(stderr, "gnutls_priority_init error code %d", **err_pos);
  }*/

  gnutls_certificate_set_dh_params (*x509_cred, *dh_params);
  //*x509_cred = lx509_cred;
  //*priority_cache = lpriority_cache;
}

