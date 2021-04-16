#include "scheduler_functions.h"
#include "data_structures.h"
#include "auxiliary_functions.h"
#include "check_functions.h"
#include "allocation_functions.h"

/*
    Preconditions: 
        Input: {File pointer to input file}
        fd!=NULL

    Purpose of the function: Takes input from the file and returns a structure of the task set. 

    Postconditions:
        Output: {Pointer to the structure of taskset created}
        task_set!=NULL
    
*/
task_set_struct *get_taskset(FILE *fd)
{
    int num_task, criticality_lvl;
    int tasks;

    task_set_struct *task_set = (task_set_struct *)malloc(sizeof(task_set_struct));

    //Number of task_list
    fscanf(fd, "%d", &(task_set->total_tasks));
    tasks = task_set->total_tasks;
    task_set->task_list = (task *)malloc(sizeof(task) * tasks);

    for (num_task = 0; num_task < tasks; num_task++)
    {
        fscanf(fd, "%lf%lf%d", &task_set->task_list[num_task].phase, &task_set->task_list[num_task].relative_deadline, &task_set->task_list[num_task].criticality_lvl);

        //As it is an implicit-deadline taskset, period = deadline.
        task_set->task_list[num_task].period = task_set->task_list[num_task].relative_deadline;
        task_set->task_list[num_task].job_number = 0;
        task_set->task_list[num_task].util = (double *)malloc(sizeof(double) * MAX_CRITICALITY_LEVELS);
        task_set->task_list[num_task].core = -1;

        for (criticality_lvl = 0; criticality_lvl < MAX_CRITICALITY_LEVELS; criticality_lvl++)
        {
            fscanf(fd, "%lf", &task_set->task_list[num_task].WCET[criticality_lvl]);
            task_set->task_list[num_task].util[criticality_lvl] = (double)task_set->task_list[num_task].WCET[criticality_lvl] / (double)task_set->task_list[num_task].period;
        }
    }

    //Sort the tasks list based on their periods.
    qsort((void *)task_set->task_list, tasks, sizeof(task_set->task_list[0]), period_comparator);

    return task_set;
}

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
    for(num_task = 0; num_task < task_set->total_tasks; num_task++) {
        lcm = (lcm * task_set->task_list[num_task].period) / gcd(lcm, task_set->task_list[num_task].period);
    }

    return lcm;
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
    int i;
    double max_slack = deadline - curr_time;
    
    // fprintf(output[core_no], "Function to find maximum slack\n");
    // fprintf(output[core_no], "Max slack: %.2lf\n", max_slack);

    job *temp = ready_queue->job_list_head;
    // fprintf(output[core_no], "Traversing ready queue\n");
    
    //First traverse the ready queue and update the maximum slack according to remaining execution time of jobs.
    while (temp)
    {
        max_slack -= temp->rem_exec_time;
        // fprintf(output[core_no], "Job: %d, rem execution time: %.2lf, max slack: %.2lf\n", temp->task_number, temp->rem_exec_time, max_slack);
        temp = temp->next;
    }

    // fprintf(output[core_no], "Traversing task list\n");

    //Then, traverse the task list and update the maximum slack according to future invocations of the tasks.
    for (i = 0; i < task_set->total_tasks; i++)
    {
        if (task_set->task_list[i].core == core_no && task_set->task_list[i].criticality_lvl >= crit_level)
        {

            int curr_jobs = task_set->task_list[i].job_number;
            double exec_time = task_set->task_list[i].WCET[crit_level];
            // curr_jobs++;
            while ((task_set->task_list[i].phase + task_set->task_list[i].period * curr_jobs) < deadline)
            {
                max_slack -= exec_time;

                // fprintf(output[core_no], "Task: %d, exec time: %.2lf, max slack: %.2lf\n", i, exec_time, max_slack);
                curr_jobs++;
            }
        }
    }

    return max(max_slack, 0.00);
}

/*
    Preconditions:
        Input: {pointer to taskset, current criticality level} 
    
    Purpose of the function: This function finds the time of earliest arriving job. 

    Postconditions:
        Output: {The arrival time of earliest arriving job}

*/
double find_earliest_arrival_job(task_set_struct *task_set, int core_no)
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
    int i;

    decision.core_no = -1;

    // fprintf(output_file, "Finding decision point\n");
    for (i = 0; i < processor->total_cores; i++)
    {
        completion_time = INT_MAX;
        expiry_time = INT_MAX;
        WCET_counter = INT_MAX;
        arrival_time = INT_MAX;

        if (processor->cores[i].state == ACTIVE)
        {
            arrival_time = find_earliest_arrival_job(task_set, i);
        }
        else {
            expiry_time = processor->cores[i].next_invocation_time;
        }

        if (processor->cores[i].curr_exec_job != NULL)
        {
            completion_time = processor->cores[i].curr_exec_job->completed_job_time;
            if (processor->crit_level < (MAX_CRITICALITY_LEVELS - 1) && processor->crit_level <= processor->cores[i].threshold_crit_lvl)
            {
                WCET_counter = processor->cores[i].WCET_counter;
            }
        }

        min_time = min(min(min(arrival_time, completion_time), WCET_counter), expiry_time);
        if(decision_time > min_time) {
            decision_time = min_time;
            decision_core = i;

            if(decision_time == completion_time) {
                decision_point = COMPLETION;
            }
            else if(decision_time == expiry_time) {
                decision_point = TIMER_EXPIRE;
            }
            else if(decision_time == WCET_counter) {
                decision_point = CRIT_CHANGE;
            }
            else if(decision_time == arrival_time) {
                decision_point = ARRIVAL;
            }
        }
        // fprintf(output_file, "Core: %d, Arrival time: %.2lf, Completion time: %.2lf, Timer expiry: %.2lf, WCET counter: %.2lf\n", i, arrival_time, completion_time, expiry_time, WCET_counter);
    
    }

    decision.core_no = decision_core;
    decision.decision_point = decision_point;
    decision.decision_time = decision_time;

    return decision;
}

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

    // fprintf(output[core_no], "Discarded job list\n");
    // print_job_list((*discarded_queue)->job_list_head);
    
    if ((*discarded_queue)->num_jobs == 0)
        return;

    // fprintf(output[core_no], "Accommodate discarded jobs in ready queue of core %d\n", core_no);

    //First try to accommodate the jobs belonging to this core.
    while ((*discarded_queue)->num_jobs != 0)
    {
        if(same_core == 0 || (same_core == 1 && task_set->task_list[(*discarded_queue)->job_list_head->task_number].core == core_no)) {
            discarded_job = (*discarded_queue)->job_list_head;
            max_slack = find_max_slack(task_set, curr_crit_level, core_no, discarded_job->absolute_deadline, curr_time, (*ready_queue));
            rem_exec_time = discarded_job->rem_exec_time;
            // fprintf(output[core_no], "Discarded job: %d, max slack: %.2lf, rem exec time: %.2lf\n", discarded_job->task_number, max_slack, rem_exec_time);

            if (max_slack > rem_exec_time)
            {
                (*discarded_queue)->job_list_head = (*discarded_queue)->job_list_head->next;
                (*discarded_queue)->num_jobs--;
                discarded_job->next = NULL;
                // fprintf(output[core_no], "Job %d,%d inserted in ready queue of core %d\n", discarded_job->task_number, discarded_job->job_number, core_no);
                insert_job_in_ready_queue(ready_queue, discarded_job);
            }
            else
            {
                break;
            }
        }
        else {
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
            // fprintf(output[core_no], "Discarded job: %d, max slack: %.2lf, rem exec time: %.2lf\n", discarded_job->task_number, max_slack, rem_exec_time);

            if (max_slack > rem_exec_time)
            {
                ready_job = temp->next;
                temp->next = temp->next->next;
                ready_job->next = NULL;
                (*discarded_queue)->num_jobs--;
                // fprintf(output[core_no], "Job %d,%d inserted in ready queue of core %d\n", temp->task_number, temp->job_number, core_no);
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

void accommodate_discarded_jobs(job_queue_struct **ready_queue, job_queue_struct **discarded_queue, task_set_struct *task_set, int core_no, int curr_crit_level, double curr_time) {
    //First try to accommodate discarded jobs from the same core.
    insert_discarded_jobs_in_ready_queue(ready_queue, discarded_queue, task_set, core_no, curr_crit_level, curr_time, 1);
    
    if((*discarded_queue)->num_jobs == 0)
        return;

    //Then, try to accommodate more jobs from the discarded queue.
    insert_discarded_jobs_in_ready_queue(ready_queue, discarded_queue, task_set, core_no, curr_crit_level, curr_time, 0);
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
    actual_exec_time = find_actual_execution_time(task_list[task_number].WCET[curr_crit_level], task_list[task_number].criticality_lvl, curr_crit_level);

    if (task_list[task_number].criticality_lvl <= curr_crit_level)
    {
        new_job->execution_time = actual_exec_time;
    }
    else
    {
        new_job->execution_time = min(actual_exec_time, task_list[task_number].WCET[(curr_crit_level + 1 == MAX_CRITICALITY_LEVELS) ? MAX_CRITICALITY_LEVELS - 1 : curr_crit_level + 1]);
    }

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
void update_job_arrivals(job_queue_struct **ready_queue, job_queue_struct **discarded_queue, task_set_struct *task_set, int curr_crit_level, double arrival_time, int core_no)
{
    int total_tasks = task_set->total_tasks;
    task *task_list = task_set->task_list;
    int curr_task, crit_level;
    job *new_job;

    fprintf(output[core_no], "INSERTING JOBS IN READY/DISCARDED QUEUE\n");

    for(crit_level = MAX_CRITICALITY_LEVELS - 1; crit_level >= 0; crit_level--){
        for (curr_task = 0; curr_task < total_tasks; curr_task++)
        {
            if (task_list[curr_task].criticality_lvl == crit_level && task_list[curr_task].core == core_no)
            {
                double max_exec_time = task_list[curr_task].WCET[curr_crit_level];
                double release_time = (task_list[curr_task].phase + task_list[curr_task].period * task_list[curr_task].job_number);

                while (release_time <= arrival_time)
                {
                    double deadline = release_time + task_list[curr_task].virtual_deadline;
                    if(deadline > arrival_time) {
                        new_job = (job *)malloc(sizeof(job));
                        find_job_parameters(task_list, new_job, curr_task, task_list[curr_task].job_number, release_time, curr_crit_level);

                        fprintf(output[core_no], "Job %d,%d arrived | ", curr_task, task_list[curr_task].job_number);
                        if (crit_level >= curr_crit_level)
                        {
                            fprintf(output[core_no], "Normal job\n");
                            insert_job_in_ready_queue(ready_queue, new_job);
                        }
                        else
                        {
                            fprintf(output[core_no], "Discarded job | ");
                            double max_slack = find_max_slack(task_set, curr_crit_level, core_no, deadline, arrival_time, (*ready_queue));
                            fprintf(output[core_no], "Max slack: %.2lf, Max Exec Time: %.2lf | ", max_slack, max_exec_time);
                            if (max_slack >= max_exec_time)
                            {
                                fprintf(output[core_no], "Inserting in ready queue\n");
                                insert_job_in_ready_queue(ready_queue, new_job);
                            }
                            else
                            {
                                fprintf(output[core_no], "Inserting in discarded queue\n");
                                insert_job_in_discarded_queue(discarded_queue, new_job, task_set->task_list);
                            }
                        }
                    }
                    task_list[curr_task].job_number++;

                    release_time = (task_list[curr_task].phase + task_list[curr_task].period * task_list[curr_task].job_number);
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
    (*core).curr_exec_job->completed_job_time = (*core).total_time + (*core).curr_exec_job->rem_exec_time;
    (*core).WCET_counter = (*core).curr_exec_job->scheduled_time + ((*core).curr_exec_job->WCET_counter);

    return;
}

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
                new_job->execution_time = curr_task.WCET[MAX_CRITICALITY_LEVELS-1];
                new_job->rem_exec_time = curr_task.WCET[MAX_CRITICALITY_LEVELS-1];
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
    int i;
    double release_time, absolute_deadline;
    double timer_expiry = 0, total_utilisation = 0;
    double earliest_task_WCET = 0;
    job *job_list, *free_job;

    // fprintf(output[core_no], "Finding procrastination interval:\n");

    for (i = 0; i < task_set->total_tasks; i++)
    {
        if (task_set->task_list[i].criticality_lvl >= curr_crit_level && task_set->task_list[i].core == core_no)
        {
            release_time = task_set->task_list[i].phase + task_set->task_list[i].period * task_set->task_list[i].job_number;
            absolute_deadline = release_time + task_set->task_list[i].virtual_deadline;
            if (next_deadline1 > absolute_deadline && curr_time < absolute_deadline)
            {
                next_deadline1 = absolute_deadline;
                earliest_task_WCET = task_set->task_list[i].WCET[curr_crit_level];
            }
        }
    }

    // fprintf(output[core_no], "Earliest arriving job Deadline: %.2lf\n", next_deadline1);

    if ((next_deadline1 - curr_time - earliest_task_WCET) < SHUTDOWN_THRESHOLD)
    {
        // fprintf(output[core_no], "Interval using Dnext1: %.2lf. Less than SDT\n", (next_deadline1 - curr_time - earliest_task.WCET[curr_crit_level]));
        return 0.00;
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

    // fprintf(output[core_no], "Latest arriving job deadline: %.2lf\n", next_deadline2);

    job_list = find_job_list(curr_time, next_deadline2, task_set, curr_crit_level, core_no);
    // fprintf(output[core_no], "Job list:\n");
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

    // fprintf(output[core_no], "Calculation:\n");
    while (job_list != NULL)
    {
        // fprintf(output[core_no], "Job: %d, Release time: %.2lf, Execution time: %.2lf, Total utilisation: %.2lf\n", job_list->task_number, job_list->release_time, job_list->execution_time, total_utilisation);
        if (job_list->absolute_deadline > next_deadline2)
        {
            timer_expiry -= ((next_deadline2 - job_list->release_time) * ((double)job_list->execution_time / (double)task_set->task_list[job_list->task_number].period) * total_utilisation);
            // fprintf(output[core_no], "If statement\n");
        }
        else
        {
            // fprintf(output[core_no], "Else statement\n");
            timer_expiry -= job_list->execution_time;
        }

        if (job_list->next && timer_expiry > job_list->next->absolute_deadline)
        {
            timer_expiry = job_list->next->absolute_deadline;
        }

        // fprintf(output[core_no], "Time expiry: %.2lf\n", timer_expiry);

        free_job = job_list;
        job_list = job_list->next;
        free(free_job);
    }
    // fprintf(output[core_no], "Time expiry: %.2lf\n", timer_expiry);

    return timer_expiry - curr_time;
}

/*
    Preconditions: 
        Input: {pointer to taskset, pointer to core, pointer to output file}

    Purpose of the function: This function performs the scheduling of the taskset according to edf-vd. 
                             The scheduling will be done for superhyperperiod of the tasks.
                             A job queue will contain the jobs which have arrived at the current time unit, sorted according to their virtual deadlines. 
                             The core will always take the head of the queue for scheduling.
                             If any job exceeds its WCET, a counter will indicate the same and the core's criticality level will change.
                             At that time, all the LOW criticality jobs will be removed from the ready queue and only HIGH criticality jobs will be scheduled from now on.

    Postconditions:
        Return value: {void}
        Output: The output will be stored in the output file. Each line will give the information about:
                The type of decision point, 
                core's total execution time, core's current criticality level, The currently executing job, its total execution time, its actual execution time and its absolute deadline.
*/
void schedule_taskset(task_set_struct *task_set, processor_struct *processor)
{

    double super_hyperperiod, decision_time, prev_decision_time;
    decision_struct decision;
    int decision_point, decision_core, num_core;

    task *task_list = task_set->task_list;

    job_queue_struct *discarded_queue;
    discarded_queue = (job_queue_struct *)malloc(sizeof(job_queue_struct));
    discarded_queue->num_jobs = 0;
    discarded_queue->job_list_head = NULL;

    //Find the hyperperiod of all the cores. The scheduler will run for the max of all hyperperiods.
    super_hyperperiod = find_superhyperperiod(task_set);
    fprintf(output_file, "Super hyperperiod: %.2lf", super_hyperperiod);

    while (1)
    {
        //Find the decision point. The decision point will be the minimum of the earliest arrival job, the completion of the currently executing job and the WCET counter for criticality change.
        decision = find_decision_point(task_set, processor, super_hyperperiod);
        decision_point = decision.decision_point;
        decision_time = decision.decision_time;
        decision_core = decision.core_no;

        if(decision_time >= super_hyperperiod) {
            for(num_core = 0; num_core < processor->total_cores; num_core++) {
                if(processor->cores[num_core].state == SHUTDOWN || processor->cores[num_core].curr_exec_job == NULL) {
                    processor->cores[num_core].total_idle_time += (super_hyperperiod - processor->cores[num_core].total_time);
                }
                else {
                    processor->cores[num_core].curr_exec_job->rem_exec_time -= (super_hyperperiod - processor->cores[num_core].total_time);
                }
                processor->cores[num_core].total_time = super_hyperperiod;
            }
            break;
        }

        fprintf(output[decision_core], "Decision point: %s, Decision time: %.2lf, Core no: %d, Crit level: %d\n", decision_point == ARRIVAL ? "ARRIVAL" : ((decision_point == COMPLETION) ? "COMPLETION" : (decision_point == TIMER_EXPIRE ? "TIMER EXPIRE" : "CRIT_CHANGE")), decision_time, decision_core, processor->crit_level);
        
        //Remove the jobs from discarded queue that have missed their deadlines.
        remove_jobs_from_discarded_queue(&discarded_queue, decision_time);

        //Store the previous decision time of core for any further use.
        prev_decision_time = processor->cores[decision_core].total_time;
        //Update the total time of the core.
        processor->cores[decision_core].total_time = decision_time;

        //If the decision point is due to arrival of a job
        if (decision_point == ARRIVAL)
        {
            //Update the newly arrived jobs in the ready queue. Discarded jobs can be inserted in ready queue or discarded queue depeneding on the maximum slack available.
            update_job_arrivals(&(processor->cores[decision_core].ready_queue), &discarded_queue, task_set, processor->crit_level, decision_time, decision_core);

            //If the currently executing job in the core is NULL, then schedule a new job from the ready queue.
            if (processor->cores[decision_core].curr_exec_job == NULL)
            {
                processor->cores[decision_core].total_idle_time += (decision_time - prev_decision_time);
                if(processor->cores[decision_core].ready_queue->num_jobs != 0)
                    schedule_new_job(&(processor->cores[decision_core]), processor->cores[decision_core].ready_queue, task_set);
            }
            else
            {
                //Update the time for which the job has executed in the core and the WCET counter of the job.
                double exec_time = processor->cores[decision_core].total_time - prev_decision_time;
                processor->cores[decision_core].curr_exec_job->rem_exec_time -= exec_time;
                processor->cores[decision_core].curr_exec_job->WCET_counter -= exec_time;
            }

            //If the currently executing job is not the head of the ready queue, then a job with earlier deadline has arrived.
            //Preempt the current job and schedule the new job for execution.
            if (compare_jobs(processor->cores[decision_core].curr_exec_job, processor->cores[decision_core].ready_queue->job_list_head) == 0)
            {
                fprintf(output[decision_core], "Preempt current job | ");
                schedule_new_job(&(processor->cores[decision_core]), processor->cores[decision_core].ready_queue, task_set);
            }
        }

        //If the decision point was due to completion of the currently executing job.
        else if (decision_point == COMPLETION)
        {
            double procrastionation_interval;
            fprintf(output[decision_core], "Job %d, %d completed execution | ", processor->cores[decision_core].curr_exec_job->task_number, processor->cores[decision_core].curr_exec_job->job_number);

            double deadline = processor->cores[decision_core].curr_exec_job->absolute_deadline;
            if(deadline < processor->cores[decision_core].total_time)
            {
                fprintf(output[decision_core], "Deadline missed. Completing scheduling\n");
                processor->cores[decision_core].curr_exec_job = NULL;
                break;
            }

            processor->cores[decision_core].curr_exec_job = NULL;
            //Remove the completed job from the ready queue.
            update_job_removal(task_set, &(processor->cores[decision_core].ready_queue));

            //If ready queue is null, no job is ready for execution. Put the processor to sleep and find the next invocation time of processor.
            if (processor->cores[decision_core].ready_queue->num_jobs == 0)
            {
                fprintf(output[decision_core], "No job to execute | ");

                if(processor->cores[decision_core].num_tasks_allocated == 0) {
                    fprintf(output[decision_core], "No tasks allocated to core. Putting to shutdown\n");
                    processor->cores[decision_core].state = SHUTDOWN;
                    processor->cores[decision_core].next_invocation_time = INT_MAX;
                }
                else {
                    procrastionation_interval = find_procrastination_interval(processor->cores[decision_core].total_time, task_set, processor->crit_level, decision_core);
                    if (procrastionation_interval > SHUTDOWN_THRESHOLD)
                    {
                        fprintf(output[decision_core], "Procrastination interval %.2lf greater than SDT | Putting core to sleep\n", procrastionation_interval);
                        processor->cores[decision_core].state = SHUTDOWN;
                        processor->cores[decision_core].next_invocation_time = procrastionation_interval + processor->cores[decision_core].total_time;
                    }
                    else
                    {
                        fprintf(output[decision_core], "Procrastination interval %.2lf less than shutdown threshold | Not putting core to sleep\n", procrastionation_interval);
                        processor->cores[decision_core].state = ACTIVE;
                        
                        //Accommodate discarded jobs in ready queue.
                        accommodate_discarded_jobs(&(processor->cores[decision_core].ready_queue), &discarded_queue, task_set, decision_core, processor->crit_level, processor->cores[decision_core].total_time);
                    }
                }
            }
            
            if(processor->cores[decision_core].ready_queue->num_jobs != 0) {
                schedule_new_job(&(processor->cores[decision_core]), processor->cores[decision_core].ready_queue, task_set);
            }
        }

        //If the decision point is due to timer expiry, wakeup the processor and schedule a new job from the ready queue.
        else if (decision_point == TIMER_EXPIRE)
        {
            //Wakeup the core and schedule the high priority process.
            processor->cores[decision_core].state = ACTIVE;
            processor->cores[decision_core].total_idle_time += (decision_time - prev_decision_time);

            fprintf(output[decision_core], "Timer expired. Waking up scheduler\n");

            update_job_arrivals(&(processor->cores[decision_core].ready_queue), &discarded_queue, task_set, processor->crit_level, processor->cores[decision_core].total_time, decision_core);

            if (processor->cores[decision_core].ready_queue->num_jobs != 0)
            {
                schedule_new_job(&(processor->cores[decision_core]), processor->cores[decision_core].ready_queue, task_set);
            }
            else
            {
                fprintf(output[decision_core], "No jobs to execute\n\n");
            }
        }

        //If decision point is due to criticality change, then the currently executing job has exceeded its WCET.
        else if (decision_point == CRIT_CHANGE)
        {
            double core_prev_decision_time, procrastination_interval;
            //Increase the criticality level of the processor.
            processor->crit_level = min(processor->crit_level + 1, MAX_CRITICALITY_LEVELS - 1);

            fprintf(output[decision_core], "Criticality changed for each core\n");

            //Remove all the low criticality jobs from the ready queue of each core and reset the virtual deadlines of high criticality jobs.
            for (num_core = 0; num_core < processor->total_cores; num_core++)
            {
                if (processor->crit_level > processor->cores[num_core].threshold_crit_lvl)
                    reset_virtual_deadlines(&task_set, num_core, processor->cores[num_core].threshold_crit_lvl);

                fprintf(output[num_core], "Core: %d | Criticality changed | Crit level: %d | State: %s\n", num_core, processor->crit_level, processor->cores[num_core].state == ACTIVE ? "ACTIVE": "SHUTDOWN");
                
                if(processor->cores[num_core].state == ACTIVE) {

                    //Need the core's prevision decision time for updating the execution time of currently executing job.
                    if(num_core != decision_core)
                        core_prev_decision_time = processor->cores[num_core].total_time;
                    else
                        core_prev_decision_time = prev_decision_time;
                    processor->cores[num_core].total_time = decision_time;

                    //Update the time for which the current job has executed.
                    if(processor->cores[num_core].curr_exec_job != NULL) {
                        processor->cores[num_core].curr_exec_job->rem_exec_time -= (processor->cores[num_core].total_time - core_prev_decision_time);
                    }
                    processor->cores[num_core].curr_exec_job = NULL;

                    if(processor->cores[num_core].ready_queue->num_jobs != 0){
                        // fprintf(output[decision_core], "Ready queue of core %d before removal\n", num_core);
                        // print_job_list(num_core, processor->cores[num_core].ready_queue->job_list_head);
                        // fprintf(output[decision_core], "Core: %d, Removing low crit jobs from ready queue\n", num_core);
                        remove_jobs_from_ready_queue(&processor->cores[num_core].ready_queue, &discarded_queue, task_list, processor->crit_level, processor->cores[num_core].threshold_crit_lvl, num_core);
                    }

                    accommodate_discarded_jobs(&(processor->cores[num_core].ready_queue), &discarded_queue, task_set, num_core, processor->crit_level, processor->cores[num_core].total_time);

                    if(processor->cores[num_core].ready_queue->num_jobs != 0) {
                        // fprintf(output[decision_core], "Ready queue of core %d after removal\n", num_core);
                        // print_job_list(num_core, processor->cores[num_core].ready_queue->job_list_head);
                        schedule_new_job(&processor->cores[num_core], processor->cores[num_core].ready_queue, task_set);
                    }
                }
                else 
                {
                    processor->cores[num_core].state = ACTIVE;
                    procrastination_interval = find_procrastination_interval(processor->cores[num_core].total_time, task_set, processor->crit_level, num_core);
                    if (procrastination_interval > SHUTDOWN_THRESHOLD)
                    {
                        fprintf(output[num_core], "Procrastination interval %.2lf greater than SDT | Putting core to sleep\n", procrastination_interval);
                        processor->cores[num_core].state = SHUTDOWN;
                        processor->cores[num_core].next_invocation_time = procrastination_interval + processor->cores[num_core].total_time;
                    }
                    else
                    {
                        fprintf(output[num_core], "Procrastination interval %.2lf less than shutdown threshold | Not putting core to sleep\n", procrastination_interval);
                        
                        //Accommodate discarded jobs in ready queue.
                        accommodate_discarded_jobs(&(processor->cores[num_core].ready_queue), &discarded_queue, task_set, num_core, processor->crit_level, processor->cores[num_core].total_time);
                    }

                    if(processor->cores[num_core].ready_queue->num_jobs != 0) {
                        // fprintf(output[decision_core], "Ready queue of core %d after removal\n", num_core);
                        // print_job_list(processor->cores[num_core].ready_queue->job_list_head);
                        schedule_new_job(&processor->cores[num_core], processor->cores[num_core].ready_queue, task_set);
                    }
                }
            }
        }

        if(processor->cores[decision_core].curr_exec_job != NULL) {
        fprintf(output[decision_core], "Scheduled job: %d,%d  Exec time: %.2lf  Rem exec time: %.2lf  WCET_counter: %.2lf  Deadline: %.2lf\n", 
                                processor->cores[decision_core].curr_exec_job->task_number, 
                                processor->cores[decision_core].curr_exec_job->job_number, 
                                processor->cores[decision_core].curr_exec_job->execution_time, 
                                processor->cores[decision_core].curr_exec_job->rem_exec_time, 
                                processor->cores[decision_core].WCET_counter, 
                                processor->cores[decision_core].curr_exec_job->absolute_deadline);
        }
        fprintf(output[decision_core], "____________________________________________________________________________________________________\n\n");
    }
    return;
}

/*
    Preconditions: 
        Input: {pointer to taskset, pointer to core, pointer to output file}

    Purpose of the function: This function will perform the offline preprocessing phase and the runtime scheduling of edf-vd.
                            If the taskset is not schedulable, it will return after displaying the same message. Else, it will start the runtime scheduling of the tasket.
                        
    Postconditions: 
        Output: {void}
*/
void runtime_scheduler(task_set_struct *task_set, processor_struct *processor)
{
    int result = allocate_tasks_to_cores(task_set, processor);
    print_task_list(task_set);

    if (result == 0.00)
    {
        fprintf(output_file, "Not schedulable\n");
        return;
    }
    else
    {
        fprintf(output_file, "Schedulable\n");
    }

    srand(time(NULL));

    schedule_taskset(task_set, processor);
    print_processor(processor);

    return;
}
