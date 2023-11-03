#include <stdio.h>
#include <stdlib.h>

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

#define MAX_PROCESSES 1000  // Adjust this value as needed

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


int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    Process processes[MAX_PROCESSES];
    int num_processes;
    read_processes(argv[1], processes, &num_processes);

    for (int i = 0; i < num_processes; i++) {
        Process *process = &processes[i];
        printf("Priority: %d\n", process->priority);
        printf("Name: %s\n", process->name);
        printf("Process ID: %d\n", process->process_id);
        printf("Activity Status: %d\n", process->activity_status);
        printf("CPU Burst Time: %d\n", process->cpu_burst_time);
        printf("Base Register: %d\n", process->base_register);
        printf("Limit Register: %lli\n", process->limit_register);
        printf("Number of Files: %d\n\n", process->number_of_files);
    }

    return 0;
}
