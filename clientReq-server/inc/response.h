#ifndef _RESPONSE_HH
#define _RESPONSE_HH

//struct to comprehend everything the server wants to send back to the user's client
struct Response{
	int passCode; //code the user will use in combo with its username to access the service, if equal to 0 it means the service requested is not well formulated
	char *errorString; //string to use in case passCode = 0
};

#endif
