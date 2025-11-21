/* Group 50
Tuyen Tran - William Bu
PA2-Concurrent Hash Table
Due Date: 11/21/2025
*/

#include "hash.h"
#include <sys/time.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// Jenkins one-at-a-time hash function
uint32_t jenkins_one_at_a_time_hash(const char *key) {
    uint32_t hash = 0;
    size_t i;
    
    for (i = 0; key[i] != '\0'; i++) {
        hash += (unsigned char)key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

// Get current timestamp in microseconds
long long current_timestamp() {
    struct timeval te;
    gettimeofday(&te, NULL);
    long long microseconds = (te.tv_sec * 1000000LL) + te.tv_usec;
    return microseconds;
}

// Thread-safe logging to hash.log
void log_message(const char *format, ...) {
    pthread_mutex_lock(&log_mutex);
    
    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);
    fflush(log_file);
    
    pthread_mutex_unlock(&log_mutex);
}

// Thread-safe console output
void console_message(const char *format, ...) {
    pthread_mutex_lock(&console_mutex);
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    fflush(stdout);
    
    pthread_mutex_unlock(&console_mutex);
}
