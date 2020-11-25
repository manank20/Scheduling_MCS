#include "functions.h"
#include "data_structures.h"
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>

/*x factor by which the relative deadlines are multiplied.*/
double x;
double utilisation[MAX_TASKS][MAX_CRITICALITY_LEVELS];
int MAX_FREQUENCY;

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
    if(a == 0)
        return b;
    if(b == 0)
        return a;
    
    if(a == b)
        return a;
    
    if(a > b)
        return gcd(a-b, b);
    return gcd(a, b-a);
} 

/*Function to find min of two numbers*/
double min(double a, double b){
    return (a<b)?a:b;
}

double max(double a, double b) {
    return (a>b)?a:b;
}

/*
    Function to print the taskset.
*/
void print_task_list(task_set_struct* task_set, FILE* output_file){
    int i, j, total_tasks;
    task* task_list;
    
    total_tasks = task_set->total_tasks;
    task_list = task_set->task_list;

    fprintf(output_file, "\nTaskset:\n");
    for (i = 0; i < total_tasks; i++)
    {
        fprintf(output_file, "Task %d, criticality level %d, phase %.2lf, relative deadline %.2lf, virtual deadline %.2lf, ", i, task_list[i].criticality_lvl, task_list[i].phase, task_list[i].relative_deadline, task_list[i].virtual_deadline);
        fprintf(output_file, "WCET ");
        for(j = 0; j < MAX_CRITICALITY_LEVELS; j++) {
            fprintf(output_file, "%f ", task_list[i].WCET[j]);
        }
        fprintf(output_file, "\n");
    }
    fprintf(output_file, "\n");
}

/*
    Function to print the ready queue
*/
void print_job_list(job* job_list_head, FILE* output_file){
    job* job_temp = job_list_head;
    fprintf(output_file, "\n");
    while(job_temp!=NULL) {
        fprintf(output_file, "Job:: Task no: %d  Exec time: %.2lf  Deadline: %.2lf\n", job_temp->task_number, job_temp->execution_time, job_temp->absolute_deadline);
        job_temp = job_temp->next;
    }
}

void print_utilisation(int total_tasks, FILE* output_file) {
    fprintf(output_file, "\nUtilisation values for each task at each criticality level: \n");
    for(int i=0; i<total_tasks; i++){
        for(int j=0; j<MAX_CRITICALITY_LEVELS; j++){
            fprintf(output_file, "%lf  ", utilisation[i][j]);
        }
        fprintf(output_file, "\n");
    }
}

/*
    Function to print the utilisation matrix. 
*/
void print_total_utilisation(double total_utilisation[][MAX_CRITICALITY_LEVELS], FILE* output_file){
    int i, j;
    fprintf(output_file, "\nTotal utilisation:\n");
    for(i=0; i<MAX_CRITICALITY_LEVELS; i++){
        for(j=0; j<MAX_CRITICALITY_LEVELS; j++){
            fprintf(output_file, "%lf  ", total_utilisation[i][j]);
        }
        fprintf(output_file, "\n");
    }
}

void print_frequencies(freq_struct* frequency, FILE* output_file) {
    int i;
    fprintf(output_file, "\nFrequency values:\n");
    for(i=0; i<frequency->num_freq; i++){
        fprintf(output_file, "%d ", frequency->freq_values[i]);
    }
    fprintf(output_file, "\n");
}

/*
    A comparator function to check whether two jobs are equal or not.
*/
int compare_jobs(job* A, job* B) {
    if(A==NULL || B==NULL)
        return 0;

    if(A->task_number == B->task_number && A->absolute_deadline == B->absolute_deadline)
        return 1;
    return 0;
}

/*
    Random number generator to generate a number greater than or less than the given number.
    This will generate execution time greater than the worst case execution time 25% of the time and less than the worst case execution time 75% of the time.
*/
double find_actual_execution_time(double exec_time){
    int n = rand()%3;
    exec_time = max(1.00, exec_time - n);
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
task_set_struct* get_taskset(FILE* fd) {
    int num_task, criticality_lvl;
    int tasks;

    task_set_struct* task_set = (task_set_struct*)malloc(sizeof(task_set_struct));

    //Number of tasks_list
    fscanf(fd, "%d", &(task_set->total_tasks));
    tasks = task_set->total_tasks;
    task_set->task_list = (task*)malloc(sizeof(task)*tasks);

    for(num_task=0; num_task<tasks; num_task++){
        fscanf(fd, "%lf%lf%d", &task_set->task_list[num_task].phase, &task_set->task_list[num_task].relative_deadline, &task_set->task_list[num_task].criticality_lvl);
        
        //As it is an implicit-deadline taskset, period = deadline.
        task_set->task_list[num_task].period = task_set->task_list[num_task].relative_deadline;
        task_set->task_list[num_task].job_number = 0;

        for(criticality_lvl=0; criticality_lvl<MAX_CRITICALITY_LEVELS; criticality_lvl++){
            fscanf(fd, "%lf", &task_set->task_list[num_task].WCET[criticality_lvl]);
        }
    }

    return task_set;
}

/*
    Preconditions: 
        Input: {void}

    Purpose of the function: Creates a new kernel structure and initializes all the values of kernel.

    Postconditions:
        Output: {pointer to the kernel structure created}
        kernel!=NULL
*/
kernel_struct* initialize_kernel() {
    kernel_struct* kernel;
    kernel = (kernel_struct*)malloc(sizeof(kernel_struct));
    
    kernel->curr_exec_job = NULL;
    kernel->total_time = 0.0;
    kernel->curr_crit_level = 0;
    kernel->total_idle_time = 0.0;
    kernel->frequency = MAX_FREQUENCY;

    return kernel;

}

/*
    Preconditions:
        Input: {Pointer to input file}

    Purpose of the function: Takes input from the task file for the frequency array.

    Postconditions:
        Output: {pointer to the frequency structure}
        frequency!=NULL

*/
freq_struct* initialize_frequencies(FILE* fd) {

    int num_freq;    
    freq_struct* frequency = (freq_struct*)malloc(sizeof(freq_struct));

    fscanf(fd, "%d", &(frequency->num_freq));
    num_freq = frequency->num_freq;

    frequency->freq_values = (int*)malloc(sizeof(int)*num_freq);
    for(int i=0; i<num_freq; i++){
        fscanf(fd, "%d", &(frequency->freq_values[i]));
    }

    MAX_FREQUENCY = frequency->freq_values[num_freq-1];

    return frequency;
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
void find_utilisation(task_set_struct* task_set){

    int total_tasks = task_set->total_tasks;
    task* tasks_list = task_set->task_list;

    int i, j;

    for(i=0; i<total_tasks; i++){
        for(j=0; j<MAX_CRITICALITY_LEVELS; j++){
            //Do not need to calculate utilisation for criticality level greater than the task's criticality level.
            utilisation[i][j] = (double)tasks_list[i].WCET[j] / (double)tasks_list[i].relative_deadline;
        }
    }

}

void find_total_utilisation(int total_tasks, task* tasks_list, double total_utilisation[][MAX_CRITICALITY_LEVELS]) {
    int i, l, k;

    for(l=0; l<MAX_CRITICALITY_LEVELS; l++){
        for(k=0; k<MAX_CRITICALITY_LEVELS; k++){
            total_utilisation[l][k] = 0;
            for(i=0; i<total_tasks; i++){
                if(tasks_list[i].criticality_lvl == l){
                    total_utilisation[l][k] += utilisation[i][k];
                }
            }
        }
    }
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
int check_schedulability(task_set_struct* task_set, FILE* output_file){

    double total_utilisation[MAX_CRITICALITY_LEVELS][MAX_CRITICALITY_LEVELS];
    find_utilisation(task_set);

    int total_tasks = task_set->total_tasks;
    task* tasks_list = task_set->task_list;
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
    for(criticality_lvl=0; criticality_lvl<MAX_CRITICALITY_LEVELS; criticality_lvl++){
        check_utilisation += total_utilisation[criticality_lvl][criticality_lvl];
    }

    //If all tasks are able to execute the worst case execution time of their respective criticality level, that is, check_utilisation <= 1
    //then the taskset is schedulable and only EDF is required.
    if(check_utilisation <= 1){
        //x = 1 for all tasksets.
        for(num_task=0; num_task<total_tasks; num_task++)  
            tasks_list[num_task].virtual_deadline = tasks_list[num_task].relative_deadline;
        x = 1.00;
        return 1;
    }

    x = (double)total_utilisation[HIGH][LOW] / (double)(1 - total_utilisation[LOW][LOW]);

    check_utilisation = x*total_utilisation[HIGH][HIGH] + total_utilisation[LOW][LOW];

    if(check_utilisation > 1)
        return 0;
    else{
        for(num_task = 0; num_task < total_tasks; num_task++) {
            if(tasks_list[num_task].criticality_lvl <= LOW){
                tasks_list[num_task].virtual_deadline = tasks_list[num_task].relative_deadline;
            }
            else
            {
                tasks_list[num_task].virtual_deadline = x*tasks_list[num_task].relative_deadline;
            }   
        }
        return 1;
    }
}

/*
    Preconditions: 
        Input: {the pointer to taskset}
                task_set!=NULL

    Purpose of the function: The function will find the hyperperiod of all the tasks in the taskset. The kernel will run for exactly one hyperperiod.

    Postconditions:
        Output: {The hyperperiod is returned}
        
*/
double find_hyperperiod(task_set_struct* task_set){
    task* tasks_list = task_set->task_list;
    int total_tasks = task_set->total_tasks;

    double lcm = tasks_list[0].period;
    int num_task;

    for(num_task=1; num_task<total_tasks; num_task++){
        lcm = ((tasks_list[num_task].period*lcm)/gcd(lcm, tasks_list[num_task].period));
    }
    
    return lcm;
}

/*
    Preconditions: 
        Input: {pointer to taskset, pointer to kernel, pointer to frequency array}

    Purpose of the function: Selects the lowest frequency such that the taskset is still schedulable.

    Postconditions:
        Output: {void}
        Result: The kernel's frequency is updated to the lowest frequency found. The kernel will now operate at this frequency.
*/

void select_frequency(task_set_struct* taskset, kernel_struct** kernel, freq_struct* frequency) {
    int* freq_values = frequency->freq_values;
    int num_freq = frequency->num_freq;
    int curr_freq;

    int total_tasks = taskset->total_tasks;
    task* tasks_list = taskset->task_list;

    double total_utilisation[MAX_CRITICALITY_LEVELS][MAX_CRITICALITY_LEVELS];
    find_total_utilisation(total_tasks, tasks_list, total_utilisation);   

    double sum_utilisation = 0.0;
    for (int i = 0; i < MAX_CRITICALITY_LEVELS; i++)
    {
        sum_utilisation += total_utilisation[i][i];
    }
    
    double freq_ratio;
    for(curr_freq=0; curr_freq<num_freq; curr_freq++){
        freq_ratio = (double)freq_values[curr_freq] / (double)freq_values[num_freq - 1];
        if(sum_utilisation < freq_ratio){
            (*kernel)->frequency = freq_values[curr_freq];
            return;
        }
    }
}

/*
    Preconditions:
        Input: {pointer to taskset, current criticality level} 
    
    Purpose of the function: This function finds the time of earliest arriving job. 

    Postconditions:
        Output: {The arrival time of earliest arriving job}

*/
int find_earliest_arrival_job(task_set_struct* task_set, int criticality_level) {

    double min_arrival_time = __INT16_MAX__;

    for(int i=0; i<task_set->total_tasks; i++){
        if(task_set->task_list[i].criticality_lvl >= criticality_level)
            min_arrival_time = min(min_arrival_time, task_set->task_list[i].phase + task_set->task_list[i].period*task_set->task_list[i].job_number);
    }

    return min_arrival_time;
}

/*
    Preconditions: 
        Input: {pointer to taskset, pointer to ready queue, pointer to kernel}

    Purpose of the function: This function will find the next decision point of the kernel. 
                             The decision point will be the minimum of the earliest arrival job, the completion time of currently executing job and the WCET counter of currently executing job.

    Postconditions: 
        Output: {the decision point which will be one of ARRIVAL, COMPLETION or CRIT_CHANGE}
  
*/
decision_struct find_decision_point(task_set_struct* task_set, job* ready_queue, kernel_struct* kernel) {

    double arrival_time = find_earliest_arrival_job(task_set, kernel->curr_crit_level); 
    double completion_time, WCET_counter;
    completion_time = __INT16_MAX__;
    WCET_counter = __INT16_MAX__;
    decision_struct decision;

    //If ready queue is not null, then update the completion time and WCET counter of the job.
    if(ready_queue){
        completion_time = ready_queue->completed_job_time;
        // if(kernel->curr_crit_level == LOW)
        //     WCET_counter = kernel->WCET_counter;
    }

    // fprintf(output_file, "Arrival: %d | Completion = %d | WCET_counter = %d\n", arrival_time, completion_time, WCET_counter);

    double decision_time = min(min(arrival_time, completion_time), WCET_counter);

    //If arrival time = completion time or arrival time = wcet counter, then give preference to COMPLETION or CRIT_CHANGE.
    if(decision_time == completion_time){
        decision.decision_point = COMPLETION;
        decision.decision_time = completion_time;
        return decision;
    }
    // else if(decision_time == WCET_counter){
    //     decision.decision_point = CRIT_CHANGE;
    //     decision.decision_time = WCET_counter;
    //     return decision;
    // }
    else {
        decision.decision_time = arrival_time;
        decision.decision_point = ARRIVAL;
        return decision;
    }
}

/*
    Preconditions:
        Input: {pointer to the job queue, pointer to the taskset}
                job_queue!=NULL
                tasks_list!=NULL
    
    Purpose of the function: This function will remove all the low-criticality jobs from the ready queue.

    Postconditions:
        Output: {void}
        Result: The job queue will now contain only high criticality jobs.
*/
void remove_jobs_from_queue(job_queue_struct** job_queue, task* tasks_list){
    while((*job_queue)->num_jobs != 0 && tasks_list[(*job_queue)->job_list_head->task_number].criticality_lvl == LOW) {
        job* free_job = (*job_queue)->job_list_head;
        (*job_queue)->job_list_head = (*job_queue)->job_list_head->next;
        (*job_queue)->num_jobs--;
        free(free_job);
    }

    if((*job_queue)->job_list_head==NULL)
        return;

    job* temp = (*job_queue)->job_list_head;

    while(temp && temp->next) {
        if(tasks_list[temp->next->task_number].criticality_lvl == LOW) {
            job* free_job = temp->next;
            temp->next = temp->next->next;
            (*job_queue)->num_jobs--;
            free(free_job);
        }
        else
            temp = temp->next;
    }

}


/*
    Preconditions:
        Input: {pointer to ready queue (passed by pointer), pointer to job to be inserted}
                (*job_queue)!=NULL
                new_job!=NULL

    Purpose of the function: This function enters a new job in the ready queue in the appropriate location. The ready queue is sorted according to the deadlines.
                            
    Postconditions: 
        Output: {void}
        Result: A new ready queue with the newly arrived job inserted in the correct position.
*/
void insert_job_in_queue(job_queue_struct** job_queue, job* new_job) {
    if((*job_queue)->num_jobs == 0) {
        (*job_queue)->job_list_head = new_job;
        (*job_queue)->num_jobs = 1;
    }
    else{

        if(new_job->absolute_deadline < (*job_queue)->job_list_head->absolute_deadline) {
            new_job->next = (*job_queue)->job_list_head;
            (*job_queue)->job_list_head = new_job;
        }
        else{
         
            job* temp = (*job_queue)->job_list_head;

            while(temp->next!=NULL && temp->next->absolute_deadline <= new_job->absolute_deadline) {     
                temp = temp->next;
            }
            new_job->next = temp->next;
            temp->next = new_job;
        }

        (*job_queue)->num_jobs ++ ;
    }
}


/*
    Preconditions: 
        Input: {pointer to taskset, pointer to the newly arrived job, the task number of job, the release time of the job, pointer to the kernel}
                tasks_list!=NULL
                new_job!=NULL
                kernel!=NULL

    Purpose of the function: This function will initialize all the fields in the newly arrived job. The fields updated will be
        release time, actual execution time, remaining execution time (=actual execution time), WCET counter of job, task number, release time of job, next pointer which points to next job in the ready queue.

    Postconditions: 
        Output: {void}
        Result: A newly arrived job with all the fields initialized.
*/
void find_job_parameters(task* tasks_list, job* new_job, int task_number, double release_time, int curr_crit_level){
    new_job->release_time = release_time; 

    double actual_exec_time;
    if(tasks_list[task_number].criticality_lvl == HIGH) {
        actual_exec_time = min(find_actual_execution_time(tasks_list[task_number].WCET[curr_crit_level]), tasks_list[task_number].WCET[HIGH]);
    }
    else{
        actual_exec_time = find_actual_execution_time(tasks_list[task_number].WCET[curr_crit_level]);
    }
    
    new_job->execution_time = actual_exec_time;
    new_job->actual_execution_time = 0;
    new_job->WCET_counter = tasks_list[task_number].WCET[curr_crit_level];
    new_job->task_number = task_number;    
    new_job->absolute_deadline = new_job->release_time + tasks_list[task_number].virtual_deadline;
    new_job->next = NULL;
}

/*
    Preconditions:
        Input: {pointer to job queue, pointer to taskset, pointer to kernel}
                job_queue!=NULL
                task_set!=NULL
                kernel!=NULL

    Purpose of the function: This function will insert all the jobs which have arrived at the current time unit in the ready queue. The ready queue is sorted according to the deadlines.

    Postconditions: 
        Output: {If a job has arrived, then it will return 1, else 0}
        Result: An updated ready queue with all the newly arrived jobs inserted in their right positions.
*/
void update_job_arrivals(job_queue_struct** job_queue, task_set_struct* task_set, int curr_crit_level, int arrival_time) {
    int total_tasks = task_set->total_tasks;
    task* tasks_list = task_set->task_list;

    //If the job has arrived and its criticality level is greater than the criticality level of kernel, then only update the job in the ready queue.
    for(int curr_task = 0; curr_task < total_tasks; curr_task++) {
        double release_time = (tasks_list[curr_task].phase + tasks_list[curr_task].period * tasks_list[curr_task].job_number);        
        int criticality_lvl = tasks_list[curr_task].criticality_lvl;

        if(release_time == arrival_time && criticality_lvl >= curr_crit_level) {
            job* new_job = malloc(sizeof(job));
            find_job_parameters(tasks_list, new_job, curr_task, release_time, curr_crit_level);
            new_job->job_number = tasks_list[curr_task].job_number;

            insert_job_in_queue(job_queue, new_job);

            //Update the utilisation value of the current job. This utilisation value will be used to calculate the next frequency value.
            utilisation[curr_task][criticality_lvl] = (double)tasks_list[curr_task].WCET[criticality_lvl] / (double)tasks_list[curr_task].relative_deadline;

            tasks_list[curr_task].job_number++;
        }

    }

    return;
}

void update_job_removal(task_set_struct* taskset, job_queue_struct** job_queue, int curr_freq) {
    //Remove the currently executing job from the ready queue.
    job* completed_job = (*job_queue)->job_list_head;
    (*job_queue)->job_list_head = (*job_queue)->job_list_head->next;
    (*job_queue)->num_jobs -- ;

    int task_num = completed_job->task_number;
    double cycles_completed = completed_job->max_cycles;
    double relative_deadline = taskset->task_list[task_num].relative_deadline;
    int crit_level = taskset->task_list[task_num].criticality_lvl;

    //Update the utilisation value of the currently completed job. The utilisation value will depend on the actual execution time taken by the job. 
    //This utilisation value will be used for the calculation of new frequency.
    utilisation[task_num][crit_level] = cycles_completed / relative_deadline;
    free(completed_job);
}

/*
    Precondition: 
        Input: {pointer to kernel, pointer to ready queue, pointer to the taskset}

    Purpose of the function: This function will schedule a new job in the kernel. 
                             The time of scheduling of job and the time at which job will be completed is updated.
                             The WCET counter of job is updated to indicate the time at which the job will cross its WCET.

    Postconditions:
        Output: {void}
        Result: A new job is scheduled in the kernel and its scheduling time, completion time and WCET counter of kernel is updated.
*/
void schedule_new_job(kernel_struct *kernel, job_queue_struct* job_queue, task_set_struct* task_set) {
    kernel->curr_exec_job = job_queue->job_list_head;
    job_queue->job_list_head->scheduled_time = kernel->total_time; 
    job_queue->job_list_head->completed_job_time = kernel->total_time + (job_queue->job_list_head->execution_time - job_queue->job_list_head->actual_execution_time); 
    kernel->WCET_counter = job_queue->job_list_head->scheduled_time + (job_queue->job_list_head->WCET_counter - job_queue->job_list_head->actual_execution_time);
}

void update_execution_times(job** job_list, int curr_freq, int prev_freq) {
    job* temp = (*job_list);
    double freq_ratio = (double)curr_freq / (double)prev_freq;
    while(temp != NULL) {
        double rem_exec_time = temp->execution_time - temp->actual_execution_time;
        double prev_exec_time = temp->execution_time;
        rem_exec_time /= freq_ratio;
        temp->execution_time = temp->actual_execution_time + rem_exec_time;
        temp->completed_job_time -= prev_exec_time;
        temp->completed_job_time += temp->execution_time;
        temp = temp->next;
    }
    
}

/*
    Preconditions: 
        Input: {pointer to taskset, pointer to kernel, pointer to output file}

    Purpose of the function: This function performs the scheduling of the taskset according to edf-vd. 
                             The scheduling will be done for one hyperperiod of the tasks.
                             A job queue will contain the jobs which have arrived at the current time unit, sorted according to their virtual deadlines. 
                             The kernel will always take the head of the queue for scheduling.
                             If any job exceeds its WCET, a counter will indicate the same and the kernel's criticality level will change.
                             At that time, all the LOW criticality jobs will be removed from the ready queue and only HIGH criticality jobs will be scheduled from now on.

    Postconditions:
        Return value: {void}
        Output: The output will be stored in the output file. Each line will give the information about:
                The type of decision point, 
                Kernel's total execution time, Kernel's current criticality level, The currently executing job, its total execution time, its actual execution time and its absolute deadline.
*/
void schedule_taskset(task_set_struct* task_set, kernel_struct* kernel, freq_struct* frequency, FILE* output_file){

    double hyperperiod;
    int prev_freq;

    print_task_list(task_set, output_file);

    int total_tasks  = task_set->total_tasks;
    task* tasks_list = task_set->task_list;

    // Creating a job array of dynamic size. Thus, it is implemented as a linked list. This array will contain only those jobs which have arrived at the current timestamp.
    //The head of the linked list will always contain the job to be executed.
    job_queue_struct* job_queue = (job_queue_struct*)malloc(sizeof(job_queue_struct)); 
    job_queue->num_jobs = 0;
    job_queue->job_list_head = NULL;

    //Finding the hyperperiod of all tasks. The kernel will execute for one hyperperiod.
    hyperperiod = find_hyperperiod(task_set);
    fprintf(output_file, "Hyperperiod = %.2lf\n\n", hyperperiod);

    while(kernel->total_time <= hyperperiod) {
        //Find the decision point. The decision point will be the minimum of the earliest arrival job, the completion of the currently executing job and the WCET counter for criticality change.
        decision_struct decision = find_decision_point(task_set, job_queue->job_list_head, kernel);
        int decision_point = decision.decision_point;
        double decision_time = decision.decision_time;
        fprintf(output_file, "Decision point: %s \n", decision_point == ARRIVAL ? "ARRIVAL" : ((decision_point == COMPLETION) ? "COMPLETION" : "CRIT CHANGE") );

        //Break from loop if total time is equal to hyperperiod.
        if(decision_time == hyperperiod) {
            kernel->total_idle_time += (decision_time - kernel->total_time);
            kernel->total_time = decision_time;
            fprintf(output_file, "Hyperperiod completed | Completing Scheduling\n");
            break;
        }

        //If the decision point is due to arrival of a job
        if(decision_point == ARRIVAL) {
            double total_time = kernel->total_time;

            //The total time elapsed will be equal to the arrival time of new job now.
            kernel->total_time = decision_time;

            //Update the newly arrived jobs in the ready queue. Only those jobs whose criticality level is greater than the current criticality level of kernel will be inserted in ready queue.
            update_job_arrivals(&job_queue, task_set, kernel->curr_crit_level, decision_time);
            prev_freq = kernel->frequency;
            select_frequency(task_set, &kernel, frequency);

            //If the currently executing job in the kernel is NULL, then schedule a new job from the ready queue.
            if(kernel->curr_exec_job == NULL) {
                kernel->total_idle_time += (decision_time - total_time);
                fprintf(output_file, "Scheduling new job %d | ", job_queue->job_list_head->task_number);
                schedule_new_job(kernel, job_queue, task_set);
            }

            //Update the time for which the job has executed in the kernel and the WCET counter of the job.
            kernel->curr_exec_job->actual_execution_time = kernel->total_time - kernel->curr_exec_job->scheduled_time;
            kernel->curr_exec_job->WCET_counter -= kernel->curr_exec_job->actual_execution_time;

            update_execution_times(&(job_queue->job_list_head), kernel->frequency, prev_freq);
            double freq_ratio = (double)kernel->frequency / (double)MAX_FREQUENCY;
            kernel->curr_exec_job->max_cycles += kernel->curr_exec_job->actual_execution_time / freq_ratio;

            fprintf(output_file, "Kernel time = %.2lf | Crit level = %d | Freq = %d | ", kernel->total_time, kernel->curr_crit_level, kernel->frequency);

            //If the currently executing job is not the head of the ready queue, then a job with earlier deadline has arrived.
            //Preempt the current job and schedule the new job for execution.
            if(!compare_jobs(kernel->curr_exec_job, job_queue->job_list_head)) {
                fprintf(output_file, "Preempt current job | ");
                schedule_new_job(kernel, job_queue, task_set);
            }
        }

        //If the decision point was due to completion of the currently executing job.
        else if(decision_point == COMPLETION) {
            //Update the total time of kernel.
            kernel->total_time = decision_time;
            kernel->curr_exec_job->actual_execution_time = kernel->total_time - kernel->curr_exec_job->scheduled_time;

            int completed_task = kernel->curr_exec_job->task_number;

            update_job_removal(task_set, &job_queue, kernel->frequency);
            prev_freq = kernel->frequency;
            select_frequency(task_set, &kernel, frequency);
            update_execution_times(&(job_queue->job_list_head), kernel->frequency, prev_freq);

            fprintf(output_file, "Kernel time = %.2lf | Crit level = %d | Freq = %d | ", kernel->total_time, kernel->curr_crit_level, kernel->frequency);
            fprintf(output_file, "Job %d completed execution | ", completed_task);

            //Check if any new job has arrived at this time stamp only. Update the ready queue with the newly arrived jobs.
            double min_arrival_time = find_earliest_arrival_job(task_set, kernel->curr_crit_level);
            if(min_arrival_time == kernel->total_time) {
                fprintf(output_file, "New job arrived | ");
                update_job_arrivals(&job_queue, task_set, kernel->curr_crit_level, min_arrival_time);
            }

            //If job queue is null, no job is ready for execution.
            if(job_queue->job_list_head==NULL) {
                fprintf(output_file, "No job to execute | Kernel idle \n\n");
                kernel->curr_exec_job = NULL;
                continue;
            }
            else{
                schedule_new_job(kernel, job_queue, task_set);
            }


        }

        //If decision point is due to criticality change, then the currently executing job has exceeded its WCET.
        // else if(decision_point == CRIT_CHANGE) {    
        //     //Increase the criticality level of kernel.
        //     kernel->curr_crit_level = min(kernel->curr_crit_level + 1, MAX_CRITICALITY_LEVELS - 1);
        //     fprintf(output_file, "CRITICALITY CHANGED | ");

        //     //Update the total time of kernel.
        //     kernel->total_time = decision_time;

        //     if(kernel->total_time == hyperperiod) {
        //         fprintf(output_file, "\nHyperperiod completed | Completing Scheduling\n");
        //         break;
        //     }

        //     fprintf(output_file, "Kernel time = %d | Crit level = %d | ", kernel->total_time, kernel->curr_crit_level);

        //     //Update the time for which the current job has executed.
        //     kernel->curr_exec_job->actual_execution_time = kernel->total_time - kernel->curr_exec_job->scheduled_time;

        //     //Remove all the low criticality jobs from the ready queue.
        //     remove_jobs_from_queue(&job_queue, tasks_list);
            
        //     //Check if any new jobs have arrived at the current time unit. Update the ready queue with the newly arrived jobs.
        //     int min_arrival_time = find_earliest_arrival_job(task_set, kernel->curr_crit_level);
        //     if(min_arrival_time == kernel->total_time) {
        //         update_job_arrivals(&job_queue, task_set, kernel->curr_crit_level, min_arrival_time);
        //     }

        //     //If ready queue is null, kernel will be idle.
        //     if(job_queue->job_list_head==NULL) {
        //         fprintf(output_file, "No job to execute | Kernel idle \n\n");
        //         kernel->curr_exec_job = NULL;
        //         continue;
        //     }
        //     else{
        //         schedule_new_job(kernel, job_queue, task_set);
        //     }

        // }

        fprintf(output_file, "Scheduled job: %d  Exec time: %.2lf  Actual exec time: %.2lf  Deadline: %.2lf ", kernel->curr_exec_job->task_number, kernel->curr_exec_job->execution_time, kernel->curr_exec_job->actual_execution_time, kernel->curr_exec_job->absolute_deadline);
        // print_utilisation(total_tasks, output_file);
        fprintf(output_file, "\n\n");

    }

    
}

/*
    Preconditions: 
        Input: {pointer to taskset, pointer to kernel, pointer to output file}

    Purpose of the function: This function will perform the offline preprocessing phase and the runtime scheduling of edf-vd.
                            If the taskset is not schedulable, it will return after displaying the same message. Else, it will start the runtime scheduling of the tasket.
                        
    Postconditions: 
        Output: {void}
*/
void edf_vd_scheduler(task_set_struct* task_set, kernel_struct* kernel, freq_struct* frequency, FILE* output_file) {
    int result = check_schedulability(task_set, output_file);

    if(result == 0){
        fprintf(output_file, "Not schedulable\n");
        return;
    }
    else
    {
        fprintf(output_file, "Schedulable\nx factor = %.2lf\n", x);
    }

    srand(time(NULL));
    
    print_frequencies(frequency, output_file);

    schedule_taskset(task_set, kernel, frequency, output_file);
    fprintf(output_file, "Idle time of cpu = %.2lf, busy time = %.2lf\n", kernel->total_idle_time, kernel->total_time - kernel->total_idle_time);
}
