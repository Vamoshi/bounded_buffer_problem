#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>

#define GEN_THREADS 3
#define OP_THREADS 3
#define BUFFER_SIZE 10
#define OUTPUT_SIZE 10

// function prototypes
void *timerThread(void *args);

// barriers
pthread_barrier_t barrierGen;

// mutexes
pthread_mutex_t mutexBuffer;
pthread_mutex_t mutexGen;
pthread_mutex_t mutexOp;
pthread_mutex_t mutexOutput;
pthread_mutex_t mutexSameString;
pthread_mutex_t mutexStopTimer;

// semaphores
sem_t semAvailable;
sem_t semOccupied;
sem_t semTools;

// conds
pthread_cond_t condFull;
pthread_cond_t condEmpty;
pthread_cond_t condSameString;
pthread_cond_t condStopTimer;

// buffer
char buffer[BUFFER_SIZE];
int bufferCurrIndex = 0;

// output
char **outputArr;
int outputArrSize = OUTPUT_SIZE;
int outputArrIndex = 0;

// materials
char materials[3] = "abc";

// time to run
int currTime = 0;
// runs for ttr seconds
int ttr = 15;

// statistics
int matsGen[3] = {0, 0, 0};
int opGen[9] = {0};

void *generator(void *args)
{
    // Thread "number"
    int genNum = *(char *)args;
    char material;
    while (currTime < ttr - 1)
    {
        // Redundancy (also using semaphore)
        // if buffer current index == size of buffer,
        // then wait for the consumer to consume from buffer
        while (bufferCurrIndex == BUFFER_SIZE)
        {
            pthread_cond_wait(&condFull, &mutexGen);
        }

        // Wait for all 3 threads to generate something
        pthread_barrier_wait(&barrierGen);

        // deduct from available space flag in buffer
        sem_wait(&semAvailable);

        // add material to buffer
        pthread_mutex_lock(&mutexBuffer);
        material = materials[genNum];
        buffer[bufferCurrIndex] = material;
        bufferCurrIndex++;
        // printf("Gen %d Added %c\n", genNum, material);
        pthread_mutex_unlock(&mutexBuffer);
        pthread_cond_broadcast(&condEmpty);

        // print the state of input buffer
        // once a generator thread has finished adding something to buffer
        char string[BUFFER_SIZE + 1];
        for (int i = 0; i < BUFFER_SIZE; i++)
        {
            string[i] = buffer[i];
        }
        string[BUFFER_SIZE + 1] = '\0';
        printf("Generator Thread #(%d) has '%s' in buffer \n", genNum, string);

        // increment flag that shows how many occupied space in buffer
        sem_post(&semOccupied);

        // count how many materials produced
        matsGen[genNum]++;
    }
}

void *operator(void *args)
{
    int opNum = *(char *)args;
    char res[3];
    res[2] = '\0';
    // resIndex = 0 means that we can start creating new products
    int resIndex = 0;
    while (currTime < ttr - 1)
    {
        // Redundancy (also using semaphore)
        // if buffer is empty,
        // then wait for the generator to add to buffer
        while (bufferCurrIndex < 0)
        {
            printf("Opnum %d waiting index %d\n", opNum, bufferCurrIndex);
            pthread_cond_wait(&condEmpty, &mutexOp);
        }

        // decrement occupied flag
        sem_wait(&semOccupied);
        pthread_mutex_lock(&mutexBuffer);
        // printf("bufferIndex %d \n", bufferCurrIndex);
        res[resIndex] = buffer[bufferCurrIndex - 1];
        if (resIndex == 0)
        {
            resIndex = 1;
        }
        // add to outputQ
        else if (resIndex == 1)
        {
            // resize array if index == size of output array
            if (outputArrIndex == outputArrSize - 1)
            {
                printf("Resizing buffer \n");
                // increase outputArr
                outputArrSize += OUTPUT_SIZE;
                // resize array
                outputArr = realloc(outputArr, outputArrSize * sizeof(char *));
            }

            // if the most recent product in array is == res (created product)
            // wait til another thread has added to it
            // but if waiting time has exceeded 10 * 10 microseconds,
            // just add to output regardless;
            // because if all threads have the same product, let's say
            // thread 1 == aa, 2 == aa, 3 == aa, then just add to output queue
            int waitingTime = 0;
            while (outputArr[outputArrIndex] == res || waitingTime < 10)
            {
                // printf("Thread %d Im waiting\n", opNum);
                usleep(10);
                waitingTime++;
                pthread_cond_wait(&condSameString, &mutexSameString);
            }

            // get tools
            int toolNum = 0;
            while (toolNum < 2)
            {
                // decrement available tools
                sem_wait(&semTools);
                toolNum++;
                // printf("Opnum %d has %d tools\n", opNum, toolNum);
            }
            // release tools back
            for (int i = 0; i < toolNum; i++)
            {
                sem_post(&semTools);
            }
            sem_post(&semTools);

            // allocate memory for string in end of array of string
            outputArr[outputArrIndex] = malloc(3 * sizeof(char));
            // add string in memory allocated for it
            strcpy(outputArr[outputArrIndex], res);

            // reset result index so that the next material grabbed goes to res[0]
            resIndex = 0;
            // printf("Op %d produced %s \n", opNum, outputArr[outputArrIndex]);
            // increment output array index
            outputArrIndex++;
            // Sleep for 0.01-1 second because "manufacturing" time
            int secs = (rand() % (100000 - 1000 + 1)) + 1000;
            usleep(secs);
        }
        bufferCurrIndex--;
        pthread_mutex_unlock(&mutexBuffer);
        pthread_cond_broadcast(&condFull);
        pthread_cond_broadcast(&condSameString);
        // pthread_cond_signal(&condStopTimer);
        sem_post(&semAvailable);
    }
}

int main()
{
    // init rand
    srand(time(NULL));

    // init pthread_t
    pthread_t genTh[GEN_THREADS];
    pthread_t opTh[OP_THREADS];
    pthread_t timerTh;

    // init barriers
    pthread_barrier_init(&barrierGen, NULL, 3);

    // init mutexes
    pthread_mutex_init(&mutexBuffer, NULL);
    pthread_mutex_init(&mutexGen, NULL);
    pthread_mutex_init(&mutexOp, NULL);
    pthread_mutex_init(&mutexBuffer, NULL);
    pthread_mutex_init(&mutexStopTimer, NULL);

    // init semaphores
    sem_init(&semAvailable, 0, BUFFER_SIZE);
    sem_init(&semOccupied, 0, 0);
    sem_init(&semTools, 0, 3);

    // init conds
    pthread_cond_init(&condFull, NULL);
    pthread_cond_init(&condEmpty, NULL);
    pthread_cond_init(&condSameString, NULL);
    pthread_cond_init(&condStopTimer, NULL);

    // create output array
    outputArr = malloc(OUTPUT_SIZE * sizeof(char *));

    int i = 0;
    // Create generators
    for (i = 0; i < GEN_THREADS; i++)
    {
        if (pthread_create(&genTh[i], NULL, &generator, &i) != 0)
        {
            perror("Failed to create generator thread \n");
        }
    }
    // Create operators
    for (i = 0; i < OP_THREADS; i++)
    {
        if (pthread_create(&opTh[i], NULL, &operator, & i) != 0)
        {
            perror("Failed to create operator thread \n");
        }
    }

    // Create timer
    if (pthread_create(&timerTh, NULL, &timerThread, NULL) != 0)
    {
        perror("Failed to create operator thread \n");
    }

    // Join generators
    for (i = 0; i < GEN_THREADS; i++)
    {
        if (pthread_join(genTh[i], NULL) != 0)
        {
            perror("Failed to join generator thread \n");
        }
    }
    // Join operators
    for (i = 0; i < OP_THREADS; i++)
    {
        if (pthread_join(opTh[i], NULL) != 0)
        {
            perror("Failed to join operator thread \n");
        }
    }

    if (pthread_join(timerTh, NULL) != 0)
    {
        perror("Failed to join timer thread \n");
    }

    // destroy barriers
    pthread_barrier_destroy(&barrierGen);

    // destroy mutexes
    pthread_mutex_destroy(&mutexBuffer);
    pthread_mutex_destroy(&mutexOp);
    pthread_mutex_destroy(&mutexGen);
    pthread_mutex_destroy(&mutexBuffer);
    pthread_mutex_destroy(&mutexStopTimer);

    // destroy semaphores
    sem_destroy(&semAvailable);
    sem_destroy(&semOccupied);
    sem_destroy(&semTools);

    // destroy conds
    pthread_cond_destroy(&condFull);
    pthread_cond_destroy(&condEmpty);
    pthread_cond_destroy(&condSameString);
    pthread_cond_destroy(&condStopTimer);

    // free outputArr memory
    for (int i = 0; i < outputArrSize; i++)
    {
        free(outputArr[i]);
    }

    free(outputArr);

    return 0;
}

void *timerThread(void *args)
{
    // keeps track of how long program has been running
    // stops running when ttr is reached
    while (currTime < ttr)
    {
        sleep(1);
        currTime++;
    }
    // sleep(1);
    printf("Finished Running. Time = %d\n\n", currTime);

    printf("Generated: Material 'a' = %d \t Material 'b' = %d \t Material 'c' = %d \n\n", matsGen[0], matsGen[1], matsGen[2]);

    for (int i = 0; i < outputArrIndex; i++)
    {
        if (strcmp(outputArr[i], "aa") == 0)
        {
            opGen[0]++;
        }
        else if (strcmp(outputArr[i], "ab") == 0)
        {
            opGen[1]++;
        }
        else if (strcmp(outputArr[i], "ac") == 0)
        {
            opGen[2]++;
        }
        else if (strcmp(outputArr[i], "ba") == 0)
        {
            opGen[3]++;
        }
        else if (strcmp(outputArr[i], "bb") == 0)
        {
            opGen[4]++;
        }
        else if (strcmp(outputArr[i], "bc") == 0)
        {
            opGen[5]++;
        }
        else if (strcmp(outputArr[i], "ca") == 0)
        {
            opGen[6]++;
        }
        else if (strcmp(outputArr[i], "cb") == 0)
        {
            opGen[7]++;
        }
        else if (strcmp(outputArr[i], "cc") == 0)
        {
            opGen[8]++;
        }
    }
    printf("aa = %d ab = %d ac = %d ba = %d bb = %d bc = %d ca = %d cb = %d cc = %d \n\n", opGen[0], opGen[1], opGen[2], opGen[3], opGen[4], opGen[5], opGen[6], opGen[7], opGen[8]);

    printf("Produced %d number of products\n\n", outputArrIndex);

    printf("Products Generated: [");
    for (int i = 0; i < outputArrIndex; i++)
    {
        printf("%s, ", outputArr[i]);
    }
    printf("]\n");
}
