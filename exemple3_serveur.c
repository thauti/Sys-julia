#include <stdio.h>
#include <stdlib.h>
#include "conduct.h"
#include <unistd.h>

#include <pthread.h>

#include <sys/stat.h>
#include <fcntl.h>

int main (int argc, char *argv[])
{


    char buffer[100];
    char res[5];

	struct conduct *c;

    c = conduct_create("cache", 100,100);
    int l;
  
    while(1)
    {
        conduct_write(c, "test\n", 5);
    }

	
	return 0;
}
