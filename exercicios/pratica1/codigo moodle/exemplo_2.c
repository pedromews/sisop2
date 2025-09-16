#include <stdio.h>
#include <pthread.h>

int g;

void *do_it_1(void *arg) {
    int i, n = *(int *) arg;
    for(i = 0; i < n; i++)
        g = i;
}

void *do_it_2(void *arg) {
    int i, n = *(int *) arg;
    for(i = 0; i < n; i++)
        printf("%d\n", g);
}

int main( int argc, char **argv) {
    pthread_t th1, th2;
    int n = 10;

    pthread_create(&th1, NULL, do_it_1, &n);
    pthread_create(&th2, NULL, do_it_2, &n);
	
    printf("acabando...\n");
}
