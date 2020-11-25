#include "processor_functions.h"
#include "check_functions.h"
#include "auxiliary_functions.h"

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
        processor->cores[i].curr_crit_level = LOW;
        processor->cores[i].state = ACTIVE;
        processor->cores[i].next_invocation_time = -1;

        processor->cores[i].rem_util = (double *)malloc(sizeof(double) * MAX_CRITICALITY_LEVELS);
        processor->cores[i].rem_util[LOW] = 1.00;
        processor->cores[i].rem_util[HIGH] = 1.00;
        processor->cores[i].completed_scheduling = 0;
    }

    return processor;
}

int allocate_tasks_to_cores(task_set_struct *task_set, processor_struct *processor, FILE *output_file)
{
    int total_tasks = task_set->total_tasks;
    int num_task, num_core;

    int flag = 1;
    num_core = 0;
    int i;

    double total_high_util[processor->total_cores];
    for (i = 0; i < processor->total_cores; i++)
        total_high_util[i] = 0.00;

    for (num_task = 0; num_task < total_tasks;)
    {
        if (task_set->task_list[num_task].criticality_lvl == HIGH && task_set->task_list[num_task].util[HIGH] <= MAX_HIGH_UTIL)
        {
            total_high_util[num_core] += task_set->task_list[num_task].util[HIGH];
            if (total_high_util[num_core] <= MAX_HIGH_UTIL && processor->cores[num_core].rem_util[HIGH] >= task_set->task_list[num_task].util[HIGH])
            {
                task_set->task_list[num_task].core = num_core;
                double x_factor = check_schedulability(task_set, output_file, num_core);
                if (x_factor == 0.00)
                {
                    fprintf(output_file, "Cannot allocate %d task to %d core. Trying next core\n", num_task, num_core);
                    task_set->task_list[num_task].core = -1;
                    total_high_util[num_core] -= task_set->task_list[num_task].util[HIGH];
                }
                else
                {
                    processor->cores[num_core].x_factor = x_factor;
                    processor->cores[num_core].rem_util[HIGH] -= task_set->task_list[num_task].util[HIGH];
                    fprintf(output_file, "Allocating %d task to %d core\n", num_task, num_core);
                    num_task++;
                }
            }
            else
            {
                total_high_util[num_core] -= task_set->task_list[num_task].util[HIGH];
            }
            num_core = (num_core + 1) % NUM_CORES;
        }
        else
        {
            num_task++;
        }
    }

    if (flag == 0)
    {
        fprintf(output_file, "Insufficient number of cores\n");
        return 0;
    }

    for (num_task = 0; num_task < total_tasks;)
    {
        if (task_set->task_list[num_task].criticality_lvl == HIGH && task_set->task_list[num_task].core == -1)
        {
            if (processor->cores[num_core].rem_util[HIGH] >= task_set->task_list[num_task].util[HIGH])
            {
                task_set->task_list[num_core].core = num_core;
                double x_factor = check_schedulability(task_set, output_file, num_core);
                if (x_factor == 0.00)
                {
                    fprintf(output_file, "Cannot allocate %d task to %d core. Trying next core\n", num_task, num_core);
                    task_set->task_list[num_task].core = -1;

                    if (num_core == NUM_CORES - 1)
                    {
                        flag = 0;
                        break;
                    }
                    else
                    {
                        num_core++;
                    }
                }
                else
                {
                    fprintf(output_file, "Allocating %d task to %d core\n", num_task, num_core);
                    processor->cores[num_core].x_factor = x_factor;
                    processor->cores[num_core].rem_util[HIGH] -= task_set->task_list[num_task].util[HIGH];
                    num_task++;
                }
            }
            else
            {
                if (num_core == NUM_CORES - 1)
                {
                    flag = 0;
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
            num_task++;
        }
    }

    if (flag == 0)
    {
        fprintf(output_file, "Insufficient number of cores\n");
        return 0;
    }

    num_core = 0;
    flag = 1;
    for (num_task = 0; num_task < total_tasks;)
    {
        if (task_set->task_list[num_task].criticality_lvl == LOW)
        {
            if (processor->cores[num_core].rem_util[LOW] >= task_set->task_list[num_task].util[LOW])
            {
                task_set->task_list[num_task].core = num_core;
                double x_factor = check_schedulability(task_set, output_file, num_core);
                if (x_factor == 0.00)
                {
                    // fprintf(output_file, "Cannot allocate %d task to %d core. Trying next core\n", num_task, num_core);
                    task_set->task_list[num_task].core = -1;

                    if (num_core == NUM_CORES - 1)
                    {
                        flag = 0;
                        break;
                    }
                    else
                    {
                        num_core++;
                    }
                }
                else
                {
                    fprintf(output_file, "Allocating %d task to %d core\n", num_task, num_core);
                    processor->cores[num_core].x_factor = x_factor;
                    processor->cores[num_core].rem_util[LOW] -= task_set->task_list[num_task].util[LOW];
                    num_task++;
                }
            }
            else
            {
                if (num_core == NUM_CORES - 1)
                {
                    flag = 0;
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
            num_task++;
        }
    }

    if (flag == 0)
    {
        fprintf(output_file, "Insufficient cores\n");
        return 0;
    }

    fprintf(output_file, "\n");
    for (i = 0; i < processor->total_cores; i++)
    {
        fprintf(output_file, "Core: %d, x factor: %.5lf\n", i, processor->cores[i].x_factor);
        set_virtual_deadlines(&task_set, i, processor->cores[i].x_factor);
    }
    fprintf(output_file, "\n");

    return 1;
}