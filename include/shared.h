#ifndef _SHARED_H
#define _SHARED_H

#define SHARED_BUFFER_SIZE 65535

typedef struct _SharedBuffer {
    char buffer[SHARED_BUFFER_SIZE];
    size_t size;
    bool data_available;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} SharedBuffer;

#endif