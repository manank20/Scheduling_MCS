#include "check_functions.h"
#include "auxiliary_functions.h"

void find_total_utilisation(int total_tasks, task *tasks_list, double total_utilisation[][MAX_CRITICALITY_LEVELS], int core_no, FILE *output_file)
{
    int i, l, k;

    for (l = 0; l < MAX_CRITICALITY_LEVELS; l++)
    {
        for (k = 0; k < MAX_CRITICALITY_LEVELS; k++)
        {
            total_utilisation[l][k] = 0;
            for (i = 0; i < total_tasks; i++)
            {
                if (tasks_list[i].criticality_lvl == l && tasks_list[i].core == core_no)
                {
                    total_utilisation[l][k] += tasks_list[i].util[k];
                }
            }
        }
    }

    return;
}

/*
    Preconditions:  
        Input: {pointer to taskset, file pointer to output file}

    Purpose of the function: This function checks whether the taskset is schedulable or not. The taskset is schedulable if:
                            U[LOW][LOW] + U[HIGH][HIGH] <= 1 ====> This implies that taskset can be scheduled according to EDF only.
                            U[LOW][LOW] + x*U[HIGH][HIGH] <= 1 ====> This implies that EDF-VD is required.
                            If the taskset is schedulable, then compute the virtual deadlines of all the tasks according to their criticality levels.

    Postconditions: 
        Output: {if taskset is schedulable, return 1. Else return 0}
        Result: The taskset is checked for schedulability and the virtual deadlines of all the tasks is calculated.
*/
double check_schedulability(task_set_struct *task_set, FILE *output_file, int core_no)
{

    double total_utilisation[MAX_CRITICALITY_LEVELS][MAX_CRITICALITY_LEVELS];
    int total_tasks = task_set->total_tasks;
    task *tasks_list = task_set->task_list;
    double check_utilisation = 0.0;
    int criticality_lvl, num_task;
    double x;

    find_total_utilisation(total_tasks, tasks_list, total_utilisation, core_no, output_file);
    // print_total_utilisation(total_utilisation, output_file);

    check_utilisation = 0.0;
    for (criticality_lvl = 0; criticality_lvl < MAX_CRITICALITY_LEVELS; criticality_lvl++)
    {
        check_utilisation += total_utilisation[criticality_lvl][criticality_lvl];
    }

    //If all tasks are able to execute the worst case execution time of their respective criticality level, that is, check_utilisation <= 1
    //then the taskset is schedulable and only EDF is required.
    if (check_utilisation <= 1)
    {
        //x = 1 for all tasksets.
        for (num_task = 0; num_task < total_tasks; num_task++)
            tasks_list[num_task].virtual_deadline = tasks_list[num_task].relative_deadline;
        x = 1.00;
        // fprintf(output_file, "x factor: %.2lf\n", x);
        return x;
    }

    x = (double)total_utilisation[HIGH][LOW] / (double)(1 - total_utilisation[LOW][LOW]);
    // fprintf(output_file, "x factor: %.2lf\n", x);

    check_utilisation = x * total_utilisation[HIGH][HIGH] + total_utilisation[LOW][LOW];

    if (check_utilisation > 1)
        return 0.00;
    else
    {
        return x;
    }
}