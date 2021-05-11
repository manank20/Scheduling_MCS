#include "functions.h"

/*
    Preconditions: 
        Input: {Start time, end time, task set, criticality level of core, core number}

    Purpose of the function: Used for calculating the procrastination interval. A list of jobs occurring between the start time and end time is calculated.

    Postconditions:
        Output: Job list containing jobs between start time and end time
                job_list != NULL

*/
job *find_job_list(double start_time, double end_time, task_set_struct *task_set, int curr_crit_level, int core_no)
{
    job *job_head = NULL, *new_job, *temp;
    int i;
    double release_time;
    task curr_task;
    int total_jobs = 0;
    
    for (i = 0; i < task_set->total_tasks; i++)
    {
        if (task_set->task_list[i].criticality_lvl >= curr_crit_level && task_set->task_list[i].core == core_no)
        {
            curr_task = task_set->task_list[i];
            release_time = task_set->task_list[i].phase + task_set->task_list[i].period * task_set->task_list[i].job_number;
            while (release_time >= start_time && release_time < end_time)
            {
                new_job = malloc(sizeof(job));
                new_job->execution_time = curr_task.WCET[curr_task.criticality_lvl];
                new_job->rem_exec_time = curr_task.WCET[curr_task.criticality_lvl];
                new_job->release_time = release_time;
                new_job->task_number = i;
                new_job->absolute_deadline = release_time + task_set->task_list[i].virtual_deadline;
                new_job->next = NULL;

                if (job_head == NULL)
                {
                    job_head = new_job;
                }
                else
                {
                    if (new_job->absolute_deadline > job_head->absolute_deadline)
                    {
                        new_job->next = job_head;
                        job_head = new_job;
                    }
                    else
                    {
                        temp = job_head;
                        while (temp->next && temp->next->absolute_deadline > new_job->absolute_deadline)
                        {
                            temp = temp->next;
                        }

                        new_job->next = temp->next;
                        temp->next = new_job;
                    }
                }

                release_time += task_set->task_list[i].period;
                total_jobs++;
            }
        }
    }

    return job_head;
}

/*
    Preconditions: 
        Input: {Pointer to taskset, current time, current criticality level, currently executing core}

    Purpose of the function: Used to compute the procrastination interval for the core.
                             The computation is dynamic and is done using the jobs arriving in the future.

    Postconditions:
        Output: The next invocation time of the core 
*/
double find_procrastination_interval(double curr_time, task_set_struct *task_set, int curr_crit_level, int core_no)
{
    double next_deadline1 = INT_MAX, next_deadline2 = INT_MIN;
    int i, crit_level;
    double release_time, absolute_deadline;
    double timer_expiry = 0, total_utilisation = 0;
    double earliest_task_WCET = 0;
    job *job_list, *free_job;

    printf("Finding procrastination interval:\n");

    for (i = 0; i < task_set->total_tasks; i++)
    {
        if (task_set->task_list[i].criticality_lvl >= curr_crit_level && task_set->task_list[i].core == core_no)
        {
            int job_number = task_set->task_list[i].job_number;
            while((release_time = task_set->task_list[i].phase + task_set->task_list[i].period * job_number) < curr_time)
                job_number++;
            absolute_deadline = release_time + task_set->task_list[i].virtual_deadline;
            crit_level = task_set->task_list[i].criticality_lvl;
            if (next_deadline1 > absolute_deadline && curr_time < absolute_deadline)
            {
                next_deadline1 = absolute_deadline;
                earliest_task_WCET = task_set->task_list[i].WCET[crit_level];
            }
        }
    }

    printf("Earliest arriving job Deadline: %.5lf\n", next_deadline1);

    if ((next_deadline1 - curr_time - earliest_task_WCET) < SHUTDOWN_THRESHOLD)
    {
        printf("Interval using Dnext1: %.5lf. Less than SDT\n", (next_deadline1 - curr_time - earliest_task_WCET));
        return (next_deadline1 - curr_time - earliest_task_WCET);
    }

    next_deadline2 = next_deadline1;
    for (i = 0; i < task_set->total_tasks; i++)
    {
        if (task_set->task_list[i].criticality_lvl >= curr_crit_level && task_set->task_list[i].core == core_no)
        {
            int job_number = task_set->task_list[i].job_number;
            while((release_time = task_set->task_list[i].phase + task_set->task_list[i].period * job_number) <= next_deadline1)
                job_number++;
            job_number--;
            release_time = task_set->task_list[i].phase + task_set->task_list[i].period * job_number;
            absolute_deadline = release_time + task_set->task_list[i].virtual_deadline;
            if (next_deadline2 < absolute_deadline && release_time <= next_deadline1)
            {
                next_deadline2 = absolute_deadline;
            }
        }
    }

    printf("Latest arriving job deadline: %.5lf\n", next_deadline2);

    job_list = find_job_list(curr_time, next_deadline2, task_set, curr_crit_level, core_no);
    // printf("Job list:\n");
    // print_job_list(core_no, job_list);

    timer_expiry = next_deadline2;

    for (i = 0; i < task_set->total_tasks; i++)
    {
        if (task_set->task_list[i].criticality_lvl >= curr_crit_level && task_set->task_list[i].core == core_no)
        {
            total_utilisation += task_set->task_list[i].util[curr_crit_level];
        }
    }

    // total_utilisation = (double)((int)(total_utilisation * 100)) / 100;

    printf("Calculation:\n");
    while (job_list != NULL)
    {
        printf("Job: %d, Release time: %.5lf, Execution time: %.5lf, Total utilisation: %.5lf\n", job_list->task_number, job_list->release_time, job_list->execution_time, total_utilisation);
        if (job_list->absolute_deadline > next_deadline2)
        {
            timer_expiry -= ((next_deadline2 - job_list->release_time) * ((double)job_list->execution_time / (double)task_set->task_list[job_list->task_number].period) * total_utilisation);
            printf("If statement\n");
        }
        else
        {
            printf("Else statement\n");
            timer_expiry -= job_list->execution_time;
        }

        if (job_list->next && timer_expiry > job_list->next->absolute_deadline)
        {
            timer_expiry = job_list->next->absolute_deadline;
        }

        printf("Time expiry: %.5lf\n", timer_expiry);

        free_job = job_list;
        job_list = job_list->next;
        free(free_job);
    }
    printf("Time expiry: %.5lf\n", timer_expiry);

    return timer_expiry - curr_time;
}