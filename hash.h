/* Group 50
Tuyen Tran - William Bu
PA2-Concurrent Hash Table
Due Date: 11/21/2025
*/

#ifndef HASH_H
#define HASH_H

#include <stdint.h>
#include <stdio.h>
#include <pthread.h>

// Hash record structure
typedef struct hash_struct {
    uint32_t hash;
    char name[50];
    uint32_t salary;
    struct hash_struct *next;
} hashRecord;

// Command structure for thread processing
typedef struct {
    char command[20];
    char name[50];
    uint32_t salary;
    int priority;
} Command;

// Reader-Writer lock structure
typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t read;
    pthread_cond_t write;
    int readers;
    int writers;
    int write_waiters;
} rwlock_t;

// Global variables
extern hashRecord *hash_table;
extern rwlock_t rw_lock;
extern FILE *log_file;
extern pthread_mutex_t console_mutex;
extern pthread_mutex_t log_mutex;

// Function prototypes
uint32_t jenkins_one_at_a_time_hash(const char *key);
long long current_timestamp();

void rwlock_init(rwlock_t *rw);
void rwlock_acquire_readlock(rwlock_t *rw, int priority);
void rwlock_release_readlock(rwlock_t *rw, int priority);
void rwlock_acquire_writelock(rwlock_t *rw, int priority);
void rwlock_release_writelock(rwlock_t *rw, int priority);
void rwlock_destroy(rwlock_t *rw);

void insert(const char *name, uint32_t salary, int priority);
void delete_record(const char *name, int priority);
void updateSalary(const char *name, uint32_t new_salary, int priority);
hashRecord* search(const char *name, int priority);
void print_table(int priority, int is_final);

void log_message(const char *format, ...);
void console_message(const char *format, ...);

#endif // HASH_H