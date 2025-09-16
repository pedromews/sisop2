#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void *test_routine (void *arg) {
    double *value;
    value = (double*)malloc(sizeof(double));
    *value = 100.0;
    /*** fix me ***/
}

int main( int argc, char **argv) {
    pthread_t thr;
    double *res;
    int status;

    status = pthread_create( /*** fix me ***/ );
    if (status != 0)
        exit(1);

    status = pthread_join ( /*** fix me ***/ );
    if (status != 0)
        exit(1);

    printf("resultado retornado: %.2f\n", *res);
    free(res);
}
