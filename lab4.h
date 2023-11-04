#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdio.h>
#include <pthread.h>

#define MAX_PROCESSES 1000
#define MAX_PROCESSORS 4
#define QUANTUM 2

typedef struct {
    char priority;
    char name[24];
    int process_id;
    char activity_status;
    int cpu_burst_time;
    int base_register;
    long long limit_register;
    int number_of_files;
} Process;

typedef struct {
    int id;
    int scheduling_algorithm;
    Process **ready_queue;
    int queue_size;
    int initial_load;
    pthread_t thread;
} Processor;

typedef struct {
    Processor *processor;
    Process *processes;
    Processor *processors;
    int num_processors;
} ProcessorArgs;

void read_processes(const char *filename, Process *processes, int *num_processes);
void initialize_processor(Processor *processor, int id, int scheduling_algorithm, int initial_load);
void assign_processes(Process *processes, int num_processes, Processor *processors, int num_processors);
void load_balance(Process *processes, Processor *processors, int num_processors);
void fcfs(Process **ready_queue, int *queue_size);
void sjf(Process **ready_queue, int *queue_size);
void priority_scheduling(Process **ready_queue, int *queue_size);
void round_robin(Process **ready_queue, int *queue_size);
void *run_processor(void *arg);
int handle_input(int argc, char *argv[]);

#endif
