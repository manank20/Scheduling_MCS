#ifndef __FUNCTIONS_H_
#define __FUNCTIONS_H_

#include "data_structures.h"

/*---------------------------ALLOCATION FUNCTIONS---------------------------*/
/*
    Preconditions:
        Input: {void}
    
    Purpose of the function: It is used to initialize the processors and create the necessary number of cores.

    Postconditions:
        Output: {Pointer to the processor structure}
                processor!=NULL
*/
extern processor_struct *initialize_processor();

/*
    Preconditions:
        Input: {Pointer to taskset, pointer to processor}

    Purpose of the function: It is used to allocate the tasks to each core.
                             Priority will be given to high criticality tasks. Each core will be alloted high criticality tasks having a utilization of MAX_HIGH_UTIL (defined in data_structures.h)
                            
    Postconditions: 
        Output: If the number of cores is sufficient and all the tasks were allocated to the cores, then it will return 1 to indicate success.
                Else it will return 0.
*/
extern int allocate_tasks_to_cores(task_set_struct *task_set, processor_struct *processor);
/*--------------------------------------------------------------------------*/

/*---------------------------SCHEDULER FUNCTIONS---------------------------*/
/*
    Preconditions: 
        Input: {File pointer to input file}
        fd!=NULL

    Purpose of the function: Takes input from the file and returns a structure of the task set. 

    Postconditions:
        Output: {Pointer to the structure of taskset created}
        task_set!=NULL
    
*/
extern task_set_struct *get_taskset(FILE *fd);

/*
    Preconditions: 
        Input: {pointer to taskset, pointer to kernel, pointer to output file}

    Purpose of the function: This function will perform the offline preprocessing phase and the runtime scheduling of edf-vd.
                             If the taskset is not schedulable, it will return after displaying the same message. Else, it will start the runtime scheduling of the tasket.
                        
    Postconditions: 
        Output: {void}
*/
extern void runtime_scheduler(task_set_struct *task_set, processor_struct *processor);
extern double find_max_slack(task_set_struct *task_set, int crit_level, int core_no, double deadline, double curr_time, job_queue_struct *ready_queue);
/*-------------------------------------------------------------------------*/

/*---------------------------PROCRASTINATION FUNCTIONS---------------------------*/
extern double find_procrastination_interval(double curr_time, task_set_struct *task_set, int curr_crit_level, int core_no);
/*-------------------------------------------------------------------------------*/

/*---------------------------DVFS FUNCTIONS---------------------------*/
extern void select_frequency(core_struct *core, task_set_struct *task_set, int curr_crit_level, int core_no);
/*--------------------------------------------------------------------*/

/*---------------------------CHECK FUNCTIONS---------------------------*/
extern x_factor_struct check_schedulability(task_set_struct *task_set, int core_no);
/*---------------------------------------------------------------------*/

/*---------------------------QUEUE FUNCTIONS---------------------------*/
extern void insert_job_in_discarded_queue(job_queue_struct **discarded_queue, job *new_job, task *task_list);
extern void remove_jobs_from_discarded_queue(job_queue_struct **discarded_queue, double curr_time);
extern void insert_job_in_ready_queue(job_queue_struct **ready_queue, job *new_job);
extern void remove_jobs_from_ready_queue(job_queue_struct **ready_queue, job_queue_struct **discarded_queue, task *task_list, int curr_crit_lvl, int k, int core_no);
extern void insert_discarded_jobs_in_ready_queue(job_queue_struct **ready_queue, job_queue_struct **discarded_queue, task_set_struct *task_set, int core_no, int curr_crit_level, double curr_time, int same_core);
/*---------------------------------------------------------------------*/

/*---------------------------AUXILIARY FUNCTIONS---------------------------*/
extern double gcd(double a, double b);
extern double min(double a, double b);
extern double max(double a, double b);
extern int max_int(int a, int b);
extern int min_int(int a, int b);
extern int period_comparator(const void *p, const void *q);
extern void print_task_list(task_set_struct *task_set);
extern void print_job_list(int core_no, job *job_list_head);
extern void print_total_utilisation(double total_utilisation[][MAX_CRITICALITY_LEVELS]);
extern void print_processor(processor_struct *processor);
extern int compare_jobs(job *A, job *B);
extern double find_actual_execution_time(double exec_time, int task_crit_lvl, int core_crit_lvl);
extern void set_virtual_deadlines(task_set_struct **task_set, int core_no, double x, int k);
extern void reset_virtual_deadlines(task_set_struct **task_set, int core_no, int k);
extern void set_execution_times(job* curr_job, double frequency);
extern void reset_execution_times(job* curr_job, double frequency);
extern void set_utilisation(task* task, int curr_crit_level, double exec_time);
extern void reset_utilisation(task* task, int curr_crit_level);
extern int check_all_cores(processor_struct *processor);
extern int find_max_level(processor_struct *processor, task_set_struct *task_set);
/*-------------------------------------------------------------------------*/

#endif