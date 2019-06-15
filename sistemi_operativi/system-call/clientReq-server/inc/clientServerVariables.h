#ifndef _CLIENTSERVERVARIABLES_HH
#define _CLIENTSERVERVARIABLES_HH

char *serverFIFO = "serverFIFO"; //pathName where to create FIFO
struct Request request; //struct type which contains client's request
struct Response response; //struct type which containts server's response
int server; //serverFIFO file descriptor

#endif
