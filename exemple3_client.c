#include <stdio.h>
#include <stdlib.h>
#include "conduct.h"
#include <unistd.h>

#include <pthread.h>



int main (int argc, char *argv[])
{

    FILE* src = NULL;

    char buffer[5];

	struct conduct *c;

    
	c = conduct_open("cache");

    int l;
   
    while(1){
    conduct_read(c, &buffer, 5);
    printf("%s", c->buffer);
    }
	return 0;
}
