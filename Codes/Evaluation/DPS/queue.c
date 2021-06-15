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
    job *free_job, *prev, *curr, *dummy_node;
    dummy_node = malloc(sizeof(job));
    dummy_node->next = (*discarded_queue)->job_list_head;
    dummy_node->task_number = -1;
    dummy_node->job_number = -1;

    prev = dummy_node;
    curr = (*discarded_queue)->job_list_head;

    while(curr != NULL)
    {
        // printf("Prev: %d,%d. Curr: %d,%d\n", prev->task_number, prev->job_number, curr->task_number, curr->job_number);
        if(curr->absolute_deadline <= curr_time)
        {
            free_job = curr;
            prev->next = curr->next;
            curr = curr->next;
            free_job->next = NULL;
            (*discarded_queue)->num_jobs--;
            // free(free_job);
        }
        else
        {
            prev = curr;
            curr = curr->next;
        }
    }

    (*discarded_queue)->job_list_head = dummy_node->next;
    dummy_node->next = NULL;
    free(dummy_node);

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
    job *free_job, *dummy_node, *prev, *curr;
    dummy_node = malloc(sizeof(job));
    dummy_node->task_number = -1;
    dummy_node->job_number = -1;
    dummy_node->next = (*ready_queue)->job_list_head;
    prev = dummy_node;
    curr = (*ready_queue)->job_list_head;

    while(curr != NULL)
    {
        // printf("Prev: %d,%d. Curr: %d,%d\n", prev->task_number, prev->job_number, curr->task_number, curr->job_number);
        if(task_list[curr->task_number].criticality_lvl < curr_crit_lvl)
        {
            free_job = curr;
            prev->next = curr->next;
            curr = curr->next;
            (*ready_queue)->num_jobs--;
            free_job->next = NULL;
            insert_job_in_discarded_queue(discarded_queue, free_job, task_list);
        }
        else
        {
            if(curr_crit_lvl > k)
            {
                curr->absolute_deadline -= task_list[curr->task_number].virtual_deadline;
                curr->absolute_deadline += task_list[curr->task_number].relative_deadline;
            }
            curr->WCET_counter -= (task_list[curr->task_number].WCET[curr_crit_lvl - 1]);
            curr->WCET_counter += (task_list[curr->task_number].WCET[curr_crit_lvl]);
            prev = curr;
            curr = curr->next;
        }
    }

    (*ready_queue)->job_list_head = dummy_node->next;
    dummy_node->next = NULL;
    free(dummy_node);

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
    job *ready_job, *prev, *curr, *dummy_node;

    dummy_node = (job*)malloc(sizeof(job));
    dummy_node->next = (*discarded_queue)->job_list_head;

    prev = dummy_node;
    curr = (*discarded_queue)->job_list_head;

    double max_slack, rem_exec_time;
    int crit_level;

    fprintf(output[core_no], "Discarded job list\n");
    print_job_list(core_no, (*discarded_queue)->job_list_head);

    if ((*discarded_queue)->num_jobs == 0)
        return;

    fprintf(output[core_no], "Accommodating discarded jobs in ready queue of core %d\n", core_no);

    while(curr != NULL)
    {
        if(same_core == 0 || (same_core == 1 && task_set->task_list[curr->task_number].core == core_no))
        {
            crit_level = task_set->task_list[curr->task_number].criticality_lvl;            
            rem_exec_time = task_set->task_list[curr->task_number].WCET[crit_level] - (curr->execution_time - curr->rem_exec_time);
            fprintf(output[core_no], "Discarded job: %d,%d, Exec time: %5lf, ", curr->task_number, curr->job_number, rem_exec_time);
            
            max_slack = find_max_slack(task_set, curr_crit_level, core_no, curr->absolute_deadline, curr_time, (*ready_queue));
            fprintf(output[core_no], "Max slack: %.5lf | ", max_slack);

            if(max_slack > rem_exec_time)
            {
                ready_job = curr;
                prev->next = curr->next;
                curr = curr->next;
                ready_job->next = NULL;
                (*discarded_queue)->num_jobs--;
                fprintf(output[core_no], "Job %d,%d inserted in ready queue of core %d\n", ready_job->task_number, ready_job->job_number, core_no);
                insert_job_in_ready_queue(ready_queue, ready_job); 
            }
            else
            {
                prev = curr;
                curr = curr->next;
            }
        }
        else
        {
            prev = curr;
            curr = curr->next;
        }
    }

    (*discarded_queue)->job_list_head = dummy_node->next;
    dummy_node->next = NULL;
    free(dummy_node);

    return;
}