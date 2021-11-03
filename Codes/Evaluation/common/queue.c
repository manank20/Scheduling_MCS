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
void insert_job_in_discarded_queue(job_queue_struct **discarded_queue, job *new_job, task *task_list, int core_no)
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
            stats->total_discarded_jobs_available[core_no] += free_job->rem_exec_time;
            insert_job_in_discarded_queue(discarded_queue, free_job, task_list, core_no);
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