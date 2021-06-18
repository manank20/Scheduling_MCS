#include "functions.h"

processor_struct *initialize_processor()
{
    processor_struct *processor = (processor_struct *)malloc(sizeof(processor_struct));
    int i;

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
        processor->cores[i].state = ACTIVE;
        processor->cores[i].next_invocation_time = INT_MAX;
        processor->cores[i].is_shutdown = -1;
        processor->cores[i].frequency = 1.00;

        processor->cores[i].rem_util = (double *)malloc(sizeof(double) * MAX_CRITICALITY_LEVELS);
        for (int j = 0; j < MAX_CRITICALITY_LEVELS; j++)
        {
            processor->cores[i].rem_util[j] = 1.00;
        }
        processor->cores[i].num_tasks_allocated = 0;
    }

    return processor;
}

int allocate(task_set_struct *task_set, int task_number, processor_struct *processor, double total_util[][MAX_CRITICALITY_LEVELS], double MAX_UTIL[], int exceptional_task, int shutdown, int non_shutdown_cores)
{
    int crit_level = task_set->task_list[task_number].criticality_lvl;
    int k;
    int num_core = 0;

L1:;
    fprintf(output_file, "Task: %d, Core: %d, Crit level: %d | ", task_number, num_core, crit_level);
    int flag = 1;
    for (k = 0; k < crit_level; k++)
    {
        if (processor->cores[num_core].rem_util[k] < task_set->task_list[task_number].util[k])
        {
            fprintf(output_file, "Rem util %.2lf less than task util %.2lf at crit level %d for core %d\n", processor->cores[num_core].rem_util[crit_level], task_set->task_list[task_number].util[k], crit_level, num_core);
            flag = 0;
            break;
        }
    }

    if (flag != 0)
    {
        flag = 0;

        if (exceptional_task == EXCEPTIONAL || (total_util[num_core][crit_level] + task_set->task_list[task_number].util[crit_level]) <= MAX_UTIL[crit_level])
        {
            task_set->task_list[task_number].core = num_core;
            x_factor_struct x_factor = check_schedulability(task_set, num_core);
            if (x_factor.x == 0.00)
            {
                fprintf(output_file, "Schedulability conditions not satisified.\n");
                task_set->task_list[task_number].core = -1;
            }
            else
            {
                flag = 1;
                total_util[num_core][crit_level] += task_set->task_list[task_number].util[crit_level];

                if (processor->cores[num_core].is_shutdown == -1)
                {
                    if (task_set->task_list[task_number].shutdown == SHUTDOWN_TASK)
                    {
                        processor->cores[num_core].is_shutdown = SHUTDOWN_CORE;
                    }
                    else
                    {
                        processor->cores[num_core].is_shutdown = NON_SHUTDOWN_CORE;
                    }
                }

                for (int k = 0; k < crit_level; k++)
                    processor->cores[num_core].rem_util[k] -= task_set->task_list[task_number].util[k];
                processor->cores[num_core].x_factor = x_factor.x;
                processor->cores[num_core].threshold_crit_lvl = x_factor.k;
                processor->cores[num_core].num_tasks_allocated++;
                fprintf(output_file, "Allocating task %d to core %d.\n", task_number, num_core);
            }
        }
        else
        {
            fprintf(output_file, "Max util reached. Util: %.2lf\n", total_util[num_core][crit_level] + task_set->task_list[task_number].util[crit_level]);
        }
    }

    if (flag == 0)
    {
        // fprintf(output_file, "Entered flag = 0 for task %d\n", task_number);
        num_core++;
        if (shutdown == NON_SHUTDOWN_TASK && num_core == non_shutdown_cores)
        {
            non_shutdown_cores++;
        }
        if (num_core == processor->total_cores)
        {
            fprintf(output_file, "Insufficient number of cores\n");
            return 0;
        }
        goto L1;
    }
    else
    {
        return 1;
    }
}

int allocate_tasks_to_cores(task_set_struct *task_set, processor_struct *processor)
{
    int i;
    double MAX_UTIL[MAX_CRITICALITY_LEVELS], total_util[processor->total_cores][MAX_CRITICALITY_LEVELS];
    int crit_level, num_core;
    double non_shutdown_utilisation = 0.00;
    int total_tasks = task_set->total_tasks;

    //Maximum utilisation per criticality level allowed for each core.
    MAX_UTIL[0] = 1.00;

    for (i = 0; i < processor->total_cores; i++)
    {
        for (crit_level = 0; crit_level < MAX_CRITICALITY_LEVELS; crit_level++)
        {
            total_util[i][crit_level] = 0.00;
        }
    }

    //Find the non-shutdown tasks and the shutdown tasks based on the value of 2*P - 2*E.
    for (i = 0; i < total_tasks; i++)
    {
        double interval = 2 * task_set->task_list[i].period - 2 * task_set->task_list[i].WCET[task_set->task_list[i].criticality_lvl];
        if (interval > SHUTDOWN_THRESHOLD)
        {
            task_set->task_list[i].shutdown = SHUTDOWN_TASK;
        }
        else
        {
            task_set->task_list[i].shutdown = NON_SHUTDOWN_TASK;
        }
    }

    //Calculate the total utilisation of non-shutdown tasks.
    fprintf(output_file, "Non shutdown tasks are:\n");
    for (i = 0; i < total_tasks; i++)
    {
        if (task_set->task_list[i].shutdown == NON_SHUTDOWN_TASK)
        {
            fprintf(output_file, "%d ", i);
            non_shutdown_utilisation += task_set->task_list[i].util[task_set->task_list[i].criticality_lvl];
        }
    }
    fprintf(output_file, "\n");

    int non_shutdown_cores = ceil(non_shutdown_utilisation);

    //First allocate the exceptional tasks to the cores.
    fprintf(output_file, "Allocating exceptional tasks first\n");
    for (i = 0; i < total_tasks; i++)
    {
        int curr_crit_lvl = task_set->task_list[i].criticality_lvl;

        if (task_set->task_list[i].util[curr_crit_lvl] > MAX_UTIL[curr_crit_lvl])
        {
            int result = allocate(task_set, i, processor, total_util, MAX_UTIL, EXCEPTIONAL, 0, 0);

            if (result == 0)
            {
                fprintf(output_file, "Insufficient number of cores\n");
                return 0;
            }
        }
    }

    //Now start allocating the non-shutdown tasks.
    fprintf(output_file, "Allocating the non-shutdown tasks\n");
    for (crit_level = MAX_CRITICALITY_LEVELS; crit_level >= 0; crit_level--)
    {
        num_core = 0;
        i = 0;

        for (i = 0; i < total_tasks; i++)
        {
            if (task_set->task_list[i].shutdown == NON_SHUTDOWN_TASK && task_set->task_list[i].core == -1 && task_set->task_list[i].criticality_lvl == crit_level)
            {
                int result = allocate(task_set, i, processor, total_util, MAX_UTIL, 0, NON_SHUTDOWN_TASK, non_shutdown_cores);

                if (result == 0)
                {
                    fprintf(output_file, "Insufficient number of cores\n");
                    return 0;
                }
            }
        }
    }

    //Next, start allocating the shutdown tasks.
    fprintf(output_file, "Allocating the shutdown tasks\n");
    for (crit_level = MAX_CRITICALITY_LEVELS; crit_level >= 0; crit_level--)
    {
        num_core = 0;
        i = 0;
        for (i = 0; i < total_tasks; i++)
        {
            if (task_set->task_list[i].shutdown == SHUTDOWN_TASK && task_set->task_list[i].core == -1 && task_set->task_list[i].criticality_lvl == crit_level)
            {
                int result = allocate(task_set, i, processor, total_util, MAX_UTIL, 0, SHUTDOWN_TASK, 0);

                if (result == 0)
                {
                    fprintf(output_file, "Insufficient number of cores\n");
                    return 0;
                }
            }
        }
    }

    for (i = 0; i < processor->total_cores; i++)
    {
        if (processor->cores[i].num_tasks_allocated == 0)
        {
            processor->cores[i].state = SHUTDOWN;
        }
        else
        {
            processor->cores[i].state = ACTIVE;
            fprintf(output_file, "Core: %d, x factor: %.5lf, K value: %d\n", i, processor->cores[i].x_factor, processor->cores[i].threshold_crit_lvl);
            set_virtual_deadlines(&task_set, i, processor->cores[i].x_factor, processor->cores[i].threshold_crit_lvl);
        }
    }
    fprintf(output_file, "\n");
    return 1;
}