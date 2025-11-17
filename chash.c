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

// Thread function to process commands
void* process_command(void *arg) {
    Command *cmd = (Command *)arg;
    
    if (strcmp(cmd->command, "insert") == 0) {
        insert(cmd->name, cmd->salary, cmd->priority);
    } else if (strcmp(cmd->command, "delete") == 0) {
        delete_record(cmd->name, cmd->priority);
    } else if (strcmp(cmd->command, "update") == 0) {
        updateSalary(cmd->name, cmd->salary, cmd->priority);
    } else if (strcmp(cmd->command, "search") == 0) {
        hashRecord *result = search(cmd->name, cmd->priority);
        if (result != NULL) {
            console_message("Found: %u,%s,%u\n", result->hash, result->name, result->salary);
            free(result);
        } else {
            console_message("Not Found:  %s not found.\n", cmd->name);
        }
    } else if (strcmp(cmd->command, "print") == 0) {
        print_table(cmd->priority);
    }
    
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
    
    // Count commands and allocate array
    int command_count = 0;
    char line[256];
    while (fgets(line, sizeof(line), cmd_file) != NULL) {
        if (strlen(line) > 1) command_count++;
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
            // Parse name
            token = strtok(NULL, ",");
            if (token == NULL) {
                free(cmd);
                continue;
            }
            strncpy(cmd->name, token, 49);
            cmd->name[49] = '\0';
            
            cmd->salary = 0;
            
            // Parse priority
            token = strtok(NULL, ",");
            if (token == NULL) {
                free(cmd);
                continue;
            }
            cmd->priority = atoi(token);
            
        } else if (strcmp(cmd->command, "update") == 0) {
            // Parse name
            token = strtok(NULL, ",");
            if (token == NULL) {
                free(cmd);
                continue;
            }
            strncpy(cmd->name, token, 49);
            cmd->name[49] = '\0';
            
            // Parse new salary value
            token = strtok(NULL, ",");
            if (token == NULL) {
                free(cmd);
                continue;
            }
            cmd->salary = atoi(token);
            
            // Update doesn't have priority in command format, use 0 as default
            // But check if there's an optional priority parameter
            token = strtok(NULL, ",");
            if (token != NULL) {
                cmd->priority = atoi(token);
            } else {
                cmd->priority = 0;
            }
            
        } else if (strcmp(cmd->command, "print") == 0) {
            // Parse priority
            token = strtok(NULL, ",");
            if (token == NULL) {
                free(cmd);
                continue;
            }
            cmd->priority = atoi(token);
            cmd->name[0] = '\0';
            cmd->salary = 0;
        }
        
        commands[idx++] = cmd;
    }
    
    fclose(cmd_file);
    
    // Sort commands by priority
    qsort(commands, command_count, sizeof(Command *), compare_commands);
    
    // Create and execute threads in priority order
    pthread_t *threads = (pthread_t *)malloc(command_count * sizeof(pthread_t));
    
    for (int i = 0; i < command_count; i++) {
        pthread_create(&threads[i], NULL, process_command, commands[i]);
        // Small delay to help ensure priority ordering
        usleep(1000);
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < command_count; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Always print final database state, even if last command was PRINT
    // Find the highest priority to use for final print
    int max_priority = 0;
    for (int i = 0; i < command_count; i++) {
        if (commands[i]->priority > max_priority) {
            max_priority = commands[i]->priority;
        }
    }
    print_table(max_priority + 1); // Use priority higher than any command
    
    // Cleanup
    free(threads);
    free(commands);
    
    // Free hash table
    hashRecord *current = hash_table;
    while (current != NULL) {
        hashRecord *next = current->next;
        free(current);
        current = next;
    }
    
    // Cleanup mutexes and locks
    rwlock_destroy(&rw_lock);
    pthread_mutex_destroy(&console_mutex);
    pthread_mutex_destroy(&log_mutex);
    
    fclose(log_file);
    
    return 0;
}
