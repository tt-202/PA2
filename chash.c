/* Group 50
Tuyen Tran - William Bu
PA2-Concurrent Hash Table
Due Date: 11/21/2025
*/


#include "hash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

// Global variables
hashRecord *hash_table = NULL;
rwlock_t rw_lock;
FILE *log_file = NULL;
pthread_mutex_t console_mutex;
pthread_mutex_t log_mutex;

// Priority synchronization
pthread_mutex_t priority_mutex;
pthread_cond_t priority_cond;
int current_priority = 0;
int total_commands = 0;
int threads_waiting = 0;
int all_threads_ready = 0;

// Lock statistics
int lock_acquisitions = 0;
int lock_releases = 0;

// Thread function to process commands
void* process_command(void *arg) {
    Command *cmd = (Command *)arg;
    
    // Wait for our turn based on priority
    pthread_mutex_lock(&priority_mutex);
    log_message("%lld: THREAD %d WAITING FOR MY TURN\n", current_timestamp(), cmd->priority);
    threads_waiting++;
    
    // Notify others that we've incremented (helps Thread 0 wake up when count reaches total)
    pthread_cond_broadcast(&priority_cond);
    
    // Check if it's our turn
    if (current_priority == cmd->priority) {
        // We can proceed, but log AWAKENED and wait for all threads to be ready
        log_message("%lld: THREAD %d AWAKENED FOR WORK\n", current_timestamp(), cmd->priority);
        
        // Wait for all threads to log WAITING
        while (threads_waiting < total_commands) {
            pthread_cond_wait(&priority_cond, &priority_mutex);
        }
        all_threads_ready = 1;
        pthread_cond_broadcast(&priority_cond);  // Wake everyone to check all_threads_ready
    } else {
        // Wait for our turn
        while (current_priority != cmd->priority || !all_threads_ready) {
            pthread_cond_wait(&priority_cond, &priority_mutex);
        }
        log_message("%lld: THREAD %d AWAKENED FOR WORK\n", current_timestamp(), cmd->priority);
    }
    
    pthread_mutex_unlock(&priority_mutex);
    
    // Log and execute the command
    if (strcmp(cmd->command, "insert") == 0) {
        uint32_t hash = jenkins_one_at_a_time_hash(cmd->name);
        log_message("%lld: THREAD %d INSERT,%u,%s,%u\n", 
                    current_timestamp(), cmd->priority, hash, cmd->name, cmd->salary);
        insert(cmd->name, cmd->salary, cmd->priority);
    } else if (strcmp(cmd->command, "delete") == 0) {
        uint32_t hash = jenkins_one_at_a_time_hash(cmd->name);
        log_message("%lld: THREAD %d DELETE,%u,%s\n", 
                    current_timestamp(), cmd->priority, hash, cmd->name);
        delete_record(cmd->name, cmd->priority);
    } else if (strcmp(cmd->command, "update") == 0) {
        uint32_t hash = jenkins_one_at_a_time_hash(cmd->name);
        log_message("%lld: THREAD %d UPDATE,%u,%s,%u\n", 
                    current_timestamp(), cmd->priority, hash, cmd->name, cmd->salary);
        updateSalary(cmd->name, cmd->salary, cmd->priority);
    } else if (strcmp(cmd->command, "search") == 0) {
        uint32_t hash = jenkins_one_at_a_time_hash(cmd->name);
        log_message("%lld: THREAD %d SEARCH,%u,%s\n", 
                    current_timestamp(), cmd->priority, hash, cmd->name);
        hashRecord *result = search(cmd->name, cmd->priority);
        if (result != NULL) {
            console_message("Found: %u,%s,%u\n", result->hash, result->name, result->salary);
            free(result);
        } else {
            console_message("%s not found.\n", cmd->name);
        }
    } else if (strcmp(cmd->command, "print") == 0) {
        log_message("%lld: THREAD %d PRINT\n", current_timestamp(), cmd->priority);
        print_table(cmd->priority);
    }
    
    // Signal next thread
    pthread_mutex_lock(&priority_mutex);
    current_priority++;
    pthread_cond_broadcast(&priority_cond);  // Wake threads: next priority OR thread 0 checking count
    pthread_mutex_unlock(&priority_mutex);
    
    free(cmd);
    return NULL;
}

// Comparison function for sorting commands by priority
int compare_commands(const void *a, const void *b) {
    Command *cmdA = *(Command **)a;
    Command *cmdB = *(Command **)b;
    return cmdA->priority - cmdB->priority;
}

int main() {
    // Initialize mutexes and locks
    pthread_mutex_init(&console_mutex, NULL);
    pthread_mutex_init(&log_mutex, NULL);
    pthread_mutex_init(&priority_mutex, NULL);
    pthread_cond_init(&priority_cond, NULL);
    rwlock_init(&rw_lock);
    
    // Open log file
    log_file = fopen("hash.log", "w");
    if (log_file == NULL) {
        fprintf(stderr, "Error: Cannot open hash.log for writing\n");
        return 1;
    }
    
    // Read commands from file
    FILE *cmd_file = fopen("commands.txt", "r");
    if (cmd_file == NULL) {
        fprintf(stderr, "Error: Cannot open commands.txt\n");
        fclose(log_file);
        return 1;
    }
    
    // Count commands (excluding threads command) and allocate array
    int command_count = 0;
    char line[256];
    while (fgets(line, sizeof(line), cmd_file) != NULL) {
        if (strlen(line) > 1) {
            // Check if it's a threads command
            char line_check[256];
            strncpy(line_check, line, sizeof(line_check) - 1);
            line_check[sizeof(line_check) - 1] = '\0';
            line_check[strcspn(line_check, "\n")] = 0;
            char *token_check = strtok(line_check, ",");
            if (token_check != NULL && strcmp(token_check, "threads") != 0) {
                command_count++;
            }
        }
    }
    
    rewind(cmd_file);
    
    Command **commands = (Command **)malloc(command_count * sizeof(Command *));
    int idx = 0;
    
    // Parse commands
    while (fgets(line, sizeof(line), cmd_file) != NULL) {
        if (strlen(line) <= 1) continue;
        
        Command *cmd = (Command *)malloc(sizeof(Command));
        char *token;
        
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        
        // Parse command type
        token = strtok(line, ",");
        if (token == NULL) {
            free(cmd);
            continue;
        }
        strncpy(cmd->command, token, 19);
        cmd->command[19] = '\0';
        
        // Skip threads command
        if (strcmp(cmd->command, "threads") == 0) {
            free(cmd);
            continue;
        }
        
        if (strcmp(cmd->command, "insert") == 0) {
            // Parse name
            token = strtok(NULL, ",");
            if (token == NULL) {
                free(cmd);
                continue;
            }
            strncpy(cmd->name, token, 49);
            cmd->name[49] = '\0';
            
            // Parse salary
            token = strtok(NULL, ",");
            if (token == NULL) {
                free(cmd);
                continue;
            }
            cmd->salary = atoi(token);
            
            // Parse priority
            token = strtok(NULL, ",");
            if (token == NULL) {
                free(cmd);
                continue;
            }
            cmd->priority = atoi(token);
            
        } else if (strcmp(cmd->command, "delete") == 0 || strcmp(cmd->command, "search") == 0) {
            // Parse name (first token after command)
            token = strtok(NULL, ",");
            if (token == NULL) {
                free(cmd);
                continue;
            }
            strncpy(cmd->name, token, 49);
            cmd->name[49] = '\0';
            
            cmd->salary = 0;
            
            // Parse priority - skip middle tokens, take the last one
            char *last_token = NULL;
            token = strtok(NULL, ",");
            while (token != NULL) {
                last_token = token;
                token = strtok(NULL, ",");
            }
            if (last_token == NULL) {
                free(cmd);
                continue;
            }
            cmd->priority = atoi(last_token);
            
        } else if (strcmp(cmd->command, "update") == 0) {
            // Parse name
            token = strtok(NULL, ",");
            if (token == NULL) {
                free(cmd);
                continue;
            }
            strncpy(cmd->name, token, 49);
            cmd->name[49] = '\0';
            
            // Parse new salary value (second token after command)
            token = strtok(NULL, ",");
            if (token == NULL) {
                free(cmd);
                continue;
            }
            cmd->salary = atoi(token);
            
            // Parse priority - skip middle tokens, take the last one
            char *last_token = NULL;
            token = strtok(NULL, ",");
            while (token != NULL) {
                last_token = token;
                token = strtok(NULL, ",");
            }
            if (last_token != NULL) {
                cmd->priority = atoi(last_token);
            } else {
                cmd->priority = 0;
            }
            
        } else if (strcmp(cmd->command, "print") == 0) {
            // Parse priority - skip middle tokens, take the last one
            char *last_token = NULL;
            token = strtok(NULL, ",");
            while (token != NULL) {
                last_token = token;
                token = strtok(NULL, ",");
            }
            if (last_token == NULL) {
                free(cmd);
                continue;
            }
            cmd->priority = atoi(last_token);
            cmd->name[0] = '\0';
            cmd->salary = 0;
        } else {
            // Unknown command, skip it
            free(cmd);
            continue;
        }
        
        commands[idx++] = cmd;
    }
    
    fclose(cmd_file);
    
    // Update command_count to actual number of parsed commands
    command_count = idx;
    
    // Sort commands by priority
    qsort(commands, command_count, sizeof(Command *), compare_commands);
    
    // Set total_commands for barrier
    total_commands = command_count;
    
    // Create and execute threads
    pthread_t *threads = (pthread_t *)malloc(command_count * sizeof(pthread_t));
    
    for (int i = 0; i < command_count; i++) {
        pthread_create(&threads[i], NULL, process_command, commands[i]);
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < command_count; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Write statistics and final table to log file
    fprintf(log_file, "Number of lock acquisitions: %d\n", lock_acquisitions);
    fprintf(log_file, "Number of lock releases: %d\n", lock_releases);
    fprintf(log_file, "Final Table:\n");
    
    // Count records
    int count = 0;
    hashRecord *curr = hash_table;
    while (curr != NULL) {
        count++;
        curr = curr->next;
    }
    
    if (count > 0) {
        // Copy records to array for sorting
        hashRecord **records = (hashRecord **)malloc(count * sizeof(hashRecord *));
        curr = hash_table;
        int i = 0;
        while (curr != NULL) {
            records[i++] = curr;
            curr = curr->next;
        }
        
        // Bubble sort by hash
        for (i = 0; i < count - 1; i++) {
            for (int j = 0; j < count - i - 1; j++) {
                if (records[j]->hash > records[j + 1]->hash) {
                    hashRecord *temp = records[j];
                    records[j] = records[j + 1];
                    records[j + 1] = temp;
                }
            }
        }
        
        // Write to log file
        for (i = 0; i < count; i++) {
            fprintf(log_file, "%u,%s,%u\n", records[i]->hash, 
                   records[i]->name, records[i]->salary);
        }
        
        // Also print final database to console
        console_message("Current Database:\n");
        for (i = 0; i < count; i++) {
            console_message("%u,%s,%u\n", records[i]->hash, 
                           records[i]->name, records[i]->salary);
        }
        
        free(records);
    } else {
        // Empty database
        console_message("Current Database:\n");
    }
    
    // Cleanup
    free(threads);
    free(commands);
    
    // Free hash table
    curr = hash_table;
    while (curr != NULL) {
        hashRecord *next = curr->next;
        free(curr);
        curr = next;
    }
    
    // Cleanup mutexes and locks
    rwlock_destroy(&rw_lock);
    pthread_mutex_destroy(&console_mutex);
    pthread_mutex_destroy(&log_mutex);
    pthread_mutex_destroy(&priority_mutex);
    pthread_cond_destroy(&priority_cond);
    
    fclose(log_file);
    
    return 0;
}
