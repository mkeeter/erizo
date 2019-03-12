#include <pthread.h>

typedef struct platform_thread {
    pthread_t data;
} platform_thread_t;

typedef struct platform_mutex {
    pthread_mutex_t data;
} platform_mutex_t;

typedef struct platform_cond {
    pthread_cond_t data;
} platform_cond_t;
