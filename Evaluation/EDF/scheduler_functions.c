#include "../common/functions.h"

/*
    Preconditions: 
        Input: {File pointer to input file}
        fd!=NULL

    Purpose of the function: Takes input from the file and returns a structure of the task set. 

    Postconditions:
        Output: {Pointer to the structure of taskset created}
        task_set!=NULL
    
*/
task_set_struct *get_taskset()
{
    int num_task, criticality_lvl;
    int tasks;

    FILE *input, *exec, *allocation;
    input = fopen("../input_rts.txt", "r");

    if (input == NULL)
    {
        printf("ERROR: Cannot open input file. Format of execution is ./test input.txt\n");
        return 0;
    }

    exec = fopen("../input_times.txt", "r");
    allocation = fopen("../input_allocation.txt", "r");

    task_set_struct *task_set = (task_set_struct *)malloc(sizeof(task_set_struct));

    //Number of task_list
    fscanf(input, "%d", &(task_set->total_tasks));
    tasks = task_set->total_tasks;
    task_set->task_list = (task *)malloc(sizeof(task) * tasks);

    int cores[tasks];
    for(int i=0; i<tasks; i++)
    {
        int task, core;
        fscanf(allocation, "%d%d", &task, &core);
        cores[task] = core;
    }

    for (num_task = 0; num_task < tasks; num_task++)
    {
        fscanf(input, "%lf%lf%d", &task_set->task_list[num_task].phase, &task_set->task_list[num_task].relative_deadline, &task_set->task_list[num_task].criticality_lvl);

        //As it is an implicit-deadline taskset, period = deadline.
        task_set->task_list[num_task].period = task_set->task_list[num_task].relative_deadline;
        task_set->task_list[num_task].job_number = 0;
        task_set->task_list[num_task].util = (double *)malloc(sizeof(double) * MAX_CRITICALITY_LEVELS);
        task_set->task_list[num_task].core = cores[num_task];
        task_set->task_list[num_task].WCET = malloc(sizeof(double) * MAX_CRITICALITY_LEVELS);

        for (criticality_lvl = 0; criticality_lvl < MAX_CRITICALITY_LEVELS; criticality_lvl++)
        {
            fscanf(input, "%lf", &task_set->task_list[num_task].WCET[criticality_lvl]);
            task_set->task_list[num_task].util[criticality_lvl] = (double)task_set->task_list[num_task].WCET[criticality_lvl] / (double)task_set->task_list[num_task].period;
        }

        int num_jobs;
        fscanf(exec, "%d", &num_jobs);
        task_set->task_list[num_task].exec_times = malloc(sizeof(double)*num_jobs);
        for(int i=0; i<num_jobs; i++)
        {
            fscanf(exec, "%lf ", &task_set->task_list[num_task].exec_times[i]);
        }
    }

    //Sort the tasks list based on their periods.
    qsort((void *)task_set->task_list, tasks, sizeof(task_set->task_list[0]), period_comparator);

    fclose(input);
    fclose(exec);
    fclose(allocation);


    return task_set;
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

        if (decision_time >= super_hyperperiod)
        {
            for (num_core = 0; num_core < processor->total_cores; num_core++)
            {
                if (processor->cores[num_core].state == SHUTDOWN)
                {
                    processor->cores[num_core].total_idle_time += (super_hyperperiod - processor->cores[num_core].total_time);
                    stats->total_shutdown_time[num_core] += (super_hyperperiod - processor->cores[num_core].total_time);
                }
                else if (processor->cores[num_core].curr_exec_job == NULL) 
                {
                    processor->cores[num_core].total_idle_time += (super_hyperperiod - processor->cores[num_core].total_time);
                    stats->total_idle_energy[num_core] += (super_hyperperiod - processor->cores[num_core].total_time);
                }
                else
                {
                    processor->cores[num_core].curr_exec_job->rem_exec_time -= (super_hyperperiod - processor->cores[num_core].total_time);
                    stats->total_active_energy[num_core] += (super_hyperperiod - processor->cores[num_core].total_time);
                }
                processor->cores[num_core].total_time = super_hyperperiod;
                update_job_arrivals(&(processor->cores[num_core].ready_queue), &discarded_queue, task_set, processor->crit_level, processor->cores[num_core].total_time, num_core, &(processor->cores[num_core]), EDF, 0);
            }
            break;
        }

        fprintf(output[decision_core], "Decision point: %s, Decision time: %.5lf, Crit level: %d\n", decision_point == ARRIVAL ? "ARRIVAL" : ((decision_point == COMPLETION) ? "COMPLETION" : (decision_point == TIMER_EXPIRE ? "TIMER EXPIRE" : "CRIT_CHANGE")), decision_time, processor->crit_level);

        switch (decision_point)
        {
        case ARRIVAL:
            stats->total_arrival_points[decision_core]++;
            break;
        case COMPLETION:
            stats->total_completion_points[decision_core]++;
            break;
        case TIMER_EXPIRE:
            stats->total_wakeup_points[decision_core]++;
            break;
        case CRIT_CHANGE:
            stats->total_criticality_change_points[decision_core]++;
            break;
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

            //Update the newly arrived jobs in the ready queue. Discarded jobs can be inserted in ready queue or discarded queue depeneding on the maximum slack available.
            update_job_arrivals(&(processor->cores[decision_core].ready_queue), &discarded_queue, task_set, processor->crit_level, decision_time, decision_core, &(processor->cores[decision_core]), EDF, 0);

            //If the currently executing job is not the head of the ready queue, then a job with earlier deadline has arrived.
            //Preempt the current job and schedule the new job for execution.
            if (compare_jobs(processor->cores[decision_core].curr_exec_job, processor->cores[decision_core].ready_queue->job_list_head) == 0)
            {
                if (processor->cores[decision_core].curr_exec_job != NULL)
                {
                    fprintf(output[decision_core], "Preempt current job | ");
                }
                if (processor->cores[decision_core].ready_queue->num_jobs != 0)
                {
                    schedule_new_job(&(processor->cores[decision_core]), processor->cores[decision_core].ready_queue, task_set);
                }
                stats->total_context_switches[decision_core]++;
            }
        }

        //If the decision point was due to completion of the currently executing job.
        else if (decision_point == COMPLETION)
        {
            double procrastionation_interval;
            fprintf(output[decision_core], "Job %d, %d completed execution | ", processor->cores[decision_core].curr_exec_job->task_number, processor->cores[decision_core].curr_exec_job->job_number);

            //Check to see if the job has missed its deadline or not.
            double deadline = processor->cores[decision_core].curr_exec_job->absolute_deadline;
            if (deadline < processor->cores[decision_core].total_time)
            {
                fprintf(output[decision_core], "Deadline missed. Completing scheduling\n");
                processor->cores[decision_core].curr_exec_job = NULL;
                break;
            }

            processor->cores[decision_core].curr_exec_job = NULL;
            //Remove the completed job from the ready queue.
            update_job_removal(task_set, &(processor->cores[decision_core].ready_queue));

            stats->total_active_energy[decision_core] += (decision_time - prev_decision_time);

            //If ready queue is null, no job is ready for execution. Put the processor to sleep and find the next invocation time of processor.
            if (processor->cores[decision_core].ready_queue->num_jobs == 0)
            {
                // fprintf(output[decision_core], "No job to execute | ");

                procrastionation_interval = find_procrastination_interval(processor->cores[decision_core].total_time, task_set, processor->crit_level, decision_core);
                fprintf(output[decision_core], "Procrastination interval: %.2lf | ", procrastionation_interval);
                if (procrastionation_interval > SHUTDOWN_THRESHOLD)
                {
                    fprintf(output[decision_core], "Putting core to sleep\n");
                    processor->cores[decision_core].state = SHUTDOWN;
                    processor->cores[decision_core].next_invocation_time = procrastionation_interval + processor->cores[decision_core].total_time;
                }
                else
                {
                    fprintf(output[decision_core], "Not putting core to sleep\n");
                    processor->cores[decision_core].state = ACTIVE;
                }
            }
            else
            {
                stats->total_context_switches[decision_core]++;
                schedule_new_job(&(processor->cores[decision_core]), processor->cores[decision_core].ready_queue, task_set);
            }
        }

        //If the decision point is due to timer expiry, wakeup the processor and schedule a new job from the ready queue.
        else if (decision_point == TIMER_EXPIRE)
        {
            //Wakeup the core and schedule the high priority process.
            processor->cores[decision_core].state = ACTIVE;
            processor->cores[decision_core].total_idle_time += (decision_time - prev_decision_time);
            stats->total_shutdown_time[decision_core] += (decision_time - prev_decision_time);

            fprintf(output[decision_core], "Timer expired. Waking up scheduler\n");

            update_job_arrivals(&(processor->cores[decision_core].ready_queue), &discarded_queue, task_set, processor->crit_level, processor->cores[decision_core].total_time, decision_core, &(processor->cores[decision_core]), EDF, 1);

            if (processor->cores[decision_core].ready_queue->num_jobs != 0)
            {
                stats->total_context_switches[decision_core]++;
                schedule_new_job(&(processor->cores[decision_core]), processor->cores[decision_core].ready_queue, task_set);
            }
            else
            {
                double procrastination_interval = find_procrastination_interval(processor->cores[decision_core].total_time, task_set, processor->crit_level, decision_core);
                fprintf(output[decision_core], "Procrastination interval: %.2lf\n", procrastination_interval);
                if (procrastination_interval > SHUTDOWN_THRESHOLD)
                {
                    fprintf(output[decision_core], "Putting core to sleep again\n");
                    processor->cores[decision_core].state = SHUTDOWN;
                    processor->cores[decision_core].next_invocation_time = procrastination_interval + processor->cores[decision_core].total_time;
                }
                else
                {
                    fprintf(output[decision_core], "Not putting core to sleep\n");
                }
            }
        }

        if (processor->cores[decision_core].curr_exec_job != NULL)
        {
            fprintf(output[decision_core], "Scheduled job: %d,%d  Exec time: %.5lf  Rem exec time: %.5lf  WCET_counter: %.5lf  Deadline: %.5lf\n",
                    processor->cores[decision_core].curr_exec_job->task_number,
                    processor->cores[decision_core].curr_exec_job->job_number,
                    processor->cores[decision_core].curr_exec_job->execution_time,
                    processor->cores[decision_core].curr_exec_job->rem_exec_time,
                    processor->cores[decision_core].WCET_counter,
                    processor->cores[decision_core].curr_exec_job->absolute_deadline);
        }
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
    // int result = 1;
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
