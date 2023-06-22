#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define RIGHT 0
#define LEFT 1
#define NONE 2

#define MAX_QUEUE_SIZE 100

typedef struct {
    int items[MAX_QUEUE_SIZE];
    int front;
    int rear;
    int size;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
} FIFOQueue;
typedef struct{
    int aCityCount;
    int *aCityTab;
    int aWaitCount;
    int *aWaitTab;

    int bCityCount;
    int *bCityTab;
    int bWaitCount;
    int *bWaitTab;
}Bridge;
void enqueue(FIFOQueue* queue, pthread_t item) {
    pthread_mutex_lock(&queue->mutex);
    
    queue->items[queue->rear] = item;
    queue->rear = (queue->rear + 1) % MAX_QUEUE_SIZE;
    queue->size++;

    pthread_cond_signal(&queue->condition);
    pthread_mutex_unlock(&queue->mutex);
}
pthread_t dequeue(FIFOQueue* queue) {
    pthread_mutex_lock(&queue->mutex);
    
    while (queue->size <= 0) {
        pthread_cond_wait(&queue->condition, &queue->mutex);
    }
    
    pthread_t item = queue->items[queue->front];

    pthread_cond_signal(&queue->condition);
    pthread_mutex_unlock(&queue->mutex);
    
    return item;
}
void popqueue(FIFOQueue* queue){
    pthread_mutex_lock(&queue->mutex);
    
    while (queue->size <= 0) {
        pthread_cond_wait(&queue->condition, &queue->mutex);
    }

    queue->front = (queue->front + 1) % MAX_QUEUE_SIZE;
    queue->size--;

    pthread_cond_signal(&queue->condition);
    pthread_mutex_unlock(&queue->mutex);
}

FIFOQueue queue;
Bridge bridge;
int N = 0;
short infoFlag = 0;

pthread_mutex_t mutexBridge;

void city();
void *car_thread( void *ptr );
void display(int id,int direction);

int main(int argc, char *argv[]){
    int i=0;
    pthread_t *cars;
    
    queue.front = 0;
    queue.rear = 0;
    queue.size = 0;
    pthread_mutex_init(&queue.mutex, NULL);
    pthread_cond_init(&queue.condition, NULL);
    // Check if the N parameter was given
    if(argc < 2){
        printf("Too few parameters.\n");
        return 1;
    }
    N = atoi(argv[1]);

    // Check the optional parameter -info
    if(argc == 3 && strcmp(argv[2], "-info") == 0){
         infoFlag = 1;
    }

    // Initialize car array
    bridge.aCityTab = malloc(sizeof(int)*N);
    bridge.aWaitTab = malloc(sizeof(int)*N);
    bridge.bCityTab = malloc(sizeof(int)*N);
    bridge.bWaitTab = malloc(sizeof(int)*N);
    for(i=0;i<N;i++){
        bridge.aCityTab[i] = -1;
        bridge.aWaitTab[i] = -1;
        bridge.bCityTab[i] = -1;
        bridge.bWaitTab[i] = -1;
    }


    // Initialize mutex
    pthread_mutex_init(&mutexBridge, NULL);
    // Seed the random number generator
    srand(time(NULL));

    // Generate cars
    cars = malloc(sizeof(pthread_t)*N);
    for(i=0;i<N;i++){
        if(pthread_create(&(cars[i]),NULL,car_thread,(void*)i) != 0){
            printf("Cannot generate car.\n");
            return 1;
        }
    }

    // Join threads
    for (i = 0; i < N; i++) {
        pthread_join(cars[i], NULL);
    }

    // Clean
    pthread_mutex_destroy(&mutexBridge);
    pthread_mutex_destroy(&queue.mutex);
    pthread_cond_destroy(&queue.condition);
    free(cars);

    return 0;
}

void *car_thread( void *ptr ){
    int id = (int)ptr;
    int direction = rand()%2; //Random spawn | 0->cityA | 1->CityB

    if(direction == 0){
        bridge.aCityCount++;
        bridge.aCityTab[id] = 1;
    }
    else{
        bridge.bCityCount++;
        bridge.bCityTab[id] = 1;
    }

    while(1){
        // Time in city
        city();
        // Going from city A to B
        if(direction == RIGHT){
            bridge.aCityCount--;
            bridge.aCityTab[id] = -1;
            bridge.aWaitCount++;
            bridge.aWaitTab[id] = 1;
            display(id,NONE);
            enqueue(&queue, pthread_self());
            while (1) {
                pthread_t item = dequeue(&queue);
                if ((int)item == (int)pthread_self()) {
                    pthread_mutex_lock(&mutexBridge);
                    popqueue(&queue);
                    break;
                }
                usleep(1000);
            }   
            //Crossing bridge
            bridge.aWaitCount--;
            bridge.aWaitTab[id] = -1;
            display(id,RIGHT);
            // Simulate time on the bridge
            usleep(100000);
            bridge.bCityCount++;
            bridge.bCityTab[id] = 1;
            display(id,NONE);
            pthread_mutex_unlock(&mutexBridge);
            direction = 1;
        }
        //Going from city B to A
        else{
            bridge.bCityCount--;
            bridge.bCityTab[id] = -1;
            bridge.bWaitCount++;
            bridge.bWaitTab[id] = 1;
            display(id,NONE);

            enqueue(&queue, pthread_self());
            while (1) {
                pthread_t item = dequeue(&queue);
                if ((int)item == (int)pthread_self()) {
                    pthread_mutex_lock(&mutexBridge);
                    popqueue(&queue);
                    break;
                }
            }   
            //Crossing bridge
            bridge.bWaitCount--;
            bridge.bWaitTab[id] = -1;
            display(id,LEFT);
            // Simulate time on the bridge
            usleep(100000);
            bridge.aCityCount++;
            bridge.aCityTab[id] = 1;
            display(id,NONE);
            pthread_mutex_unlock(&mutexBridge);
            direction =0;
        }
    }
};

void city() {
    int sleepTime = rand() % 5 + 1;
    sleep(sleepTime);
}

void display(int id,int direction){
    if(!infoFlag){
        switch (direction){
        case RIGHT:
            printf("A-%d %d <-- [>> %d >>] --> %d %d-B\n\n", bridge.aCityCount, bridge.aWaitCount, id, bridge.bCityCount, bridge.bWaitCount);
            break;
        case LEFT:
            printf("A-%d %d <-- [<< %d <<] --> %d %d-B\n\n", bridge.aCityCount, bridge.aWaitCount, id, bridge.bCityCount, bridge.bWaitCount);
            break;
        case NONE:
            printf("A-%d %d <-- [-- - --] --> %d %d-B\n\n", bridge.aCityCount, bridge.aWaitCount, bridge.bCityCount, bridge.bWaitCount);
            break;
        }
    }
    else{
        printf("A-%d(",bridge.aCityCount);
        for(int i=0;i<N;i++){
            if(bridge.aCityTab[i]!=-1) printf("%d ",i);
        }
        printf(") %d(",bridge.aWaitCount);
        for(int i=0;i<N;i++){
            if(bridge.aWaitTab[i]!=-1) printf("%d ",i);
        }

        switch (direction){
        case RIGHT:
            printf(") <-- [>> %d >>] --> %d(",id,bridge.bWaitCount);
            break;
        case LEFT:
            printf(") <-- [<< %d <<] --> %d(",id,bridge.bWaitCount);
            break;
        case NONE:
            printf(") <-- [-- - --] --> %d(",bridge.bWaitCount);
            break;
        }
        for(int i=0;i<N;i++){
            if(bridge.bWaitTab[i]!=-1) printf("%d ",i);
        }
        printf(") %d(",bridge.bCityCount);
        for(int i=0;i<N;i++){
            if(bridge.bCityTab[i]!=-1) printf("%d ",i);
        }
        printf(")-B\n\n");
    }
}