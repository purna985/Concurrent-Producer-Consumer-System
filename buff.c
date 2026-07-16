#include <stdio.h>
#include "buff.h"
void initQ(Queue *q,int tp) {
    int i;
    q->front =0;
    q->rear =0;
    q->size =BUFFER_SIZE;
    q->count = 0;

    q->tp= tp;
    q->tc = 0;

    for (i = 0; i<BUFFER_SIZE;++i)
        q->data[i]=0;
}

int isEmptyQ(Queue *q) {
    return (q->count==0);
}

int isFullQ(Queue *q) {
    return (q->count==q->size);
}

void addQ(Queue *q, int item) {
    q->data[q->rear]=item;
    q->rear = (q->rear+1)%q->size;
    q->count++;
}

int deleteQ(Queue *q) {
    int item = q->data[q->front];
    q->front = (q->front+1)%q->size;
    q->count--;
    q->tc++;
    return item;
}

int frontQ(Queue *q) {
    if (isEmptyQ(q))
        return -1;
    return q->data[q->front];
}
