#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

sem_t vagas;
int chegada_max = 0;
int uso_max = 0;

void *carro(void *arg) {
    int id = *(int*)arg;

    int atraso = rand() % (chegada_max + 1);
    usleep(atraso * 1000);
    printf("Carro %d chegou (atraso %d ms)\n", id, atraso);

    sem_wait(&vagas);
    printf("Carro %d entrou no estacionamento\n", id);

    int uso = rand() % (uso_max + 1);
    usleep(uso * 1000);
    printf("Carro %d saiu (uso %d ms)\n", id, uso);

    sem_post(&vagas);

    return NULL;
}

int main(int argc, char *argv[]) {
    int N = atoi(argv[1]);
    int S = atoi(argv[2]);
    chegada_max = atoi(argv[3]);
    uso_max = atoi(argv[4]);

    pthread_t threads[N];
    int ids[N];

    srand(time(NULL));

    sem_init(&vagas, 0, S);

    for (int i = 0; i < N; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, carro, &ids[i]);
    }

    for (int i = 0; i < N; i++) {
        pthread_join(threads[i], NULL);
    }

    sem_destroy(&vagas);

    printf("Simulação encerrada.\n");
    return 0;
}
