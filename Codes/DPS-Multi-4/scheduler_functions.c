#include "functions.h"

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

    // fprintf(output[decision_core], "Finding decision point\n");
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
            completion_time = processor->cores[i].curr_exec_job->completion_time;
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
        // fprintf(output[decision_core], "Core: %d, Arrival time: %.5lf, Completion time: %.5lf, Timer expiry: %.5lf, WCET counter: %.5lf\n", i, arrival_time, completion_time, expiry_time, WCET_counter);
    
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
    int i;
    double max_slack = deadline - curr_time;
    
    // fprintf(output[core_no], "Function to find maximum slack\n");
    // fprintf(output[core_no], "Max slack: %.5lf\n", max_slack);

    job *temp = ready_queue->job_list_head;
    // fprintf(output[core_no], "Traversing ready queue\n");
    
    //First traverse the ready queue and update the maximum slack according to remaining execution time of jobs.
    while (temp)
    {
        max_slack -= temp->rem_exec_time;
        // fprintf(output[core_no], "Job: %d, rem execution time: %.5lf, max slack: %.5lf\n", temp->task_number, temp->rem_exec_time, max_slack);
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

                // fprintf(output[core_no], "Task: %d, exec time: %.5lf, max slack: %.5lf\n", i, exec_time, max_slack);
                curr_jobs++;
            }
        }
    }

    return max(max_slack, 0.00);
}

void accommodate_discarded_jobs(job_queue_struct **ready_queue, job_queue_struct **discarded_queue, task_set_struct *task_set, int core_no, int curr_crit_level, double curr_time) {
    //First try to accommodate discarded jobs from the same core.
    insert_discarded_jobs_in_ready_queue(ready_queue, discarded_queue, task_set, core_no, curr_crit_level, curr_time, 1);
    
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
void find_job_parameters(task *task_list, job *new_job, int task_number, int job_number, double release_time, int curr_crit_level, double frequency)
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

    new_job->execution_time = new_job->execution_time / frequency;
    new_job->rem_exec_time = new_job->execution_time;
    new_job->WCET_counter = task_list[task_number].WCET[curr_crit_level] / frequency;
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
void update_job_arrivals(job_queue_struct **ready_queue, job_queue_struct **discarded_queue, task_set_struct *task_set, int curr_crit_level, double arrival_time, int core_no, core_struct* core)
{
    int total_tasks = task_set->total_tasks;
    task *task_list = task_set->task_list;
    int curr_task, crit_level;
    job *new_job;

    fprintf(output[core_no], "INSERTING JOBS IN READY/DISCARDED QUEUE\n");

    //Update the job arrivals from highest criticality level to the lowest.
    for(crit_level = MAX_CRITICALITY_LEVELS - 1; crit_level >= 0; crit_level--){
        for (curr_task = 0; curr_task < total_tasks; curr_task++)
        {
            if (task_list[curr_task].criticality_lvl == crit_level && task_list[curr_task].core == core_no)
            {
                double max_exec_time = task_list[curr_task].WCET[curr_crit_level];
                double release_time = (task_list[curr_task].phase + task_list[curr_task].period * task_list[curr_task].job_number);
                double deadline = release_time + task_list[curr_task].virtual_deadline;

                while(deadline < arrival_time)
                {
                    task_list[curr_task].job_number++;
                    release_time = (task_list[curr_task].phase + task_list[curr_task].period * task_list[curr_task].job_number);
                    deadline = release_time + task_list[curr_task].virtual_deadline;
                }

                if(release_time <= arrival_time){
                    new_job = (job *)malloc(sizeof(job));
                    find_job_parameters(task_list, new_job, curr_task, task_list[curr_task].job_number, release_time, curr_crit_level, (*core).frequency);

                    fprintf(output[core_no], "Job %d,%d arrived | ", curr_task, task_list[curr_task].job_number);
                    if (crit_level >= curr_crit_level)
                    {
                        fprintf(output[core_no], "Normal job| Exec time: %.5lf\n", new_job->execution_time);

                        //Reset the utilisation of newly arrived task.
                        for(int l=0; l<crit_level; l++)
                        {
                            task_set->task_list[curr_task].util[l] = task_set->task_list[curr_task].WCET[l] / task_set->task_list[curr_task].period;
                        }

                        insert_job_in_ready_queue(ready_queue, new_job);
                    }
                    else
                    {
                        fprintf(output[core_no], "Discarded job | ");
                        double max_slack = find_max_slack(task_set, curr_crit_level, core_no, deadline, arrival_time, (*ready_queue));
                        fprintf(output[core_no], "Max slack: %.5lf, Max Exec Time: %.5lf | ", max_slack, max_exec_time);
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
    (*core).WCET_counter = (*core).curr_exec_job->scheduled_time + ((*core).curr_exec_job->WCET_counter);

    return;
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
    fprintf(output_file, "Super hyperperiod: %.5lf\n", super_hyperperiod);

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
                    stats->total_idle_energy[num_core] += (super_hyperperiod - processor->cores[num_core].total_time);
                }
                else {
                    processor->cores[num_core].curr_exec_job->rem_exec_time -= (super_hyperperiod - processor->cores[num_core].total_time);
                    stats->total_active_energy[num_core] += (super_hyperperiod - processor->cores[num_core].total_time);
                }
                processor->cores[num_core].total_time = super_hyperperiod;
            }
            break;
        }

        fprintf(output[decision_core], "Decision point: %s, Decision time: %.5lf, Core no: %d, Crit level: %d\n", decision_point == ARRIVAL ? "ARRIVAL" : ((decision_point == COMPLETION) ? "COMPLETION" : (decision_point == TIMER_EXPIRE ? "TIMER EXPIRE" : "CRIT_CHANGE")), decision_time, decision_core, processor->crit_level);
        
        switch(decision_point){
            case ARRIVAL: stats->total_arrival_points[decision_core]++; break;
            case COMPLETION: stats->total_completion_points[decision_core]++; break;
            case TIMER_EXPIRE: stats->total_wakeup_points[decision_core]++; break;
            case CRIT_CHANGE: stats->total_criticality_change_points[decision_core]++; break;
        }

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
            update_job_arrivals(&(processor->cores[decision_core].ready_queue), &discarded_queue, task_set, processor->crit_level, decision_time, decision_core, &(processor->cores[decision_core]));

            //If the currently executing job in the core is NULL, then schedule a new job from the ready queue.
            if (processor->cores[decision_core].curr_exec_job == NULL)
            {
                stats->total_idle_energy[decision_core] += (decision_time - prev_decision_time);
                processor->cores[decision_core].total_idle_time += (decision_time - prev_decision_time);
            }
            else
            {
                stats->total_active_energy[decision_core] += (decision_time - prev_decision_time);
                //Update the time for which the job has executed in the core and the WCET counter of the job.
                double exec_time = processor->cores[decision_core].total_time - prev_decision_time;
                processor->cores[decision_core].curr_exec_job->rem_exec_time -= exec_time;
                processor->cores[decision_core].curr_exec_job->WCET_counter -= exec_time;
            }

            select_frequency(&(processor->cores[decision_core]), task_set, processor->crit_level, decision_core);
            //If the currently executing job is not the head of the ready queue, then a job with earlier deadline has arrived.
            //Preempt the current job and schedule the new job for execution.
            if (compare_jobs(processor->cores[decision_core].curr_exec_job, processor->cores[decision_core].ready_queue->job_list_head) == 0)
            {
                if(processor->cores[decision_core].curr_exec_job != NULL)
                    fprintf(output[decision_core], "Preempt current job | ");   
                stats->total_context_switches[decision_core]++;
                schedule_new_job(&(processor->cores[decision_core]), processor->cores[decision_core].ready_queue, task_set);
            }
        }

        //If the decision point was due to completion of the currently executing job.
        else if (decision_point == COMPLETION)
        {
            double procrastionation_interval;
            fprintf(output[decision_core], "Job %d, %d completed execution | ", processor->cores[decision_core].curr_exec_job->task_number, processor->cores[decision_core].curr_exec_job->job_number);

            //Check to see if the job has missed its deadline or not.
            int task_number = processor->cores[decision_core].curr_exec_job->task_number;
            double deadline = processor->cores[decision_core].curr_exec_job->absolute_deadline;
            if(deadline < processor->cores[decision_core].total_time)
            {
                fprintf(output[decision_core], "Deadline missed. Completing scheduling\n");
                processor->cores[decision_core].curr_exec_job = NULL;
                break;
            }

            for(int l=processor->crit_level; l<=(task_set->task_list[task_number].criticality_lvl); l++)
            {
                task_set->task_list[task_number].util[l] = processor->cores[decision_core].curr_exec_job->execution_time / task_set->task_list[task_number].period;
            }

            processor->cores[decision_core].curr_exec_job = NULL;
            //Remove the completed job from the ready queue.
            update_job_removal(task_set, &(processor->cores[decision_core].ready_queue));

            stats->total_active_energy[decision_core] += (decision_time - prev_decision_time);

            // if(check_all_cores(processor) == 1)
            // {
            //     if(processor->crit_level > 0){
            //         fprintf(output[decision_core], "All cores are idle. Changing criticality to lowest level. | ");
            //         processor->crit_level = 0;
            //     }
            // }
            // else {
            //     int crit_level = find_max_level(processor, task_set);
            //     if(processor->crit_level > crit_level)
            //     {
            //         fprintf(output[decision_core], "Maximum crit level of all ready queues is %d. Changing crit level to that. | ", crit_level);
            //         processor->crit_level = crit_level;
            //     }
            // }

            //If ready queue is null, no job is ready for execution. Put the processor to sleep and find the next invocation time of processor.
            if (processor->cores[decision_core].ready_queue->num_jobs == 0)
            {
                fprintf(output[decision_core], "No job to execute | ");
                
                procrastionation_interval = find_procrastination_interval(processor->cores[decision_core].total_time, task_set, processor->crit_level, decision_core);
                if (procrastionation_interval > SHUTDOWN_THRESHOLD)
                {
                    fprintf(output[decision_core], "Procrastination interval %.5lf greater than SDT | Putting core to sleep\n", procrastionation_interval);
                    processor->cores[decision_core].state = SHUTDOWN;
                    processor->cores[decision_core].next_invocation_time = procrastionation_interval + processor->cores[decision_core].total_time;
                }
                else
                {
                    fprintf(output[decision_core], "Procrastination interval %.5lf less than shutdown threshold | Not putting core to sleep\n", procrastionation_interval);
                    processor->cores[decision_core].state = ACTIVE;
                    
                    //Accommodate discarded jobs in ready queue.
                    accommodate_discarded_jobs(&(processor->cores[decision_core].ready_queue), &discarded_queue, task_set, decision_core, processor->crit_level, processor->cores[decision_core].total_time);
                }
            }
            
            if(processor->cores[decision_core].ready_queue->num_jobs != 0) {
                stats->total_context_switches[decision_core]++;
                select_frequency(&(processor->cores[decision_core]), task_set, processor->crit_level, decision_core);
                schedule_new_job(&(processor->cores[decision_core]), processor->cores[decision_core].ready_queue, task_set);
            }
        }

        //If the decision point is due to timer expiry, wakeup the processor and schedule a new job from the ready queue.
        else if (decision_point == TIMER_EXPIRE)
        {
            //Wakeup the core and schedule the high priority process.
            processor->cores[decision_core].state = ACTIVE;
            processor->cores[decision_core].frequency = 1.00;
            processor->cores[decision_core].total_idle_time += (decision_time - prev_decision_time);
            stats->total_shutdown_time[decision_core] += (decision_time - prev_decision_time);

            fprintf(output[decision_core], "Timer expired. Waking up scheduler\n");

            update_job_arrivals(&(processor->cores[decision_core].ready_queue), &discarded_queue, task_set, processor->crit_level, processor->cores[decision_core].total_time, decision_core, &(processor->cores[decision_core]));

            if (processor->cores[decision_core].ready_queue->num_jobs != 0)
            {
                stats->total_context_switches[decision_core]++;
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
            double core_prev_decision_time;
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
                        stats->total_active_energy[num_core] += (processor->cores[num_core].total_time - core_prev_decision_time);
                    }
                    else {
                        processor->cores[num_core].total_idle_time += (processor->cores[num_core].total_time - core_prev_decision_time);
                        stats->total_idle_energy[num_core] += (processor->cores[num_core].total_time - core_prev_decision_time);
                    }
                    processor->cores[num_core].curr_exec_job = NULL;

                    //First remove the low criticality jobs from ready queue and insert it into discarded queue.
                    if(processor->cores[num_core].ready_queue->num_jobs != 0){
                        remove_jobs_from_ready_queue(&processor->cores[num_core].ready_queue, &discarded_queue, task_list, processor->crit_level, processor->cores[num_core].threshold_crit_lvl, num_core);
                    }

                    //Then try to accommodate the discarded jobs back in the ready queue.
                    accommodate_discarded_jobs(&(processor->cores[num_core].ready_queue), &discarded_queue, task_set, num_core, processor->crit_level, processor->cores[num_core].total_time);

                    if(processor->cores[num_core].ready_queue->num_jobs != 0) {
                        stats->total_context_switches[num_core]++;
                        select_frequency(&(processor->cores[num_core]), task_set, processor->crit_level, num_core);
                        schedule_new_job(&processor->cores[num_core], processor->cores[num_core].ready_queue, task_set);
                    }
                }
            }
        }

        if(processor->cores[decision_core].curr_exec_job != NULL) {
        fprintf(output[decision_core], "Scheduled job: %d,%d  Exec time: %.5lf  Rem exec time: %.5lf  WCET_counter: %.5lf  Deadline: %.5lf | ", 
                                processor->cores[decision_core].curr_exec_job->task_number, 
                                processor->cores[decision_core].curr_exec_job->job_number, 
                                processor->cores[decision_core].curr_exec_job->execution_time, 
                                processor->cores[decision_core].curr_exec_job->rem_exec_time, 
                                processor->cores[decision_core].WCET_counter, 
                                processor->cores[decision_core].curr_exec_job->absolute_deadline);
        }
        fprintf(output[decision_core], "Core: %d, Total time: %.5lf, Total idle time: %.5lf\n", decision_core, processor->cores[decision_core].total_time, processor->cores[decision_core].total_idle_time);
        fprintf(output[decision_core], "\n");
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
