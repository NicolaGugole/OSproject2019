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
#include <time.h>

#include "errExit.h"
#include "request.h"
#include "response.h"
#include "semaphore.h"
#include "shared_memory.h"


#define FIVEMINS 300
#define SHMSIZE 1000
#define SHMKEY 1
#define SEMKEY 2
#define AUX_SHMKEY 3


//global variables
char *serverFIFO = "serverFIFO"; //pathName where to create FIFO
struct Request request; //struct type which contains client's request
struct Response response; //struct type which containts server's response
pid_t keyManager = -1; //variable which contains server's child PID
struct Message *message;  //struct pointer which contains the shared memory location
int shmid;  //variable which contains the shared memory identification
int aux_shmid; //variablewhich contains the auxiliary shared memory identification (to count entries)
int semid; //variable which contains the semaphore identification
int fakeWriting; //fake open on the write side of the serverFIFO. This assures the server won't loop on reading from the FIFO seeing there is someone ready to write.
int server; //serverFIFO file descriptor
int *entries; //number of entries in the sharedMemory

//FAQ:
//server controlla se le chiavi sono doppie?? o ci affidiamo alla bravura dell'hashing??
//ci stiamo avvicinando alla fine, sistema la shared (ora sai il modo), non fare cazzate quindi ricorda di pulire in giro!!!

//ANSWER:
//open solo in reading blocca il figlio di puttana fino a quando un client si mette in open writing


//RAGIONA SU FUNZIONE DI HASHING, IMPLEMENTA MEMORIA CONDIVISA, CREA FIGLIO (KEYMANAGER), FUNZIONE DEI 30 SECONDI(ALARM)


//strategia per shared memory: inizio 0 entries, incremento ogni volta che ne inserisco una. Ogni volta che ne tolgo una (vuol dire che è stata nella shmem per più di 300 secondi) decremento entries e copio l'ultima entry in quella che sto guardando ora, ricomincio a controllare dalla stessa posizione che ho appena controllato (perché ci sono dei nuovi dati all'interno)




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
  if(keyManager != -1){ //if == -1 means server started and ended before any clientRequest, so keyManager not initialized, neither sharedMemory nor semaphore (open only for reading, missing open for writing)
    //killing keyManager
    if(kill(keyManager, SIGTERM) == -1)
      errExit("killing keyManager failed");
    //wait for your child to end
    while(wait(NULL) != -1);
    //detach and delete the sharedMemory segment
    printf("\nRemoving shared memory...");
    free_shared_memory(message);
    remove_shared_memory(shmid);

    //detach and delete the auxiliarySharedMemory segment
    printf("\nRemoving auxiliary shared memory...");
    free_shared_memory(entries);
    remove_shared_memory(aux_shmid);

    //delete the semaphore
    printf("\nDeleting semaphore...");
    if(semctl(semid, 0, IPC_RMID, 0) == -1)
      errExit("Semaphore removal fail");
  }
  //end
  printf("\n\nFAREWELL\nMy job here has finished. *poof*\n\n");
  exit(0);
}






void create_semaphore(){//setup the semaphore to regulate access to the sharedMemory segment
  semid = semget(SEMKEY, 1, IPC_CREAT | S_IRUSR | S_IWUSR); //create the semaphore
  if(semid == -1)
    errExit("semget by server failed");
  //set the semaphore to 1
  union semun arg;
  unsigned short value[] = {1};
  arg.array = value;
  if(semctl(semid, 0, SETALL, arg) == -1)
    errExit("setting up semaphore fail");
}

//setup the auxiliary shared memory needed to share how many entries are in the main shmem
void create_auxiliary_shared_memory(){
  aux_shmid = alloc_shared_memory(AUX_SHMKEY, sizeof(int));
  entries = (int*) get_shared_memory(aux_shmid, 0);
  *entries = 0;
}




//function which swaps last element in the sharedMemory with the one i'm pointing at, then decreases the number of entries effectively deleting the initial entry
void swap(int i){
  printf("\nSweeping out old stuff..\n");
  message[i] = message[*(entries) - 1]; //swap last with current
}



//control entries, if any of them is older than 5 minutes, delete it
void control_entries(){
  printf("\nCleaning the DB..\n");
  int i = 0; //iterator
  time_t thisTime = time(NULL); //setup time with which you'll compare the timestamps

  while(i < *entries){//stampa delle entries
    printf("\n------------\nEntry n %d\n%s\n%ld\n%ld\nDurata di permanenza nella shMem: %ld\n-----------\n", i+1, message[i].userCode, message[i].userKey, message[i].timeStamp, thisTime - message[i].timeStamp);
    if( thisTime - message[i].timeStamp >= FIVEMINS){ //more than 5 minutes in the shMem
      //swap message[]'s last value in message[i], then decrease number of entries
        swap(i);
        i--; //control again this position: it has been swapped, so it has new values
        (*entries)--; //now there is one less entry
    }
    i++; //go to next message
  }
  printf("\nNumber of entries: %d\n", *entries);
}


//main code
int main (int argc, char *argv[]) {
    printf("Starting up Server program!\n");
    //0 - prepare how to end the process: only SIGTERM is accepted
    printf("\nSetting up sigHandler..");
    sigset_t signalSet, prova;
    sigfillset(&signalSet);
    sigdelset(&signalSet, SIGTERM);
    sigprocmask(SIG_SETMASK, &signalSet, &prova);

    if(signal(SIGTERM, serverSigHandler) == SIG_ERR)
      errExit("sigHandler setUp fail");

    //1 - create the serverFIFO before reading from clientFIFO
    printf("\nSetting up serverFIFO..");
    if(mkfifo(serverFIFO, S_IWUSR | S_IRUSR) == -1)
      errExit("mkfifoServer fail");


    //2 - open the serverFIFO only to write
    server = open(serverFIFO, O_RDONLY);
    if(server == -1)
      errExit("openServerFIFO by server fail");

    //2.1 - create the sharedMemory and the semaphore to regulate it
    printf("\nSetting up sharedMemory..");
    shmid = alloc_shared_memory(SHMKEY, SHMSIZE * sizeof(struct Message));

    printf("\nSetting up semaphore..");
    create_semaphore();

    //2.2 - attach the server to the shared_memory
    message = (struct Message*) get_shared_memory(shmid, 0);

    printf("\nSetting up auxiliary sharedMemory..");
    create_auxiliary_shared_memory();


    //2.3 - create KeyManager
    printf("\nCreating keyManager..");
    keyManager = fork();

    if(keyManager == -1)
      errExit("Fork fail");

    //########################################################################################################################################################################
    if(keyManager == 0){//child's code: keyManager
      printf("\nStarting up keyManager!\n");
      while(1){
          sleep(30); //every 30 seconds wake up to check if there are keys older than 5 minutes (300 seconds)!

          semOp(semid, 0, -1); //get mutual access to the sharedMemory segment

          printf("\nSTAMPA COSA C'È NELLA SHMEM:\n");
          control_entries(); //search if there is any entry to delete

          semOp(semid, 0, 1); //give access to the sharedMemory segment
      }
    }
    //########################################################################################################################################################################

    //2.4 - do a fake write open to ensure the server won't overread the serverFIFO
    fakeWriting = open(serverFIFO, O_WRONLY);


    //variable to save what service is required (needed to produce the hashcode)
    int hasher;

    //variable to save time in which an entry appears into the sharedMemory
    time_t timeStamp;


    //3 - read from the serverFIFO
    do{
      if(read(server, &request, sizeof(struct Request)) == -1)
          errExit("readFromClient fail");


      printf("I am the server, I received the request for service: %s \nMade by: %s\nI am going to send to: %s\n", request.service, request.userCode, request.clientFIFOpath);

    //3.1 - check if the service is not a bad request

    //translate everything in lowercase
      toLowerCase(request.service);

      if(strcmp(request.service, "stampa") == 0){
        printf("\nCrea chiave per STAMPA e salva in memcondivisa con tempo\n");
        hasher = 1;
      }
      else if(strcmp(request.service, "salva") == 0){
        printf("\nCrea chiave per SALVA e salva in memcondivisa con tempo\n");
        hasher = 2;
      }
      else if(strcmp(request.service, "invia") == 0){
        printf("\nCrea chiave per INVIA e salva in memcondivisa con tempo\n");
        hasher = 3;
      }
      else{
        printf("\nC'ha detto? %s \n", request.service);
        hasher = 0; //to recognize in the if below if the service was recognized or not
        response.passCode = -1; //to let the client recognize if the service was well formulated or not
      }


      if(hasher != 0){ //request was well formulated, create code for the client and save it in sharedMemory
      //3.2 - create the code for the client
        //PER ORA HASHING STUPIDO: SOMMO HASHER CON TIMESTAMP, LA DECODIFICA DOVRÀ SEMPLICEMENTE SOTTRARRE TIMESTAMP
        timeStamp = time(NULL);
        response.passCode = hasher + timeStamp;


      //3.3 - save everything on the sharedMemory
        semOp(semid, 0, -1); //get mutual access to the sharedMemory segment
        sleep(1); //delay
        //insert data(userName, hashCode, timeStamp) (if there is enough space)
        if(*entries == SHMSIZE){ //shmem is full
          printf("\nMemory is full, désolé!\n");
          response.passCode = -2; //code to let the client recognize the memory is full
        }
        else{
          strcpy(message[*entries].userCode, request.userCode);
          message[*entries].userKey = response.passCode;
          message[*entries].timeStamp = timeStamp;
          (*entries)++; //there's a new entry in the sharedMemory
        }

        semOp(semid, 0, 1); //give back access to the sharedMemory segment
      }

    //4 - send the data back to the client
      int client = open(request.clientFIFOpath, O_WRONLY); //connecting with the correct clientFIFO
      if(client == -1)
        errExit("openclientFIFO by server fail");

      if(write(client, &response, sizeof(struct Response)) == -1) //sending response
        errExit("writeOnClientFIFO fail");
      //each time the clientFIFO will be different, so close it at every iteration
      close(client);

    }while(1);
    return 0;
}
