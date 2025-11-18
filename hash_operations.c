/* Group 50
Tuyen Tran - William Bu
PA2-Concurrent Hash Table
Due Date: 11/21/2025
*/

#include "hash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Insert a record
void insert(const char *name, uint32_t salary, int priority) {
    uint32_t hash = jenkins_one_at_a_time_hash(name);
    
    rwlock_acquire_writelock(&rw_lock, priority);
    
    // Search for existing record
    hashRecord *current = hash_table;
    hashRecord *prev = NULL;
    
    while (current != NULL) {
        if (current->hash == hash) {
            // Duplicate entry found
            rwlock_release_writelock(&rw_lock, priority);
            console_message("Insert failed.  Entry %u is a duplicate.\n", hash);
            return;
        }
        prev = current;
        current = current->next;
    }
    
    // Create new record
    hashRecord *new_record = (hashRecord *)malloc(sizeof(hashRecord));
    new_record->hash = hash;
    strncpy(new_record->name, name, 49);
    new_record->name[49] = '\0';
    new_record->salary = salary;
    new_record->next = NULL;
    
    // Insert at the end
    if (hash_table == NULL) {
        hash_table = new_record;
    } else {
        prev->next = new_record;
    }
    
    rwlock_release_writelock(&rw_lock, priority);
    
    console_message("Inserted %u,%s,%u\n", hash, name, salary);
}

// Delete a record
void delete_record(const char *name, int priority) {
    uint32_t hash = jenkins_one_at_a_time_hash(name);
    
    rwlock_acquire_writelock(&rw_lock, priority);
    
    hashRecord *current = hash_table;
    hashRecord *prev = NULL;
    
    while (current != NULL) {
        if (current->hash == hash) {
            // Found the record to delete
            uint32_t del_hash = current->hash;
            char del_name[50];
            uint32_t del_salary = current->salary;
            strncpy(del_name, current->name, 49);
            del_name[49] = '\0';
            
            if (prev == NULL) {
                hash_table = current->next;
            } else {
                prev->next = current->next;
            }
            
            free(current);
            rwlock_release_writelock(&rw_lock, priority);
            
            console_message("Deleted record for %u,%s,%u\n", 
                          del_hash, del_name, del_salary);
            return;
        }
        prev = current;
        current = current->next;
    }
    
    rwlock_release_writelock(&rw_lock, priority);
    
    console_message("Entry %u not deleted.  Not in database.\n", hash);
}

// Search for a record
hashRecord* search(const char *name, int priority) {
    uint32_t hash = jenkins_one_at_a_time_hash(name);
    
    rwlock_acquire_readlock(&rw_lock, priority);
    
    hashRecord *current = hash_table;
    
    while (current != NULL) {
        if (current->hash == hash) {
            // Found the record - need to copy data before releasing lock
            hashRecord *result = (hashRecord *)malloc(sizeof(hashRecord));
            result->hash = current->hash;
            strncpy(result->name, current->name, 50);
            result->salary = current->salary;
            result->next = NULL;
            
            rwlock_release_readlock(&rw_lock, priority);
            return result;
        }
        current = current->next;
    }
    
    rwlock_release_readlock(&rw_lock, priority);
    
    return NULL;
}

// Print entire table (sorted by hash)
<<<<<<< HEAD
void print_table(int priority, int is_final) {
    log_message("%lld: THREAD %d,PRINT,%d\n", current_timestamp(), priority, priority);
    
=======
void print_table(int priority) {
>>>>>>> ee7f5a25bcba7d3c326fe29047b9d732517c317b
    rwlock_acquire_readlock(&rw_lock, priority);
    
    // Count records
    int count = 0;
    hashRecord *current = hash_table;
    while (current != NULL) {
        count++;
        current = current->next;
    }
    
    if (count == 0) {
        rwlock_release_readlock(&rw_lock, priority);
        console_message("Current Database:\n");
        return;
    }
    
    // Copy records to array for sorting
    hashRecord **records = (hashRecord **)malloc(count * sizeof(hashRecord *));
    current = hash_table;
    int i = 0;
    while (current != NULL) {
        records[i++] = current;
        current = current->next;
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
    
    console_message("Current Database:\n");
    for (i = 0; i < count; i++) {
        if (i == count - 1 && is_final) {
            // Last record of final print - don't print newline to match expected output format
            console_message("%u,%s,%u", records[i]->hash, 
                           records[i]->name, records[i]->salary);
        } else {
            console_message("%u,%s,%u\n", records[i]->hash, 
                           records[i]->name, records[i]->salary);
        }
    }
    
    free(records);
    rwlock_release_readlock(&rw_lock, priority);
}

// Update salary for a record
void updateSalary(const char *name, uint32_t new_salary, int priority) {
    uint32_t hash = jenkins_one_at_a_time_hash(name);
    
    rwlock_acquire_writelock(&rw_lock, priority);
    
    hashRecord *current = hash_table;
    
    while (current != NULL) {
        if (current->hash == hash) {
            // Found the record to update
            uint32_t old_salary = current->salary;
            char old_name[50];
            strncpy(old_name, current->name, 49);
            old_name[49] = '\0';
            
            current->salary = new_salary;
            
            rwlock_release_writelock(&rw_lock, priority);
            
            console_message("Updated record %u from %u,%s,%u to %u,%s,%u\n", 
                          hash, hash, old_name, old_salary, hash, old_name, new_salary);
            return;
        }
        current = current->next;
    }
    
    rwlock_release_writelock(&rw_lock, priority);
    
    console_message("Update failed. Entry %u not found.\n", hash);
}
