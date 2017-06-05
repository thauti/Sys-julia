#include <stdio.h>
#include <stdlib.h>
#include "conduct.h"
#include <unistd.h>

#include <pthread.h>

void* ecrire(void* arg)
{
	struct conduct* c = (struct conduct*) arg;
	char* str = malloc(40);
	int i = 0;
	for(i=0;i<2;i++){
		conduct_write(c,str,40);
	}
	printf("Thread Ecrivain ok \n");
}
void* lire(void* arg)
{
	struct conduct* c = (struct conduct*) arg;
	char* str = malloc(40);
	conduct_read(c,str,40);
	printf("Thread Lecteur ok \n");
	fflush(0);
}

int main(void)
{
	pthread_t tw;
	pthread_t tr, tr2;
	struct conduct *c;
	c = conduct_create("aze", 400,400);

	pthread_create(&tw, NULL, ecrire, (void *) c);
	pthread_create(&tr, NULL, lire, (void*) c);
	pthread_create(&tr2, NULL, lire, (void*) c);


	pthread_join(tw, 0);

	pthread_join(tr,0 );
	pthread_join(tr2,0 );


	return 0;
}
