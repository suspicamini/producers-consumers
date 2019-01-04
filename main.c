#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

FILE *fp;
char *line = NULL;
size_t len = 0;
ssize_t readLine;

pthread_mutex_t bookLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t bufferLock = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t Out_CV = PTHREAD_COND_INITIALIZER;
pthread_cond_t In_CV = PTHREAD_COND_INITIALIZER;

int position = 0;
int in = 0;
int out = 0;
char buffer[1000][100];
int n = 1000;


void *producer(void *arg) {
    int *myId;
    myId = (int *) arg;
    int id = *myId;

    while (1) {
        pthread_mutex_lock(&bookLock);
        lseek(fileno(fp), position, SEEK_SET);

        readLine = getline(&line, &len, fp);

        if (readLine == -1) {
            pthread_mutex_unlock(&bookLock);
            break;
        }

        /* inserts line into buffer*/
        pthread_mutex_lock(&bufferLock);
        while ((in + 1) % n == out) pthread_cond_wait(&Out_CV, &bufferLock);
        strcpy(buffer[in], line); /* producing buffer[in] */
        printf("producer-%d\tin=%d\tproducing: %s", id, in, buffer[in]);

        in = (in + 1) % n;
        pthread_cond_signal(&In_CV);
        pthread_mutex_unlock(&bufferLock);

        int read_v = (int) readLine;
        position = position + read_v;

        pthread_mutex_unlock(&bookLock);
    }
}


void *consumer(void *arg) {
    int *myId;
    myId = (int *) arg;
    int id = *myId;

    while (1) {
        pthread_mutex_lock(&bufferLock);
        while (in == out) pthread_cond_wait(&In_CV, &bufferLock);
        printf("consumer-%d\tout=%d\tconsuming: %s", id, out, buffer[out]);
        out = (out + 1) % n;
        pthread_mutex_unlock(&bufferLock);
        pthread_cond_signal(&Out_CV);
    }
}

int main() {
    int p = 5;
    int k = 1;
    char *fileName = "pan-tadeusz.txt";
    int l = 100;
    int fullDescription = 1;

    fp = fopen(fileName, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    pthread_t producers[p];
    pthread_t consumers[k];


    /* uruchamianie wątków producenckich i konsumenckich */
    for (int i = 0; i < p; i++) {
        if (pthread_create(&producers[i], NULL, producer, &i)) {
            perror("pthread_create");
            exit(0);
        }
    }

    for (int l = 0; l < k; ++l) {
        if (pthread_create(&consumers[l], NULL, consumer, &l)) {
            perror("pthread_create consumer");
            exit(0);
        }
    }

    /* czekanie na zakończenie pracy wątków przez watek główny */
    for (int j = 0; j < p; ++j) {
        if (pthread_join(producers[j], NULL)) {
            perror("pthread join");
            exit(0);
        }
    }

    for (int j = 0; j < k; ++j) {
        if (pthread_join(consumers[j], NULL)) {
            perror("pthread join");
            exit(0);
        }
    }


    // zamykanie pliku i zwalnianie pamięci

    fclose(fp);
    if (line)
        free(line);
    return 0;
}