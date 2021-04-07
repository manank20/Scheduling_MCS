#ifndef __CHECK_FUNCTIONS_H_
#define __CHECK_FUNCTIONS_H_

#include "data_structures.h"

extern void find_utilisation(task_set_struct *task_set);
extern void find_total_utilisation(int total_tasks, task *tasks_list, double total_utilisation[][MAX_CRITICALITY_LEVELS], int core_no);
extern x_factor_struct check_schedulability(task_set_struct *task_set, int core_no);

#endif