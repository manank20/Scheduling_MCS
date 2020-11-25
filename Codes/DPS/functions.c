#include "functions.h"
#include "data_structures.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

/*x factor by which the relative deadlines are multiplied.*/
double x;
double utilisation[MAX_TASKS][MAX_CRITICALITY_LEVELS];

int rand50()
{
    return rand() & 1;
}

int rand75()
{
    return rand50() | rand50();
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

/*Custom comparator for sorting the task list*/
int period_comparator(const void *p, const void *q)
{
    double l = ((task *)p)->period;
    double r = ((task *)q)->period;

    return (l - r);
}

int deadline_comparator(const void *p, const void *q) {
    double l = ((job *)p)->absolute_deadline;
    double r = ((job *)q)->absolute_deadline;

    return (l - r);
}

/*
    Function to print the taskset.
*/
void print_task_list(task_set_struct *task_set, FILE *output_file)
{
    int i, j, total_tasks;
    task *task_list;

    total_tasks = task_set->total_tasks;
    task_list = task_set->task_list;

    printf("\nTaskset:\n");
    for (i = 0; i < total_tasks; i++)
    {
        printf("Task %d, criticality level %d, phase %.2lf, relative deadline %.2lf, virtual deadline %.2lf, ", i, task_list[i].criticality_lvl, task_list[i].phase, task_list[i].relative_deadline, task_list[i].virtual_deadline);
        printf("WCET ");
        for (j = 0; j < MAX_CRITICALITY_LEVELS; j++)
        {
            printf("%f ", task_list[i].WCET[j]);
        }
        printf("\n");
    }
    printf("\n");

    return;
}

/*
    Function to print the ready queue
*/
void print_job_list(job *job_list_head, FILE *output_file)
{
    job *job_temp = job_list_head;
    printf("\n");
    while (job_temp != NULL)
    {
        printf("Job:: Task no: %d  Release time: %.2lf  Exec time: %.2lf  Deadline: %.2lf\n", job_temp->task_number, job_temp->release_time, job_temp->execution_time, job_temp->absolute_deadline);
        job_temp = job_temp->next;
    }

    return;
}

/*
    Function to print the utilisation of each task at each criticality level.
*/
void print_utilisation(int total_tasks, FILE *output_file)
{
    printf("\nUtilisation values for each task at each criticality level: \n");
    for (int i = 0; i < total_tasks; i++)
    {
        for (int j = 0; j < MAX_CRITICALITY_LEVELS; j++)
        {
            printf("%lf  ", utilisation[i][j]);
        }
        printf("\n");
    }

    return;
}

/*
    Function to print the utilisation matrix. 
*/
void print_total_utilisation(double total_utilisation[][MAX_CRITICALITY_LEVELS], FILE *output_file)
{
    int i, j;
    printf("\nTotal utilisation:\n");
    for (i = 0; i < MAX_CRITICALITY_LEVELS; i++)
    {
        for (j = 0; j < MAX_CRITICALITY_LEVELS; j++)
        {
            printf("%lf  ", total_utilisation[i][j]);
        }
        printf("\n");
    }
    return;
}

void print_processor(processor_struct* processor) {
    printf("Num cores: %d\n", processor->num_cores);
    for(int i=0; i<processor->num_cores; i++) {
        printf("Core: %d, total time: %.2lf\n", i, processor->cores[0].total_time);
    }
    printf("\n");
}

/*
    A comparator function to check whether two jobs are equal or not.
*/
int compare_jobs(job *A, job *B)
{
    if (A == NULL || B == NULL)
        return 0;

    if (A->task_number == B->task_number && A->absolute_deadline == B->absolute_deadline)
        return 1;
    return 0;
}

/*
    Random number generator to generate a number greater than or less than the given number.
    This will generate execution time greater than the worst case execution time 25% of the time and less than the worst case execution time 75% of the time.
*/
double find_actual_execution_time(double exec_time)
{
    double n = (double)(rand() % 3);
    exec_time = max(1, exec_time - n);
    return exec_time;
}

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

    //Number of tasks_list
    fscanf(fd, "%d", &(task_set->total_tasks));
    tasks = task_set->total_tasks;
    task_set->task_list = (task *)malloc(sizeof(task) * tasks);

    for (num_task = 0; num_task < tasks; num_task++)
    {
        fscanf(fd, "%lf%lf%d", &task_set->task_list[num_task].phase, &task_set->task_list[num_task].relative_deadline, &task_set->task_list[num_task].criticality_lvl);

        //As it is an implicit-deadline taskset, period = deadline.
        task_set->task_list[num_task].period = task_set->task_list[num_task].relative_deadline;
        task_set->task_list[num_task].job_number = 0;

        for (criticality_lvl = 0; criticality_lvl < MAX_CRITICALITY_LEVELS; criticality_lvl++)
        {
            fscanf(fd, "%lf", &task_set->task_list[num_task].WCET[criticality_lvl]);
        }
    }

    //Sort the tasks list based on their periods.
    qsort((void *)task_set->task_list, tasks, sizeof(task_set->task_list[0]), period_comparator);

    return task_set;
}

processor_struct* initialize_processor() {
    processor_struct* processor = (processor_struct*)malloc(sizeof(processor_struct));
    int i;

    processor->num_cores = NUM_CORES;
    processor->cores = malloc(sizeof(core_struct)*(processor->num_cores));

    for(int i=0; i<processor->num_cores; i++) {
        processor->cores[i].curr_exec_job = NULL;
        processor->cores[i].total_time = 0.0f;
        processor->cores[i].total_idle_time = 0.0f;
        processor->cores[i].curr_crit_level = LOW;
        processor->cores[i].state = ACTIVE;
        processor->cores[i].timer = -1;
    }

    return processor;

}


/*
    Preconditions:
        Input: {pointer to taskset, a 2D matrix of utilisation}
                task_set!=NULL

    Purpose of the function:Finding the utilisation for all criticality levels. 
                            The utilisation structure is a 2-D matrix.

    Postconditions:
        Output: {void}
        Result: The utilisation of all jobs at each criticality level.
*/
void find_utilisation(task_set_struct *task_set)
{

    int total_tasks = task_set->total_tasks;
    task *tasks_list = task_set->task_list;

    int i, j;

    for (i = 0; i < total_tasks; i++)
    {
        for (j = 0; j < MAX_CRITICALITY_LEVELS; j++)
        {
            //Do not need to calculate utilisation for criticality level greater than the task's criticality level.
            utilisation[i][j] = (double)tasks_list[i].WCET[j] / (double)tasks_list[i].relative_deadline;
        }
    }

    return;
}

void find_total_utilisation(int total_tasks, task *tasks_list, double total_utilisation[][MAX_CRITICALITY_LEVELS])
{
    int i, l, k;

    for (l = 0; l < MAX_CRITICALITY_LEVELS; l++)
    {
        for (k = 0; k < MAX_CRITICALITY_LEVELS; k++)
        {
            total_utilisation[l][k] = 0;
            for (i = 0; i < total_tasks; i++)
            {
                if (tasks_list[i].criticality_lvl == l)
                {
                    total_utilisation[l][k] += utilisation[i][k];
                }
            }
        }
    }

    return;
}

/*
    Preconditions:  
        Input: {pointer to taskset, file pointer to output file}

    Purpose of the function: This function checks whether the taskset is schedulable or not. The taskset is schedulable if:
                            U[LOW][LOW] + U[HIGH][HIGH] <= 1 ====> This implies that taskset can be scheduled according to EDF only.
                            U[LOW][LOW] + x*U[HIGH][HIGH] <= 1 ====> This implies that EDF-VD is required.
                            If the taskset is schedulable, then compute the virtual deadlines of all the tasks according to their criticality levels.

    Postconditions: 
        Output: {if taskset is schedulable, return 1. Else return 0}
        Result: The taskset is checked for schedulability and the virtual deadlines of all the tasks is calculated.
*/
int check_schedulability(task_set_struct *task_set, FILE *output_file)
{

    double total_utilisation[MAX_CRITICALITY_LEVELS][MAX_CRITICALITY_LEVELS];
    find_utilisation(task_set);

    int total_tasks = task_set->total_tasks;
    task *tasks_list = task_set->task_list;
    int i, l, k;

    find_total_utilisation(total_tasks, tasks_list, total_utilisation);

    double check_utilisation = 0.0;
    int flag = 0;
    int check_feasibility = 1;
    int criticality_lvl, num_task;
    double left_ratio, right_ratio, left_ratio_div, right_ratio_div;
    double utilisation_1_to_l = 0.0;
    double utilisation_l_to_MAX = 0.0;
    double utilisation_K_to_MAX = 0.0;

    print_total_utilisation(total_utilisation, output_file);

    check_utilisation = 0.0;
    for (criticality_lvl = 0; criticality_lvl < MAX_CRITICALITY_LEVELS; criticality_lvl++)
    {
        check_utilisation += total_utilisation[criticality_lvl][criticality_lvl];
    }

    //If all tasks are able to execute the worst case execution time of their respective criticality level, that is, check_utilisation <= 1
    //then the taskset is schedulable and only EDF is required.
    if (check_utilisation <= 1)
    {
        //x = 1 for all tasksets.
        for (num_task = 0; num_task < total_tasks; num_task++)
            tasks_list[num_task].virtual_deadline = tasks_list[num_task].relative_deadline;
        x = 1.00;
        return 1;
    }

    x = (double)total_utilisation[HIGH][LOW] / (double)(1 - total_utilisation[LOW][LOW]);

    check_utilisation = x * total_utilisation[HIGH][HIGH] + total_utilisation[LOW][LOW];

    if (check_utilisation > 1)
        return 0;
    else
    {
        for (num_task = 0; num_task < total_tasks; num_task++)
        {
            if (tasks_list[num_task].criticality_lvl <= LOW)
            {
                tasks_list[num_task].virtual_deadline = tasks_list[num_task].relative_deadline;
            }
            else
            {
                tasks_list[num_task].virtual_deadline = x * tasks_list[num_task].relative_deadline;
            }
        }
        return 1;
    }
}

/*
    Preconditions: 
        Input: {the pointer to taskset}
                task_set!=NULL

    Purpose of the function: The function will find the hyperperiod of all the tasks in the taskset. The core will run for exactly one hyperperiod.

    Postconditions:
        Output: {The hyperperiod is returned}
        
*/
double find_hyperperiod(task_set_struct *task_set)
{
    task *tasks_list = task_set->task_list;
    int total_tasks = task_set->total_tasks;

    double lcm = tasks_list[0].period;
    int num_task;

    for (num_task = 1; num_task < total_tasks; num_task++)
    {
        lcm = ((tasks_list[num_task].period * lcm) / gcd(lcm, tasks_list[num_task].period));
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
double find_earliest_arrival_job(task_set_struct *task_set, int criticality_level)
{

    double min_arrival_time = INT_MAX;

    for (int i = 0; i < task_set->total_tasks; i++)
    {
        if (task_set->task_list[i].criticality_lvl >= criticality_level)
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
                             The decision point will be the minimum of the earliest arrival job, the completion time of currently executing job and the WCET counter of currently executing job.

    Postconditions: 
        Output: {the decision point, decision time}
        Decision point = ARRIVAL | COMPLETION | TIMER_EXPIRE
        
  
*/
decision_struct find_decision_point(task_set_struct *task_set, job *ready_queue, core_struct core)
{

    double arrival_time;
    double completion_time, timer;
    completion_time = INT_MAX;
    timer = INT_MAX;
    decision_struct decision;

    if(core.state == SLEEPING) {
        arrival_time = INT_MAX;
    }   
    else{
        arrival_time = find_earliest_arrival_job(task_set, core.curr_crit_level);
    }

    //If ready queue is not null, then update the completion time and WCET counter of the job.
    if (core.curr_exec_job != NULL)
    {
        completion_time = core.curr_exec_job->completed_job_time;
    }

    //if the core is sleeping, update the timer value.
    if (core.state == SLEEPING)
    {
        timer = core.timer;
    }


    double decision_time = min(min(arrival_time, completion_time), timer);

    //If arrival time = completion time or arrival time = wcet counter, then give preference to COMPLETION or CRIT_CHANGE.
    if (decision_time == completion_time)
    {
        decision.decision_point = COMPLETION;
        decision.decision_time = completion_time;
    }
    else if (decision_time == timer)
    {
        decision.decision_point = TIMER_EXPIRE;
        decision.decision_time = timer;
    }
    else if (decision_time == arrival_time)
    {
        decision.decision_point = ARRIVAL;
        decision.decision_time = arrival_time;
    }

    return decision;
}

/*
    Preconditions:
        Input: {pointer to the job queue, pointer to the taskset}
                ready_queue!=NULL
                tasks_list!=NULL
    
    Purpose of the function: This function will remove all the low-criticality jobs from the ready queue.

    Postconditions:
        Output: {void}
        Result: The job queue will now contain only high criticality jobs.
*/
void remove_jobs_from_queue(job_queue_struct **ready_queue, task *tasks_list)
{
    while ((*ready_queue)->num_jobs != 0 && tasks_list[(*ready_queue)->job_list_head->task_number].criticality_lvl == LOW)
    {
        job *free_job = (*ready_queue)->job_list_head;
        (*ready_queue)->job_list_head = (*ready_queue)->job_list_head->next;
        (*ready_queue)->num_jobs--;
        free(free_job);
    }

    if ((*ready_queue)->job_list_head == NULL)
        return;

    job *temp = (*ready_queue)->job_list_head;

    while (temp && temp->next)
    {
        if (tasks_list[temp->next->task_number].criticality_lvl == LOW)
        {
            job *free_job = temp->next;
            temp->next = temp->next->next;
            (*ready_queue)->num_jobs--;
            free(free_job);
        }
        else
            temp = temp->next;
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
void insert_job_in_queue(job_queue_struct **ready_queue, job *new_job)
{
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

            job *temp = (*ready_queue)->job_list_head;

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
        Input: {pointer to taskset, pointer to the newly arrived job, the task number of job, the release time of the job, pointer to the core}
                tasks_list!=NULL
                new_job!=NULL
                core!=NULL

    Purpose of the function: This function will initialize all the fields in the newly arrived job. The fields updated will be
        release time, actual execution time, remaining execution time (=actual execution time), WCET counter of job, task number, release time of job, next pointer which points to next job in the ready queue.

    Postconditions: 
        Output: {void}
        Result: A newly arrived job with all the fields initialized.
*/
void find_job_parameters(task *tasks_list, job *new_job, int task_number, double release_time, int curr_crit_level)
{
    new_job->release_time = release_time;

    double actual_exec_time;
    actual_exec_time = find_actual_execution_time(tasks_list[task_number].WCET[curr_crit_level]);

    new_job->execution_time = actual_exec_time;
    new_job->actual_execution_time = 0;
    new_job->WCET_counter = tasks_list[task_number].WCET[curr_crit_level];
    new_job->task_number = task_number;
    new_job->absolute_deadline = new_job->release_time + tasks_list[task_number].virtual_deadline;
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
void update_job_arrivals(job_queue_struct **ready_queue, task_set_struct *task_set, int curr_crit_level, double arrival_time)
{
    int total_tasks = task_set->total_tasks;
    task *tasks_list = task_set->task_list;

    //If the job has arrived and its criticality level is greater than the criticality level of core, then only update the job in the ready queue.
    for (int curr_task = 0; curr_task < total_tasks; curr_task++)
    {
        double release_time = (tasks_list[curr_task].phase + tasks_list[curr_task].period * tasks_list[curr_task].job_number);
        int criticality_lvl = tasks_list[curr_task].criticality_lvl;

        if (release_time <= arrival_time && criticality_lvl >= curr_crit_level)
        {
            job *new_job = malloc(sizeof(job));
            find_job_parameters(tasks_list, new_job, curr_task, release_time, curr_crit_level);
            new_job->job_number = tasks_list[curr_task].job_number;
            new_job->execution_time = tasks_list[curr_task].WCET[LOW];

            insert_job_in_queue(ready_queue, new_job);
            tasks_list[curr_task].job_number++;
        }
    }

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
    ready_queue->job_list_head->scheduled_time = (*core).total_time;
    ready_queue->job_list_head->completed_job_time = (*core).total_time + (ready_queue->job_list_head->execution_time - ready_queue->job_list_head->actual_execution_time);
    (*core).WCET_counter = ready_queue->job_list_head->scheduled_time + (ready_queue->job_list_head->WCET_counter - ready_queue->job_list_head->actual_execution_time);

    return;
}


job* find_job_list(double start_time, double end_time, task_set_struct* task_set) {
    job *job_head = NULL;
    int i;
    double release_time; 
    task curr_task;
    int total_jobs = 0;

    for(i=0; i<task_set->total_tasks; i++) {
        curr_task = task_set->task_list[i];
        release_time = task_set->task_list[i].phase + task_set->task_list[i].period * task_set->task_list[i].job_number;
        while(release_time >= start_time && release_time < end_time) {
            job* new_job = malloc(sizeof(job));
            new_job->execution_time = curr_task.WCET[LOW];
            new_job->release_time = release_time; 
            new_job->task_number = i;
            new_job->absolute_deadline = release_time + task_set->task_list[i].virtual_deadline;
            new_job->next = NULL;

            if(job_head == NULL) {
                job_head = new_job;
            }
            else{
                if(new_job->absolute_deadline > job_head->absolute_deadline) {
                    new_job->next = job_head;
                    job_head = new_job;
                }
                else{
                    job* temp = job_head;
                    while(temp->next && temp->next->absolute_deadline > new_job->absolute_deadline) {
                        temp = temp->next;
                    }

                    new_job->next = temp->next;
                    temp->next = new_job;
                }
            }

            release_time += task_set->task_list[i].period;
            total_jobs ++;

        }
    }

    return job_head;

}

double find_procrastination_interval(double curr_time, task_set_struct* task_set, FILE* output_file) {
    double next_deadline1 = INT_MAX, next_deadline2;
    int i, latest_task_no, earliest_task_no;
    double release_time, absolute_deadline;
    double timer_expiry = 0, total_utilisation = 0;
    double latest_time = INT_MIN;
    task earliest_task;


    for(i=0; i<task_set->total_tasks; i++) {
        release_time = task_set->task_list[i].phase + task_set->task_list[i].period * task_set->task_list[i].job_number;
        absolute_deadline = release_time + task_set->task_list[i].virtual_deadline;
        if(next_deadline1 > absolute_deadline && curr_time < absolute_deadline) {
            next_deadline1 = absolute_deadline;
            earliest_task = task_set->task_list[i];
            earliest_task_no = i;
        }
    }

    if((next_deadline1 - curr_time - earliest_task.WCET[LOW]) < SHUTDOWN_THRESHOLD) {
        return 0.00; 
    }


    for(i=0; i<task_set->total_tasks; i++) {
        release_time = task_set->task_list[i].phase + task_set->task_list[i].period * task_set->task_list[i].job_number;
        if(latest_time < release_time && release_time < next_deadline1) {
            latest_task_no = i;
            next_deadline2 = release_time + task_set->task_list[i].virtual_deadline;
            latest_time = release_time;
        }
    }

    job* job_list = find_job_list(curr_time, next_deadline2, task_set);

    timer_expiry = next_deadline2;

    for(i=0; i<task_set->total_tasks; i++) {
        total_utilisation += utilisation[i][LOW];
    }

    total_utilisation = (double)((int)(total_utilisation*100)) / 100;

    while(job_list != NULL) {
        if(job_list->absolute_deadline > next_deadline2) {
            timer_expiry -= ((next_deadline2 - job_list->release_time)*((double)job_list->execution_time / (double)task_set->task_list[job_list->task_number].period)*total_utilisation);
        }
        else{
            timer_expiry -= job_list->execution_time;
        }

        if(job_list->next && timer_expiry > job_list->next->absolute_deadline) {
            timer_expiry = job_list->next->absolute_deadline;
        }

        job* free_job = job_list;
        job_list = job_list->next;
        free(free_job);
    }

    return timer_expiry;

}

/*
    Preconditions: 
        Input: {pointer to taskset, pointer to core, pointer to output file}

    Purpose of the function: This function performs the scheduling of the taskset according to edf-vd. 
                             The scheduling will be done for one hyperperiod of the tasks.
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
void schedule_taskset(task_set_struct *task_set, processor_struct *processor, FILE *output_file)
{

    double hyperperiod;

    print_task_list(task_set, output_file);

    int total_tasks = task_set->total_tasks;
    task *tasks_list = task_set->task_list;

    // Creating a job array of dynamic size. Thus, it is implemented as a linked list. This array will contain only those jobs which have arrived at the current timestamp.
    //The head of the linked list will always contain the job to be executed.
    job_queue_struct *ready_queue = (job_queue_struct *)malloc(sizeof(job_queue_struct));
    ready_queue->num_jobs = 0;
    ready_queue->job_list_head = NULL;

    //Finding the hyperperiod of all tasks. The core will execute for one hyperperiod.
    hyperperiod = find_hyperperiod(task_set);
    printf("Hyperperiod = %.2lf\n\n", hyperperiod);


    while (processor->cores[0].total_time <= hyperperiod)
    {
        //Find the decision point. The decision point will be the minimum of the earliest arrival job, the completion of the currently executing job and the WCET counter for criticality change.
        decision_struct decision = find_decision_point(task_set, ready_queue->job_list_head, (processor->cores[0]));
        int decision_point = decision.decision_point;
        double decision_time = decision.decision_time;

        //Break from loop if total time is equal to hyperperiod.
        if (decision_time >= hyperperiod)
        {
            processor->cores[0].total_idle_time += (decision_time - processor->cores[0].total_time);
            processor->cores[0].total_time = hyperperiod;
            printf("Hyperperiod completed | Completing Scheduling\n");
            break;
        }

        printf("Decision point: %s, Decision time: %.2lf \n", decision_point == ARRIVAL ? "ARRIVAL" : ((decision_point == COMPLETION) ? "COMPLETION" : "TIMER EXPIRE"), decision_time);
        //If the decision point is due to arrival of a job
        if (decision_point == ARRIVAL)
        {
            double total_prev_time = processor->cores[0].total_time;

            //The total time elapsed will be equal to the arrival time of new job now.
            processor->cores[0].total_time = decision_time;

            //Update the newly arrived jobs in the ready queue. Only those jobs whose criticality level is greater than the current criticality level of core will be inserted in ready queue.
            update_job_arrivals(&ready_queue, task_set, processor->cores[0].curr_crit_level, decision_time);

            //If the core is active, schedule a job from the ready queue
            if (processor->cores[0].state == ACTIVE)
            {
                //If the currently executing job in the core is NULL, then schedule a new job from the ready queue.
                if (processor->cores[0].curr_exec_job == NULL)
                {
                    processor->cores[0].total_idle_time += (decision_time - total_prev_time);
                    schedule_new_job(&(processor->cores[0]), ready_queue, task_set);
                }

                //Update the time for which the job has executed in the core and the WCET counter of the job.
                processor->cores[0].curr_exec_job->actual_execution_time = processor->cores[0].total_time - processor->cores[0].curr_exec_job->scheduled_time;
                processor->cores[0].curr_exec_job->WCET_counter -= processor->cores[0].curr_exec_job->actual_execution_time;

                printf("core time = %.2lf | Crit level = %d | ", processor->cores[0].total_time, processor->cores[0].curr_crit_level);
                //If the currently executing job is not the head of the ready queue, then a job with earlier deadline has arrived.
                //Preempt the current job and schedule the new job for execution.
                if (!compare_jobs(processor->cores[0].curr_exec_job, ready_queue->job_list_head))
                {
                    printf("Preempt current job | ");
                    schedule_new_job(&(processor->cores[0]), ready_queue, task_set);
                }
            }
        }

        //If the decision point was due to completion of the currently executing job.
        else if (decision_point == COMPLETION)
        {
            //Update the total time of core.
            processor->cores[0].total_time = decision_time;
            processor->cores[0].curr_exec_job->actual_execution_time = processor->cores[0].total_time - processor->cores[0].curr_exec_job->scheduled_time;

            int completed_task = processor->cores[0].curr_exec_job->task_number;
            int completed_job = processor->cores[0].curr_exec_job->job_number;
            update_job_removal(task_set, &ready_queue);
            
            printf("core time = %.2lf | Crit level = %d | ", processor->cores[0].total_time, processor->cores[0].curr_crit_level);
            printf("Job %d, %d completed execution | ", completed_task, completed_job);

            //If ready queue is null, no job is ready for execution. Put the processor to sleep and initialize the timer as INT_MAX.
            if (ready_queue->job_list_head == NULL)
            {
                printf("No job to execute | ");
                processor->cores[0].curr_exec_job = NULL;

                double next_invocation_time = find_procrastination_interval(processor->cores[0].total_time, task_set, output_file);

                if((next_invocation_time - processor->cores[0].total_time) > SHUTDOWN_THRESHOLD) {
                    printf("Idle duration %.2lf greater than SDT | Putting core to sleep\n\n", next_invocation_time - processor->cores[0].total_time);                    
                    processor->cores[0].state = SLEEPING;
                    processor->cores[0].timer = next_invocation_time;
                }
                else{
                    printf("Idle duration %.2lf less than SDT | Not putting core to sleep\n\n", next_invocation_time - processor->cores[0].total_time);
                    processor->cores[0].state = ACTIVE;
                }
                continue;
            }
            else
            {
                schedule_new_job(&(processor->cores[0]), ready_queue, task_set);
            }
        }

        //If the decision point is due to timer expiry, wakeup the processor and schedule a new job from the ready queue.
        else if (decision_point == TIMER_EXPIRE)
        {
            double total_time = processor->cores[0].total_time;
            processor->cores[0].total_time = decision_time;
            //Wakeup the core and schedule the high priority process.
            printf("core time = %.2lf | Crit level = %d | ", processor->cores[0].total_time, processor->cores[0].curr_crit_level);

            processor->cores[0].state = ACTIVE;
            processor->cores[0].total_idle_time += (decision_time - total_time);
            printf("Timer expired. Waking up scheduler | ");
            
            update_job_arrivals(&ready_queue, task_set, processor->cores[0].curr_crit_level, processor->cores[0].total_time);

            if (ready_queue->job_list_head != NULL)
            {
                schedule_new_job(&(processor->cores[0]), ready_queue, task_set);
            }
            else
            {
                continue;
            }
        }

        printf("Scheduled job: %d,%d  Exec time: %.2lf  Actual exec time: %.2lf  Deadline: %.2lf", processor->cores[0].curr_exec_job->task_number, processor->cores[0].curr_exec_job->job_number, processor->cores[0].curr_exec_job->execution_time, processor->cores[0].curr_exec_job->actual_execution_time, processor->cores[0].curr_exec_job->absolute_deadline);
        // print_utilisation(total_tasks, output_file);
        printf("\n\n"); 
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
void runtime_scheduler(task_set_struct *task_set, processor_struct *processor, FILE *output_file)
{
    int result = check_schedulability(task_set, output_file);

    if (result == 0)
    {
        printf("Not schedulable\n");
        return;
    }
    else
    {
        printf("Schedulable\nx factor = %.2lf\n", x);
    }

    srand(time(NULL));

    schedule_taskset(task_set, processor, output_file);
    printf("Idle time of cpu = %.2lf, busy time = %.2lf\n", processor->cores[0].total_idle_time, processor->cores[0].total_time - processor->cores[0].total_idle_time);

    return;
}
