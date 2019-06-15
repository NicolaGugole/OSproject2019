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
#include "sharedMemAndSemaphoreVariables.h"


//TO DEHASH THE STRING, TO CRACK THE PASSCODE AND GET THE ORIGINAL STRING

//#################################################################################################
//get the string from the passCode
unsigned long long int hashToString(char *string){
  int i, a = 0; //i is iterator, a counter needed to navigate through final string
  char service[6]; //where to place the string from the passCode
  for(i = 0; string[i] != '\0'; i++, a++){//go through passCode, decode it
    if(string[i] == '1'){ //means its a character over 100 so write using the string's next 3 chars
      service[a] = (char)((string[i] - 48) * 100 + (string[i + 1]- 48) * 10 + (string[i + 2]- 48));
      i += 2; //go to next char to decode
    }
    else{ //means its a character under 100 so write in service usinge the string's next 2 chars
      service[a] = (char)((string[i] - 48) * 10 + (string[i + 1] - 48));
      i++; //go to next char to decode
    }
  }
  //return code needed for the next switch
  if(strcmp(service, "stampa") == 0) return 1;
  else if(strcmp(service, "salva") == 0) return 2;
  else if(strcmp(service, "invia") == 0) return 3;
  return 404;
}

//decrypt function
unsigned long long int deHashThis(unsigned long long int key, long int timing){
  key /= 3898; //decrypt doing the crypting actions backwards
  key -= timing;

  char serviceLong[20]; //where to save the passCode as a string

  sprintf(serviceLong, "%llu", key);

  return hashToString(serviceLong); //recognize the service asked by user
}

//#################################################################################################



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
  for(i = 0; i < *entries; i++){
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
    unsigned long long int userKey = atol(argv[2]); //as this process formatted input wants



    semOp(semid, 0, -1);
    long int timing = search_entry(userCode, userKey); //get the decryption material or the error code from reading the shMem
    semOp(semid, 0, 1);




    free_shared_memory(message); //detach from main shMem

    free_shared_memory(entries); //detach from auxiliary shMem

    if(timing > 0) timing = deHashThis(userKey, timing); //if NOT an error then decrypt

    switch(timing){ //dependently on result give service
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
        printf("\nERROR: USER and KEY matching in shMem, but not in the same structure.\n");
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
        printf("\nDecrypting was not a success!\n"); //not defined response
        break;
    }
    return 0;
}
