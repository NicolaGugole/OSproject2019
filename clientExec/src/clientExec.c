#include <stdlib.h>
#include <stdio.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>


#include "errExit.h"
#include "semaphore.h"
#include "shared_memory.h"



#define SHMKEY 1
#define SEMKEY 2
#define AUX_SHMKEY 3
#define SHMSIZE 1000

//global variables
struct Message *message;  //struct pointer which contains the shared memory location
int shmid;  //variable which contains the shared memory identification
int aux_shmid; //variablewhich contains the auxiliary shared memory identification (to count entries)
int semid; //variable which contains the semaphore identification
int *entries; //number of entries in the sharedMemory

//function to transform each UpperCase in LowerCase
void toLowerCase(char *someString){
  int iterator;
  for(iterator = 0; iterator < strlen(someString); iterator++)
    if(someString[iterator] < 97)
      someString[iterator] += 32;
}


//function which swaps last element in the sharedMemory with the one i'm pointing at, then decreases the number of entries effectively deleting the initial entry
void swap(int i){
  printf("\nSweeping out obsolete stuff..\n");
  message[i] = message[*(entries) - 1]; //swap last with current
}

//function that goes through the shMem searching for the userCode and userKey
long int search_entry(char *userCode, long int userKey){
  int i; //iterator
  long needForDecrypt = 0; //this will copy the service (encrypted) if the entry given by user corresponds in the shMem
  for(i = 0; i < *entries && needForDecrypt != -3; i++){
    if(message[i].userKey == userKey){ //found a match, check if the userCode corresponds
      if(strcmp(message[i].userCode, userCode) == 0){ //fully matching, take what needed for decrypting, delete entry
        needForDecrypt = message[i].timeStamp;
        swap(i); //put the matching entry to the end of the shMem
        (*entries)--; //now there is one less entry
        break; //no need for the search to continue
      }
      else{ //userKey correct, userCode not matching, ERROR [code 1] or if already found a userCode in precedent iteration [code 2]. If true then this is another type of error [code 3]
        needForDecrypt = (needForDecrypt == -2) ? -3 : -1;
      }
    }
    else if(strcmp(message[i].userCode, userCode) == 0){ //userCode matching, but userKey does not [code 2], unless already found a userKey in precedent iteration [code 1]. If true then this is another type of error [code 3]
      needForDecrypt = (needForDecrypt == -1) ? -3 : -2;
    }
  }
  return needForDecrypt;
}


int main (int argc, char *argv[]) {
    printf("Starting ClientExec..\n");

    if(argc < 3){ //not enough argvs
      printf("\nERROR: not enough arguments. Please input in this format:\n ./clientExec userName userKey nOthers\n");
      exit(0);
    }
    //open the semaphore which regulates access to sharedMemory
    semid = semget(SEMKEY, 1, S_IRUSR | S_IWUSR);
    if(semid == -1)
      errExit("semget by clientExec failed");

    //open and attach this process to the main sharedMemory
    shmid = shmget(SHMKEY, SHMSIZE * sizeof(struct Message), S_IRUSR | S_IWUSR);
    if(shmid == -1)
      errExit("shmget by clientExec fail");
    message = (struct Message*) get_shared_memory(shmid, 0);

    //apri aux_shm giusta
    aux_shmid = shmget(AUX_SHMKEY, sizeof(int), S_IRUSR | S_IWUSR);
    if(aux_shmid == -1)
      errExit("auxiliary shmget by clientExec fail");
    entries = (int*) get_shared_memory(aux_shmid, 0);

    //read arguments sent by user
    char *userCode = argv[1]; //as this process formatted input wants
    toLowerCase(userCode); //in the shMem the userCodes are all in lower case
    long int userKey = atol(argv[2]); //as this process formatted input wants



    semOp(semid, 0, -1);
    long int service = search_entry(userCode, userKey); //get the decryption material or the error code from reading the shMem
    semOp(semid, 0, 1);




    free_shared_memory(message); //detach from main shMem

    free_shared_memory(entries); //detach from auxiliary shMem

    if(service > 0) service = userKey - service; //if NOT an error then decrypt

    switch(service){ //dependently on result give service
      case 0:
        printf("\nERROR: USER NOT matching in shMem, KEY NOT matching in shMem.\n");
        break;
      case -1:
        printf("\nERROR: KEY matching in shMem, USER NOT matching in shMem.\n");
        break;
      case -2:
        printf("\nERROR: USER matching in shMem, KEY NOT matching in shMem.\n");
        break;
      case -3:
        printf("\nERROR: USER and KEY matching in shMeme, but not in the same structure.\n");
        break;
      case 1:
        printf("\nBooting 'stampa' for user %s\n", userCode);
        execv("stampa", argv); //execute "stampa" process
        perror("Execv stampa");
        break;
      case 2:
        printf("\nBooting 'salva' for user %s\n", userCode);
        execv("salva", argv); //execute "salva" process
        perror("Execv salva");
        break;
      case 3:
        printf("\nBooting 'invia' for user %s\n", userCode);
        execv("invia", argv); //execute "invia" process
        perror("Execv invia");
        break;
      default:
        printf("\nIDK!\n"); //not defined response
        break;
    }
    return 0;
}
