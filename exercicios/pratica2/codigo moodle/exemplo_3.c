#include	<pthread.h>
#include	<stdio.h>

#define     TRUE        1
#define		MAX_ITEMS	3

pthread_cond_t 	cond_empty, cond_full;
pthread_mutex_t mutex;
int count = 0, in = 0, out = 0;

int buffer[MAX_ITEMS];

 void *produtor(void *arg) {

     while (TRUE) {
		 sleep(rand()%5);
	     pthread_mutex_lock(&mutex);
	     while (count == MAX_ITEMS)
	         pthread_cond_wait(&cond_empty, &mutex);
	     buffer[in] = rand()%100;
	     count++;
         printf("Produzindo buffer[%d] = %d\n", in, buffer[in]);
         in = (in + 1) % MAX_ITEMS;
	     pthread_cond_signal(&cond_full);
	     pthread_mutex_unlock(&mutex);
     }
 }

 void *consumidor(void *arg) {
     int my_task;
     while (TRUE) {
		 sleep(rand()%5);
	     pthread_mutex_lock(&mutex);
	     while (count == 0)
	         pthread_cond_wait(&cond_full, &mutex);
	     my_task = buffer[out];
	     count--;
         printf("Consumindo buffer[%d] = %d\n", out, my_task);
         out = (out + 1) % MAX_ITEMS;
	     pthread_cond_signal(&cond_empty);
	     pthread_mutex_unlock(&mutex);
     }
 }

 int main(int argc, char *argv[]) {
     pthread_t prod, cons;

     pthread_cond_init(&cond_empty, NULL);
     pthread_cond_init(&cond_full, NULL);
     pthread_mutex_init(&mutex, NULL);

     pthread_create(&prod, NULL, (void *)produtor, NULL);
     pthread_create(&cons, NULL, (void *)consumidor, NULL);

     pthread_exit(0);
 }

