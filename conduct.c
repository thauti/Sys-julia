
#include <stdio.h>
#include <stdlib.h>
#include "conduct.h"

#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h> /* For O_* constants */
#include <string.h>
#include <pthread.h>

#define DEBUG 1

struct conduct *conduct_create(const char *name, size_t a, size_t c)
{
	 struct conduct* conduit;
    
    if(name == NULL)
    {
        if(DEBUG)
            printf("Conduit anonyme \n");
        /* Anonyme */
        conduit =  mmap(NULL, sizeof(struct conduct), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if(conduit == MAP_FAILED){
          fprintf(stderr, "Erreur lors du mmap d'un tube anonyme\n");
          return NULL;
        }
    }
    else
    {
        /* Nomme */
        int fd, chk;
        fd = open(name, O_CREAT | O_RDWR, 0644);
        if(fd == -1)
        {
            fprintf(stderr, "Erreur lors du open de %s\n", name);
            return NULL;
        }
        chk = ftruncate(fd, sizeof(struct conduct));
        if(chk == -1){
          fprintf(stderr, "Erreur lors du ftruncate de %s\n", name);
          unlink(name);
          return NULL;
        }
        conduit =  mmap(NULL, sizeof(struct conduct), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if(conduit == MAP_FAILED){
          fprintf(stderr, "Erreur lors du mmap de %s\n", name);
         unlink(name);
          return NULL;
        }
    }
    if(name != NULL)
    {
        conduit->filename = name;
        int fd2;
        fd2 = shm_open(name,  O_CREAT | O_RDWR|O_TRUNC, S_IRUSR | S_IWUSR);
        ftruncate(fd2, c);
        conduit->buffer = mmap(NULL, c, PROT_READ | PROT_WRITE, MAP_SHARED, fd2, 0);
    }
    else
    {
        conduit->buffer = mmap(NULL, c, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    }
    conduit->c = c;
    conduit->a = a;
    conduit->pos_read = 0;
    conduit->pos_write = 0;
    if(DEBUG)
        printf("Création d'un conduit de taille %d \n", c);
    conduit->eof = 0;
    conduit->placeUtilise =0;

    pthread_mutex_init(&conduit->mWrite, NULL);
    pthread_mutex_init(&conduit->mRead, NULL);

    pthread_mutex_init(&conduit->mCondEcrit, NULL);
    pthread_mutex_init(&conduit->mCondLire, NULL);
    pthread_cond_init(&conduit->condLire,NULL);
    pthread_cond_init(&conduit->condEcrit,NULL);

    pthread_mutex_init(&conduit->mProtege, NULL);
    return conduit;
}
struct conduct *conduct_open(const char *name)
{
	struct conduct* conduit;
    int fd;

    fd = open(name, O_RDWR, 0644);  
    if(fd == -1)
    {
        fprintf(stderr, "Erreur lors du open de %s\n", name);
        return NULL;
    }
    conduit = mmap(NULL, sizeof(struct conduct), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(conduit == MAP_FAILED){
        fprintf(stderr, "Erreur lors du mmap de %s\n", name);
        return NULL;
    }
    int fd2;
    fd2 = shm_open(name, O_RDWR, S_IRUSR | S_IWUSR);
    conduit->buffer = mmap(NULL, conduit->c, PROT_READ | PROT_WRITE, MAP_SHARED, fd2, 0);

    //printf("Place->utilise %s", conduit->buffer);
    fflush(0);
    return conduit;
}
ssize_t conduct_read(struct conduct *c, void *buf, size_t count)
{
    int n;
    pthread_mutex_lock(&c->mProtege);
    if(c->placeUtilise == 0 && c->eof == 1)
    {
        pthread_mutex_unlock(&c->mProtege);
        return 0;
    }
    if(c->placeUtilise != 0)
    {
    	if(c->placeUtilise < count)
        {
            n = c->placeUtilise;
        }
        else
        {
            n = count;
        }
    }
    else
    {
        pthread_mutex_unlock(&c->mProtege);
        
        boucle: while(c->placeUtilise == 0 && c->eof == 0)
        {
           if(DEBUG)
                printf("Blocage lecture \n");
            pthread_mutex_lock(&c->mCondEcrit);
            pthread_cond_wait(&c->condEcrit, &c->mCondEcrit); // on attend
            pthread_mutex_unlock(&c->mCondEcrit);
        }
        pthread_mutex_lock(&c->mProtege);
       if(c->eof == 1)
        {
            if(c->placeUtilise == 0){
                pthread_mutex_unlock(&c->mProtege);
                return 0;
            }
            else
            {
                pthread_mutex_unlock(&c->mProtege);
                return -1;
            }
        }
        if(c->placeUtilise < count)
        {
            n  = c->placeUtilise;
        }
        else
        {
            n = count;
        }
    }
    int j;

    if(c->placeUtilise <= 0)
    {
        if(DEBUG){
            printf("C->placeUtilise < 0 %d %d \n", c->placeUtilise, n);
            fflush(0);
        }
        pthread_mutex_unlock(&c->mProtege);

        //_exit(-1);
        goto boucle;
    }
    for(j=0; j<n;j++)
    {

        memcpy(buf+j, c->buffer+c->pos_read, 1);
        c->placeUtilise--;
        c->pos_read =  (c->pos_read + 1) % c->c;
    }
    pthread_mutex_unlock(&c->mProtege);

    pthread_mutex_lock(&c->mCondLire);
    pthread_cond_signal(&c->condLire);
    pthread_mutex_unlock(&c->mCondLire);

    return n;
}
ssize_t conduct_write(struct conduct *c, const void *buf, size_t count)
{
    int nbOctetEcrit;
    if(c->eof == 1)
    {
        return -1; // TO DO: errno
    }
    if(count <= c->a)
    {
        //on bloque
        pthread_mutex_lock(&c->mWrite);
        while(c->placeUtilise+count > c->c && c->eof == 0)
        {
            if(DEBUG)
                printf("Attente ecriture /n");
            pthread_mutex_lock(&c->mCondLire);
            pthread_cond_wait(&c->condLire, &c->mCondLire); // On attend une lecture pour verifier la condition
            pthread_mutex_unlock(&c->mCondLire);
        }
        pthread_mutex_lock(&c->mProtege);
        if(c->eof == 1)
        {
            if(c->placeUtilise == 0){
                pthread_mutex_unlock(&c->mProtege);
                return 0;
            }
            else{
                pthread_mutex_unlock(&c->mProtege);
                return -1;
            }
        }
        //Pas beau
        int i;
        if(DEBUG){
            printf("Ecriture de %d octets \n", count);
            printf("Place : %d/%d [%d] \n", c->placeUtilise, c->c, c->a);
        }
        for(i=0;i<count;i++)
        {
            memcpy(c->buffer+c->pos_write, buf+i, 1); // Moche
            c->placeUtilise++;
            c->pos_write = (c->pos_write + 1) % c->c;
            if(DEBUG)
                printf("Place Utilise :  %d /  %d\n", c->placeUtilise, c->c);
        }
        nbOctetEcrit = count;
        pthread_mutex_lock(&c->mCondEcrit);
        pthread_cond_signal (&c->condEcrit); // Envoyer le signal qu'on a ecrit qqchose
        pthread_mutex_unlock(&c->mCondEcrit);
        pthread_mutex_unlock(&c->mWrite);
        pthread_mutex_unlock(&c->mProtege);

    }
    else
    {/*
        printf("STOOOOOOOOOOOOOOOOOP");
        // Ecriture partielle (peut Ãªtre Ã  revoir)
        int n;
         //on bloque
        pthread_mutex_lock(&c->mWrite);
        while(c->placeUtilise+c->a > c->c) // AtomicitÃ©
        {
            if(c->eof == 1)
            {
                return -1;
            }
            // On attend une lecture pour verifier la condition
        }
        //Pas beau
        int i;
        for(i=0;i<c->a;i++)
        {
            memcpy(c->buffer+c->pos_write,buf+i, 1);
            c->placeUtilise++;
            c->pos_write = (c->pos_write + 1) % c->c;
        }
        pthread_mutex_unlock(&c->mWrite);
        pthread_cond_signal (&c->condEcrit); // Envoi le signal qu'on a ecrit qqchose
        int placeLibre;
        placeLibre = c->c - c->placeUtilise;
        if(placeLibre < count)
        {
            n = placeLibre;
        }
        else
        {
            n = count;
        }
        for(i=0;i<n;i++)
        {
            memcpy(c->buffer+c->pos_write,buf+i, 1);
            c->placeUtilise++;            
            c->pos_write = (c->pos_write + 1) % c->c;
        }
        pthread_cond_signal (&c->condEcrit); // Envoi le signal qu'on a ecrit qqchose
        nbOctetEcrit = c->a + n;
        */
        if(DEBUG)
            printf("STOOOP\n");
        return -1;
    }
	return nbOctetEcrit;
}
int conduct_write_eof(struct conduct *c)
{
        if(DEBUG)
	        printf("WRITE EOF \n");
    if(c->eof == 0)
    {
        if(DEBUG)
            printf("WRITE EOF EFF \n");
        c->eof = 1;
        // ?
        pthread_mutex_lock(&c->mCondEcrit);
        pthread_cond_broadcast (&c->condEcrit);
         pthread_mutex_unlock(&c->mCondEcrit);
        pthread_mutex_lock(&c->mCondLire);
        pthread_cond_broadcast (&c->condLire);
        pthread_mutex_unlock(&c->mCondLire);
    }
    return 0;
}
void conduct_close(struct conduct *conduct)
{
	return;
}
void conduct_destroy(struct conduct *conduct)
{
    if(conduct != NULL){
        if(conduct->filename != NULL)
        {
            unlink(conduct->filename);
        }
        conduct_close(conduct);
        munmap(conduct, sizeof(struct conduct));
    }
	return;
}