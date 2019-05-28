#ifndef _REQUEST_HH
#define _REQUEST_HH


//definition of request structure between clientReq and server
struct Request{	
  char userCode[100]; //string to define the user code the server will memorize
  char service[100];  //string to define the service the client requests
  char clientFIFOpath[100]; //string to let the server know where the clientFIFO is (every client has his own)
};

#endif
