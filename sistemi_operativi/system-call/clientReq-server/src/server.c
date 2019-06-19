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
#include "clientServerVariables.h"
#include "sharedMemAndSemaphoreVariables.h"

#define FIVEMINS 300


int fakeWriting; //fake open on the write side of the serverFIFO. This assures the server won't loop on reading from the FIFO seeing there is someone ready to write.
pid_t keyManager = -1; //variable which contains server's child PID



//TO HASH THE STRING, TO CREATE THE PASSCODE
//#################################################################################################
//string to int
void stringToHash(char *string, char serviceInt[]){
  int i;
  char temp[3];
  for(i = 0; string[i] != '\0'; i++){
    sprintf(temp, "%d", string[i]);
    serviceInt = strcat(serviceInt, temp);
  }
}


//function to create the passCode
unsigned long long int hashIt(char *service, time_t timed){
  char serviceInt[18] = "";
  stringToHash(service, serviceInt);

  return ((atol(serviceInt) + timed) * 3898);
} 

//#################################################################################################




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

  printf("\n\n--------------LET'S WRAP THIS UP--------------\n\n");

  //close the fake FIFO side
  close(fakeWriting);

  //process is terminating, delete the FIFO before leaving
  printf("\nUnlinking serverFIFO...");
  unlink(serverFIFO);
  //send SIGTERM to your child, wait for its termination
  if(keyManager != -1){ //if keyManager initialized

    if(kill(keyManager, SIGTERM) == -1)
      errExit("killing keyManager failed");

    while(wait(NULL) != -1);

    printf("\nRemoving shared memory...");
    free_shared_memory(message);
    remove_shared_memory(shmid);

    printf("\nRemoving auxiliary shared memory...");
    free_shared_memory(entries);
    remove_shared_memory(aux_shmid);

    printf("\nDeleting semaphore...");
    if(semctl(semid, 0, IPC_RMID, 0) == -1)
      errExit("Semaphore removal fail");
  }
  //end
  printf("\n\nFAREWELL\nMy job here has finished. *poof*\n\n");
  exit(0);
}




void create_semaphore(){//setup the semaphore to regulate access to the sharedMemory segment
  semid = semget(SEMKEY, 1, IPC_CREAT | S_IRUSR | S_IWUSR);
  if(semid == -1)
    errExit("semget by server failed");
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




//swap last with current
void swap(int i){
  printf("\nSweeping out old stuff..\n");
  message[i] = message[*(entries) - 1];
}



//check entries and delete them if needed
void control_entries(){
  printf("\nCleaning the DB..\n");
  int i = 0;
  time_t thisTime = time(NULL);

  printf("\nShowing entries in the DB..\n");

  while(i < *entries){
    printf("\n------------\nEntry n %d\n%s\n%llu\n%ld\nDurata di permanenza nella shMem: %ld\n-----------\n", i+1, message[i].userCode, message[i].userKey, message[i].timeStamp, thisTime - message[i].timeStamp);
    if( thisTime - message[i].timeStamp >= FIVEMINS){
        swap(i);
        i--;
        (*entries)--;
    }
    i++;
  }
  printf("\nNumber of entries left: %d\n", *entries);
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


    //2 - open the serverFIFO only to read
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
    if(keyManager == 0){//child's code
      while(1){
          sleep(30);

          semOp(semid, 0, -1);

          printf("\nSTAMPA COSA C'È NELLA SHMEM:\n");
          control_entries();

          semOp(semid, 0, 1);
      }
    }
    //########################################################################################################################################################################

    //2.4 - do a fake write open to ensure the server won't overread the serverFIFO
    fakeWriting = open(serverFIFO, O_WRONLY);


    //3 - read from the serverFIFO
    do{
      if(read(server, &request, sizeof(struct Request)) == -1)
          errExit("readFromClient fail");


      printf("I am the server, I received the request for service: %s \nMade by: %s\nI am going to send to: %s\n", request.service, request.userCode, request.clientFIFOpath);

    //3.1 - check if the service is not a bad request
    int badRequest = 1;

      toLowerCase(request.service);

      if(strcmp(request.service, "stampa") == 0){
        printf("\nCreating key for STAMPA, saving message in shMem\n");
      }
      else if(strcmp(request.service, "salva") == 0){
        printf("\nCreating key for SALVA, saving message in shMem\n");
      }
      else if(strcmp(request.service, "invia") == 0){
        printf("\nCreating key for INVIA, saving message in shMem\n");
      }
      else{
        printf("\nService not recognized: %s \n", request.service);
        badRequest--;
        response.passCode = -1;
      }


      if(badRequest != 0){ //request well formulated
      //3.2 - create the code for the client
        time_t timeStamp = time(NULL);
        response.passCode = hashIt(request.service, timeStamp);


      //3.3 - save everything on the sharedMemory
        semOp(semid, 0, -1);
        sleep(1);

        if(*entries == SHMSIZE){ //shmem is full
          printf("\nMemory is full, désolé!\n");
          response.passCode = -2;
        }
        else{
          strcpy(message[*entries].userCode, request.userCode);
          message[*entries].userKey = response.passCode;
          message[*entries].timeStamp = timeStamp;
          (*entries)++;
        }

        semOp(semid, 0, 1);
      }

    //4 - send the data back to the client
      int client = open(request.clientFIFOpath, O_WRONLY);
      if(client == -1)
        errExit("openclientFIFO by server fail");

      if(write(client, &response, sizeof(struct Response)) == -1)
        errExit("writeOnClientFIFO fail");
      
      close(client);

    }while(1);
    return 0;
}
