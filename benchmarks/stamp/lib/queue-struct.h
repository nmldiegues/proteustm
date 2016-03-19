#ifndef QUEUE_H
#define QUEUE_H 1

typedef struct queue queue_t;

struct queue {
    long pop; /* points before element to pop */
    long push;
    long capacity;
    void** elements;
};

enum config {
    QUEUE_GROWTH_FACTOR = 2,
};

#endif
