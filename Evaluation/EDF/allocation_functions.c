#include "../common/functions.h"

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
        processor->cores[i].x_factor = 0.00;
        processor->cores[i].threshold_crit_lvl = -1;

        processor->cores[i].rem_util = (double *)malloc(sizeof(double) * MAX_CRITICALITY_LEVELS);
        for (int j = 0; j < MAX_CRITICALITY_LEVELS; j++)
        {
            processor->cores[i].rem_util[j] = 1.00;
        }
    }

    return processor;
}

int allocate_tasks_to_cores(task_set_struct *task_set, processor_struct *processor)
{
    int i;

    FILE* cores_file;
    cores_file = fopen("../input_cores.txt", "r");

    for (i = 0; i < processor->total_cores; i++)
    {
        fscanf(cores_file, "%lf%d", &(processor->cores[i].x_factor), &(processor->cores[i].threshold_crit_lvl));
        if (processor->cores[i].x_factor == 0)
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
    fclose(cores_file);
    return 1;
}