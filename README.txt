CONCURRENT HASH TABLE PROJECT
=============================

COMPILATION:
-----------
Run 'make' to compile the project. This will create the 'chash' executable.

EXECUTION:
----------
1. Ensure 'commands.txt' is in the same directory as the executable
2. Run './chash'
3. Output will be displayed on console
4. Log file 'hash.log' will be created with detailed timing information

PROJECT STRUCTURE:
------------------
chash.c              - Main program that reads commands and coordinates threads
hash.h               - Header file with structure definitions and function prototypes
hash_operations.c    - Implementation of insert, delete, search, and print operations
hash_utuls.c         - Utility functions (hash function, timestamp, logging)
rwlock.c             - Reader-writer lock implementation with writer preference
Makefile             - Build configuration

IMPLEMENTATION NOTES:
---------------------
1. The hash table uses a singly-linked list structure
2. Jenkins one-at-a-time hash function is used for key hashing
3. Reader-writer locks implement writer preference to avoid writer starvation
4. Commands are sorted by priority before execution to ensure proper ordering
5. All console output and log file writes are protected by mutexes for thread safety
6. Memory is properly allocated and freed to avoid leaks

THREAD SYNCHRONIZATION:
-----------------------
- INSERT and DELETE operations acquire write locks
- SEARCH operations acquire read locks
- PRINT operations acquire read locks
- The rwlock implementation uses writer preference (writers get priority over readers)

AI USAGE CITATION:
------------------
I used Claude (Anthropic's AI assistant) to help structure this project. The AI was used for:

1. Code Organization: I provided the assignment requirements and asked for help organizing 
   the code into modular components (separate files for different functionality).
   Prompt: "Help me create a modular concurrent hash table in C with separate files for 
   hash operations, locking mechanisms, and utilities."

2. Reader-Writer Lock Implementation: I asked for guidance on implementing a reader-writer 
   lock with writer preference using POSIX threads.
   Prompt: "Implement a reader-writer lock in C with writer preference using pthread 
   condition variables and mutexes."

3. Thread-Safe Logging: I requested help implementing thread-safe logging mechanisms.
   Prompt: "How can I implement thread-safe logging to a file with multiple threads 
   writing simultaneously?"

4. Memory Management: I asked for best practices on handling dynamic memory with threads.
   Prompt: "What's the best way to handle memory allocation and deallocation in a 
   multi-threaded hash table implementation?"

The AI provided code examples and structure that I then adapted and modified to meet the 
specific requirements of this assignment. I verified all logic, tested the implementation, 
and made adjustments to ensure correctness and compliance with the assignment specifications.

KNOWN LIMITATIONS:
------------------
- Thread execution order may vary between runs due to OS scheduling
- The usleep(1000) call in main helps encourage priority ordering but doesn't guarantee it
- Hash collisions are not handled per assignment specifications

TESTING:
--------
The program was tested with the provided sample commands and various edge cases including:
- Multiple inserts and updates
- Deleting non-existent records
- Searching for records
- Printing empty and populated tables
- Concurrent operations from multiple threads