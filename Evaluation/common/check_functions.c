#include "functions.h"

void find_total_utilisation(int total_tasks, task *tasks_list, double total_utilisation[][MAX_CRITICALITY_LEVELS], int core_no)
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
x_factor_struct check_schedulability(task_set_struct *task_set, int core_no)
{
    // fprintf(output_file, "\n--------------SCHEDULABILITY CHECKS------------\n");

    double total_utilisation[MAX_CRITICALITY_LEVELS][MAX_CRITICALITY_LEVELS];
    int total_tasks = task_set->total_tasks;
    task *tasks_list = task_set->task_list;

    double x, check1;
    double check_utilisation = 0.0;
    int check_feasibility = 1;
    int criticality_lvl;
    int i, j;
    double util_LO_LO = 0.0;
    double util_HI_HI = 0.0;
    double util_HI_LO = 0.0;
    x_factor_struct x_factor;

    find_total_utilisation(total_tasks, tasks_list, total_utilisation, core_no);
    print_total_utilisation(total_utilisation);

    //Condition to be checked for feasible tasksets. The total utilisation at each criticality level should be less than 1.
    for (i = 0; i < MAX_CRITICALITY_LEVELS; i++)
    {
        check_utilisation = 0.0;
        for (j = i; j < MAX_CRITICALITY_LEVELS; j++)
        {
            check_utilisation += total_utilisation[j][i];
        }

        //If check_utilisation is greater than 1, the task set is not feasible on unit speed processor.
        if (check_utilisation > 1)
        {
            check_feasibility = 0;
            break;
        }
    }

    //If check_feasibility is zero, then the taskset is not feasible on a unit speed processor.
    if (check_feasibility == 0)
    {
        x_factor.x = 0.00;
        return x_factor;
    }

    check_utilisation = 0.0;
    for (criticality_lvl = 0; criticality_lvl < MAX_CRITICALITY_LEVELS; criticality_lvl++)
    {
        check_utilisation += total_utilisation[criticality_lvl][criticality_lvl];
    }

    //If all tasks are able to execute the worst case execution time of their respective criticality level, that is, check_utilisation <= 1
    //then the taskset is schedulable and thus, the virtual deadlines are equal to their relative deadlines.
    if (check_utilisation <= 1)
    {
        x_factor.x = 1.00;
        x_factor.k = MAX_CRITICALITY_LEVELS - 1;
        return x_factor;
    }

    //We find the first k for which the required condition is satisfied. Condition given in 2015 Baruah's paper - Page 10, Section 3.2
    for (criticality_lvl = 0; criticality_lvl < MAX_CRITICALITY_LEVELS - 1; criticality_lvl++)
    {
        x = 0.00;
        check1 = 0.00;

        util_LO_LO = 0;
        util_HI_LO = 0;
        util_HI_HI = 0;

        //Calculating the required quantities as specified in paper.
        for (j = 0; j <= criticality_lvl; j++)
        {
            util_LO_LO += total_utilisation[j][j];
        }

        if (util_LO_LO > 1)
            continue;

        for (j = criticality_lvl + 1; j < MAX_CRITICALITY_LEVELS; j++)
        {
            util_HI_HI += total_utilisation[j][j];
            util_HI_LO += total_utilisation[j][criticality_lvl];
        }

        x = (double)util_HI_LO / (double)(1 - util_LO_LO);

        check1 = x * util_LO_LO + util_HI_HI;

        fprintf(output_file, "crit_level: %d, util_LO_LO: %.2lf, util_HI_HI: %.2lf, util_HI_LO: %.2lf, x: %.2lf, check1: %.2lf\n", criticality_lvl, util_LO_LO, util_HI_HI, util_HI_LO, x, check1);

        if (check1 <= 1.00)
        {
            x_factor.x = x;
            x_factor.k = criticality_lvl;
            return x_factor;
        }
    }

    //If no such k exists, then the taskset is not schedulable.
    x_factor.x = 0.00;
    return x_factor;
}