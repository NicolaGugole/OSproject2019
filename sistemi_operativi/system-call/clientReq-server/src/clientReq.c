#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "errExit.h"
#include "request.h"
#include "response.h"
#include "clientServerVariables.h"


char clientFIFO[50]  = "clientFIFO";

int main (int argc, char *argv[]) {
    printf("Hi, I'm ClientReq program!\n");

    //1 - welcome the user and list the services you offer
    printf("\n\n================================================\n\t\tBenvenuto Utente\n================================================\n\n\n");
    printf("I servizi di sistema offerti sono tre:\n1 - Stampa\n2 - Salva\n3 - Invia\n");

    printf("\n\nPer favore inserire i dati\n\nCodice identificativo:  ");
    scanf("%s", request.userCode);
    printf("Servizio richiesto:  ");
    scanf("%s", request.service);

    //2 - create the clientFIFO before sending data to serverFIFO
    pid_t myPid = getpid();
    char needForStringPid[10];
    sprintf(needForStringPid, "%d", myPid);
    strcat(clientFIFO, needForStringPid);

    strcpy(request.clientFIFOpath,clientFIFO);



    if(mkfifo(clientFIFO, S_IWUSR | S_IRUSR) == -1)
      errExit("mkfifoClient fail");

    //3 - open and send data to server through serverFIFO
    int server = open(serverFIFO,O_WRONLY);

    if(server == -1){
      unlink(clientFIFO);
      errExit("openServerFIFO by client fail");
    }

    if(write(server, &request, sizeof(struct Request)) == -1)
      errExit("writeOnServerFIFO fail");


    //4 - receive data (key for the user) from the server through serverFIFO

    int myFIFO = open(clientFIFO, O_RDONLY);
    if(myFIFO == -1)
      errExit("openClientFIFO by client fail");


    if(read(myFIFO, &response, sizeof(struct Response)) == -1)
      errExit("readfromClientFIFO fail");

    //5 - check if there were any error in the service's Request, else show key to user
    if(response.passCode == -1) //service was not well formulated
      printf("\nThe service you requested is not in our offer, you can choose between:\n-Salva\n-Stampa\n-Invia\n\nYou typed: %s\nYou unfortunately might want to try again.\n\n", request.service);
    else if(response.passCode == -2) //DB is full
      printf("\nYou might want to try again in 5 minutes or less.\n");
    else{
      printf("\n**************************************************************\n");
      printf("\nThis is the code you requested for  service %s: %llu\n", request.service, response.passCode);
      printf("\n**************************************************************\n");
    }

    //6 - remove clientFIFO before ending, close client's side of the server
    close(server);
    unlink(clientFIFO);

    //end
    return 0;
}
