#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include <stdarg.h>

#define MAX_PROCESSES 10
#define NUM_PHILOSOPHERS 5
#define THINKING_TIME 2
#define EATING_TIME 3

// Process structure
typedef struct {
    int pid;
    int burst_time;
    int arrival_time;
} Process;

// Global variables for Mutex version
pthread_mutex_t forks[NUM_PHILOSOPHERS];
pthread_mutex_t print_mutex;

// Global variables for Semaphore version
sem_t fork_sems[NUM_PHILOSOPHERS];
sem_t table;

//thread safe print function
void safe_print(const char *format, ...) {
    pthread_mutex_lock(&print_mutex);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    fflush(stdout);
    pthread_mutex_unlock(&print_mutex);
} 

// process simulation
void* process_thread(void* arg) {
    Process* proc = (Process*)arg;
    
    safe_print("[Process %d] Started (Burst Time: %d seconds)\n", 
               proc->pid, proc->burst_time);
    
    // Simulate CPU burst
    sleep(proc->burst_time);
    
    safe_print("[Process %d] Finished\n", proc->pid);
    
    free(proc);
    return NULL;
}

void simulate_processes(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open %s\n", filename);
        return;
    }
    
    Process processes[MAX_PROCESSES];
    pthread_t threads[MAX_PROCESSES];
    int count = 0;
    
    printf("\n========== PROCESS SIMULATION ==========\n\n");
    
    // read processes file
    int first_read = fscanf(file, "%d %d %d", 
                            &processes[0].pid, 
                            &processes[0].arrival_time, 
                            &processes[0].burst_time);
    
    if (first_read == 3) {
        // Successfully read numbers, it's a data line
        count = 1;
    } else {
        // Failed to read numbers, probably a header - skip this line
        char buffer[256];
        fgets(buffer, sizeof(buffer), file);
        count = 0;
    }
    
    // Read remaining processes from file
    while (fscanf(file, "%d %d %d", 
                  &processes[count].pid, 
                  &processes[count].arrival_time, 
                  &processes[count].burst_time) == 3) {
        count++;
        if (count >= MAX_PROCESSES) break;
    }
    fclose(file);
    
    printf("Loaded %d processes from file\n\n", count);
    
    // Create threads for each process
    for (int i = 0; i < count; i++) {
        Process* proc = malloc(sizeof(Process));
        *proc = processes[i];
        
        if (pthread_create(&threads[i], NULL, process_thread, proc) != 0) {
            printf("Error creating thread for process %d\n", proc->pid);
            free(proc);
        }
        
        // Simulate arrival time
        if (i < count - 1 && processes[i+1].arrival_time > processes[i].arrival_time) {
            sleep(processes[i+1].arrival_time - processes[i].arrival_time);
        }
    }
    
    // Wait for all process threads to complete
    for (int i = 0; i < count; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("\nAll processes completed.\n");
}

// Mutex Dining Philosophers Problem

void* philosopher(void* arg) {
    int id = *(int*)arg;  // This will be 1-5 now
    int left_fork = id - 1;  // Convert to 0-4 for array indexing
    int right_fork = id % NUM_PHILOSOPHERS;
    
    // Strategy to avoid deadlock: pick up lower-numbered fork first
    int first_fork = (left_fork < right_fork) ? left_fork : right_fork;
    int second_fork = (left_fork < right_fork) ? right_fork : left_fork;
    
    for (int meal = 0; meal < 3; meal++) {  // Each philosopher eats 3 times
        // Thinking
        safe_print("[Philosopher %d] Thinking...\n", id);
        sleep(THINKING_TIME);
        
        // Hungry - waiting for forks
        safe_print("[Philosopher %d] Hungry, waiting for forks %d and %d...\n", 
                   id, left_fork, right_fork);
        
        // Pick up forks in order to avoid deadlock
        pthread_mutex_lock(&forks[first_fork]);
        safe_print("[Philosopher %d] Picked up fork %d\n", id, first_fork);
        
        pthread_mutex_lock(&forks[second_fork]);
        safe_print("[Philosopher %d] Picked up fork %d\n", id, second_fork);
        
        // Eating
        safe_print("[Philosopher %d] Eating (meal %d/3)...\n", id, meal + 1);
        sleep(EATING_TIME);
        
        // Put down forks
        pthread_mutex_unlock(&forks[second_fork]);
        safe_print("[Philosopher %d] Released fork %d\n", id, second_fork);
        
        pthread_mutex_unlock(&forks[first_fork]);
        safe_print("[Philosopher %d] Released fork %d\n", id, first_fork);
    }
    
    safe_print("[Philosopher %d] Finished all meals, leaving table.\n", id);
    free(arg);
    return NULL;
}

void dining_philosophers() {
    pthread_t philosophers[NUM_PHILOSOPHERS];
    
    printf("\n\n========== DINING PHILOSOPHERS PROBLEM (MUTEX VERSION) ==========\n\n");
    printf("Solving with deadlock prevention: picking up lower-numbered fork first\n\n");
    
    // Initialize mutexes
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_mutex_init(&forks[i], NULL);
    }
    
    // Create philosopher threads
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;  // Number philosophers 1-5 instead of 0-4
        
        if (pthread_create(&philosophers[i], NULL, philosopher, id) != 0) {
            printf("Error creating philosopher thread %d\n", i);
            free(id);
        }
    }
    
    // Wait for all philosophers to finish
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_join(philosophers[i], NULL);
    }
    
    // Cleanup
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_mutex_destroy(&forks[i]);
    }
    
    printf("\nAll philosophers have finished dining.\n");
}

// Semaphore Dining Philosophers Problem

void* philosopher_semaphore(void* arg) {
    int id = *(int*)arg;  // This will be 1-5 now
    int left_fork = id - 1;  // Convert to 0-4 for array indexing
    int right_fork = id % NUM_PHILOSOPHERS;
    
    for (int meal = 0; meal < 3; meal++) {
        // Thinking
        safe_print("[Philosopher %d] Thinking...\n", id);
        sleep(THINKING_TIME);
        
        // Hungry - wait for table seat (deadlock prevention)
        safe_print("[Philosopher %d] Hungry, waiting for table...\n", id);
        sem_wait(&table);
        
        // Pick up left fork
        sem_wait(&fork_sems[left_fork]);
        safe_print("[Philosopher %d] Picked up left fork %d\n", id, left_fork);
        
        // Pick up right fork
        sem_wait(&fork_sems[right_fork]);
        safe_print("[Philosopher %d] Picked up right fork %d\n", 
                   id, right_fork);
        
        // Eating
        safe_print("[Philosopher %d] Eating (meal %d/3)...\n", id, meal + 1);
        sleep(EATING_TIME);
        
        // Put down right fork
        sem_post(&fork_sems[right_fork]);
        safe_print("[Philosopher %d] Released right fork %d\n", 
                   id, right_fork);
        
        // Put down left fork
        sem_post(&fork_sems[left_fork]);
        safe_print("[Philosopher %d] Released left fork %d\n", id, left_fork);
        
        // Leave table
        sem_post(&table);
    }
    
    safe_print("[Philosopher %d] Finished all meals\n", id);
    free(arg);
    return NULL;
}

void dining_philosophers_semaphore() {
    pthread_t philosophers[NUM_PHILOSOPHERS];
    
    printf("\n\n========== DINING PHILOSOPHERS PROBLEM (SEMAPHORE VERSION) ==========\n\n");
    printf("Solving with semaphores: limiting to %d concurrent eaters\n\n", 
           NUM_PHILOSOPHERS - 1);
    
    // Initialize semaphores
    if (sem_init(&table, 0, NUM_PHILOSOPHERS - 1) != 0) {
        perror("Failed to initialize table semaphore");
        return;
    }
    
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        if (sem_init(&fork_sems[i], 0, 1) != 0) {
            perror("Failed to initialize fork semaphore");
            return;
        }
    }
    
    // Create philosopher threads
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        
        if (pthread_create(&philosophers[i], NULL, philosopher_semaphore, id) != 0) {
            printf("Error creating philosopher thread %d\n", i);
            free(id);
        }
    }
    
    // Wait for all philosophers
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_join(philosophers[i], NULL);
    }
    
    // Cleanup
    sem_destroy(&table);
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        sem_destroy(&fork_sems[i]);
    }
    
    printf("\nAll philosophers have finished dining (semaphore version).\n");
}

// main

int main(int argc, char* argv[]) {
    // Initialize print mutex
    pthread_mutex_init(&print_mutex, NULL);
    
    printf("Operating Systems Project 2\n");
    printf("Thread-Based Process Simulation and Synchronization\n");
    printf("====================================================\n");
    
    // Part 1: Simulate processes
    const char* filename = (argc > 1) ? argv[1] : "processes.txt";
    simulate_processes(filename);
    
    // Part 2: Dining Philosophers (Mutex version)
    dining_philosophers();
    
    // Part 3: Dining Philosophers (Semaphore version)
    dining_philosophers_semaphore();
    
    // Cleanup
    pthread_mutex_destroy(&print_mutex);
    
    printf("\n====================================================\n");
    printf("Project completed successfully!\n");
    
    return 0;
}