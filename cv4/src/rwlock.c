#include <stdio.h>
#include <pthread.h>
#include <sys/select.h>
#include <stdlib.h>

pthread_rwlock_t RWLock = PTHREAD_RWLOCK_INITIALIZER;

void *
Reader (void * Arg)
{
    struct timeval T;
    pthread_t Me = pthread_self();
    
    printf ("\033[32m");
    printf ("Reader %lx started, trying to lock for reading... ", Me);
    fflush (stdout);
    if (pthread_rwlock_rdlock (&RWLock) != 0) {
        fprintf (stderr, "%lx: Error locking for reading. Exiting.\n", Me);
        exit (1);
    }
    printf ("\033[32m");
    printf ("%lx done. Starting lengthy read.\n", Me);
    T.tv_sec = 5;
    T.tv_usec = 0;
    select (0, NULL, NULL, NULL, &T);
    printf ("\033[32m");
    printf ("%lx end reading, about to unlock... ", Me);
    if (pthread_rwlock_unlock (&RWLock) != 0) {
        fprintf (stderr, "%lx: Error unlocking. Exiting.\n", Me);
        exit (1);
    }
    printf ("\033[32m");
    printf ("%lx unlocked. Finishing.\n", Me);
    pthread_exit (NULL);
}

void *
Writer (void * Arg)
{
    struct timeval T;
    pthread_t Me = pthread_self();

    printf ("\033[31m");    
    printf ("Writer %lx started, trying to lock for writing... ", Me);
    fflush (stdout);
    if (pthread_rwlock_wrlock (&RWLock) != 0) {
        fprintf (stderr, "%lx: Error locking for writing. Exiting.\n", Me);
        exit (1);
    }
    printf ("\033[31m");
    printf ("%lx done. Starting lengthy write.\n", Me);
    T.tv_sec = 5;
    T.tv_usec = 0;
    select (0, NULL, NULL, NULL, &T);
    printf ("\033[31m");
    printf ("%lx end writing, about to unlock... ", Me);
    if (pthread_rwlock_unlock (&RWLock) != 0) {
        fprintf (stderr, "%lx: Error unlocking. Exiting.\n", Me);
        exit (1);
    }
    printf ("\033[31m");
    printf ("%lx unlocked. Finishing.\n", Me);
    pthread_exit (NULL);
}

int main (void)
{
    pthread_t P;
    struct timeval T;
    
    for (int i = 0; i < 3; i++)
        pthread_create (&P, NULL, Reader, NULL);

    T.tv_sec = 2;
    T.tv_usec = 0;
    select (0, NULL, NULL, NULL, &T);
    
    pthread_create (&P, NULL, Writer, NULL);
 
    T.tv_sec = 5;
    T.tv_usec = 0;
    select (0, NULL, NULL, NULL, &T);

    for (int i = 0; i < 3; i++)
        pthread_create (&P, NULL, Reader, NULL);

    T.tv_sec = 15;
    T.tv_usec = 0;
    select (0, NULL, NULL, NULL, &T);

    printf ("\033[m\n");
    return 0;
}
