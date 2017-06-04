#include <stdio.h>
#include <stdlib.h>
#include "conduct.h"
#include <unistd.h>

#include <pthread.h>



int main(void)
{
	char test[5];
	struct conduct *c;
	c = conduct_create("nomme", 5,5);

	conduct_write(c, "test\n", 5);
	

	conduct_read(c, &test, 5);	
	printf("%s", test);
	
	return 0;
}
