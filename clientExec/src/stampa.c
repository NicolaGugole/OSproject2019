#include <stdlib.h>
#include <stdio.h>

int main (int argc, char *argv[]) {
    printf("Hi, I'm Stampa program!\n");

    //objective of this program is to print on STDOUT all argvs received, if none is present it will be sad
    if(argc < 2)
      printf("No arguments found, nothing to print. sob");
    else{
      int iterator;
      for(iterator = 1; iterator < argc; iterator++)
        printf("%s\t", argv[iterator]);
    }
    printf("\n");

    return 0;
}
