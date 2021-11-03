#ifndef __DATA_STRUCTURES_H_
#define __DATA_STRUCTURES_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <math.h>

//Decision points
#define ARRIVAL 0
#define COMPLETION 1
#define TIMER_EXPIRE 2
#define CRIT_CHANGE 3

//State of core
#define ACTIVE 100
#define SHUTDOWN 102
#define IDLE 103

#define SHUTDOWN_TASK 201
#define NON_SHUTDOWN_TASK 202
#define SHUTDOWN_CORE 203
#define NON_SHUTDOWN_CORE 204
#define EXCEPTIONAL 1

#define INT_MIN -2147483648
#define INT_MAX 2147483647

#define SHUTDOWN_THRESHOLD 200
#define NUM_CORES 4

#define FREQUENCY_LEVELS 5

#define DPS 1001
#define EDF_VD 1002
#define EDF_VD_DJ 1003
#define EDF 1004

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
typedef struct task
{
    double phase;
    double period;
    double relative_deadline;
    int criticality_lvl;
    double *WCET;
    double virtual_deadline;
    int core;
    int task_number;
    int job_number;
    double *util;
    double *exec_times;
    int shutdown;
} task;

/*
    ADT for task list. 
        It contains the total tasks and the pointer to the tasks list array.
*/
typedef struct task_set_struct
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
        completion_time: The time at which the job will finish execution.
        WCET_counter: A counter to check whether the job exceeds the worst case execution time.
        absolute_deadline: The deadline of the job.
        next: A link to the next job in the array.

*/
struct job
{
    double release_time;
    double execution_time;
    double absolute_deadline;
    double scheduled_time;
    double rem_exec_time;
    double completion_time;
    double WCET_counter;
    int job_number;
    int task_number;
    struct job *next;
};

typedef struct job job;

/*
    ADT for job queue. 
        It contains the total number of jobs in ready queue and pointer to the ready queue.
*/
typedef struct job_queue_struct
{
    int num_jobs;
    job *job_list_head;
} job_queue_struct;

/*
    ADT for the core. The parameters for the core are:
        ready_queue: The ready queue of the core.
        curr_exec_job: The job currently executed in the core.
        total_time: The total time for which the core has run.
        total_idle_time: The total time for which the core was idle.
        WCET_counter: The WCET counter of the currently executing job.
        frequency: The frequency at which core is running.
        state: The current state of core. (ACTIVE or SHUTDOWN)
        next_invocation_time: The countdown timer for core. The core will wakeup after timer expires.
        x_factor: The factor to be used while calculating virtual deadlines.
        threshold_crit_lvl: The threshold level aboe which all tasks are considered as HI criticality and below which all tasks are considered as LO criticality.
        rem_util: The remaining utilisation of core. This is needed to check whether additional tasks can be allocated to this core.
        completed_scheduling: Flag to indicate whether this core has completed its hyperperiod.
        is_shutdown: SHUTDOWN or NON-SHUTDOWN core.
        num_tasks_allocated: The number of tasks allocated to that core.
*/
typedef struct core_struct
{
    job_queue_struct *ready_queue;
    job *curr_exec_job;

    double total_time;
    double total_idle_time;
    double WCET_counter;
    double next_invocation_time;

    double frequency;
    int state; //ACTIVE or SHUTDOWN
    double *rem_util;

    double x_factor;
    int threshold_crit_lvl;
    int is_shutdown;

} core_struct;

/*
    ADT for the processor. 
        total_cores: Total number of cores in the processor.
        crit_level: The current criticality level of the processor.
        cores: List of core structs.
*/

typedef struct processor_struct
{
    int total_cores;
    int crit_level;
    core_struct *cores;
} processor_struct;

/*
    ADT for the decision point:
        It contains the type of decision point (ARRIVAL or COMPLETION or TIMER_EXPIRE or CRIT_CHANGE), the decision time and the core for which decision has to be taken.
*/
typedef struct decision_struct
{
    int core_no;
    double decision_time;
    int decision_point;
} decision_struct;

typedef struct x_factor_struct
{
    double x;
    int k;
} x_factor_struct;
 
typedef struct stats_struct
{
    double *total_shutdown_time;
    double *total_idle_energy;
    double *total_active_energy;
    int *total_context_switches;
    int *total_arrival_points;
    int *total_completion_points;
    int *total_criticality_change_points;
    int *total_wakeup_points;
    int *total_discarded_jobs;
    double *total_discarded_jobs_executed;
    double *total_discarded_jobs_available;
} stats_struct;

typedef struct la_edf_struct
{
    int task_number;
    double deadline;
    double exec_time;
}la_edf_struct;

FILE *output_file;
FILE *output[NUM_CORES];
stats_struct *stats;

double frequency[FREQUENCY_LEVELS];
int MAX_CRITICALITY_LEVELS;

#endif