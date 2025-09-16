#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define N 1000000

int A[N];
long long SUM = 0;
int T_global;
pthread_mutex_t lock;

void *worker(void *arg) {
    int id = *(int*)arg;

    int chunk = N / T_global;
    int start = id * chunk;
    int end = (id == T_global - 1) ? N : start + chunk;

    long long parcial = 0;
    for (int i = start; i < end; i++) {
        parcial += A[i];
    }

    pthread_mutex_lock(&lock);
    SUM += parcial;
    pthread_mutex_unlock(&lock);

    return NULL;
}

int main(int argc, char *argv[]) {
    int T = atoi(argv[1]);
    T_global = T;

    for (int i = 0; i < N; i++) {
        A[i] = i + 1;
    }

    pthread_t threads[T];
    int ids[T];

    pthread_mutex_init(&lock, NULL);

    for (int i = 0; i < T; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, worker, &ids[i]);
    }

    for (int i = 0; i < T; i++) {
        pthread_join(threads[i], NULL);
    }

    long long SUM_paralelo = SUM;

    long long SUM_sequencial = 0;
    for (int i = 0; i < N; i++) {
        SUM_sequencial += A[i];
    }

    printf("SUM_paralelo:   %lld\n", SUM_paralelo);
    printf("SUM_sequencial: %lld\n", SUM_sequencial);
    if (SUM_paralelo == SUM_sequencial) {
        printf("OK\n");
    } else {
        printf("ERRO\n");
    }

    pthread_mutex_destroy(&lock);

    return 0;
}
