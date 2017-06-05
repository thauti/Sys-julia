
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
#include <errno.h>

#define DEBUG 0

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
            errno = EBUSY;
          fprintf(stderr, "Erreur lors du mmap d'un tube anonyme\n");
          return NULL;
        }
    }
    else
    {
        /* Nomme */
        int fd, chk;
        fd = open(name, O_CREAT | O_RDWR|O_TRUNC, 0644);
        if(fd == -1)
        {
            errno = EBUSY;
            fprintf(stderr, "Erreur lors du open de %s\n", name);
            return NULL;
        }
        chk = ftruncate(fd, sizeof(struct conduct));
        if(chk == -1){
        errno = EBUSY;
          fprintf(stderr, "Erreur lors du ftruncate de %s\n", name);
          unlink(name);
          return NULL;
        }
        conduit =  mmap(NULL, sizeof(struct conduct), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if(conduit == MAP_FAILED){
            errno = EBUSY;
            fprintf(stderr, "Erreur lors du mmap de %s\n", name);
            unlink(name);
          return NULL;
        }
    }
    if(name != NULL)
    {
        
        int fd2;
        fd2 = shm_open(name,  O_CREAT | O_RDWR|O_TRUNC, S_IRUSR | S_IWUSR);
        ftruncate(fd2, c);
        conduit->buffer = mmap(NULL, c, PROT_READ | PROT_WRITE, MAP_SHARED, fd2, 0);
        if(conduit->buffer == MAP_FAILED)
        {
            errno = EBUSY;
            fprintf(stderr, "Erreur lors du mmap nomme");
            return NULL;
        }
        conduit->filename = name;
    }
    else
    {
        conduit->buffer = mmap(NULL, c, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if(conduit->buffer == MAP_FAILED)
        {
            errno = EBUSY;
            fprintf(stderr, "Erreur lors du mmap nomme");
            return NULL;
        }
    }
    conduit->c = c;
    conduit->a = a;
    conduit->pos_read = 0;
    conduit->pos_write = 0;
    if(DEBUG)
        printf("Création d'un conduit de taille %d \n", c);
    conduit->eof = 0;
    conduit->placeUtilise =0;

    pthread_mutexattr_init(&conduit->mutexattr);

    pthread_condattr_init(&conduit->condattr);
    
    pthread_mutexattr_setpshared(&conduit->mutexattr,  
 				  PTHREAD_PROCESS_SHARED);
    pthread_condattr_setpshared(&conduit->condattr,  
 				  PTHREAD_PROCESS_SHARED);
    
    pthread_mutex_init(&conduit->mWrite, &conduit->mutexattr);

    pthread_mutex_init(&conduit->mCondEcrit, &conduit->mutexattr);
    pthread_mutex_init(&conduit->mCondLire, &conduit->mutexattr);
    pthread_cond_init(&conduit->condLire,&conduit->condattr);
    pthread_cond_init(&conduit->condEcrit,&conduit->condattr);

    pthread_mutex_init(&conduit->mProtege, &conduit->mutexattr);
    return conduit;
}
struct conduct *conduct_open(const char *name)
{
	struct conduct* conduit;
    int fd;

    fd = open(name, O_RDWR, 0644);  
    if(fd == -1)
    {
        errno = ENOENT;
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
    if(fd2 == -1)
    {
        errno = EBUSY;
        fprintf(stderr, "Erreur lors du shm_open de %s\n", name);
        return NULL;
    }
    conduit->buffer = mmap(NULL, conduit->c, PROT_READ | PROT_WRITE, MAP_SHARED, fd2, 0);
    if(conduit->buffer == MAP_FAILED)
    {
        errno = EBUSY;

        fprintf(stderr, "Erreur lors du mmap de %s\n", name);
        return NULL;
    }
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
        errno  = EPIPE;
        return -1; 
    }
    if(count <= c->a)
    {
        //on bloque
        pthread_mutex_lock(&c->mWrite);
        while(c->placeUtilise+count > c->c && c->eof == 0)
        {
            if(DEBUG)
                printf("Attente ecriture \n");
            pthread_mutex_lock(&c->mCondLire);
            pthread_cond_wait(&c->condLire, &c->mCondLire); // On attend une lecture pour verifier la condition
            pthread_mutex_unlock(&c->mCondLire);
        }
        pthread_mutex_lock(&c->mProtege);
        if(c->eof == 1)
        {
            if(c->placeUtilise == 0){
                pthread_mutex_unlock(&c->mProtege);
                errno  = EPIPE;
                return 0;
            }
            else{
                pthread_mutex_unlock(&c->mProtege);
                errno  = EPIPE;
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
            if(DEBUG){
                printf("Place Utilise :  %d /  %d\n", c->placeUtilise, c->c);
                fflush(0);
            }
        }
        nbOctetEcrit = count;
        pthread_mutex_lock(&c->mCondEcrit);
        pthread_cond_signal (&c->condEcrit); // Envoyer le signal qu'on a ecrit qqchose
        pthread_mutex_unlock(&c->mCondEcrit);
        pthread_mutex_unlock(&c->mWrite);
        pthread_mutex_unlock(&c->mProtege);

    }
    else
    {
        pthread_mutex_lock(&c->mWrite);

        pthread_mutex_lock(&c->mProtege);

        
        int placeRestante;
        placeRestante = c->c - c->placeUtilise;
        if(c->eof == 1)
        {
        errno  = EPIPE;
        return -1; 
        }
        int v;
        if(placeRestante < count)
        {
            v = placeRestante;
        }
        else
        {
            v = count;
        }

        //DEBUT MODIF
        if(c->pos_write + v < c->c){ /*si le nombre d'octets a lire ne depasse pas la fin du buffer*/
          memcpy(c->buffer+c->pos_write, buf, v);
          c->placeUtilise += v;
          c->pos_write += v;
        } else {
          int octetsEcrits = c->c - c->pos_write;
          memcpy(c->buffer+c->pos_write, buf, octetsEcrits);
          memcpy(c->buffer, buf+octetsEcrits, v-octetsEcrits);
          c->placeUtilise += v;
          c->pos_write = v-octetsEcrits;
        }
        if(DEBUG)
          printf("Place Utilise :  %d /  %d\n", c->placeUtilise, c->c);
        nbOctetEcrit = v;


        pthread_mutex_lock(&c->mCondEcrit);
        pthread_cond_signal (&c->condEcrit); // Envoyer le signal qu'on a ecrit qqchose
        pthread_mutex_unlock(&c->mCondEcrit);
        pthread_mutex_unlock(&c->mWrite);
        pthread_mutex_unlock(&c->mProtege);
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
    if(conduct != NULL){
    if(conduct->filename != NULL){
        munmap(conduct->buffer, conduct->c);
        munmap(conduct, sizeof(struct conduct));
    }
    }
    else
    {
         errno = ENXIO;
    }
	return;
}
void conduct_destroy(struct conduct *conduct)
{
    if(conduct != NULL){
        if(conduct->filename != NULL)
        {
            conduct_close(conduct);
            unlink(conduct->filename);
            shm_unlink(conduct->filename);
        }else{
             conduct_close(conduct);
             munmap(conduct->buffer, conduct->c);
             munmap(conduct, sizeof(struct conduct));
        }
    }
    else
    {
        errno = ENXIO;
    }
	return;
}