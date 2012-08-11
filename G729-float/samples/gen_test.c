
#include <stdio.h>

int main(int argc, char *argv[]) {

	int i;

	FILE *f = fopen("test1.g729", "r");
	for(i = 0; i < 10; i++)
		printf("0x%x, ", fgetc(f));
	fclose(f);

  	return 0;

}

