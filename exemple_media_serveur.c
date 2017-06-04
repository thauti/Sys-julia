#include <stdio.h>
#include <stdlib.h>
#include "conduct.h"
#include <unistd.h>

#include <pthread.h>

#include <sys/stat.h>
#include <fcntl.h>

int main (int argc, char *argv[])
{
    if(argc <= 1)
    {
        printf("Usage : ./executable <fichier>");
        return -1;
    }
    int src = NULL;

    char buffer[100];
    char res[5];

	struct conduct *c;

    
	

    printf("%s", argv[1]);
    src = open(argv[1], O_RDWR);
    printf("%d", src);
    fflush(0);
    c = conduct_create("cache", 100,100);
    int l;
    /*do
    {
        l = read(src, &buffer, 50);
        printf("%d", l);
        fflush(0);
        conduct_write(c, &buffer, l);
        printf("Ecriture %d octets", l);
        fflush(0);
    }while(l !=0);
    */
    while(1)
    {
        conduct_write(c, "test\n", 5);
    }

	//conduct_write(c, "test\n", 5);
    //printf("%s", buffer);
	return 0;
}
