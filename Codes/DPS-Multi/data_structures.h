#ifndef DATA_STRUCTURES_H_
#define DATA_STRUCTURES_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define MAX_CRITICALITY_LEVELS 2
#define LOW 0
#define HIGH 1
#define ARRIVAL 0
#define COMPLETION 1
#define TIMER_EXPIRE 2
#define CRIT_CHANGE 3
#define ACTIVE 1
#define SLEEPING 0
#define INT_MIN -2147483648
#define INT_MAX 2147483647
#define SHUTDOWN_THRESHOLD 1
#define NUM_CORES 2
#define MAX_HIGH_UTIL 0.4

/*
    ADT for a task. The parameters in the task are:
        phase: The time at which the first job of task arrives.
        period: The interarrival time of the jobs.
        relative_deadline: The deadline of each job.
        criticality_lvl: The criticality level of the job.
        WCET[MAX_CRITICALITY_LEVELS]: The worst case execution time for each criticality level. 
        virtual_deadline: The virtual deadline calculated for the task. 
        job_number: The number of jobs released by the task.
        util: Utilisation of the task at each criticality level.
*/
typedef struct
{
    double phase;
    double period;
    double relative_deadline;
    int criticality_lvl;
    double WCET[MAX_CRITICALITY_LEVELS];
    double virtual_deadline;
    int core;
    int job_number;
    double *util;
} task;

/*
    ADT for task list. 
        It contains the total tasks and the pointer to the tasks list array.
*/
typedef struct
{
    int total_tasks;
    task *task_list;
} task_set_struct;

/*
    ADT for a job. The parameters of the job are:
        job_number: The number of job released.
        task_number: the pid of the task which the job belongs to.
        release_time: The actual release time of the job.
        scheduled_time: The time at which job starts executing in core.
        execution_time: The actual execution time of the job.
        actual_execution_time: The time for which job has executed.
        completed_job_time: The time at which the job will finish execution.
        WCET_counter: A counter to check whether the job exceeds the worst case execution time.
        absolute_deadline: The deadline of the job.
        next: A link to the next job in the array.

*/
struct job
{
    int job_number;
    int task_number;
    double release_time;
    double scheduled_time;
    double execution_time;
    double actual_execution_time;
    double completed_job_time;
    double WCET_counter;
    double absolute_deadline;
    struct job *next;
};

typedef struct job job;

/*
    ADT for job queue. 
        It contains the total number of jobs in ready queue and pointer to the ready queue.
*/
typedef struct
{
    int num_jobs;
    job *job_list_head;
} job_queue_struct;

/*
    ADT for the core. The parameters for the core are:
        curr_exec_job: The job currently executed in the core.
        total_time: The total time for which the core has run.
        total_idle_time: The total time for which the core was idle.
        curr_crit_level: The current criticality level of the core.
        WCET_counter: The WCET counter of the currently execution job.
        frequency: The frequency at which core is running.
        state: The current state of core. (ACTIVE or SLEEPING)
        next_invocation_time: The countdown timer for core. The core will wakeup after timer expires.
        x_factor: The factor to be used while calculating virtual deadlines.
        rem_util: The remaining utilisation of core. This is needed to check whether additional tasks can be allocated to this core.
        completed_scheduling: Flag to indicate whether this core has completed its hyperperiod.
*/
typedef struct
{
    job_queue_struct *ready_queue;
    job *curr_exec_job;
    double total_time;
    double total_idle_time;
    double WCET_counter;
    int curr_crit_level;
    int frequency;
    int state; //ACTIVE or SLEEPING
    double next_invocation_time;
    double x_factor;
    double *rem_util;
    int completed_scheduling;
} core_struct;


/*
    ADT for the processor. 
        It contains the total number of cores and the array of cores.
*/

typedef struct
{
    int total_cores;
    core_struct *cores;
} processor_struct;

/*
    ADT for the decision point:
        It contains the type of decision point (ARRIVAL or COMPLETION or TIMER_EXPIRE or CRIT_CHANGE), the decision time and the core for which decision has to be taken.
*/
typedef struct
{
    int core_no;
    double decision_time;
    int decision_point;
} decision_struct;

#endif