#include <stdlib.h>
#include <stdio.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "errExit.h"

int main (int argc, char *argv[]) {
    printf("Hi, I'm Invia program!\n");

    //check if there are enough args ( > 1)
    if(argc < 4)
      errExit("A valid messageQueue key is needed.");
    else{
      int msgqkey = atoi(argv[3]);

      if(msgqkey <= 0)  //argv[3] was a string, which translates to 0 creating a msgque where it shouldn't exist
        errExit("msgget fail");

      int msqid = msgget(msgqkey, S_IWUSR | S_IRUSR); //get access to the msgqueue
      if(msqid == -1) //check for errors
        errExit("msgget fail");
      if(argc == 4) //check if there is something to write
        printf("There's nothing to write on the message queue\n");
      else{ //group all the argvs in one single string
        int iterator;
        int totalSize = 0;
        for(iterator = 5; iterator < argc; iterator++) //collect the total size of the final string + the spaces between the arguments
          totalSize += strlen(argv[iterator]) + 1;
        char totalString[++totalSize]; //allocate the total size into one string
        for(iterator = 5; iterator < argc; iterator++){ //create the string comprehensive of every argv
          strcat(totalString, argv[iterator]);
          strcat(totalString, " ");
        }

        if(msgsnd(msqid, totalString, totalSize, IPC_NOWAIT) == -1) //send the total string on the messageQueue
          errExit("msgsnd fail");

        /*char newString[totalSize]; //to test the message retrieval
        if(msgrcv(msqid, newString, totalSize, 0, 0) == -1)
          errExit("msgrcv fail");

        printf("\nStringa letta da msgqueue  %s\n", newString);*/
      }
    }
    return 0;
}
