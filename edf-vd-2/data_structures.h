#ifndef DATA_STRUCTURES_H_
#define DATA_STRUCTURES_H_

#define MAX_CRITICALITY_LEVELS 2
#define LOW 0
#define HIGH 1
#define ARRIVAL 0
#define COMPLETION 1
#define CRIT_CHANGE 2

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
    int phase;
    int period;
    int relative_deadline;
    int criticality_lvl;
    int WCET[MAX_CRITICALITY_LEVELS];
    int virtual_deadline;
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
	int release_time;
    int scheduled_time;
    int execution_time;
	int actual_execution_time;
    int completed_job_time;
    int WCET_counter;
	int absolute_deadline;
    struct job* next;
};

typedef struct job job;

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
    int total_exec_time;
    int total_idle_time;
    int WCET_counter;
    int curr_crit_level;
}kernel_struct;

#endif