
#include <stdio.h>

int main(int argc, char *argv[]) {

  int c;

  while((c = fgetc(stdin)) != EOF)
    printf("0x%x, ");

  printf("\n");

}

