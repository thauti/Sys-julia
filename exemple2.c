#include <stdio.h>
#include <stdlib.h>
#include "conduct.h"
#include <unistd.h>

#include <pthread.h>

void* ecrire(void* arg)
{
	struct conduct* c = (struct conduct*) arg;
	char* str = malloc(40);

	char* dest = malloc(500);
	conduct_write(c,str,400);
}
void* lire(void* arg)
{
	struct conduct* c = (struct conduct*) arg;
	char* str = malloc(40);
	char* dest = malloc(500);
	conduct_read(c,str,40);
	printf("Thread ok");
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

	printf("c->c %d", c->c);

	pthread_join(tw, 0);

	pthread_join(tr,0 );
	pthread_join(tr2,0 );


	return 0;
}
