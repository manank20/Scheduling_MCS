#include "functions.h"

int randnum()
{
    int num = 4;
    int val = 0;
    for (int i = 0; i < num; i++)
        val |= (rand() & 1);
    return val;
}

/*Function to calculate gcd of two numbers*/
double gcd(double a, double b)
{
    if (a == 0)
        return b;
    if (b == 0)
        return a;

    if (a == b)
        return a;

    if (a > b)
        return gcd(a - b, b);
    return gcd(a, b - a);
}

/*Function to find min of two numbers*/
double min(double a, double b)
{
    return (a < b) ? a : b;
}

/*Function to find max of two numbers*/
double max(double a, double b)
{
    return (a > b) ? a : b;
}

int max_int(int a, int b)
{
    return (a > b) ? a : b;
}

int min_int(int a, int b)
{
    return (a < b) ? a : b;
}

/*Custom comparator for sorting the task list*/
int period_comparator(const void *p, const void *q)
{
    double l = ((task *)p)->period;
    double r = ((task *)q)->period;

    return (l - r);
}

/*
    Function to print the taskset.
*/
void print_task_list(task_set_struct *task_set)
{
    int i, j, total_tasks;
    task *task_list;

    total_tasks = task_set->total_tasks;
    task_list = task_set->task_list;

    fprintf(output_file, "\nTaskset:\n");
    for (i = 0; i < total_tasks; i++)
    {
        fprintf(output_file, "Task: %d | core: %d | crit_level: %d | phase: %.2lf | rel_deadline: %.2lf | virt_deadline: %.2lf | ",
                i,
                task_list[i].core,
                task_list[i].criticality_lvl,
                task_list[i].phase,
                task_list[i].relative_deadline,
                task_list[i].virtual_deadline);
        fprintf(output_file, "WCET: ");
        for (j = 0; j < MAX_CRITICALITY_LEVELS; j++)
        {
            fprintf(output_file, "%.2lf ", task_list[i].WCET[j]);
        }
        fprintf(output_file, " | Util: ");
        for (j = 0; j < MAX_CRITICALITY_LEVELS; j++)
        {
            fprintf(output_file, "%.3f ", task_list[i].util[j]);
        }
        fprintf(output_file, "\n");
    }
    fprintf(output_file, "\n");

    return;
}

/*
    Function to print the ready queue
*/
void print_job_list(int core_no, job *job_list_head)
{
    job *job_temp = job_list_head;
    // fprintf(output_file, "\n");
    while (job_temp != NULL)
    {
        fprintf(output[core_no], "Job:: Task no: %d  Release time: %.2lf  Exec time: %.2lf  Rem Exec time: %.2lf  WCET_counter: %.2lf  Deadline: %.2lf\n",
                job_temp->task_number,
                job_temp->release_time,
                job_temp->execution_time,
                job_temp->rem_exec_time,
                job_temp->WCET_counter,
                job_temp->absolute_deadline);
        job_temp = job_temp->next;
    }

    return;
}

/*
    Function to print the utilisation matrix. 
*/
void print_total_utilisation(double total_utilisation[][MAX_CRITICALITY_LEVELS])
{
    int i, j;
    fprintf(output_file, "\nTotal utilisation:\n");
    for (i = 0; i < MAX_CRITICALITY_LEVELS; i++)
    {
        for (j = 0; j < MAX_CRITICALITY_LEVELS; j++)
        {
            fprintf(output_file, "%lf  ", total_utilisation[i][j]);
        }
        fprintf(output_file, "\n");
    }
    return;
}

void print_processor(processor_struct *processor)
{
    int i;
    fprintf(output_file, "\nProcessor statistics:\n");
    fprintf(output_file, "Num cores: %d\n", processor->total_cores);
    for (i = 0; i < processor->total_cores; i++)
    {
        fprintf(output_file, "Core: %d, total time: %.2lf, total idle time: %.2lf, total busy time: %.2lf, state: %s\n",
                i,
                processor->cores[i].total_time,
                processor->cores[i].total_idle_time,
                processor->cores[i].total_time - processor->cores[i].total_idle_time,
                (processor->cores[i].state == ACTIVE) ? "ACTIVE" : "SHUTDOWN");
    }
    fprintf(output_file, "\n");
}

/*
    A comparator function to check whether two jobs are equal or not.
*/
int compare_jobs(job *A, job *B)
{
    if (A == NULL && B == NULL)
        return 1;
    else if (A == NULL || B == NULL)
        return 0;

    if (A->task_number == B->task_number && A->absolute_deadline == B->absolute_deadline)
        return 1;
    return 0;
}

/*
    Random number generator to generate a number greater than or less than the given number.
    This will generate execution time greater than the worst case execution time 25% of the time and less than the worst case execution time 75% of the time.
*/
double find_actual_execution_time(double exec_time, int task_crit_lvl, int core_crit_lvl)
{
    double n = rand() % 3;
    if (task_crit_lvl <= core_crit_lvl)
    {
        exec_time = max(1.00, exec_time - n);
    }
    else
    {
        if (randnum() == 0)
        {
            exec_time += n;
        }
        else
        {
            exec_time = max(1.00, exec_time - n);
        }
    }

    return exec_time;
}

void set_virtual_deadlines(task_set_struct **task_set, int core_no, double x, int k)
{
    int num_task;

    for (num_task = 0; num_task < (*task_set)->total_tasks; num_task++)
    {
        if ((*task_set)->task_list[num_task].core == core_no)
        {
            if ((*task_set)->task_list[num_task].criticality_lvl <= k)
            {
                (*task_set)->task_list[num_task].virtual_deadline = (*task_set)->task_list[num_task].relative_deadline;
            }
            else
            {
                (*task_set)->task_list[num_task].virtual_deadline = x * (*task_set)->task_list[num_task].relative_deadline;
            }
        }
    }
    return;
}

void reset_virtual_deadlines(task_set_struct **task_set, int num_core, int k)
{
    int i;
    for (i = 0; i < (*task_set)->total_tasks; i++)
    {
        if ((*task_set)->task_list[i].criticality_lvl > k && (*task_set)->task_list[i].core == num_core)
            (*task_set)->task_list[i].virtual_deadline = (*task_set)->task_list[i].relative_deadline;
    }
    return;
}

void set_execution_times(job* curr_job, double frequency)
{
    curr_job->execution_time /= frequency;
    curr_job->rem_exec_time /= frequency;
    curr_job->WCET_counter /= frequency;
}

void reset_execution_times(job* curr_job, double frequency)
{
    curr_job->execution_time *= frequency;
    curr_job->rem_exec_time *= frequency;
    curr_job->WCET_counter *= frequency;
}

int check_all_cores(processor_struct *processor)
{
    int flag = 1;
    for (int i = 0; i < processor->total_cores; i++)
    {
        if (processor->cores[i].ready_queue->num_jobs != 0)
        {
            flag = 0;
            break;
        }
    }
    return flag;
}

int find_max_level(processor_struct *processor, task_set_struct *task_set)
{
    int max_crit_level = MAX_CRITICALITY_LEVELS - 1;

    for (int i = 0; i < processor->total_cores; i++)
    {
        job *temp = processor->cores[i].ready_queue->job_list_head;

        while (temp)
        {
            max_crit_level = (max_crit_level > task_set->task_list[temp->task_number].criticality_lvl) ? max_crit_level : task_set->task_list[temp->task_number].criticality_lvl;
            temp = temp->next;
        }
    }
    return max_crit_level;
}

stats_struct* initialize_stats_struct()
{
    stats = malloc(sizeof(stats_struct));
    stats->total_active_energy = calloc(NUM_CORES, sizeof(double));
    stats->total_idle_energy = calloc(NUM_CORES, sizeof(double));
    stats->total_shutdown_time = calloc(NUM_CORES, sizeof(double));
    stats->total_arrival_points = calloc(NUM_CORES, sizeof(int));
    stats->total_completion_points = calloc(NUM_CORES, sizeof(int));
    stats->total_criticality_change_points = calloc(NUM_CORES, sizeof(int));
    stats->total_wakeup_points = calloc(NUM_CORES, sizeof(int));
    stats->total_context_switches = calloc(NUM_CORES, sizeof(int));
    stats->total_discarded_jobs = calloc(NUM_CORES, sizeof(int));
    stats->total_discarded_jobs_executed = calloc(NUM_CORES, sizeof(double));
    stats->total_discarded_jobs_available = calloc(NUM_CORES, sizeof(double));

    return stats;
}