#ifndef BUFF_H
#define BUFF_H

#define BUFFER_SIZE 10
#define PRODUCER_COUNT 2
#define PRODUCE_COUNT 20

#define SEM_MUTEX 0
#define SEM_EMPTY 1
#define SEM_FULL 2

typedef struct{
    int front;
    int rear;
    int size;
    int count;
    int data[BUFFER_SIZE]; 
    int tp;
    int tc;
} Queue;

void initQ(Queue *q, int tp);
int isEmptyQ(Queue *q);
int isFullQ(Queue *q);
void addQ(Queue *q, int item);
int deleteQ(Queue *q);
int frontQ(Queue *q);

#endif
