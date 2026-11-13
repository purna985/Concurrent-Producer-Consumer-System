# Concurrent Producer-Consumer System

A solution to the classic producer-consumer problem in C, using System V shared memory and System V semaphores. Two producer processes and one consumer process share a bounded circular buffer, and semaphores keep them in sync so nothing gets overwritten, read twice, or lost.

I built this to get hands-on with the operating systems concepts behind it: inter-process communication, shared memory, process synchronization and avoiding race conditions.

## The problem

Producers generate items and put them in a buffer. The consumer takes items out. Since the buffer is shared between separate processes, a few things can go wrong without synchronization:

- two producers write to the same slot at the same time and one item gets lost
- a producer writes into a buffer that's already full
- the consumer reads from an empty buffer
- the `in` / `out` indices get corrupted because two processes update them at once

## How it works

The buffer lives in a System V shared memory segment so all three processes can see it. It's a fixed-size circular array with an `in` index (where the next item goes) and an `out` index (where the next item is read from), both wrapping around with `% BUFFER_SIZE`.

Three semaphores handle synchronization:

| Semaphore | Initial value | Purpose |
|---|---|---|
| `mutex` | 1 | only one process touches the buffer at a time |
| `empty` | `BUFFER_SIZE` | counts free slots, producers wait on it |
| `full` | 0 | counts filled slots, the consumer waits on it |

A producer does this for each item:

```c
down(empty);    // wait for a free slot
down(mutex);    // lock the buffer

buf->data[buf->in] = item;
buf->in = (buf->in + 1) % BUFFER_SIZE;

up(mutex);      // unlock
up(full);       // one more item available
```

The consumer does the mirror image: `down(full)`, `down(mutex)`, read from `out`, `up(mutex)`, `up(empty)`.

`down` and `up` are small wrappers around `semop()` that subtract or add 1. The order matters here. A process waits on `empty` / `full` before taking the `mutex`. If it locked the mutex first and then blocked on a full buffer, it would hold the lock forever and every other process would deadlock.

## System calls used

- `ftok()` to generate a shared IPC key
- `shmget()`, `shmat()`, `shmdt()`, `shmctl()` to create, attach, detach and remove the shared memory
- `semget()`, `semop()`, `semctl()` to create, operate on and remove the semaphore set

## Project structure

```
producer-consumer/
├── common.h      # shared buffer struct, keys, semaphore helpers
├── producer.c
├── consumer.c
└── README.md
```

## Building and running

You'll need Linux and gcc.

```bash
gcc producer.c -o producer
gcc consumer.c -o consumer
```

Start the consumer first, then the two producers, each in its own terminal:

```bash
./consumer
./producer 1
./producer 2
```

Each producer prints the items it adds, and the consumer prints what it takes out, so you can watch the processes interleave without any items being lost or duplicated.

## Cleaning up

System V IPC resources stick around in the kernel even after the processes exit, unless they're explicitly removed. If a run gets interrupted, check for leftovers with:

```bash
ipcs -m    # shared memory segments
ipcs -s    # semaphore sets
```

and remove them with:

```bash
ipcrm -m <shmid>
ipcrm -s <semid>
```

## Things I'd like to add

- Support any number of producers and consumers through command-line arguments
- Rewrite it with POSIX shared memory and semaphores (`shm_open`, `sem_open`) to compare the two APIs
- A pthreads version, to compare threads sharing memory against separate processes
- Measure throughput with different buffer sizes and numbers of producers

## Author

Chinthapally Purna Nanda Reddy, B.Tech Engineering Science, IIT Hyderabad
[GitHub](https://github.com/<your-username>) | [LinkedIn](https://www.linkedin.com/in/<your-profile>)
