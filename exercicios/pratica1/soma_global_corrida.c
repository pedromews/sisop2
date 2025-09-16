#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define N 1000000

int A[N];
long long SUM = 0;

void *worker(void *arg) {
    int id = *(int*)arg;
    
    extern int T_global;
    int T = T_global;
    int chunk = N / T;

    int start = id * chunk;
    int end = (id == T - 1) ? N : start + chunk;

    for (int i = start; i < end; ++i) {
        SUM += A[i];
    }
    return NULL;
}

int T_global = 1;

int main(int argc, char **argv) {
    int T = atoi(argv[1]);
    T_global = T;

    pthread_t *threads = malloc(sizeof(pthread_t) * T);
    int *ids = malloc(sizeof(int) * T);

    for (int i = 0; i < N; ++i) A[i] = i + 1;

    SUM = 0;
    for (int i = 0; i < T; ++i) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, worker, &ids[i]);
    }

    for (int i = 0; i < T; ++i) {
        pthread_join(threads[i], NULL);
    }

    long long SUM_paralelo = SUM;
    long long SUM_sequencial = 0;
    for (int i = 0; i < N; ++i) SUM_sequencial += A[i];

    printf("SUM_paralelo:   %lld\n", SUM_paralelo);
    printf("SUM_sequencial: %lld\n", SUM_sequencial);
    if (SUM_paralelo == SUM_sequencial) {
        printf("OK\n");
    } else {
        printf("ERRO\n");
    }

    free(threads);
    free(ids);
    return 0;
}
