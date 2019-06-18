#include <stdlib.h>
#include <stdio.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "errExit.h"

int main (int argc, char *argv[]) {
    printf("Hi, I'm Invia program!\n");

    if(argc < 4){
      printf("\nA valid messageQueue key is needed.");
	  exit(1);
	}
    else{
      int msgqkey = atoi(argv[3]);

      if(msgqkey <= 0){ 
        printf("\nmsgget fail");
		exit(1);
	  }

      int msqid = msgget(msgqkey, S_IWUSR | S_IRUSR); 
      if(msqid == -1) //check for errors
        errExit("msgget fail");
      if(argc == 4) 
        printf("There's nothing to write on the message queue\n");
      else{ //group all the argvs in one single string
        int iterator;
        int totalSize = 0;
        for(iterator = 5; iterator < argc; iterator++) //collect the total size of the final string + the spaces between the arguments
          totalSize += strlen(argv[iterator]) + 1;
        char totalString[++totalSize]; 
        for(iterator = 5; iterator < argc; iterator++){ 
          strcat(totalString, argv[iterator]);
          strcat(totalString, " ");
        }

        if(msgsnd(msqid, totalString, totalSize, IPC_NOWAIT) == -1) 
          errExit("msgsnd fail");

        /*char newString[totalSize]; //to test the message retrieval
        if(msgrcv(msqid, newString, totalSize, 0, 0) == -1)
          errExit("msgrcv fail");

        printf("\nStringa letta da msgqueue  %s\n", newString);*/
      }
    }
    return 0;
}
