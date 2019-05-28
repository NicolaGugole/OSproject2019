#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

//ricordarsi di fare il check per vedere SE LE LIBRERIE SONO GIUSTE O IN PIÙ

#include "errExit.h"
#include "request.h"
#include "response.h"


char clientFIFO[50]  = "clientFIFO";
char *serverFIFO = "serverFIFO";
struct Request request;

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
    //every client has his own FIFO. For convenience i decided to concat clientFIFO with the process' pid
    printf("ciao?");
    pid_t myPid = getpid(); //get the PID
    char needForStringPid[10]; //create the string needed for strcat
    printf("ciao %d", myPid);
    sprintf(needForStringPid, "%d", myPid); //fill needForStringPid
    printf("ciao2 %s", needForStringPid);
    strcat(clientFIFO, needForStringPid); //concat and save into clientFIFO

    strcpy(request.clientFIFOpath,clientFIFO); //this will be useful for the server to know where is the clientFIFO

    printf("\n\nprovaprova %s\n\n", request.clientFIFOpath);


    if(mkfifo(clientFIFO, S_IWUSR | S_IRUSR) == -1)
      errExit("mkfifoClient fail");

    //3 - open and send data to server through serverFIFO
    int server = open(serverFIFO,O_WRONLY);

    if(server == -1)
      errExit("openServerFIFO by client fail");

    if(write(server, &request, sizeof(request)) == -1)
      errExit("writeOnServerFIFO fail");


    //4 - receive data (key for the user) from the server through serverFIFO

    int myFIFO = open(clientFIFO, O_RDONLY);
    if(myFIFO == -1)
      errExit("openClientFIFO by client fail");

    char result[100];//DA CANCELLARE

    if(read(myFIFO, &result, sizeof(result)) == -1)//DA CANCELLARE
      errExit("readfromClientFIFO fail");//DA CANCELLARE

    printf("\nRisultato: %s\n", result); //DA CANCELLARE


    //5 - output the key to the user
    //6 - remove clientFIFO before ending, close client's side of the server

    close(server);
    unlink(clientFIFO);

    //end
    return 0;
}
