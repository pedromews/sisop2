#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void* thread_fn(void *arg) {
    int id = *(int*)arg;
    printf("Hello, sou a thread %d\n", id);
    return NULL;
}

int main(int argc, char **argv) {
    int T = atoi(argv[1]);

    pthread_t* threads = malloc(sizeof(pthread_t) * T);

    for (int i = 0; i < T; ++i) {
        pthread_create(&threads[i], NULL, thread_fn, &i);
    }

    for (int i = 0; i < T; ++i) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    return 0;
}
