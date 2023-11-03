#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

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
    int scheduling_algorithm;
    Process *ready_queue;
    int queue_size;
    int initial_load;
    pthread_t thread;
} Processor;

#define MAX_PROCESSES 1000 
#define MAX_PROCESSORS 4    

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

        (*num_processes)++;
    }

    fclose(file);
}

void initialize_processors(Processor *processors, int num_processors, int *scheduling_algorithms, int *initial_loads) {
    for (int i = 0; i < num_processors; i++) {
        processors[i].scheduling_algorithm = scheduling_algorithms[i];
        processors[i].ready_queue = malloc(sizeof(Process) * initial_loads[i]);
        processors[i].queue_size = 0;
        processors[i].initial_load = initial_loads[i];
    }
}

void assign_processes(Process *processes, int num_processes, Processor *processors, int num_processors) {
    for (int i = 0; i < num_processes; i++) {
        int processor_index = i % num_processors; 
        if (processors[processor_index].queue_size < processors[processor_index].initial_load) {
            processors[processor_index].ready_queue[processors[processor_index].queue_size] = processes[i];
            processors[processor_index].queue_size++;
        }
    }
}

void *run_processor(void *arg) {
    Processor *processor = (Processor *)arg;

    // Implement the FCFS scheduling algorithm
    while (processor->queue_size > 0) {
        // Get the next process in the queue
        Process *process = &processor->ready_queue[0];

        // "Execute" the process
        printf("Processor %d is executing process %d\n", processor->scheduling_algorithm, process->process_id);
        while (process->cpu_burst_time > 0) {
            process->cpu_burst_time -= 2;  // Decrement the CPU burst time by the quantum (2)
            usleep(200 * 1000);  // Sleep for 200 milliseconds to simulate process execution
        }
        printf("Processor %d has finished executing process %d\n", processor->scheduling_algorithm, process->process_id);

        // Remove the process from the queue
        for (int i = 1; i < processor->queue_size; i++) {
            processor->ready_queue[i - 1] = processor->ready_queue[i];
        }
        processor->queue_size--;
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc % 2 != 0) {
        printf("Usage: %s <filename> <algorithm1> <load1> <algorithm2> <load2> ...\n", argv[0]);
        return 1;
    }

    printf("Reading processes from file: %s\n", argv[1]);

    Process processes[MAX_PROCESSES];
    int num_processes;
    read_processes(argv[1], processes, &num_processes);

    printf("Read %d processes\n", num_processes);

    int num_processors = (argc - 2) / 2;
    Processor processors[MAX_PROCESSORS];
    for (int i = 0; i < num_processors; i++) {
        int scheduling_algorithm = atoi(argv[i * 2 + 2]);
        int initial_load = atof(argv[i * 2 + 3]) * num_processes;
        printf("Initializing processor %d with scheduling algorithm %d and initial load %d\n", i, scheduling_algorithm, initial_load);
        initialize_processors(&processors[i], 1, &scheduling_algorithm, &initial_load);
    }

    printf("Assigning processes to processors\n");

    assign_processes(processes, num_processes, processors, num_processors);

    printf("Starting processors\n");

    for (int i = 0; i < num_processors; i++) {
        pthread_create(&processors[i].thread, NULL, run_processor, &processors[i]);
    }

    printf("Waiting for processors to finish\n");

    for (int i = 0; i < num_processors; i++) {
        pthread_join(processors[i].thread, NULL);
    }

    printf("All processors finished\n");

    return 0;
}