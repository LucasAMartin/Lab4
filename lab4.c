#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "lab4.h"

void read_processes(const char *filename, Process *processes, int *num_processes) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("Failed to open file\n");
        exit(1);
    }

    *num_processes = 0;
    while (*num_processes < MAX_PROCESSES) {
        Process *process = &processes[*num_processes];
        if (fread(&process->priority, sizeof(char), 1, file) != 1) break;
        if (fread(process->name, sizeof(char), 24, file) != 24) break;
        if (fread(&process->process_id, sizeof(int), 1, file) != 1) break;
        if (fread(&process->activity_status, sizeof(char), 1, file) != 1) break;
        if (fread(&process->cpu_burst_time, sizeof(int), 1, file) != 1) break;
        if (fread(&process->base_register, sizeof(int), 1, file) != 1) break;
        if (fread(&process->limit_register, sizeof(long long), 1, file) != 1) break;
        if (fread(&process->number_of_files, sizeof(int), 1, file) != 1) break;

//        printf("Process %d:\n", *num_processes);
//        printf("  Name: %s\n", process->name);
//        printf("  Priority: %d\n", process->priority);
//        printf("  Process ID: %d\n", process->process_id);
//        printf("  Activity Status: %d\n", process->activity_status);
//        printf("  CPU Burst Time: %d\n", process->cpu_burst_time);
//        printf("  Base Register: %d\n", process->base_register);
//        printf("  Limit Register: %lld\n", process->limit_register);
//        printf("  Number of Files: %d\n", process->number_of_files);

        (*num_processes)++;
    }

    fclose(file);
}


void initialize_processor(Processor *processor, int id, int scheduling_algorithm, int initial_load) {
    processor->id = id;
    processor->scheduling_algorithm = scheduling_algorithm;
    processor->ready_queue = malloc(sizeof(Process *) * initial_load);
    processor->queue_size = 0;
    processor->initial_load = initial_load;
}



void assign_processes(Process *processes, int num_processes, Processor *processors, int num_processors) {
    int process_index = 0;
    for (int i = 0; i < num_processors; i++) {
        for (int j = 0; j < processors[i].initial_load; j++) {
            if (process_index < num_processes) {
                processors[i].ready_queue[j] = &processes[process_index++];
                processors[i].queue_size++;
            } else {
                break;
            }
        }
    }
}


void load_balance(Process *processes, Processor *processors, int num_processors) {
    // Calculate the total number of processes in all ready queues
    int total_processes = 0;

    for (int i = 0; i < num_processors; i++) {
        total_processes += processors[i].queue_size;
    }
    if (total_processes < num_processors)
        return;

    // Calculate the ideal number of processes per processor
    int ideal_load = total_processes / num_processors;

    // Create a temporary array to hold all processes
    Process **temp = malloc(total_processes * sizeof(Process *));

    // Copy all processes from the ready queues to the temporary array
    int index = 0;
    for (int i = 0; i < num_processors; i++) {
        memcpy(&temp[index], processors[i].ready_queue, processors[i].queue_size * sizeof(Process *));
        index += processors[i].queue_size;
    }

    // Distribute the processes from the temporary array back to the processors
    index = 0;
    for (int i = 0; i < num_processors; i++) {
        int load = (i == num_processors - 1) ? total_processes - index : ideal_load;
        memcpy(processors[i].ready_queue, &temp[index], load * sizeof(Process *)); 
        processors[i].queue_size = load;
        index += load;
        printf("Processor %d will have %d processes after load balancing\n", processors[i].id, processors[i].queue_size);
    }

    // Free the memory allocated for the temporary array
    free(temp);
}


void fcfs(Process **ready_queue, int *queue_size) {
    // Get the next process in the queue
    Process *process = ready_queue[0];

    // "Execute" the process
    printf("Executing process %d using FCFS\n", process->process_id);
    while (process->cpu_burst_time > 0) {
        process->cpu_burst_time -= 2;  // Decrement the CPU burst time by the quantum (2)
        printf("Process %d has %d CPU burst time left\n", process->process_id, process->cpu_burst_time);
        usleep(QUANTUM * 100000);  // Sleep for 200 milliseconds to simulate process execution
    }
    printf("Finished executing process %d\n", process->process_id);

    // Remove the process from the queue
    printf("Removing process %d from the ready queue\n", process->process_id);
    for (int i = 1; i < *queue_size; i++) {
        ready_queue[i - 1] = ready_queue[i];
    }
    (*queue_size)--;
}

void sjf(Process **ready_queue, int *queue_size) {
    // Find the process with the shortest CPU burst time
    int shortest_index = 0;
    for (int i = 1; i < *queue_size; i++) {
        if (ready_queue[i]->cpu_burst_time < ready_queue[shortest_index]->cpu_burst_time) {
            shortest_index = i;
        }
    }
    Process *process = ready_queue[shortest_index];

    // "Execute" the process
    printf("Executing process %d using SJF\n", process->process_id);
    while (process->cpu_burst_time > 0) {
        process->cpu_burst_time -= 2;  // Decrement the CPU burst time by the quantum (2)
        printf("Process %d has %d CPU burst time left\n", process->process_id, process->cpu_burst_time);
        usleep(QUANTUM * 100000);  // Sleep for 200 milliseconds to simulate process execution
    }
    printf("Finished executing process %d\n", process->process_id);

    // Remove the process from the queue
    printf("Removing process %d from the ready queue\n", process->process_id);
    for (int i = shortest_index; i < *queue_size - 1; i++) {
        ready_queue[i] = ready_queue[i + 1];
    }
    (*queue_size)--;
}

void priority_scheduling(Process **ready_queue, int *queue_size) {
    // Find the process with the highest priority (lowest priority number)
    int highest_priority_index = 0;
    for (int i = 1; i < *queue_size; i++) {
        if (ready_queue[i]->priority < ready_queue[highest_priority_index]->priority) {
            highest_priority_index = i;
        }
    }
    Process *process = ready_queue[highest_priority_index];

    // "Execute" the process
    printf("Executing process %d using priority scheduling\n", process->process_id);
    while (process->cpu_burst_time > 0) {
        process->cpu_burst_time -= 2;  // Decrement the CPU burst time by the quantum (2)
        printf("Process %d has %d CPU burst time left\n", process->process_id, process->cpu_burst_time);
        usleep(QUANTUM * 100000);  // Sleep for 200 milliseconds to simulate process execution

        // Implement aging mechanism
        static int time = 0;
        time += QUANTUM;
        if (time >= 100) {  // If 10 seconds have passed
            for (int i = 0; i < *queue_size; i++) {
                if (ready_queue[i]->priority > 1) {  // Ensure priority doesn't go below 1
                    ready_queue[i]->priority--;
                }
            }
            time = 0;
        }
    }
    printf("Finished executing process %d\n", process->process_id);

    // Remove the process from the queue
    printf("Removing process %d from the ready queue\n", process->process_id);
    for (int i = highest_priority_index; i < *queue_size - 1; i++) {
        ready_queue[i] = ready_queue[i + 1];
    }
    (*queue_size)--;
}


void round_robin(Process **ready_queue, int *queue_size) {
    // Get the next process in the queue
    Process *process = ready_queue[0];
    static int iterations = 0;  // Declare iterations as a static variable

    // "Execute" the process
    if (iterations == 0)
        printf("Executing process %d using round robin\n", process->process_id);
    if (iterations < 5 && process->cpu_burst_time > 0) {
        process->cpu_burst_time -= QUANTUM;  // Decrement the CPU burst time by the quantum
        iterations += 1;
        printf("Process %d has %d CPU burst time left\n", process->process_id, process->cpu_burst_time);
        usleep(QUANTUM * 100000);  // Sleep for quantum milliseconds to simulate process execution
    } 
    if (iterations >= 5 || process->cpu_burst_time <= 0) {
        printf("Round robin switching from process %d\n", process->process_id);

        // Move the process to the end of the queue
        for (int i = 1; i < *queue_size; i++) {
            ready_queue[i - 1] = ready_queue[i];
        }
        ready_queue[*queue_size - 1] = process;

        // Reset iterations
        iterations = 0;
    }

    // Remove the process from the queue if it has finished execution
    if (process->cpu_burst_time <= 0) {
        printf("Removing process %d from the ready queue\n", process->process_id);
        (*queue_size)--;
    }
}


void *run_processor(void *arg) {
    ProcessorArgs *args = (ProcessorArgs *)arg;
    Processor *processor = args->processor;
    Process *processes = args->processes;
    Processor *processors = args->processors;
    int num_processors = args->num_processors;
    printf("Processor %d has %d processes in the queue to start\n", processor->id, processor->queue_size);

    while (processor->queue_size > 0) {
        // Select the next process based on the scheduling algorithm
        switch (processor->scheduling_algorithm) {
            case 1:  // Priority Scheduling
                priority_scheduling(processor->ready_queue, &(processor->queue_size));
                break;
            case 2:  // Shortest Job First
                sjf(processor->ready_queue, &(processor->queue_size));
                break;
            case 3:  // Round Robin
                round_robin(processor->ready_queue, &(processor->queue_size)); 
                break;
            case 4:  // First Come First Served
                fcfs(processor->ready_queue, &(processor->queue_size));
                break;
        }

        // Print the number of processes left
        if (processor->scheduling_algorithm != 3)
            printf("Processor %d has %d processes left to complete\n", processor->id, processor->queue_size);

        // If the processor's queue is empty, balance the load
        if (processor->queue_size == 0) {
            printf("Processor %d has no more processes. Balancing load...\n", processor->id);
            load_balance(processes, processors, num_processors);
            if (processor->queue_size == 0) {
                break;
            }
        }
    }

    printf("Processor %d has completed all its processes\n", processor->id);

    return NULL;
}




int handle_input(int argc, char *argv[]){
        if (argc < 3 || argc % 2 == 1) {
        printf("Usage: %s <filename> <algorithm1> <load1> <algorithm2> <load2> ...\n", argv[0]);
        return 1;
    }

    // Check if the scheduling algorithms are in the range of 1-4
    for (int i = 2; i < argc; i += 2) {
        int scheduling_algorithm = atoi(argv[i]);
        if (scheduling_algorithm < 1 || scheduling_algorithm > 4) {
            printf("Error: Invalid scheduling algorithm '%d'. Must be in the range of 1-4.\n", scheduling_algorithm);
            return 1;
        }
    }

    // Check if the loads add up to exactly 1
    double total_load = 0.0;
    for (int i = 3; i < argc; i += 2) {
        double load = atof(argv[i]);
        total_load += load;
    }
    if (total_load != 1.0) {
        printf("Error: Total load '%f' is not equal to 1.\n", total_load);
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (handle_input(argc, argv) != 0)
        return 1;

    printf("Reading processes from file: %s\n", argv[1]);

    Process processes[MAX_PROCESSES];
    int num_processes;
    read_processes(argv[1], processes, &num_processes);

    printf("Read %d processes\n", num_processes);

    int num_processors = (argc - 2) / 2;
    Processor processors[MAX_PROCESSORS];
    ProcessorArgs args[MAX_PROCESSORS];  
    for (int i = 0; i < num_processors; i++) {
        int scheduling_algorithm = atoi(argv[i * 2 + 2]);
        int initial_load = atof(argv[i * 2 + 3]) * num_processes;
        printf("Initializing processor %d with scheduling algorithm %d and initial load %.0f%%\n", i, scheduling_algorithm, atof(argv[i * 2 + 3]) * 100);
        initialize_processor(&processors[i], i, scheduling_algorithm, initial_load);
        args[i].processor = &processors[i];  
        args[i].processes = processes;  
        args[i].processors = processors;  
        args[i].num_processors = num_processors;  
    }

    printf("Assigning processes to processors\n");

    assign_processes(processes, num_processes, processors, num_processors);

    printf("Starting processors\n");

    for (int i = 0; i < num_processors; i++) {
        pthread_create(&processors[i].thread, NULL, run_processor, &args[i]); 
    }

    for (int i = 0; i < num_processors; i++) {
        pthread_join(processors[i].thread, NULL);
    }

    printf("All processors finished\n");

    return 0;
}
