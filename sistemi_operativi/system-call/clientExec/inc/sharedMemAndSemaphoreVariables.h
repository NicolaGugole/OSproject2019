#ifndef _SHAREDMEMANDSEMAPHOREVARIABLES_HH
#define _SHAREDMEMANDSEMAPHOREVARIABLES_HH


#define SHMSIZE 1000
#define SHMKEY 1
#define SEMKEY 2
#define AUX_SHMKEY 3


int *entries; //number of entries in the sharedMemory
struct Message *message;  //struct pointer which contains the shared memory location
int shmid;  //variable which contains the shared memory identification
int aux_shmid; //variablewhich contains the auxiliary shared memory identification (to count entries)
int semid; //variable which contains the semaphore identification

#endif
