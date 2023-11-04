#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define MAX_PROCESSES 1000 
#define MAX_PROCESSORS 4   

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
    Process **ready_queue;  // Change this line
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

        printf("Process %d:\n", *num_processes);
        printf("  Name: %s\n", process->name);
        printf("  Priority: %d\n", process->priority);
        printf("  Process ID: %d\n", process->process_id);
        printf("  Activity Status: %d\n", process->activity_status);
        printf("  CPU Burst Time: %d\n", process->cpu_burst_time);
        printf("  Base Register: %d\n", process->base_register);
        printf("  Limit Register: %lld\n", process->limit_register);
        printf("  Number of Files: %d\n", process->number_of_files);

        (*num_processes)++;
    }

    fclose(file);
}


void initialize_processor(Processor *processor, int id, int scheduling_algorithm, int initial_load) {
    processor->id = id;
    processor->scheduling_algorithm = scheduling_algorithm;
    processor->ready_queue = malloc(sizeof(Process *) * initial_load);  // Allocate memory for pointers to Process
    processor->queue_size = 0;
    processor->initial_load = initial_load;
}



void assign_processes(Process *processes, int num_processes, Processor *processors, int num_processors) {
    int process_index = 0;
    for (int i = 0; i < num_processors; i++) {
        for (int j = 0; j < processors[i].initial_load; j++) {
            if (process_index < num_processes) {
                processors[i].ready_queue[j] = &processes[process_index++];  // Change this line
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
        if (total_processes < num_processors)
            return;
    }

    // Calculate the ideal number of processes per processor
    int ideal_load = total_processes / num_processors;

    // Create a temporary array to hold all processes
    Process **temp = malloc(total_processes * sizeof(Process *));  // Change this line

    // Copy all processes from the ready queues to the temporary array
    int index = 0;
    for (int i = 0; i < num_processors; i++) {
        memcpy(&temp[index], processors[i].ready_queue, processors[i].queue_size * sizeof(Process *));  // Change this line
        index += processors[i].queue_size;
    }

    // Distribute the processes from the temporary array back to the processors
    index = 0;
    for (int i = 0; i < num_processors; i++) {
        int load = (i == num_processors - 1) ? total_processes - index : ideal_load;
        memcpy(processors[i].ready_queue, &temp[index], load * sizeof(Process *));  // Change this line
        processors[i].queue_size = load;
        index += load;
        printf("Processor %d will have %d processes after load balancing\n", processors[i].id, processors[i].queue_size);
    }

    // Free the memory allocated for the temporary array
    free(temp);
}


void *run_processor(void *arg) {
    ProcessorArgs *args = (ProcessorArgs *)arg;
    Processor *processor = args->processor;
    Process *processes = args->processes;
    Processor *processors = args->processors;
    int num_processors = args->num_processors;
    printf("Processor %d has %d processes in the queue to start\n", processor->id, processor->queue_size);
    // Implement the FCFS scheduling algorithm
    while (processor->queue_size > 0) {
        // Get the next process in the queue
        Process *process = processor->ready_queue[0];  // No need to change this line

        // "Execute" the process
        printf("Processor %d (algorithm %d) is executing process %d\n", processor->id, processor->scheduling_algorithm, process->process_id);
        while (process->cpu_burst_time > 0) {
            process->cpu_burst_time -= 2;  // Decrement the CPU burst time by the quantum (2)
            printf("Process %d has %d CPU burst time left\n", process->process_id, process->cpu_burst_time);
            usleep(200);  // Sleep for 200 milliseconds to simulate process execution
        }
        printf("Processor %d has finished executing process %d\n", processor->id, process->process_id);

        // Remove the process from the queue
        printf("Removing process %d from the ready queue of processor %d\n", process->process_id, processor->id);
        for (int i = 1; i < processor->queue_size; i++) {
            processor->ready_queue[i - 1] = processor->ready_queue[i];  // No need to change this line
        }
        processor->queue_size--;

        // Print the number of processes left
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
    ProcessorArgs args[MAX_PROCESSORS];  // Add this line
    for (int i = 0; i < num_processors; i++) {
        int scheduling_algorithm = atoi(argv[i * 2 + 2]);
        int initial_load = atof(argv[i * 2 + 3]) * num_processes;
        printf("Initializing processor %d with scheduling algorithm %d and initial load %.0f%%\n", i, scheduling_algorithm, atof(argv[i * 2 + 3]) * 100);
        initialize_processor(&processors[i], i, scheduling_algorithm, initial_load);
        args[i].processor = &processors[i];  // Add this line
        args[i].processes = processes;  // Add this line
        args[i].processors = processors;  // Add this line
        args[i].num_processors = num_processors;  // Add this line
    }

    printf("Assigning processes to processors\n");

    assign_processes(processes, num_processes, processors, num_processors);

    printf("Starting processors\n");

    for (int i = 0; i < num_processors; i++) {
        pthread_create(&processors[i].thread, NULL, run_processor, &args[i]);  // Modify this line
    }

    printf("Waiting for processors to finish\n");

    for (int i = 0; i < num_processors; i++) {
        pthread_join(processors[i].thread, NULL);
    }

    printf("All processors finished\n");

    return 0;
}
