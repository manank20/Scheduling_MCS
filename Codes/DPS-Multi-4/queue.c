#include "functions.h"

/*
    Preconditions: 
        Input: {pointer to discarded job queue, pointer to discarded job, task list}
                discarded_queue!=NULL
                task_list!=NULL
                new_job!=NULL

    Purpose of the function: Insert a new job in the discarded queue, sorted according to the deadline and the criticality level.

    Postconditions:
        Output: {null}
*/
void insert_job_in_discarded_queue(job_queue_struct **discarded_queue, job *new_job, task *task_list)
{
    job *temp;

    if ((*discarded_queue)->num_jobs == 0)
    {
        (*discarded_queue)->job_list_head = new_job;
        (*discarded_queue)->num_jobs++;
    }
    else
    {
        if (task_list[new_job->task_number].criticality_lvl > task_list[(*discarded_queue)->job_list_head->task_number].criticality_lvl || (task_list[new_job->task_number].criticality_lvl == task_list[(*discarded_queue)->job_list_head->task_number].criticality_lvl && new_job->absolute_deadline < (*discarded_queue)->job_list_head->absolute_deadline))
        {
            new_job->next = (*discarded_queue)->job_list_head;
            (*discarded_queue)->job_list_head = new_job;
        }
        else
        {
            temp = (*discarded_queue)->job_list_head;

            while (temp && temp->next && (task_list[temp->next->task_number].criticality_lvl > task_list[new_job->task_number].criticality_lvl || (task_list[temp->next->task_number].criticality_lvl == task_list[new_job->task_number].criticality_lvl && temp->next->absolute_deadline <= new_job->absolute_deadline)))
            {
                temp = temp->next;
            }
            new_job->next = temp->next;
            temp->next = new_job;
        }
        (*discarded_queue)->num_jobs++;
    }
}

void remove_jobs_from_discarded_queue(job_queue_struct **discarded_queue, double curr_time)
{
    job *temp, *free_job;

    while ((*discarded_queue)->num_jobs != 0 && (*discarded_queue)->job_list_head->absolute_deadline < curr_time)
    {
        // fprintf(output_file, "Removing discarded %d,%d job\n", (*discarded_queue)->job_list_head->task_number, (*discarded_queue)->job_list_head->job_number);
        free_job = (*discarded_queue)->job_list_head;
        (*discarded_queue)->job_list_head = (*discarded_queue)->job_list_head->next;
        (*discarded_queue)->num_jobs--;
        free(free_job);
    }

    if ((*discarded_queue)->num_jobs == 0)
        return;

    temp = (*discarded_queue)->job_list_head;

    while (temp && temp->next)
    {
        if (temp->next->absolute_deadline < curr_time)
        {
            // fprintf(output_file, "Removing discarded %d,%d job\n", temp->task_number, temp->job_number);
            free_job = temp->next;
            temp->next = temp->next->next;
            (*discarded_queue)->num_jobs--;
            free(free_job);
        }
        else
        {
            temp = temp->next;
        }
    }
    return;
}

/*
    Preconditions:
        Input: {pointer to ready queue (passed by pointer), pointer to job to be inserted}
                (*ready_queue)!=NULL
                new_job!=NULL

    Purpose of the function: This function enters a new job in the ready queue in the appropriate location. The ready queue is sorted according to the deadlines.
                            
    Postconditions: 
        Output: {void}
        Result: A new ready queue with the newly arrived job inserted in the correct position.
*/
void insert_job_in_ready_queue(job_queue_struct **ready_queue, job *new_job)
{
    job *temp;

    if ((*ready_queue)->num_jobs == 0)
    {
        (*ready_queue)->job_list_head = new_job;
        (*ready_queue)->num_jobs = 1;
    }
    else
    {
        if (new_job->absolute_deadline < (*ready_queue)->job_list_head->absolute_deadline)
        {
            new_job->next = (*ready_queue)->job_list_head;
            (*ready_queue)->job_list_head = new_job;
        }
        else
        {

            temp = (*ready_queue)->job_list_head;

            while (temp->next != NULL && temp->next->absolute_deadline <= new_job->absolute_deadline)
            {
                temp = temp->next;
            }
            new_job->next = temp->next;
            temp->next = new_job;
        }

        (*ready_queue)->num_jobs++;
    }

    return;
}

/*
    Preconditions:
        Input: {pointer to the job queue, pointer to the taskset}
                ready_queue!=NULL
                task_list!=NULL
    
    Purpose of the function: This function will remove all the low-criticality jobs from the ready queue.

    Postconditions:
        Output: {void}
        Result: The job queue will now contain only high criticality jobs.
*/
void remove_jobs_from_ready_queue(job_queue_struct **ready_queue, job_queue_struct **discarded_queue, task *task_list, int curr_crit_lvl, int k, int core_no)
{
    job *temp, *free_job;

    while ((*ready_queue)->num_jobs != 0 && task_list[(*ready_queue)->job_list_head->task_number].criticality_lvl < curr_crit_lvl)
    {
        job *free_job = (*ready_queue)->job_list_head;
        (*ready_queue)->job_list_head = (*ready_queue)->job_list_head->next;
        (*ready_queue)->num_jobs--;
        // free(free_job);
        free_job->next = NULL;
        // fprintf(output[core_no], "Job %d,%d inserted in discarded queue\n", free_job->task_number, free_job->job_number);
        insert_job_in_discarded_queue(discarded_queue, free_job, task_list);
    }

    if ((*ready_queue)->num_jobs == 0)
        return;

    temp = (*ready_queue)->job_list_head;

    while (temp && temp->next)
    {
        if (task_list[temp->next->task_number].criticality_lvl < curr_crit_lvl)
        {
            free_job = temp->next;
            temp->next = temp->next->next;
            (*ready_queue)->num_jobs--;
            // free(free_job);
            // fprintf(output[core_no], "Job %d,%d inserted in discarded queue\n", free_job->task_number, free_job->job_number);
            free_job->next = NULL;
            insert_job_in_discarded_queue(discarded_queue, free_job, task_list);
        }
        else
        {
            if (curr_crit_lvl > k)
            {
                temp->absolute_deadline -= task_list[temp->task_number].virtual_deadline;
                temp->absolute_deadline += task_list[temp->task_number].relative_deadline;
            }
            temp = temp->next;
        }
    }

    return;
}

/*
    Preconditions: 
        Input: {Pointer to ready queue, pointer to discarded queue, pointer to taskset, core number, current crit level, current time}

    Purpose of the function: This function will find the maximum slack available for each job present in the discarded queue. If the slack is greater than the remaining execution time of the job, it is added to the ready queue.

    Postconditions:
        Output: {None}
        Result: The appropriate jobs are transferred from discarded queue to ready queue.
*/
void insert_discarded_jobs_in_ready_queue(job_queue_struct **ready_queue, job_queue_struct **discarded_queue, task_set_struct *task_set, int core_no, int curr_crit_level, double curr_time, int same_core)
{
    job *discarded_job, *ready_job, *temp;
    double max_slack, rem_exec_time;

    fprintf(output[core_no], "Discarded job list\n");
    print_job_list(core_no, (*discarded_queue)->job_list_head);

    if ((*discarded_queue)->num_jobs == 0)
        return;

    fprintf(output[core_no], "Accommodate discarded jobs in ready queue of core %d\n", core_no);

    //First try to accommodate the jobs belonging to this core.
    while ((*discarded_queue)->num_jobs != 0)
    {
        if (same_core == 0 || (same_core == 1 && task_set->task_list[(*discarded_queue)->job_list_head->task_number].core == core_no))
        {
            discarded_job = (*discarded_queue)->job_list_head;
            max_slack = find_max_slack(task_set, curr_crit_level, core_no, discarded_job->absolute_deadline, curr_time, (*ready_queue));
            rem_exec_time = discarded_job->rem_exec_time;
            fprintf(output[core_no], "Discarded job: %d, max slack: %.5lf, rem exec time: %.5lf\n", discarded_job->task_number, max_slack, rem_exec_time);

            if (max_slack > rem_exec_time)
            {
                (*discarded_queue)->job_list_head = (*discarded_queue)->job_list_head->next;
                (*discarded_queue)->num_jobs--;
                discarded_job->next = NULL;
                fprintf(output[core_no], "Job %d,%d inserted in ready queue of core %d\n", discarded_job->task_number, discarded_job->job_number, core_no);
                insert_job_in_ready_queue(ready_queue, discarded_job);
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    if ((*discarded_queue)->num_jobs == 0)
        return;

    temp = (*discarded_queue)->job_list_head;
    while (temp && temp->next)
    {
        if (same_core == 0 || (same_core == 1 && task_set->task_list[temp->next->task_number].core == core_no))
        {
            max_slack = find_max_slack(task_set, curr_crit_level, core_no, temp->next->absolute_deadline, curr_time, (*ready_queue));
            rem_exec_time = temp->next->rem_exec_time;
            fprintf(output[core_no], "Discarded job: %d, max slack: %.5lf, rem exec time: %.5lf\n", temp->next->task_number, max_slack, rem_exec_time);

            if (max_slack > rem_exec_time)
            {
                ready_job = temp->next;
                temp->next = temp->next->next;
                ready_job->next = NULL;
                (*discarded_queue)->num_jobs--;
                fprintf(output[core_no], "Job %d,%d inserted in ready queue of core %d\n", temp->task_number, temp->job_number, core_no);
                insert_job_in_ready_queue(ready_queue, ready_job);
            }
            else
            {
                temp = temp->next;
            }
        }
        else
        {
            temp = temp->next;
        }
    }

    return;
}