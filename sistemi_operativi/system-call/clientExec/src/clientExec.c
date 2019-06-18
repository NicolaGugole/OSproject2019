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
int hashToString(char *string){
  int i, a = 0;
  char service[6];
  for(i = 0; string[i] != '\0'; i++, a++){
    if(string[i] == '1'){
      service[a] = (char)((string[i] - 48) * 100 + (string[i + 1]- 48) * 10 + (string[i + 2]- 48));
      i += 2;
    }
    else{
      service[a] = (char)((string[i] - 48) * 10 + (string[i + 1] - 48));
      i++;
    }
  }
  //return code needed for the next switch
  if(strcmp(service, "stampa") == 0) return 1;
  else if(strcmp(service, "salva") == 0) return 2;
  else if(strcmp(service, "invia") == 0) return 3;
  return 404;
}

//decrypt function
int deHashThis(unsigned long long int key, long int timing){
  key /= 3898;
  key -= timing;

  char serviceLong[20];

  sprintf(serviceLong, "%llu", key);

  return hashToString(serviceLong);
}

//#################################################################################################



//function to transform each UpperCase in LowerCase
void toLowerCase(char *someString){
  int iterator;
  for(iterator = 0; iterator < strlen(someString); iterator++)
    if(someString[iterator] < 97)
      someString[iterator] += 32;
}


//swap last with current
void swap(int i){
  printf("\nSweeping out obsolete stuff..\n");
  message[i] = message[*(entries) - 1];
}

//function that goes through the shMem searching for the userCode and userKey
long int search_entry(char *userCode, long int userKey){
  int i;
  long needForDecrypt = 0;
  for(i = 0; i < *entries; i++){
    if(message[i].userKey == userKey){ //found a match, check if the userCode corresponds
      if(strcmp(message[i].userCode, userCode) == 0){
        needForDecrypt = message[i].timeStamp;
        swap(i);
        (*entries)--; 
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

    if(argc < 3){
      printf("\nERROR: not enough arguments. Please input in this format:\n ./clientExec userName userKey nOthers\n");
      exit(1);
    }

    semid = semget(SEMKEY, 1, S_IRUSR | S_IWUSR);
    if(semid == -1)
      errExit("semget by clientExec failed");


    shmid = shmget(SHMKEY, SHMSIZE * sizeof(struct Message), S_IRUSR | S_IWUSR);
    if(shmid == -1)
      errExit("shmget by clientExec fail");

    message = (struct Message*) get_shared_memory(shmid, 0);


    aux_shmid = shmget(AUX_SHMKEY, sizeof(int), S_IRUSR | S_IWUSR);
    if(aux_shmid == -1)
      errExit("auxiliary shmget by clientExec fail");

    entries = (int*) get_shared_memory(aux_shmid, 0);

    //read arguments sent by user
    char *userCode = argv[1];
    toLowerCase(userCode);
    unsigned long long int userKey = atol(argv[2]);



    semOp(semid, 0, -1);
    long int timing = search_entry(userCode, userKey);
    semOp(semid, 0, 1);




    free_shared_memory(message);

    free_shared_memory(entries);

    if(timing > 0) timing = deHashThis(userKey, timing); //if NOT an error then decrypt

    switch(timing){
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
