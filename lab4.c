#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <signal.h>
#include "lab4.h"

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void read_processes(const char *filename, Process *processes, int *num_processes)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        printf("Failed to open file\n");
        exit(1);
    }

    *num_processes = 0;
    while (*num_processes < MAX_PROCESSES)
    {
        Process *process = &processes[*num_processes];
        if (fread(&process->priority, sizeof(char), 1, file) != 1)
            break;
        if (fread(process->name, sizeof(char), 24, file) != 24)
            break;
        if (fread(&process->process_id, sizeof(int), 1, file) != 1)
            break;
        if (fread(&process->activity_status, sizeof(char), 1, file) != 1)
            break;
        if (fread(&process->cpu_burst_time, sizeof(int), 1, file) != 1)
            break;
        if (fread(&process->base_register, sizeof(int), 1, file) != 1)
            break;
        if (fread(&process->limit_register, sizeof(long long), 1, file) != 1)
            break;
        if (fread(&process->number_of_files, sizeof(int), 1, file) != 1)
            break;

        // Check if the process name starts with "Process_"
        if (*num_processes == 0 && strncmp(process->name, "Process_", 8) != 0)
        {
            printf("Invalid file\n");
            exit(1);
        }

        (*num_processes)++;
    }
    fclose(file);
}

void handle_signal(int sig)
{
    printf("All processors finished\n");
    exit(0);
}

void initialize_processor(Processor *processor, int id, int scheduling_algorithm, int initial_load)
{
    processor->id = id;
    processor->scheduling_algorithm = scheduling_algorithm;
    processor->ready_queue = malloc(sizeof(Process *) * initial_load);
    processor->iterations = 0;
    if (processor->ready_queue == NULL)
    {
        printf("Memory allocation failed\n");
        exit(1);
    }
    processor->queue_size = 0;
    processor->initial_load = initial_load;
}

void assign_processes(Process *processes, int num_processes, Processor *processors, int num_processors)
{
    int process_index = 0;
    for (int i = 0; i < num_processors; i++)
    {
        for (int j = 0; j < processors[i].initial_load; j++)
        {
            if (process_index < num_processes)
            {
                processors[i].ready_queue[j] = &processes[process_index++];
                processors[i].queue_size++;
            }
            else
            {
                break;
            }
        }
    }
}

void load_balance(Process *processes, Processor *processors, int num_processors, int processor_id)
{
    // Calculate the total number of processes in all ready queues
    int total_processes = 0;

    for (int i = 0; i < num_processors; i++)
    {
        total_processes += processors[i].queue_size;
    }
    if (total_processes < num_processors)
    {
        printf("Load balancing skipped for processor %d\n", processors[processor_id].id);
        return;
    }
    printf("Balancing the load for processor %d\n", processors[processor_id].id);
    // Calculate the ideal number of processes per processor
    int ideal_load = total_processes / num_processors;

    // Create a temporary array to hold all processes
    Process **temp = malloc(total_processes * sizeof(Process *));

    // Copy all processes from the ready queues to the temporary array
    int index = 0;
    for (int i = 0; i < num_processors; i++)
    {
        memcpy(&temp[index], processors[i].ready_queue, processors[i].queue_size * sizeof(Process *));
        index += processors[i].queue_size;
    }

    // Distribute the processes from the temporary array back to the processors
    index = 0;
    for (int i = 0; i < num_processors; i++)
    {
        int load = ideal_load;
        // If there are remaining processes, add one to the load
        if (total_processes - index > num_processors * ideal_load)
        {
            load++;
        }
        memcpy(processors[i].ready_queue, &temp[index], load * sizeof(Process *));
        processors[i].queue_size = load;
        index += load;
        printf("Processor %d will have %d processes after load balancing\n", processors[i].id, processors[i].queue_size);
    }

    // Free the memory allocated for the temporary array
    free(temp);
}

void print_cpu_burst_time(Process *process)
{
    int cpu_burst_time = process->cpu_burst_time > 0 ? process->cpu_burst_time : 0;
    printf("Process %d has %d CPU burst time left\n", process->process_id, cpu_burst_time);
}

void execute_process(Process *process, char *scheduling_algorithm, int processor_id)
{
    printf("Processor %d is executing process %d using %s\n", processor_id, process->process_id, scheduling_algorithm);
    while (process->cpu_burst_time > 0)
    {
        process->cpu_burst_time -= QUANTUM; // Decrement the CPU burst time by the quantum (2)
        print_cpu_burst_time(process);
        usleep(MILLISECONDS * 100000); // Sleep for 200 milliseconds to simulate process execution
    }
    printf("Processor %d has finished executing process %d\n", processor_id, process->process_id);
}

void remove_process(Process **ready_queue, int *queue_size, int index, int processor_id)
{
    printf("Processor %d is removing process %d from the ready queue\n", processor_id, ready_queue[index]->process_id);
    for (int i = index; i < *queue_size - 1; i++)
    {
        if (i + 1 < *queue_size)
        {
            ready_queue[i] = ready_queue[i + 1];
        }
    }
    (*queue_size)--;
}

void fcfs(Process **ready_queue, int *queue_size, int processor_id)
{
    Process *process = ready_queue[0];
    execute_process(process, "FCFS", processor_id);
    remove_process(ready_queue, queue_size, 0, processor_id);
}

void sjf(Process **ready_queue, int *queue_size, int processor_id)
{
    int shortest_index = 0;
    for (int i = 1; i < *queue_size; i++)
    {
        if (ready_queue[i]->cpu_burst_time < ready_queue[shortest_index]->cpu_burst_time)
        {
            shortest_index = i;
        }
    }
    Process *process = ready_queue[shortest_index];
    execute_process(process, "SJF", processor_id);
    remove_process(ready_queue, queue_size, shortest_index, processor_id);
}

void priority_scheduling(Process **ready_queue, int *queue_size, int processor_id)
{
    int highest_priority_index = 0;
    for (int i = 1; i < *queue_size; i++)
    {
        if (ready_queue[i]->priority < ready_queue[highest_priority_index]->priority)
        {
            highest_priority_index = i;
        }
    }
    Process *process = ready_queue[highest_priority_index];
    execute_process(process, "Priority Scheduling", processor_id);
    remove_process(ready_queue, queue_size, highest_priority_index, processor_id);
}

void round_robin(Process **ready_queue, int *queue_size, Processor *processor)
{
    Process *process = ready_queue[0];
    if (processor->iterations == 0)
    {
        printf("Processor %d is executing process %d using Round Robin\n", processor->id, process->process_id);
    }
    if (processor->iterations < ROUND_ROBIN_TURNS && process->cpu_burst_time > 0)
    {
        process->cpu_burst_time -= QUANTUM; // Decrement the CPU burst time by the quantum
        processor->iterations += 1;
        print_cpu_burst_time(process);
        usleep(MILLISECONDS * 100000); // Sleep for quantum milliseconds to simulate process execution
    }
    if (processor->iterations >= ROUND_ROBIN_TURNS || process->cpu_burst_time <= 0)
    {
        printf("Processor %d is switching from process %d\n", processor->id, process->process_id);
        // Move the process to the end of the queue
        for (int i = 1; i < *queue_size; i++)
        {
            ready_queue[i - 1] = ready_queue[i];
        }
        ready_queue[*queue_size - 1] = process;
        // Reset iterations
        processor->iterations = 0;
    }
    // Remove the process from the queue if it has finished execution
    if (process->cpu_burst_time <= 0)
    {
        printf("Process %d has finished execution. Removing it from the queue.\n", process->process_id);
        int process_index = 0;
        for (int i = 0; i < *queue_size; i++)
        {
            if (ready_queue[i]->process_id == process->process_id)
            {
                process_index = i;
                break;
            }
        }
        remove_process(ready_queue, queue_size, process_index, processor->id);
    }
}


void *run_processor(void *arg)
{
    ProcessorArgs *args = (ProcessorArgs *)arg;
    Processor *processor = args->processor;
    Process *processes = args->processes;
    Processor *processors = args->processors;
    int num_processors = args->num_processors;
    printf("Processor %d has %d processes in the queue to start\n", processor->id, processor->queue_size);
    while (processor->queue_size > 0)
    {
        // Select the next process based on the scheduling algorithm
        switch (processor->scheduling_algorithm)
        {
        case 1: // Priority Scheduling
            priority_scheduling(processor->ready_queue, &(processor->queue_size), processor->id);
            break;
        case 2: // Shortest Job First
            sjf(processor->ready_queue, &(processor->queue_size), processor->id);
            break;
        case 3: // Round Robin
            round_robin(processor->ready_queue, &(processor->queue_size), processor);
            break;
        case 4: // First Come First Served
            fcfs(processor->ready_queue, &(processor->queue_size), processor->id);
            break;
        }
        // Print the number of processes left
        if (processor->scheduling_algorithm != 3)
        {
            printf("Processor %d has %d processes left to complete\n", processor->id, processor->queue_size);
        }
        // If the processor's queue is empty, balance the load
        if (processor->queue_size == 0)
        {
            printf("Processor %d has no more processes.\n", processor->id);
            pthread_mutex_lock(&lock);
            load_balance(processes, processors, num_processors, processor->id);
            pthread_mutex_unlock(&lock);
            if (processor->queue_size == 0)
            {
                break;
            }
        }
    }
    printf("Processor %d has completed all its processes\n", processor->id);
    return NULL;
}

int is_number(char *str)
{
    for (int i = 0; str[i] != '\0'; i++)
    {
        if (!isdigit(str[i]) && str[i] != '.')
        {
            return 0;
        }
    }
    return 1;
}

int handle_input(int argc, char *argv[])
{
    if (argc < 3 || argc % 2 == 1)
    {
        printf("Usage: %s <filename> <algorithm1> <load1> <algorithm2> <load2> ...\n", argv[0]);
        return 1;
    }

    // Check if the scheduling algorithms are in the range of 1-4 and positive
    for (int i = 2; i < argc; i += 2)
    {
        if (!is_number(argv[i]))
        {
            printf("Error: Invalid input '%s'. Input must be a number.\n", argv[i]);
            return 1;
        }
        int scheduling_algorithm = atoi(argv[i]);
        if (scheduling_algorithm < 1 || scheduling_algorithm > 4)
        {
            printf("Error: Invalid scheduling algorithm '%d'. Must be in the range of 1-4.\n", scheduling_algorithm);
            return 1;
        }
    }

    // Check if the loads add up to approximately 1, are positive, and are not smaller than 0.1
    double total_load = 0.0;
    double epsilon = 0.00001;
    for (int i = 3; i < argc; i += 2)
    {
        if (!is_number(argv[i]))
        {
            printf("Error: Invalid input '%s'. Input must be a number.\n", argv[i]);
            return 1;
        }
        double load = atof(argv[i]);
        if (load < 0)
        {
            printf("Error: Invalid load '%f'. Load must be positive.\n", load);
            return 1;
        }
        if (load < 0.1)
        {
            printf("Error: Invalid load '%f'. Load must not be smaller than 0.1.\n", load);
            return 1;
        }
        total_load += load;
    }
    double diff = total_load - 1.0;
    if (diff > epsilon || diff < -epsilon)
    {
        printf("Error: Total load '%f' is not equal to 1.\n", total_load);
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
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
    for (int i = 0; i < num_processors; i++)
    {
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

    for (int i = 0; i < num_processors; i++)
    {
        printf("Starting processor %d\n", i);
        pthread_create(&processors[i].thread, NULL, run_processor, &args[i]);
    }

    signal(SIGABRT, handle_signal);
    for (int i = 0; i < num_processors; i++)
    {
        int result = pthread_join(processors[i].thread, NULL);
        if (result != 0) {
            printf("Error joining thread %d: %s\n", i, strerror(result));

        }
    }

    pthread_mutex_destroy(&lock);
    printf("All processors finished\n");

    return 0;
}