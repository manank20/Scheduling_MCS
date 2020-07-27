#ifndef DATA_STRUCTURES_H_
#define DATA_STRUCTURES_H_

#define MAX_CRITICALITY_LEVELS 4
// #define LOW 0
// #define HIGH 1

typedef struct{
    int job_number;
    int phase;
    int period;
    int relative_deadline;
    int criticality_lvl;
    int WCET[MAX_CRITICALITY_LEVELS];
    int virtual_deadline;
}task;

//Create structure for a job too. Additional fields - Remaining execution time. 
struct job{
    int task_number;
	int release_time;
    int execution_time;
	int remaining_execution_time;
	int absolute_deadline;
    struct job* next;
    // int state;
};

typedef struct job job;

typedef struct{
    int curr_exec_process;
    int curr_exec_time;
    int total_exec_time;
    int total_idle_time;
    int curr_crit_level;
}kernel_struct;

// kernel_struct cpu;

//Output of a function. Should not include these variables in header files.
double x;
int threshold_criticality_level;
// FILE* output_file;

#endif