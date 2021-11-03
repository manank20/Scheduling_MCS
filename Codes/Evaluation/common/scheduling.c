#include "functions.h"

/*
    Preconditions: 
        Input: {the pointer to taskset}
                task_set!=NULL

    Purpose of the function: The function will find the hyperperiod of all the tasks in the taskset. The core will run for exactly one hyperperiod.

    Postconditions:
        Output: {The hyperperiod is returned}
        
*/
double find_superhyperperiod(task_set_struct *task_set)
{
    double lcm;
    int num_task;

    lcm = 1;
    for (num_task = 0; num_task < task_set->total_tasks; num_task++)
    {
        lcm = (lcm * task_set->task_list[num_task].period) / gcd(lcm, task_set->task_list[num_task].period);
    }

    return lcm;
}

/*
    Preconditions:
        Input: {pointer to taskset, current criticality level} 
    
    Purpose of the function: This function finds the time of earliest arriving job. 

    Postconditions:
        Output: {The arrival time of earliest arriving job}

*/
double find_earliest_arrival_job(task_set_struct *task_set, int core_no, int curr_crit_level)
{

    double min_arrival_time = INT_MAX;
    int i;

    for (i = 0; i < task_set->total_tasks; i++)
    {
        if (task_set->task_list[i].core == core_no)
        {
            min_arrival_time = min(min_arrival_time, task_set->task_list[i].phase + task_set->task_list[i].period * task_set->task_list[i].job_number);
        }
    }

    return min_arrival_time;
}

/*
    Preconditions: 
        Input: {pointer to taskset, pointer to ready queue, pointer to core}

    Purpose of the function: This function will find the next decision point of the core. 
                             The decision point will be the minimum of the earliest arrival job, the completion time of currently executing job, the WCET counter of currently executing job and the timer expiry of the core.

    Postconditions: 
        Output: {the decision point, decision time}
        Decision point = ARRIVAL or COMPLETION or TIMER_EXPIRE or CRIT_CHANGE
        
  
*/
decision_struct find_decision_point(task_set_struct *task_set, processor_struct *processor, double super_hyperperiod)
{

    double arrival_time, completion_time, expiry_time, WCET_counter;
    double min_time;
    decision_struct decision;
    double decision_time = INT_MAX;
    int decision_core;
    int decision_point;
    int i, flag = 0;

    decision.core_no = -1;

    // fprintf(output[decision_core], "Finding decision point\n");
    for (i = 0; i < processor->total_cores; i++)
    {
        completion_time = INT_MAX;
        expiry_time = INT_MAX;
        WCET_counter = INT_MAX;
        arrival_time = INT_MAX;
        flag = 0;

        if (processor->cores[i].state == ACTIVE)
        {
            arrival_time = find_earliest_arrival_job(task_set, i, processor->crit_level);
        }
        else
        {
            expiry_time = processor->cores[i].next_invocation_time;
        }

        if (processor->cores[i].curr_exec_job != NULL)
        {
            flag = 1;
            completion_time = processor->cores[i].curr_exec_job->completion_time;
            if (processor->crit_level < (MAX_CRITICALITY_LEVELS - 1))
            {
                WCET_counter = processor->cores[i].WCET_counter;
            }
        }

        min_time = min(min(min(arrival_time, completion_time), WCET_counter), expiry_time);
        if (decision_time > min_time)
        {
            decision_time = min_time;
            decision_core = i;

            if (decision_time == completion_time)
            {
                decision_point = COMPLETION;
            }
            else if (decision_time == expiry_time)
            {
                decision_point = TIMER_EXPIRE;
            }
            else if (decision_time == WCET_counter)
            {
                decision_point = CRIT_CHANGE;
            }
            else if (decision_time == arrival_time)
            {
                decision_point = ARRIVAL;
            }
            // fprintf(output[i], "Core: %d, Curr job: %d,%d. Arrival time: %.5lf, Completion time: %.5lf, Timer expiry: %.5lf, WCET counter: %.5lf\n", i, (flag == 1 ? processor->cores[i].curr_exec_job->task_number : -1), (flag == 1 ? processor->cores[i].curr_exec_job->job_number : -1), arrival_time, completion_time, expiry_time, WCET_counter);
        }
    }

    decision.core_no = decision_core;
    decision.decision_point = decision_point;
    decision.decision_time = decision_time;

    return decision;
}

/*
    Preconditions:
        Input: {pointer to taskset, pointer to ready queue, the current crit level, the core number, the curr time and the deadline}
                task_set!=NULL
                ready_queue!=NULL

    Function to find the maximum slack between the discarded job's deadline and the current time.

    Postconditions:
        Output: {The maximum slack available between the current time and the deadline for the given core}
*/
double find_max_slack(task_set_struct *task_set, int crit_level, int core_no, double deadline, double curr_time, job_queue_struct *ready_queue)
{
    int i, task_number, task_crit_level;
    double max_slack = deadline - curr_time;

    fprintf(output[core_no], "Function to find maximum slack\n");
    fprintf(output[core_no], "Max slack: %.5lf, Deadline: %.5lf, Curr time: %.5lf\n", max_slack, deadline, curr_time);

    job *temp = ready_queue->job_list_head;

    fprintf(output[core_no], "Traversing ready queue\n");

    //First traverse the ready queue and update the maximum slack according to remaining execution time of jobs.
    while (temp)
    {
        task_number = temp->task_number;
        task_crit_level = task_set->task_list[task_number].criticality_lvl;
        double rem_exec_time = task_set->task_list[task_number].WCET[task_crit_level] - (temp->execution_time - temp->rem_exec_time);

        if(temp->absolute_deadline > deadline) {
            max_slack -= (deadline - curr_time) / (temp->absolute_deadline - curr_time) * rem_exec_time;
        }
        else {
            max_slack -= rem_exec_time;
        }
        fprintf(output[core_no], "Job: %d, rem execution time: %.5lf, deadline: %.5lf, max slack: %.5lf\n", temp->task_number, rem_exec_time, temp->absolute_deadline, max_slack);
        temp = temp->next;
    }

    fprintf(output[core_no], "Traversing task list\n");
    //Then, traverse the task list and update the maximum slack according to future invocations of the tasks.
    for (i = 0; i < task_set->total_tasks; i++)
    {
        if (task_set->task_list[i].core == core_no && task_set->task_list[i].criticality_lvl >= crit_level)
        {
            int curr_jobs = task_set->task_list[i].job_number;
            task_crit_level = task_set->task_list[i].criticality_lvl;
            double exec_time = task_set->task_list[i].WCET[task_crit_level];
            double release_time;
            while ((release_time = (task_set->task_list[i].phase + task_set->task_list[i].period * curr_jobs)) < deadline)
            {
                double task_deadline = (release_time + task_set->task_list[i].virtual_deadline);
                if(task_deadline > deadline) {
                    max_slack -= (deadline - release_time) / task_set->task_list[i].period * exec_time;
                }
                else {
                    max_slack -= exec_time;
                }
                fprintf(output[core_no], "Task: %d, exec time: %.5lf, deadline: %.5lf, max slack: %.5lf\n", i, exec_time, task_deadline, max_slack);
                curr_jobs++;
            }
        }
    }

    return max(max_slack, 0.00);
}

void accommodate_discarded_jobs(job_queue_struct **ready_queue, job_queue_struct **discarded_queue, task_set_struct *task_set, int core_no, int curr_crit_level, double curr_time)
{
    job *ready_job, *prev, *curr, *dummy_node;

    dummy_node = (job*)malloc(sizeof(job));
    dummy_node->next = (*discarded_queue)->job_list_head;

    double max_slack, rem_exec_time;
    int crit_level;

    fprintf(output[core_no], "Discarded job list\n");
    print_job_list(core_no, (*discarded_queue)->job_list_head);

    if ((*discarded_queue)->num_jobs == 0)
        return;

    fprintf(output[core_no], "Accommodating discarded jobs in ready queue of core %d\n", core_no);

    for(int i=MAX_CRITICALITY_LEVELS - 1; i >= 0; i--) {
        prev = dummy_node;
        curr = (*discarded_queue)->job_list_head;
        while(curr != NULL)
        {
            if(task_set->task_list[curr->task_number].criticality_lvl == i && task_set->task_list[curr->task_number].core == core_no)
            {
                crit_level = task_set->task_list[curr->task_number].criticality_lvl;            
                rem_exec_time = task_set->task_list[curr->task_number].WCET[crit_level] - (curr->execution_time - curr->rem_exec_time);
                fprintf(output[core_no], "Discarded job: %d,%d, Exec time: %5lf\n", curr->task_number, curr->job_number, rem_exec_time);
                
                max_slack = find_max_slack(task_set, curr_crit_level, core_no, curr->absolute_deadline, curr_time, (*ready_queue));
                fprintf(output[core_no], "Max slack: %.5lf | ", max_slack);

                if(max_slack >= rem_exec_time)
                {
                    ready_job = curr;
                    prev->next = curr->next;
                    curr = curr->next;
                    ready_job->next = NULL;
                    (*discarded_queue)->num_jobs--;
                    fprintf(output[core_no], "Job %d,%d inserted in ready queue of core %d\n", ready_job->task_number, ready_job->job_number, core_no);
                    insert_job_in_ready_queue(ready_queue, ready_job); 
                    stats->total_discarded_jobs[core_no]++;
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

        prev = dummy_node;
        curr = (*discarded_queue)->job_list_head;
        while(curr != NULL)
        {
            if(task_set->task_list[curr->task_number].criticality_lvl == i && task_set->task_list[curr->task_number].core != core_no)
            {
                crit_level = task_set->task_list[curr->task_number].criticality_lvl;            
                rem_exec_time = task_set->task_list[curr->task_number].WCET[crit_level] - (curr->execution_time - curr->rem_exec_time);
                fprintf(output[core_no], "Discarded job: %d,%d, Exec time: %5lf\n", curr->task_number, curr->job_number, rem_exec_time);
                
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
                    stats->total_discarded_jobs[core_no]++;
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

    }

    (*discarded_queue)->job_list_head = dummy_node->next;
    dummy_node->next = NULL;
    free(dummy_node);

    return;
}

/*
    Preconditions: 
        Input: {pointer to taskset, pointer to the newly arrived job, the task number of job, the release time of the job, pointer to the core}
                task_list!=NULL
                new_job!=NULL
                core!=NULL

    Purpose of the function: This function will initialize all the fields in the newly arrived job. The fields updated will be
        release time, actual execution time, remaining execution time (=actual execution time), WCET counter of job, task number, release time of job, next pointer which points to next job in the ready queue.

    Postconditions: 
        Output: {void}
        Result: A newly arrived job with all the fields initialized.
*/
void find_job_parameters(task *task_list, job *new_job, int task_number, int job_number, double release_time, int curr_crit_level)
{
    double actual_exec_time;

    new_job->release_time = release_time;

    actual_exec_time = task_list[task_number].exec_times[job_number];

    new_job->execution_time = actual_exec_time;
    new_job->rem_exec_time = new_job->execution_time;
    new_job->WCET_counter = task_list[task_number].WCET[curr_crit_level];
    new_job->task_number = task_number;
    new_job->absolute_deadline = new_job->release_time + task_list[task_number].virtual_deadline;
    new_job->job_number = job_number;
    new_job->next = NULL;

    return;
}

/*
    Preconditions:
        Input: {pointer to job queue, pointer to taskset, pointer to core}
                ready_queue!=NULL
                task_set!=NULL
                core!=NULL

    Purpose of the function: This function will insert all the jobs which have arrived at the current time unit in the ready queue. The ready queue is sorted according to the deadlines.
                             It will also compute the procrastination length which is the minimum of the procrastination intervals of all newly arrived jobs.
    Postconditions: 
        Output: {Returns the procrastination length to update the core timer}
        Result: An updated ready queue with all the newly arrived jobs inserted in their right positions.
*/
void update_job_arrivals(job_queue_struct **ready_queue, job_queue_struct **discarded_queue, task_set_struct *task_set, int curr_crit_level, double curr_time, int core_no, core_struct *core, int algo, int timer_expiry)
{
    int total_tasks = task_set->total_tasks;
    task *task_list = task_set->task_list;
    int curr_task, crit_level;
    job *new_job;

    fprintf(output[core_no], "INSERTING JOBS IN READY/DISCARDED QUEUE\n");

    //Update the job arrivals from highest criticality level to the lowest.
    for (crit_level = MAX_CRITICALITY_LEVELS - 1; crit_level >= 0; crit_level--)
    {
        for (curr_task = 0; curr_task < total_tasks; curr_task++)
        {
            if (task_list[curr_task].criticality_lvl == crit_level && task_list[curr_task].core == core_no)
            {
                double max_exec_time = task_list[curr_task].WCET[curr_crit_level];
                double release_time = (task_list[curr_task].phase + task_list[curr_task].period * task_list[curr_task].job_number);
                double deadline = release_time + task_list[curr_task].virtual_deadline;

                while (deadline < curr_time)
                {
                    task_list[curr_task].job_number++;
                    release_time = (task_list[curr_task].phase + task_list[curr_task].period * task_list[curr_task].job_number);
                    deadline = release_time + task_list[curr_task].virtual_deadline;
                }

                if (release_time <= curr_time)
                {
                    new_job = (job *)malloc(sizeof(job));
                    find_job_parameters(task_list, new_job, curr_task, task_list[curr_task].job_number, release_time, curr_crit_level);

                    fprintf(output[core_no], "Job %d,%d arrived | ", curr_task, task_list[curr_task].job_number);
                    if (crit_level >= curr_crit_level)
                    {
                        fprintf(output[core_no], "Normal job| Exec time: %.5lf | %s\n", new_job->execution_time, (new_job->execution_time > task_set->task_list[curr_task].WCET[curr_crit_level]) ? "More" : "Less");
                        insert_job_in_ready_queue(ready_queue, new_job);
                    }
                    else if (algo != EDF_VD && timer_expiry != 1)
                    {
                        fprintf(output[core_no], "Discarded job | ");
                        double max_slack = 0.00;
                        max_slack = find_max_slack(task_set, curr_crit_level, core_no, deadline, curr_time, (*ready_queue));
                        fprintf(output[core_no], "Max slack: %.5lf, Max exec: %.5lf | ", max_slack, max_exec_time);

                        stats->total_discarded_jobs_available[core_no] += new_job->rem_exec_time;

                        if (max_slack >= max_exec_time)
                        {
                            fprintf(output[core_no], "Inserting in ready queue\n");
                            stats->total_discarded_jobs[core_no]++;
                            insert_job_in_ready_queue(ready_queue, new_job);
                        }
                        else
                        {
                            fprintf(output[core_no], "Inserting in discarded queue\n");
                            // stats->total_discarded_jobs_available[core_no] += new_job->rem_exec_time;
                            insert_job_in_discarded_queue(discarded_queue, new_job, task_set->task_list, core_no);
                        }
                    }
                    task_list[curr_task].job_number++;
                }
            }
        }
    }

    return;
}

/*
    Preconditions: 
        Input: {pointer to taskset, pointer to the ready queue}

    Purpose of the function: Remove the currently completed job from the ready queue.

    Postconditions: 
        Output: void
        Result: The completed job is freed and the ready queue is updated.
*/
void update_job_removal(task_set_struct *taskset, job_queue_struct **ready_queue)
{
    //Remove the currently executing job from the ready queue.
    job *completed_job = (*ready_queue)->job_list_head;
    (*ready_queue)->job_list_head = (*ready_queue)->job_list_head->next;
    (*ready_queue)->num_jobs--;

    free(completed_job);

    return;
}

/*
    Precondition: 
        Input: {pointer to core, pointer to ready queue, pointer to the taskset}

    Purpose of the function: This function will schedule a new job in the core. 
                             The time of scheduling of job and the time at which job will be completed is updated.
                             The WCET counter of job is updated to indicate the time at which the job will cross its WCET.

    Postconditions:
        Output: {void}
        Result: A new job is scheduled in the core and its scheduling time, completion time and WCET counter of core is updated.
*/
void schedule_new_job(core_struct *core, job_queue_struct *ready_queue, task_set_struct *task_set)
{
    (*core).curr_exec_job = ready_queue->job_list_head;
    (*core).curr_exec_job->scheduled_time = (*core).total_time;
    (*core).curr_exec_job->completion_time = (*core).total_time + (*core).curr_exec_job->rem_exec_time;
    (*core).WCET_counter = (*core).curr_exec_job->scheduled_time + (*core).curr_exec_job->WCET_counter;

    return;
}