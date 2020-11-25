#include "processor_functions.h"
#include "check_functions.h"
#include "auxiliary_functions.h"

processor_struct *initialize_processor()
{
    processor_struct *processor = (processor_struct *)malloc(sizeof(processor_struct));
    int i, j;

    processor->total_cores = NUM_CORES;
    processor->cores = malloc(sizeof(core_struct) * (processor->total_cores));

    for (i = 0; i < processor->total_cores; i++)
    {
        processor->cores[i].ready_queue = (job_queue_struct *)malloc(sizeof(job_queue_struct));
        processor->cores[i].ready_queue->num_jobs = 0;
        processor->cores[i].ready_queue->job_list_head = NULL;
        processor->cores[i].curr_exec_job = NULL;
        processor->cores[i].total_time = 0.0f;
        processor->cores[i].total_idle_time = 0.0f;
        processor->cores[i].curr_crit_level = 0;
        processor->cores[i].state = ACTIVE;
        processor->cores[i].next_invocation_time = -1;
        processor->cores[i].rem_util = 1.00;
        processor->cores[i].completed_scheduling = 0;
        processor->cores[i].num_jobs = 0;
    }

    return processor;
}

int allocate_tasks_to_cores(task_set_struct *task_set, processor_struct *processor, FILE *output_file)
{
    int i, j;
    double MAX_UTIL[MAX_CRITICALITY_LEVELS], total_util[processor->total_cores][MAX_CRITICALITY_LEVELS];

    MAX_UTIL[0] = 1.00, MAX_UTIL[1] = 0.60, MAX_UTIL[2] = 0.40, MAX_UTIL[3] = 0.20;

    for (i = 0; i < (processor->total_cores); i++)
    {
        for (j = 0; j < MAX_CRITICALITY_LEVELS; j++)
        {
            total_util[i][j] = 0.00;
        }
    }

    // for (i = 0; i < total_tasks; i++)
    // {
    //     num_tasks[task_set->task_list[i].criticality_lvl]++;
    // }

    // for (i = 0; i < MAX_CRITICALITY_LEVELS; i++)
    // {
    //     num_tasks_each_core[i] = ceil((double)num_tasks[i] / (double)processor->total_cores);
    // }

    // for (i = MAX_CRITICALITY_LEVELS - 1; i >= 0; i--)
    // {
    //     num_core = 0;
    //     num_task = 0;
    //     while (num_task < total_tasks)
    //     {
    //         // fprintf(output_file, "Alloting task %d\n", num_task);

    //         if (task_set->task_list[num_task].criticality_lvl == i && task_set->task_list[num_task].util[i] <= MAX_UTIL[i])
    //         {
    //             if (tasks_alloted_each_core[num_core][i] == num_tasks_each_core[i])
    //             {
    //                 // fprintf(output_file, "Max tasks of crit %d allocated to %d core. Going to next core\n", i, num_core);
    //                 if (num_core == (processor->total_cores - 1))
    //                 {
    //                     break;
    //                 }
    //                 else
    //                 {
    //                     num_core++;
    //                 }
    //                 continue;
    //             }

    //             total_util[num_core][i] += task_set->task_list[num_task].util[i];

    //             if (processor->cores[num_core].rem_util[i] >= task_set->task_list[num_task].util[i])
    //             {
    //                 // fprintf(output_file, "1\n");
    //                 task_set->task_list[num_task].core = num_core;
    //                 x_factor_struct x_factor = check_schedulability(task_set, output_file, num_core);
    //                 if (x_factor.x == 0.00)
    //                 {
    //                     // fprintf(output_file, "Cannot allocate %d task to %d core. Trying next core\n", num_task, num_core);
    //                     task_set->task_list[num_task].core = -1;
    //                     if (num_core == (processor->total_cores - 1))
    //                     {
    //                         break;
    //                     }
    //                     else
    //                     {
    //                         num_core++;
    //                     }
    //                 }
    //                 else
    //                 {
    //                     // fprintf(output_file, "2\n");
    //                     fprintf(output_file, "Allocating %d task to %d core\n", num_task, num_core);
    //                     tasks_alloted_each_core[num_core][i]++;
    //                     processor->cores[num_core].x_factor = x_factor.x;
    //                     processor->cores[num_core].threshold_crit_lvl = x_factor.k;
    //                     processor->cores[num_core].rem_util[i] -= task_set->task_list[num_task].util[i];
    //                     processor->cores[num_core].num_jobs++;
    //                     num_task++;
    //                 }
    //             }
    //             else
    //             {
    //                 // fprintf(output_file, "3\n");
    //                 total_util[num_core][i] -= task_set->task_list[num_task].util[i];
    //                 if (num_core == (processor->total_cores - 1))
    //                 {
    //                     break;
    //                 }
    //                 else
    //                 {
    //                     num_core++;
    //                 }
    //             }
    //         }
    //         else
    //         {
    //             num_task++;
    //         }
    //     }

    //     num_core = 0;
    //     num_task = 0;
    //     while (num_task < total_tasks)
    //     {
    //         if (task_set->task_list[num_task].criticality_lvl == i && task_set->task_list[num_task].core == -1)
    //         {
    //             if (processor->cores[num_core].rem_util[i] >= task_set->task_list[num_task].util[i])
    //             {
    //                 task_set->task_list[num_task].core = num_core;
    //                 x_factor_struct x_factor = check_schedulability(task_set, output_file, num_core);
    //                 if (x_factor.x == 0.00)
    //                 {
    //                     // fprintf(output_file, "Cannot allocate %d task to %d core. Trying next core...\n", num_task, num_core);
    //                     task_set->task_list[num_task].core = -1;
    //                     if (num_core == (processor->total_cores - 1))
    //                     {
    //                         flag = 0;
    //                         break;
    //                     }
    //                     else
    //                     {
    //                         num_core++;
    //                     }
    //                 }
    //                 else
    //                 {
    //                     fprintf(output_file, "Allocating %d task to %d core\n", num_task, num_core);
    //                     processor->cores[num_core].x_factor = x_factor.x;
    //                     processor->cores[num_core].threshold_crit_lvl = x_factor.k;
    //                     processor->cores[num_core].rem_util[i] -= task_set->task_list[num_task].util[i];
    //                     num_task++;
    //                 }
    //             }
    //             else
    //             {
    //                 if (num_core == (processor->total_cores - 1))
    //                 {
    //                     flag = 0;
    //                     break;
    //                 }
    //                 else
    //                 {
    //                     num_core++;
    //                 }
    //             }
    //         }
    //         else
    //         {
    //             num_task++;
    //         }
    //     }

    //     if (flag == 0)
    //         break;
    // }

    int flag = 1, crit_level;
    double low_period_utilisation = 0.00, high_period_utilisation = 0.00;

    int total_tasks = task_set->total_tasks;
    int low_period_tasks[total_tasks];
    int high_period_tasks[total_tasks];

    for (i = 0; i < total_tasks; i++)
    {
        low_period_tasks[i] = -1;
        high_period_tasks[i] = -1;
    }

    int low_index = 0, high_index = 0;
    for (i = 0; i < total_tasks; i++)
    {
        int interval = 2 * task_set->task_list[i].period - 2 * task_set->task_list[i].WCET[task_set->task_list[i].criticality_lvl];
        if (interval > LOW_PERIOD_THRESHOLD)
        {
            high_period_tasks[high_index] = i;
            high_index++;
        }
        else
        {
            low_period_tasks[low_index] = i;
            low_index++;
        }
    }

    i = 0;
    fprintf(output_file, "Low period tasks are:\n");
    while (i < total_tasks && low_period_tasks[i] != -1)
    {
        fprintf(output_file, "%d\t", low_period_tasks[i]);
        int task_number = low_period_tasks[i];
        low_period_utilisation += task_set->task_list[task_number].util[task_set->task_list[task_number].criticality_lvl];
        i++;
    }
    i = 0;
    fprintf(output_file, "\n");
    fprintf(output_file, "High period tasks are:\n");
    while (i < total_tasks && high_period_tasks[i] != -1)
    {
        fprintf(output_file, "%d\t", high_period_tasks[i]);
        i++;
    }
    fprintf(output_file, "\n");

    fprintf(output_file, "Low period utilisation: %.2lf\nHigh period utilisation: %.2lf\n", low_period_utilisation, high_period_utilisation);

    int num_cores_lp_tasks = ceil(low_period_utilisation);

    for (crit_level = MAX_CRITICALITY_LEVELS; crit_level >= 0; crit_level--)
    {
        int num_core = 0;
        i = 0;
        while (i < total_tasks && low_period_tasks[i] != -1)
        {
            int task_number = low_period_tasks[i];
            if (task_set->task_list[task_number].core == -1 && task_set->task_list[task_number].criticality_lvl == crit_level && task_set->task_list[task_number].util[crit_level] <= MAX_UTIL[crit_level])
            {
                total_util[num_core][crit_level] += task_set->task_list[task_number].util[crit_level];
                if (total_util[num_core][crit_level] <= MAX_UTIL[crit_level] && processor->cores[num_core].rem_util >= task_set->task_list[task_number].util[0])
                {
                    task_set->task_list[task_number].core = num_core;
                    x_factor_struct x_factor = check_schedulability(task_set, output_file, num_core);
                    if (x_factor.x == 0.00)
                    {
                        // fprintf(output_file, "For task %d, x factor is 0\t", task_number);
                        task_set->task_list[task_number].core = -1;
                        total_util[num_core][crit_level] -= task_set->task_list[task_number].util[crit_level];
                        if (num_core == num_cores_lp_tasks - 1)
                        {
                            // fprintf(output_file, "Cores over1\n");
                            break;
                        }
                        else
                        {
                            // fprintf(output_file, "Next core1\n");
                            num_core++;
                        }
                    }
                    else
                    {
                        fprintf(output_file, "Allocating %d task to %d core\n", task_number, num_core);
                        processor->cores[num_core].rem_util -= task_set->task_list[task_number].util[crit_level];
                        processor->cores[num_core].x_factor = x_factor.x;
                        processor->cores[num_core].threshold_crit_lvl = x_factor.k;
                        processor->cores[num_core].num_jobs++;
                        i++;
                    }
                }
                else
                {
                    // fprintf(output_file, "For task %d, Total high util reached\t", task_number);
                    total_util[num_core][crit_level] -= task_set->task_list[task_number].util[crit_level];
                    if (num_core == num_cores_lp_tasks - 1)
                    {
                        // fprintf(output_file, "Cores over2\n");
                        break;
                    }
                    else
                    {
                        // fprintf(output_file, "Next core1\n");
                        num_core++;
                    }
                }
            }
            else
            {
                i++;
            }
        }

        num_core = 0;
        i = 0;
        while (i < total_tasks && low_period_tasks[i] != -1)
        {
            int task_number = low_period_tasks[i];
            if (task_set->task_list[task_number].core == -1 && task_set->task_list[task_number].criticality_lvl == crit_level)
            {
                if (processor->cores[num_core].rem_util >= task_set->task_list[task_number].util[0])
                {
                    task_set->task_list[task_number].core = num_core;
                    x_factor_struct x_factor = check_schedulability(task_set, output_file, num_core);
                    if (x_factor.x == 0.00)
                    {
                        task_set->task_list[task_number].core = -1;
                        total_util[num_core][crit_level] -= task_set->task_list[task_number].util[crit_level];
                        if (num_core == processor->total_cores - 1)
                        {
                            break;
                        }
                        else
                        {
                            num_core++;
                        }
                    }
                    else
                    {
                        fprintf(output_file, "Allocating %d task to %d core\n", task_number, num_core);
                        processor->cores[num_core].rem_util -= task_set->task_list[task_number].util[crit_level];
                        processor->cores[num_core].x_factor = x_factor.x;
                        processor->cores[num_core].threshold_crit_lvl = x_factor.k;
                        processor->cores[num_core].num_jobs++;
                        i++;
                    }
                }
                else
                {
                    // fprintf(output_file, "For task %d and util %.2lf, core %d has rem util %.2lf\t", task_number, task_set->task_list[task_number].util[crit_level], num_core, processor->cores[num_core].rem_util);
                    if (num_core == processor->total_cores - 1)
                    {
                        // fprintf(output_file, "Cores over\n");
                        break;
                    }
                    else
                    {
                        // fprintf(output_file, "Next core\n");
                        num_core++;
                    }
                }
            }
            else
            {
                i++;
            }
        }
    }

    for (crit_level = MAX_CRITICALITY_LEVELS; crit_level >= 0; crit_level--)
    {
        int num_core = 0;
        i = 0;
        while (i < total_tasks && high_period_tasks[i] != -1)
        {
            int task_number = high_period_tasks[i];
            if (task_set->task_list[task_number].core == -1 && task_set->task_list[task_number].criticality_lvl == crit_level && task_set->task_list[task_number].util[crit_level] <= MAX_UTIL[crit_level])
            {
                total_util[num_core][crit_level] += task_set->task_list[task_number].util[crit_level];
                if (total_util[num_core][crit_level] <= MAX_UTIL[crit_level] && processor->cores[num_core].rem_util >= task_set->task_list[task_number].util[0])
                {
                    task_set->task_list[task_number].core = num_core;
                    x_factor_struct x_factor = check_schedulability(task_set, output_file, num_core);
                    if (x_factor.x == 0.00)
                    {
                        task_set->task_list[task_number].core = -1;
                        total_util[num_core][crit_level] -= task_set->task_list[task_number].util[crit_level];
                        if (num_core == processor->total_cores - 1)
                        {
                            break;
                        }
                        else
                        {
                            num_core++;
                        }
                    }
                    else
                    {
                        fprintf(output_file, "Allocating %d task to %d core\n", task_number, num_core);
                        processor->cores[num_core].rem_util -= task_set->task_list[task_number].util[crit_level];
                        processor->cores[num_core].x_factor = x_factor.x;
                        processor->cores[num_core].threshold_crit_lvl = x_factor.k;
                        processor->cores[num_core].num_jobs++;
                        i++;
                    }
                }
                else
                {
                    total_util[num_core][crit_level] -= task_set->task_list[task_number].util[crit_level];
                    if (num_core == processor->total_cores - 1)
                    {
                        break;
                    }
                    else
                    {
                        num_core++;
                    }
                }
            }
            else
            {
                i++;
            }
        }

        num_core = 0;
        i = 0;
        while (i < total_tasks && high_period_tasks[i] != -1)
        {
            int task_number = high_period_tasks[i];
            if (task_set->task_list[task_number].core == -1 && task_set->task_list[task_number].criticality_lvl == crit_level)
            {
                if (processor->cores[num_core].rem_util >= task_set->task_list[task_number].util[0])
                {
                    task_set->task_list[task_number].core = num_core;
                    x_factor_struct x_factor = check_schedulability(task_set, output_file, num_core);
                    if (x_factor.x == 0.00)
                    {
                        // fprintf(output_file, "For task %d, x factor is 0\t", task_number);
                        task_set->task_list[task_number].core = -1;
                        total_util[num_core][crit_level] -= task_set->task_list[task_number].util[crit_level];
                        if (num_core == num_cores_lp_tasks - 1)
                        {
                            // fprintf(output_file, "Cores over1\n");
                            flag = 0;
                            break;
                        }
                        else
                        {
                            // fprintf(output_file, "Next core1\n");
                            num_core++;
                        }
                    }
                    else
                    {
                        fprintf(output_file, "Allocating %d task to %d core\n", task_number, num_core);
                        processor->cores[num_core].rem_util -= task_set->task_list[task_number].util[crit_level];
                        processor->cores[num_core].x_factor = x_factor.x;
                        processor->cores[num_core].threshold_crit_lvl = x_factor.k;
                        processor->cores[num_core].num_jobs++;
                        i++;
                    }
                }
                else
                {
                    if (num_core == processor->total_cores - 1)
                    {
                        // fprintf(output_file, "Cores over2\n");
                        flag = 0;
                        break;
                    }
                    else
                    {
                        // fprintf(output_file, "Next core2\n");
                        num_core++;
                    }
                }
            }
            else
            {
                i++;
            }
        }

        if(flag == 0)
            break;
    }

    if (flag == 0)
    {
        fprintf(output_file, "Insufficient number of cores\n");
        return 0;
    }

    for (i = 0; i < processor->total_cores; i++)
    {
        fprintf(output_file, "Core: %d, x factor: %.5lf, K value: %d\n", i, processor->cores[i].x_factor, processor->cores[i].threshold_crit_lvl);
        set_virtual_deadlines(&task_set, i, processor->cores[i].x_factor, processor->cores[i].threshold_crit_lvl);
    }
    fprintf(output_file, "\n");
    return 1;
}