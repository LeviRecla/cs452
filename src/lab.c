#include "lab.h"
#include <pthread.h>
#include <stdio.h>

/**
 * Definition of the queue structure
 */
struct queue {
    //Array to hold the data
    void **buffer;
    //Maximum capacity of the queue and number of elements        
    int capacity;            
    int size; 

    // Index of the front and rear elements                  
    int front;
    int rear;

    // Shutdown flag
    bool shutdown;

    // Mutex for thread synchronization
    pthread_mutex_t lock;

    // Condition variables for blocking when the queue is full and empty       
    pthread_cond_t not_full;  
    pthread_cond_t not_empty;
};

/**
 * Initialize a new queue
 * @param capacity the maximum capacity of the queue
 */
queue_t queue_init(int capacity) {
    if (capacity <= 0) {
        return NULL; // Invalid capacity
    }

    // Allocate memory for the queue structure
    queue_t q = malloc(sizeof(struct queue));
    if (!q) {
        perror("Failed to allocate queue");
        return NULL;
    }

    // Allocate memory for the buffer array
    q->buffer = malloc(sizeof(void *) * capacity);
    if (!q->buffer) {
        perror("Could not allocate buffer");
        free(q);
        return NULL;
    }

    // Initialize the queue structure
    q->capacity = capacity;
    q->size = 0;
    q->front = 0;
    q->rear = 0;
    q->shutdown = false;

    // Initialize the mutex for the queue
    if (pthread_mutex_init(&q->lock, NULL) != 0) {
        perror("Mutex was not initialized");
        //Free the buffer and the queue structure
        free(q->buffer);
        free(q);
        return NULL;
    }

    // Initialize the condition variables for the queue
    if (pthread_cond_init(&q->not_full, NULL) != 0 ||
        pthread_cond_init(&q->not_empty, NULL) != 0) {
        perror("Conditional initialization failed");
        // Destroy the mutex
        pthread_mutex_destroy(&q->lock);
        //Free the buffer and the queue structure
        free(q->buffer);
        free(q);
        return NULL;
    }

    return q;
}

/**
 * Frees all memory and related data signals all waiting threads.
 * @param q a queue to free
 */
void queue_destroy(queue_t q) {
    if (q == NULL) {
        return; // Nothing to destroy
    }

    pthread_mutex_lock(&q->lock);

    // Set shutdown to true if not already set
    if (!q->shutdown) {
        q->shutdown = true;
        pthread_cond_broadcast(&q->not_full);
        pthread_cond_broadcast(&q->not_empty);
    }

    pthread_mutex_unlock(&q->lock);

    // Destroy mutex and condition variables
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->not_full);
    pthread_cond_destroy(&q->not_empty);

    // Free the buffer array and the queue structure
    free(q->buffer);
    free(q);
}

/**
 * Adds an element to the back of the queue
 * @param q the queue
 * @param data the data to add
 */
void enqueue(queue_t q, void *data) {
    //Lock the queue
    pthread_mutex_lock(&q->lock);

//Wait while the queue is full and not shutdown
    while (q->size == q->capacity && !q->shutdown) {
        pthread_cond_wait(&q->not_full, &q->lock);
    }

    if (q->shutdown) {
        pthread_mutex_unlock(&q->lock);
        return; // Or handle shutdown appropriately
    }

    //add item to the queue
    q->buffer[q->rear] = data;
    q->rear = (q->rear + 1) % q->capacity;
    q->size++;

    //Signal any waiting threads
    pthread_cond_signal(&q->not_empty);
    //Unlock the queue mutex
    pthread_mutex_unlock(&q->lock);
}

/**
 * Removes the first element in the queue.
 * @param q the queue
 */
void *dequeue(queue_t q) {
    pthread_mutex_lock(&q->lock);

    // Wait while the queue is empty and not shutdown
    while (q->size == 0 && !q->shutdown) {
        pthread_cond_wait(&q->not_empty, &q->lock);
    }

    // If queue is empty and shutdown, return NULL
    if (q->size == 0 && q->shutdown) {
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }

    // Remove item from the queue
    void *data = q->buffer[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;

    // Signal any waiting threads
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);
    return data;
}




/**
 * Set the shutdown flag in the queue so all threads can
 * complete and exit properly
 * @param q The queue
 */
void queue_shutdown(queue_t q) {
    //Lock the queue
    pthread_mutex_lock(&q->lock);
    q->shutdown = true;
    //Wake up all threads waiting on the not_full and not_empty condition variables
    pthread_cond_broadcast(&q->not_full);
    pthread_cond_broadcast(&q->not_empty);
    //Unlock the queue mutex
    pthread_mutex_unlock(&q->lock);
}


/**
 * Returns true is the queue is empty
 * @param q the queue
 */
bool is_empty(queue_t q) {
    bool empty;
    pthread_mutex_lock(&q->lock);
    empty = (q->size == 0);
    pthread_mutex_unlock(&q->lock);
    return empty;
}

/**
 * Returns true if the queue is shutdown
 * @param q the queue
 */
bool is_shutdown(queue_t q) {
    bool shutdown_status;
    pthread_mutex_lock(&q->lock);
    shutdown_status = q->shutdown;
    pthread_mutex_unlock(&q->lock);
    return shutdown_status;
}

