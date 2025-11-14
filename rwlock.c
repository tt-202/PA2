/* Group 50
Tuyen Tran - William Bu
PA2-Concurrent Hash Table
Due Date: 11/21/2025
*/

#include "hash.h"
#include <stdio.h>

// Initialize reader-writer lock
void rwlock_init(rwlock_t *rw) {
    pthread_mutex_init(&rw->lock, NULL);
    pthread_cond_init(&rw->read, NULL);
    pthread_cond_init(&rw->write, NULL);
    rw->readers = 0;
    rw->writers = 0;
    rw->write_waiters = 0;
}

// Acquire read lock
void rwlock_acquire_readlock(rwlock_t *rw, int priority) {
    pthread_mutex_lock(&rw->lock);
    
    log_message("%lld: THREAD %d WAITING FOR MY TURN\n", current_timestamp(), priority);
    
    // Wait while there are writers or write waiters (writer preference)
    while (rw->writers > 0 || rw->write_waiters > 0) {
        pthread_cond_wait(&rw->read, &rw->lock);
    }
    
    log_message("%lld: THREAD %d AWAKENED FOR WORK\n", current_timestamp(), priority);
    
    rw->readers++;
    
    log_message("%lld: THREAD %d READ LOCK ACQUIRED\n", current_timestamp(), priority);
    
    pthread_mutex_unlock(&rw->lock);
}

// Release read lock
void rwlock_release_readlock(rwlock_t *rw, int priority) {
    pthread_mutex_lock(&rw->lock);
    
    rw->readers--;
    
    log_message("%lld: THREAD %d READ LOCK RELEASED\n", current_timestamp(), priority);
    
    // If no more readers, signal waiting writers
    if (rw->readers == 0) {
        pthread_cond_signal(&rw->write);
    }
    
    pthread_mutex_unlock(&rw->lock);
}

// Acquire write lock
void rwlock_acquire_writelock(rwlock_t *rw, int priority) {
    pthread_mutex_lock(&rw->lock);
    
    log_message("%lld: THREAD %d WAITING FOR MY TURN\n", current_timestamp(), priority);
    
    rw->write_waiters++;
    
    // Wait while there are readers or writers
    while (rw->readers > 0 || rw->writers > 0) {
        pthread_cond_wait(&rw->write, &rw->lock);
    }
    
    rw->write_waiters--;
    rw->writers++;
    
    log_message("%lld: THREAD %d AWAKENED FOR WORK\n", current_timestamp(), priority);
    log_message("%lld: THREAD %d WRITE LOCK ACQUIRED\n", current_timestamp(), priority);
    
    pthread_mutex_unlock(&rw->lock);
}

// Release write lock
void rwlock_release_writelock(rwlock_t *rw, int priority) {
    pthread_mutex_lock(&rw->lock);
    
    rw->writers--;
    
    log_message("%lld: THREAD %d WRITE LOCK RELEASED\n", current_timestamp(), priority);
    
    // Prioritize writers (writer preference)
    if (rw->write_waiters > 0) {
        pthread_cond_signal(&rw->write);
    } else {
        pthread_cond_broadcast(&rw->read);
    }
    
    pthread_mutex_unlock(&rw->lock);
}

// Destroy reader-writer lock
void rwlock_destroy(rwlock_t *rw) {
    pthread_mutex_destroy(&rw->lock);
    pthread_cond_destroy(&rw->read);
    pthread_cond_destroy(&rw->write);
}
