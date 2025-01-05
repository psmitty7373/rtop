#ifndef _SHARED_H
#define _SHARED_H

#define SHARED_BUFFER_SIZE 65535

typedef struct {
    char buffer[SHARED_BUFFER_SIZE];         // Adjust size based on your data
    bool data_available;       // Flag to indicate if new data is available
    pthread_mutex_t mutex;     // Mutex for synchronization
    pthread_cond_t cond;       // Condition variable for notification
} SharedBuffer;

#endif