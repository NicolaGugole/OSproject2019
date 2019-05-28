#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "errExit.h"
#include "request.h"
#include "response.h"
#include "semaphore.h"
#include "shared_memory.h"

#define RESPONSERROR "The service you requested is not in our offer, you can choose between Salva, Stampa or Invia, you typed: "


//global variables
char *serverFIFO = "serverFIFO"; //pathName where to create FIFO
struct Request request; //struct type which contains client's request
struct Response response; //struct type which containts server's response
pid_t keyManager = -1; //variable which contains server's child PID
struct Message *message;  //struct pointer which contains the shared memory location
int shmid;  //variable which contains the shared memory identification
int semid; //variable which contains the semaphore identification
int fakeWriting; //fake open on the write side of the serverFIFO. This assures the server won't loop on reading from the FIFO seeing there is someone ready to write.
int server;

//FAQ:
//perche l'hey guys del keymanager spunta solo dopo che il server riceve un qualcosa dal client?? è bloccato da qualche parte prima della fork??
//server controlla se le chiavi sono doppie?? o ci affidiamo alla bravura dell'hashing??
//sleep mi ridà possesso dopo i secondi di sospensione?? oppure meglio utilizzare alarm() e poi creare un sigHandler per il SIGALARM??


//RAGIONA SU FUNZIONE DI HASHING, IMPLEMENTA MEMORIA CONDIVISA, CREA FIGLIO (KEYMANAGER), FUNZIONE DEI 30 SECONDI(ALARM)







void toLowerCase(char *someString){//function to transform each UpperCase in LowerCase
  int iterator;
  for(iterator = 0; iterator < strlen(someString); iterator++)
    if(someString[iterator] < 97)
      someString[iterator] += 32;
}









void serverSigHandler(int sig){//function to end the server process when SIGTERM is issued

  //child's end
  if(keyManager == 0){
    printf("\nI'm KeyManager, adieu.\n");
    exit(0);
  }

  //close the FIFO fake side
  close(fakeWriting);

  //process is terminating, delete the FIFO before leaving
  printf("\nUnlinking serverFIFO...");
  unlink(serverFIFO);
  //send SIGTERM to your child, wait for its termination
  printf("\nKilling keyManager...");
  if(kill(keyManager, SIGTERM) == -1)
    errExit("killing keyManager failed");
  //wait for your child to end
  while(wait(NULL) != -1);
  //detach and delete the sharedMemory segment
  printf("\nRemoving shared memory...");
  free_shared_memory(message);
  remove_shared_memory(shmid);
  //delete the semaphore
  printf("\nDeleting semaphore...");
  if(semctl(semid, 0, IPC_RMID, 0) == -1)
    errExit("Semaphore removal fail");
  //end
  printf("\n\nFAREWELL\nMy job here has finished. *poof*\n\n");
  exit(0);
}






void create_semaphore(){//setup the semaphore to regulate access to the sharedMemory segment
  semid = semget(IPC_PRIVATE, 1, S_IRUSR | S_IWUSR); //create the semaphore
  if(semid == -1)
    errExit("semget failed");
  //set the semaphore to 1
  union semun arg;
  unsigned short value[] = {1};
  arg.array = value;
  if(semctl(semid, 0, SETALL, arg) == -1)
    errExit("setting up semaphore fail");
}







//main code
int main (int argc, char *argv[]) {
    printf("Starting up Server program!\n");
    //0 - prepare how to end the process: only SIGTERM is accepted
    sigset_t signalSet, prova;
    sigfillset(&signalSet);
    sigdelset(&signalSet, SIGTERM);
    sigprocmask(SIG_SETMASK, &signalSet, &prova);

    if(signal(SIGTERM, serverSigHandler) == SIG_ERR)
      errExit("sigHandler setUp fail");

    //1 - create the serverFIFO before reading from clientFIFO
    if(mkfifo(serverFIFO, S_IWUSR | S_IRUSR) == -1)
      errExit("mkfifoServer fail");


    //2 - open the serverFIFO only to write
    server = open(serverFIFO, O_RDONLY);
    if(server == -1)
      errExit("openServerFIFO by server fail");

    //2.1 - create the sharedMemory and the semaphore to regulate it
    shmid = alloc_shared_memory(IPC_PRIVATE, 1000 * sizeof(struct Message));

    create_semaphore();

    //2.2 - attach the server to the shared_memory
    message = (struct Message*) get_shared_memory(shmid, 0);

    //2.3 - create KeyManager
    keyManager = fork();

    if(keyManager == -1)
      errExit("Fork fail");

    if(keyManager == 0){//child's code: keyManager |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
      printf("\nStarting up keyManager!\n");
      while(1){
          sleep(5); //every 30 seconds i wake up to check if there are keys older than 5 minutes (300 seconds)!
          semOp(semid, 0, -1); //get mutual access to the sharedMemory segment
          printf("\nHEY GUYS\n"); //here you'll have to check the time each element has spent in the shared memory ||||||||||||||||||||||||||||||||||||||||||
          semOp(semid, 0, 1); //give access to the sharedMemory segment
      }
    }

    //2.4 - do a fake write open to ensure the server won't overread the serverFIFO
    fakeWriting = open(serverFIFO, O_WRONLY);


    //3 - read from the serverFIFO
    do{
      if(read(server, &request, sizeof(request)) == -1)
          errExit("readFromClient fail");


      printf("I am the server, I received the request for service: %s \nMade by: %s\nI am going to send to: %s\n", request.service, request.userCode, request.clientFIFOpath);

    //3.1 - check if the service is not a bad request

    //translate everything in lowercase
      toLowerCase(request.service);

      if(strcmp(request.service, "stampa") == 0)
        printf("\nCrea chiave per STAMPA e salva in memcondivisa con tempo\n");
      else if(strcmp(request.service, "salva") == 0)
        printf("\nCrea chiave per SALVA e salva in memcondivisa con tempo\n");
      else if(strcmp(request.service, "invia") == 0)
        printf("\nCrea chiave per INVIA e salva in memcondivisa con tempo\n");
      else printf("\nC'ha detto? %s \n", request.service);

    //3.2 - create the code for the client

      //crea dipendentemente dal servizio richiesto.

    //3.3 - save everything on the sharedMemory
    semOp(semid, 0, -1); //get mutual access to the sharedMemory segment
    //insert data
    semOp(semid, 0, 1); //give back access to the sharedMemory segment


    //4 - send the data back to the client

      int client = open(request.clientFIFOpath, O_WRONLY); //da cambiare, va rispedita response con il codice generato, questo è solo CODICE PROVA
      if(client == -1)
        errExit("openclientFIFO by server fail");

      if(write(client, &(request.clientFIFOpath), sizeof(request.clientFIFOpath)) == -1)
        errExit("writeOnClientFIFO fail");
      //each time the clientFIFO will be different, so close it at every iteration
      close(client);

    }while(1);
    return 0;
}
