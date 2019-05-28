#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "errExit.h"

int main (int argc, char *argv[]) {
    printf("Hi, I'm Salva program!\n");

    if(argc < 2)
      errExit("Not enough arguments, I need a path and values to write!");

    //open the file which path is argv[1]
    int fd = open(argv[1], O_RDWR | O_CREAT, S_IRWXU);
    if(fd == -1)
      errExit("openFile fail");

    if(argc == 2)
      printf("There's nothing to write in the file!\n");
    else{
      int iterator;
      for(iterator = 2; iterator < argc; iterator++){
        if(write(fd, argv[iterator], sizeof(char) * strlen(argv[iterator])) == -1)
          errExit("write fail");
        if(write(fd, " ", sizeof(char)) == -1)
          errExit("write fail");
      }
    }


    return 0;
}
