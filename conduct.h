struct conduct
{
    size_t c; /* Capacité du conduit*/
    size_t a; /* Atomicité */
    
    /* Entre 0 et a-1*/
    int pos_read;
    int pos_write;  
    int placeUtilise;

    
    // Mutex
    pthread_mutex_t mWrite;
    pthread_mutex_t mRead;
    pthread_mutex_t mCondEcrit;
    pthread_mutex_t mCondLire;

    pthread_mutex_t mProtege;
    // Variable de cond
    pthread_cond_t condEcrit;
    pthread_cond_t condLire;

    pthread_mutexattr_t mutexattr;
    pthread_condattr_t condattr;

    void* buffer; /* Taille c*/
    char* filename;
    char eof;

};

struct conduct *conduct_create(const char *name, size_t a, size_t c);
struct conduct *conduct_open(const char *name);
ssize_t conduct_read(struct conduct *c, void *buf, size_t count);
ssize_t conduct_write(struct conduct *c, const void *buf, size_t count);
int conduct_write_eof(struct conduct *c);
void conduct_close(struct conduct *conduct);
void conduct_destroy(struct conduct *conduct);
