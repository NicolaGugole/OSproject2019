#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "errExit.h"

//objective of this program is to save on a file (given by argv[3]) every argv[i] with i > 3
int main (int argc, char *argv[]) {
    printf("Hi, I'm Salva program!\n");

    if(argc < 4)
      errExit("Not enough arguments, I need a path and values to write!");

    //open the file which path is argv[1]
    int fd = open(argv[3], O_RDWR | O_CREAT, S_IRWXU);
    if(fd == -1)
      errExit("openFile fail");

    printf("\n###################################################\nPRINTING ARGUMENTS ON FILE..\n\n");

    if(argc == 4)
      printf("There's nothing to write in the file!\n");
    else{
      int iterator;
      for(iterator = 4; iterator < argc; iterator++){
        if(write(fd, argv[iterator], sizeof(char) * strlen(argv[iterator])) == -1)
          errExit("write fail");
        if(write(fd, " ", sizeof(char)) == -1)
          errExit("write fail");
      }
    }

    sleep(1); //delay

    printf("\nArguments succesfully saved in file '%s'\n###################################################\n", argv[3]);


    return 0;
}
