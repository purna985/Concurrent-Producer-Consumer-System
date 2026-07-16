#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>
#include "buff.h"

union semun{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};


void p(int semid,Queue *q,int id);
void c(int semid,Queue *q);
int init_semaphores(int semid);
void cleanup(int semid,int shmid,Queue *q);

int init_semaphores(int semid) {
    union semun arg;

    arg.val=1;
    if (semctl(semid,SEM_MUTEX,SETVAL, arg) == -1) {
        perror("error");
        return -1;
    }

    arg.val=BUFFER_SIZE;
    if (semctl(semid,SEM_EMPTY,SETVAL, arg) == -1) {
        perror("error");
        return -1;
    }

    arg.val=0;
    if (semctl(semid,SEM_FULL,SETVAL, arg) == -1) {
        perror("error");
        return -1;
    }
    return 0;
}

void cleanup(int semid,int shmid,Queue *q) {
    if (q != (void *)-1)
        shmdt(q);
    if (shmid != -1)
        shmctl(shmid,IPC_RMID,NULL);
    if (semid != -1)
        semctl(semid,0,IPC_RMID);
}

int main(void) {
    int shmid = -1;
    Queue *q = (void *)-1;
    int semid = -1;
    int tp = PRODUCER_COUNT*PRODUCE_COUNT;
    pid_t p1, p2, c1;
    int status;

    shmid = shmget(IPC_PRIVATE,sizeof(Queue),IPC_CREAT|0600);
    if (shmid == -1) {
        perror("error");
        exit(1);
    }

    q = (Queue *)shmat(shmid, NULL, 0);
    if (q == (void *)-1) {
        perror("error");
        shmctl(shmid, IPC_RMID,NULL);
        exit(1);
    }

    initQ(q,tp);

    semid = semget(IPC_PRIVATE, 3, IPC_CREAT|0600);
    if (semid == -1) {
        perror("error");
        cleanup(semid, shmid, q);
        exit(1);
    }

    if (init_semaphores(semid) == -1) {
        cleanup(semid, shmid, q);
        exit(1);
    }

    printf("Parent (pid=%d):shm id=%d,sem id=%d\n",getpid(), shmid, semid);
    printf("Total to produce: %d items (%d producers * %d each)\n\n",tp, PRODUCER_COUNT, PRODUCE_COUNT);

    p1 =fork();
    if (p1<0){
        perror("error");
        cleanup(semid, shmid, q);
        exit(1);
    }
    if (p1==0){
        p(semid, q, 1);
        _exit(0);
    }

    p2 =fork();
    if (p2<0){
        perror("error");
        cleanup(semid, shmid, q);
        exit(1);
    }
    if (p2==0){
        p(semid, q, 2);
        _exit(0);
    }

    c1 = fork();
    if (c1 <0){
        perror("error");
        cleanup(semid, shmid, q);
        exit(1);
    }
    if (c1 ==0){
        c(semid, q);
        _exit(0);
    }

    while (wait(&status) > 0)
        ;

    printf("\nParent: all children finished. total_consumed = %d\n",q->tc);

    cleanup(semid, shmid, q);
    return 0;
}

void p(int semid, Queue *q, int id) {
    int i;
    srand(time(NULL)^(getpid()<<16));

    for (i = 0; i < PRODUCE_COUNT; ++i) {
        int item = id * 100 + i;

        struct sembuf ops_wait[2] = {
            { SEM_EMPTY, -1, 0 },
            { SEM_MUTEX, -1, 0 }
        };
        if (semop(semid, ops_wait, 2) == -1) {
            perror("semop producer wait");
            _exit(1);
        }

        addQ(q, item);
        printf("Producer %d (pid=%d) produced %d\n",id, getpid(), item);
        fflush(stdout);

        struct sembuf ops_signal[2] = {
            { SEM_MUTEX, 1, 0 },
            { SEM_FULL,  1, 0 }
        };
        if (semop(semid, ops_signal, 2) == -1) {
            perror("error");
            _exit(1);
        }

        usleep((rand()%200+100)*1000);
    }

    printf("Producer %d (pid=%d) Finished\n",id,getpid());
    fflush(stdout);

    shmdt(q);
}

void c(int semid, Queue *q) {
    srand(time(NULL)^(getpid()<<8));

    while (1) {
        struct sembuf ops_wait[2] = {
            { SEM_FULL,  -1, 0 },
            { SEM_MUTEX, -1, 0 }
        };
        if (semop(semid, ops_wait, 2)==-1) {
            perror("error");
            _exit(1);
        }

        int item = deleteQ(q);
        int done = (q->tc >= q->tp);

        printf("\tConsumer (pid=%d) consumed %d (total=%d)\n",getpid(), item,q->tc);
        fflush(stdout);

        struct sembuf ops_signal[2] = {
            {SEM_MUTEX, 1, 0},
            {SEM_EMPTY, 1, 0}
        };
        if (semop(semid,ops_signal,2)==-1) {
            perror("error");
            _exit(1);
        }

        if (done)
            break;

        usleep((rand()%300+100)*1000);
    }

    printf("\tConsumer (pid=%d) Finished\n",getpid());
    fflush(stdout);

    shmdt(q);
}
