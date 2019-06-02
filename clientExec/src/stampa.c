#include <stdlib.h>
#include <stdio.h>

int main (int argc, char *argv[]) {
    printf("Hi, I'm Stampa program!\n");

    //objective of this program is to print on STDOUT all argvs received, if none is present it will be sad
    if(argc < 4)
      printf("No arguments found, nothing to print. sob");
    else{
      printf("\n###################################################\nPRINTING ARGUMENTS ON SCREEN..\n\n");
      int iterator;
      for(iterator = 3; iterator < argc; iterator++)
        printf("%s\t", argv[iterator]);
      printf("\n###################################################\n");
    }
    printf("\n");

    return 0;
}
