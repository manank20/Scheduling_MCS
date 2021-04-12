#ifndef __AUXILIARY_FUNCTIONS_H_
#define __AUXILIARY_FUNCTIONS_H_

#include "data_structures.h"

extern double gcd(double a, double b);
extern double min(double a, double b);
extern double max(double a, double b);
extern int period_comparator(const void *p, const void *q);
extern int deadline_comparator(const void *p, const void *q);
extern void print_task_list(task_set_struct *task_set);
extern void print_job_list(job *job_list_head);
extern void print_total_utilisation(double total_utilisation[][MAX_CRITICALITY_LEVELS]);
extern void print_processor(processor_struct *processor);
extern int compare_jobs(job *A, job *B);
extern double find_actual_execution_time(double exec_time, int task_crit_lvl, int core_crit_lvl);
extern void set_virtual_deadlines(task_set_struct **task_set, int core_no, double x, int k);
extern void reset_virtual_deadlines(task_set_struct **task_set, int core_no, int k);

#endif