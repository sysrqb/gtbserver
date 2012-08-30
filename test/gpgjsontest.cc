#include <json/json.h>
#include <curl/curl.h>
#include <iostream>
#include <cstdio>
#include "gtbexceptions.hpp"

#include "patron.pb.h"
#include "communication.pb.h"
#include "cred.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <gpgme/gpgme.h>
#include <locale.h>

#define GPGME 0

using namespace std;

struct auth_t
{
  char * ptr;
  size_t size;
} auth;

static size_t
authRequestData_callback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
  static int total = 0;
  if((size * nmemb) < 1)
    return 0;

  if(auth.size)
  {
    *(char *)ptr = *auth.ptr;
    ++auth.ptr;
    --auth.size;
    ++total;
    return 1;
  }
  cout << "Total sent: " << total << endl;
  return 0;
}

static size_t
authRequest_callback(void *buffer, size_t size,
                     size_t nmemb, void *userp)
{
  /* If valid, add authed users to DB for subsequent validation
   * Rename checkAuth to something more appropriate
   */

  cout << (char *) buffer << " " << (size * nmemb) << endl;
  return (size * nmemb);
}

string getEncryptedPackage2(Request * aPBReq)
{
  int pid, res;
  const char * filename = "authreq.json";
  string encjsonout;

  Json::ValueType type = Json::objectValue;
  Json::Value root(type);
  root["AUTH"] = aPBReq->sreqtype(); 
  root["user1"] = aPBReq->sparams(0); 
  root["user2"] = aPBReq->sparams(1); 
  root["NH"] = aPBReq->sparams(2);
  root["HASH"] = aPBReq->sparams(3);
  /*cout << "root type is object = " << (root.isObject()) << endl;
  cout << "First value = " << root["AUTH"] << endl;*/
  Json::StyledWriter writer;
  string jsonout = writer.write(root);
  /*cout << "Print via stream: " << root << endl;
  cout << "Print via writer: \n" << jsonout << endl;*/
 
  if(!(pid = fork()))
  {
    FILE * fd;
    fd = fopen(filename, "w");
    fwrite(jsonout.c_str(), 1, jsonout.size(), fd);
    fclose(fd);
    const char * const argv[] = {"/usr/bin/gpg", "--no-tty", "--batch", "--passphrase", PASSPHRASE, "-r", PUBKEYID, "-se", "authreq.json"};
    /*for(int i = 0; i < 3; ++i)
      cout << argv[i] << " ";
    cout << endl;*/
    res = execv(argv[0], (char * const *) argv);
    cout << "EXEC returned: " << res << ": " << strerror(res) << endl;
    exit(-1);
  }
  else
  {
    res = waitpid(pid, NULL, 0);
    if(res != pid)
      throw new CryptoException("Unsuccessful Return Code");
    
    FILE * fd;
    string gpgfilename(filename);
    gpgfilename.append(".gpg");
    if((fd = fopen(gpgfilename.c_str(), "r")) != NULL)
    {
      fclose(fd);
      return gpgfilename;
    }
    else
    {
      throw new CryptoException("File Does Not Exist");
    }
  }
  return NULL;
}


int
authRequest (Request * i_aPBReq)
{
  int retval;
  string filename;
  string sHash;
  char * binaryptr;
	
  if(0 != i_aPBReq->sreqtype().compare("AUTH"))
  {
    cerr << "ERROR: C: Not AUTH" << endl;
    return -3;
  }

  if (i_aPBReq->sparams_size() == 4)
  {
    FILE * fd;
    int size;
#if GPGME
    filename = getEncryptedPackage(i_aPBReq);
#else
    filename = getEncryptedPackage2(i_aPBReq);
    fd = fopen((const char *)filename.c_str(), "r");
    if(fd == NULL)
      throw new CryptoException("File Does Not Exist");
    fseek(fd , 0 , SEEK_END);
    size = ftell(fd);
    rewind (fd);

    binaryptr = (char*) malloc (sizeof(char)*(size));
    if (binaryptr == NULL)
    {
      throw new GTBException("Out of memory?");
    }
    //retval = fread (binaryptr + 5, 1, size, fd);
    retval = fread (binaryptr, 1, size, fd);
    if (retval != size)
    {
      throw new GTBException("Did not read the same number of bytes as file");
    }
    
    fclose(fd);
#endif

#if GPGME
    binaryptr = (char*) malloc (sizeof(char)*(filename.size() + 5));
#endif
//    strncpy(binaryptr + 5, filename.c_str(), filename.size());
    //sprintf(binaryptr, "auth=%s", binaryptr + 5);
    //cout << "Content: " << binaryptr << endl;
    CURL * curl;
    CURLcode res;
    struct curl_httppost *formpost=NULL;
    struct curl_httppost *lastptr=NULL;

    curl = curl_easy_init();
    if(curl == NULL)
      throw new GTBException("Could not get cURL handle");
    res = curl_easy_setopt(curl, CURLOPT_URL,
                      "http://guarddogs.uconn.edu/index.php?id=42");
    if(res != CURLE_OK)
      fprintf(stderr, "URLOPT_URL failed: %s\n",
              curl_easy_strerror(res));
    res = curl_easy_setopt(curl, CURLOPT_READFUNCTION, authRequestData_callback);
    if(res != CURLE_OK)
      fprintf(stderr, "CURLOPT_READFUNCTION failed: %s\n",
              curl_easy_strerror(res));

    //struct auth_t auth = {binaryptr, filename.size()};
    auth.size = (size + 6);
    auth.ptr = (char *)malloc(sizeof(char)*auth.size);
    sprintf(auth.ptr, "%s%s", "test=", binaryptr);

    res = curl_easy_setopt(curl, CURLOPT_READDATA, (void *)&auth);
    if(res != CURLE_OK)
      fprintf(stderr, "CURLOPT_READDATA failed: %s\n",
              curl_easy_strerror(res));
    res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, authRequest_callback);
    if(res != CURLE_OK)
      fprintf(stderr, "CURLOPT_WRITEFUNCTION failed: %s\n",
              curl_easy_strerror(res));
    res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, i_aPBReq);
    if(res != CURLE_OK)
      fprintf(stderr, "CURLOPT_WRITEDATA failed: %s\n",
              curl_easy_strerror(res));
    res = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0);
    if(res != CURLE_OK)
      fprintf(stderr, "CURLOPT_FOLLOWLOCATION failed: %s\n",
              curl_easy_strerror(res));

    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "filename",
                 CURLFORM_FILE, filename.c_str(), CURLFORM_END);

    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);


    /*res = curl_easy_setopt(curl, CURLOPT_PUT, 1L);
    if(res != CURLE_OK)
      fprintf(stderr, "CURLOPT_PUT failed: %s\n",
              curl_easy_strerror(res));
    res = curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    if(res != CURLE_OK)
      fprintf(stderr, "CURLOPT_UPLOAD failed: %s\n",
              curl_easy_strerror(res));
    res = curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
          (curl_off_t)size);
    if(res != CURLE_OK)
      fprintf(stderr, "CURLOPT_INFILESIZE_LARGE failed: %s\n",
              curl_easy_strerror(res));*/
    
    /*struct curl_slist * headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/octet-stream");
    res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    if(res != CURLE_OK)
      fprintf(stderr, "CURLOPT_HTTPHEADER failed: %s\n",
              curl_easy_strerror(res));*/
    /*res = curl_easy_setopt(curl, CURLOPT_POST, 1L);
    if(res != CURLE_OK)
      fprintf(stderr, "CURLOPT_POSTFIELDS failed: %s\n",
              curl_easy_strerror(res));*/
    /*res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, binaryptr);
    if(res != CURLE_OK)
      fprintf(stderr, "CURLOPT_POSTFIELDS failed: %s\n",
              curl_easy_strerror(res));*/
   /* res = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)(auth.size));
    if(res != CURLE_OK)
      fprintf(stderr, "CURLOPT_POSTFIELDSIZE failed: %s\n",
              curl_easy_strerror(res));
    else
      cout << "auth.size = " << auth.size << endl;*/
    /*const char * data = "auth=1";
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    if(res != CURLE_OK)
      fprintf(stderr, "CURLOPT_POSTFIELDSIZE failed: %s\n",
              curl_easy_strerror(res));*/
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#ifdef usedata
    res = curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1);
    if(res != CURLE_OK)
      fprintf(stderr, "CURLOPT_CONNECT_ONLY failed: %s\n",
              curl_easy_strerror(res));
#endif
    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
#ifdef usedata
    long socket;
    res = curl_easy_getinfo(curl, CURLINFO_LASTSOCKET, &socket);
    if(res != CURLE_OK)
      fprintf(stderr, "CURLINFO_LASTSOCKET failed: %s\n",
              curl_easy_strerror(res));
    curl_socket_t sockfd = socket;

    if(!wait_on_socket(sockfd, 0, 40000L))
    {
      printf("Error: timeout.\n");
      return 1;
    }
    size_t sent;

    char * getrequest = NULL;
    const char * getrequest1 =
              "GET /index.php?id=42 HTTP/1.0\r\nHost: guarddogs.uconn.edu\r\n";
    char * getrequest2[39];
    sprintf((char *)getrequest2, "Accept: */*\r\nContent-Length: %d\r\n", size);
    const char * getrequest3 =
              "Content-Type: application/x-www-form-urlencoded\r\n\r\n";
    size_t getreqsize = strlen(getrequest1) +
                        strlen((char *)getrequest2) + strlen(getrequest3);
    getrequest = (char *)malloc(sizeof(char)*(getreqsize));
    sprintf(getrequest, "%s%s%s", getrequest1, (char *)getrequest2, getrequest3);
    char * request = (char *)malloc(sizeof(char)*(getreqsize + size));
    sprintf(request, "%s%s", getrequest, binaryptr);
    free(binaryptr);
    res = curl_easy_send(curl, (void *)request, getreqsize + size, &sent);
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_sent() failed: %s\n",
              curl_easy_strerror(res));
    else
      cout << "Sent " << sent << " bytes." << endl;
    free(request);
    /* read the response */ 
    size_t iolen;
    for(;;)
    {
      char buf[1024];
 
      wait_on_socket(sockfd, 1, 60000L);
      res = curl_easy_recv(curl, buf, 1024, &iolen);
 
      if(CURLE_OK != res)
        break;
 
      curl_off_t nread = (curl_off_t)iolen;
 
      cout << "Received " << nread << " bytes.\n\n" << endl;
      cout << "Buffer: \n" << buf << endl;
    }
#endif

    curl_easy_cleanup(curl);
    curl_formfree(formpost);
  }
  else
  {
    cerr << "ERROR: C: Missing Paramters: Only " << i_aPBReq->sparams_size() 
        << " provided!" << endl;
    throw new GTBException("Not all fields provided");
  }

  //TODO Store IP Address, car num, etc for checking later

  return 0;
}

int main()
{
  Request areq;
  Response aresp;

  areq.set_sreqtype("AUTH");
  areq.add_sparams("abc123456");
  areq.add_sparams("dce654321");
  areq.add_sparams("def654321");
  areq.add_sparams("Hudas897a");

  system("rm authreq.json*");
  try
  {
    authRequest(&areq);
    cout << "Success" << endl;
  } catch(GTBException &e)
  {
    cout << e.what() << endl;
    cout << e.propErrorWhat() << endl;
  } catch(exception &e)
  {
    cout << e.what() << endl;
  }
  return 0;
}

gpgme_error_t
passphrase_cb(void *opaque, const char *uid_hint, const char *passphrase_info,
               int last_was_bad, int fd)
{
  return !write(fd, PASSPHRASE, strlen(PASSPHRASE));
}

/* Auxiliary function that waits on the socket. */ 
static int wait_on_socket(curl_socket_t sockfd, int for_recv, long timeout_ms)
{
  struct timeval tv;
  fd_set infd, outfd, errfd;
  int res;
 
  tv.tv_sec = timeout_ms / 1000;
  tv.tv_usec= (timeout_ms % 1000) * 1000;
 
  FD_ZERO(&infd);
  FD_ZERO(&outfd);
  FD_ZERO(&errfd);
 
  FD_SET(sockfd, &errfd); /* always check for error */ 
 
  if(for_recv)
  {
    FD_SET(sockfd, &infd);
  }
  else
  {
    FD_SET(sockfd, &outfd);
  }
 
  /* select() returns the number of signalled sockets or -1 */ 
  res = select(sockfd + 1, &infd, &outfd, &errfd, &tv);
  return res;
}
 
#if GPGME
string getEncryptedPackage(Request * aPBReq)
{
  string encjsonout;

  gpgme_ctx_t ctx;
  gpgme_error_t err;
  gpgme_data_t in, out;
  gpgme_key_t key[2] = { NULL, NULL };
  gpgme_encrypt_result_t result;
  gpgme_sign_result_t sign_result;

  Json::ValueType type = Json::objectValue;
  Json::Value root(type);
  root["AUTH"] = aPBReq->sreqtype(); 
  root["user1"] = aPBReq->sparams(0); 
  root["user2"] = aPBReq->sparams(1); 
  root["NH"] = aPBReq->sparams(2);
  root["HASH"] = aPBReq->sparams(3);
  cout << "root type is object = " << (root.isObject()) << endl;
  cout << "First value = " << root["AUTH"] << endl;
  Json::StyledWriter writer;
  string jsonout = writer.write(root);
  cout << "Print via stream: " << root << endl;
  cout << "Print via writer: \n" << jsonout << endl;
  

  gpgme_check_version(NULL);
  setlocale (LC_ALL, "");
  gpgme_set_locale (NULL, LC_CTYPE, setlocale (LC_CTYPE, NULL));
  gpgme_set_locale (NULL, LC_MESSAGES, setlocale (LC_MESSAGES, NULL));
  err = gpgme_engine_check_version (GPGME_PROTOCOL_OpenPGP);
  err = gpgme_new (&ctx);
  gpgme_set_textmode (ctx, 1);
  gpgme_set_armor (ctx, 1);

  gpgme_set_passphrase_cb (ctx, (gpgme_passphrase_cb_t) passphrase_cb, NULL);
  gpgme_passphrase_cb_t passcallbackcheck;
  gpgme_get_passphrase_cb (ctx, &passcallbackcheck, NULL);
  cout << "Callback " << (passphrase_cb == passcallbackcheck) << endl;

  err = gpgme_data_new_from_mem (&in, jsonout.c_str(), jsonout.size(), 0);
  err = gpgme_data_new (&out);
  err = gpgme_get_key (ctx, PUBKEYID,
                       &key[0], 0);
  if(err == GPG_ERR_EOF && key[0] == NULL)
    cerr << "Key not found" << endl;
  else if(err == GPG_ERR_INV_VALUE)
    cerr << "CTX or R_KEY is not a valid pointer or FPR is not a fingerprint or keyID" << endl;
  else if(err == GPG_ERR_AMBIGUOUS_NAME)
    cerr << "the key ID was not a unique specifier for a key" << endl;
  else if(err == GPG_ERR_ENOMEM)
    cerr << "at some time during the operation there was not enough memory available." << endl;
  else if(err)
    cerr << "Something else went wrong: " << gpgme_strsource(err) << ", " << gpgme_strerror(err) <<  endl;
  gpgme_get_passphrase_cb (ctx, &passcallbackcheck, NULL);
  cout << "Callback " << (passphrase_cb == passcallbackcheck) << endl;
  err = gpgme_op_encrypt_sign (ctx, key,(gpgme_encrypt_flags_t) 0, in, out);
  result = gpgme_op_encrypt_result (ctx);
  if (result->invalid_recipients)
    {
      fprintf (stderr, "Invalid recipient encountered: %s\n",
               result->invalid_recipients->fpr);
      exit (1);
    }
  sign_result = gpgme_op_sign_result (ctx);
  
  int size = gpgme_data_seek(out, 0, SEEK_END);
  char * outbuf =  (char *)malloc(sizeof(char) * size);
  gpgme_data_read(out, outbuf, size);
  encjsonout = outbuf;
  gpgme_key_unref (key[0]);
  gpgme_key_unref (key[1]);
  gpgme_data_release (in);
  gpgme_data_release (out);
  gpgme_release (ctx);
  return encjsonout;
  

 
  /*FILE * fd;
  fd = fopen(filename, "w");
  fwrite(out.c_str(), 1, out.size(), fd);
  fclose(fd);
    string args("sh -c /usr/bin/gpg --batch --passphrase "PRIVKEY " -se -r "PUBKEYID);
    args.append(" ").append(filename).append(" < /dev/null");
    cout << "Args: " << args.c_str() << endl;
    //const char * const argv[] = {(const char *)args.c_str()};
    //const char * const argv[] = {"/usr/bin/gpg", (const char *)args.c_str()};
    const char * const argv[] = {"/usr/bin/gpg", "--batch", "--passphrase", PRIVKEY, "-r", PUBKEYID, "-se", filename, " < /dev/null"};
    for(int i = 0; i < 9; ++i)
      cout << argv[i] << " ";
    cout << endl;
    res = execv(argv[0], (char * const *) argv);
    cout << "EXEC returned: " << res << ": " << strerror(res) << endl;
    //system(argv[0]);
  }
  else
  {
    res = waitpid(pid, NULL, 0);
    if(res)
      throw new CryptoException("Unsuccessful Return Code");
    
    FILE * fd;
    string gpgfilename(filename);
    gpgfilename.append(".gpg");
    if((fd = fopen(gpgfilename.c_str(), "r")) != NULL)
    {
      fclose(fd);
      return filename;
    }
    else
    {
      throw new CryptoException("File Does Not Exist");
    }
  }*/
  //return NULL;
}
#endif


