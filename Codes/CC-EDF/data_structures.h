#ifndef DATA_STRUCTURES_H_
#define DATA_STRUCTURES_H_

#define MAX_CRITICALITY_LEVELS 2
#define LOW 0
#define HIGH 1
#define ARRIVAL 0
#define COMPLETION 1
#define CRIT_CHANGE 2
#define MAX_TASKS 4

/*
    ADT for a task. The parameters in the task are:
        phase: The time at which the first job of task arrives.
        period: The interarrival time of the jobs.
        relative_deadline: The deadline of each job.
        criticality_lvl: The criticality level of the job.
        WCET[MAX_CRITICALITY_LEVELS]: The worst case execution time for each criticality level. 
        virtual_deadline: The virtual deadline calculated for the task. 
        job_number: The number of jobs released by the task.

*/
typedef struct{
    double phase;
    double period;
    double relative_deadline;
    int criticality_lvl;
    double WCET[MAX_CRITICALITY_LEVELS];
    double virtual_deadline;
    int job_number;
}task;

typedef struct {
    int total_tasks;
    task* task_list;
}task_set_struct;

/*
    ADT for a job. The parameters of the job are:
        job_number: The number of job released.
        task_number: the pid of the task which the job belongs to.
        release_time: The actual release time of the job.
        scheduled_time: The time at which job starts executing in kernel.
        execution_time: The actual execution time of the job.
        actual_execution_time: The time for which job has executed.
        completed_job_time: The time at which the job will finish execution.
        WCET_counter: A counter to check whether the job exceeds the worst case execution time.
        absolute_deadline: The deadline of the job.
        next: A link to the next job in the array.

*/
struct job{
    int job_number;
    int task_number;
	double release_time;
    double scheduled_time;
    double execution_time;
	double actual_execution_time;
    double completed_job_time;
    double WCET_counter;
	double absolute_deadline;
    double max_cycles;
    struct job* next;
};

typedef struct job job;

typedef struct {
    int num_jobs;
    job* job_list_head;
}job_queue_struct;

/*
    ADT for the kernel. The parameters for the kernel are:
        curr_exec_job: The job currently executed in the kernel.
        total_exec_time: The total time for which the kernel has run.
        total_idle_time: The total time for which the kernel was idle.
        curr_crit_level: The current criticality level of the kernel.
        WCET_counter: The WCET counter of the currently execution job
*/
typedef struct{
    job* curr_exec_job;
    double total_time;
    double total_idle_time;
    double WCET_counter;
    int curr_crit_level;
    int frequency;
}kernel_struct;

typedef struct{
    double decision_time;
    int decision_point;
}decision_struct;

typedef struct {
    int num_freq;
    int* freq_values;
}freq_struct;

#endif