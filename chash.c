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
int g_max_priority = 0;

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
            console_message("%s not found.\n", cmd->name);
        }
    } else if (strcmp(cmd->command, "print") == 0) {
        // Check if this is the final print (highest priority)
        extern int g_max_priority;
        int is_final = (cmd->priority == g_max_priority) ? 1 : 0;
        print_table(cmd->priority, is_final);
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
        }
        
        commands[idx++] = cmd;
    }
    
    fclose(cmd_file);
    
    // Sort commands by priority
    qsort(commands, command_count, sizeof(Command *), compare_commands);
    
    // Find the highest priority and mark the last PRINT command as final
    // (commands will be freed in threads)
    int max_priority = 0;
    int last_was_print = 0;
    for (int i = 0; i < command_count; i++) {
        if (commands[i]->priority > max_priority) {
            max_priority = commands[i]->priority;
        }
    }
    // Commands are sorted by priority, so the last one has highest priority
    // Mark the last PRINT command so it knows to not print trailing newline
    if (command_count > 0 && strcmp(commands[command_count - 1]->command, "print") == 0) {
        last_was_print = 1;
        // Store max_priority in a way that the thread can access it
        // Actually, we'll use a global variable or check priority == max_priority in thread
    }
    
    // Store max_priority globally so threads can check if they're the final print
    g_max_priority = max_priority;
    
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
    // (unless last command was already PRINT to match expected output format)
    if (!last_was_print) {
        print_table(max_priority + 1, 1); // Final print, no trailing newline
    } else {
        // Last command was PRINT, but we need to ensure last line has no newline
        // Re-print the last record without newline
        // Actually, we can't do this easily. Let's just call print_table with is_final=1
        // But wait, the print already happened. We need a different approach.
        // Actually, the expected output shows the last PRINT command's output ends without newline
        // So we need to modify the print_table to know if it's the final one
    }
    
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
