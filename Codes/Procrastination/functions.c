#include "functions.h"
#include "data_structures.h"
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>

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

/*Function to find max of two numbers*/
double max(double a, double b) {
    return (a>b)?a:b;
}

/*Custom comparator for sorting the task list*/
int comparator(const void* p, const void* q) {
    double l = ((task*)p)->period;
    double r = ((task*)q)->period;

    return (l-r);
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

    return;
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

    return;
}

/*
    Function to print the utilisation of each task at each criticality level.
*/
void print_utilisation(int total_tasks, FILE* output_file) {
    fprintf(output_file, "\nUtilisation values for each task at each criticality level: \n");
    for(int i=0; i<total_tasks; i++){
        for(int j=0; j<MAX_CRITICALITY_LEVELS; j++){
            fprintf(output_file, "%lf  ", utilisation[i][j]);
        }
        fprintf(output_file, "\n");
    }

    return;
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
    return;

}

/*
    Function to print the procrastination interval of each task.
*/
void print_procrast_lengths(task_set_struct* task_set, FILE* output_file) {
    int i;
    fprintf(output_file, "\nProcrastination lengths at each criticality level\n");
    for(i = 0; i < task_set->total_tasks; i++) {
        fprintf(output_file, "Task: %d, Procrastination lengths: ", i);
        for(int j=0; j<=task_set->task_list[i].criticality_lvl; j++){
            fprintf(output_file, "%lf ", task_set->task_list[i].procrast_length[j]);
        }
        fprintf(output_file, "\n");
    }
    fprintf(output_file, "\n");

    return;
}

/*
    Function to print the free run time list.
*/
void print_free_list(frt_list_struct* frt_head, FILE* output_file){ 
    fprintf(output_file, "\nFree Run time list:\n");
    frt_list_struct* temp = frt_head;
    while(temp!=NULL) {
        fprintf(output_file, "Free Time: %.2lf, Priority: %.2lf\n", temp->free_run_time, temp->priority);
        temp = temp->next;
    }
    fprintf(output_file, "\n");

    return;
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
    double n = (double)(rand()%3);
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

    //Sort the tasks list based on their periods.
    qsort((void*)task_set->task_list, tasks, sizeof(task_set->task_list[0]), comparator);

    //Find the static procrastination lengths for each task.
    for(int i = 0; i < tasks; i++) {
        int crit_lvl = task_set->task_list[num_task].criticality_lvl;
        for(int lvl = 0; lvl <= crit_lvl; lvl++) {
            double length = 0;
            for(int j=0; j<=i; j++) {
                if(task_set->task_list[j].criticality_lvl >= crit_lvl) {
                    length += task_set->task_list[j].WCET[crit_lvl] / task_set->task_list[j].period;
                }
            }

            task_set->task_list[i].procrast_length[crit_lvl] = task_set->task_list[i].period * (1 - length);
        }
    }

    //Satisfy the conditions that the procrastination interval of the current task >= procrastination interval of all previous tasks.
    for(int i=tasks-2; i >= 0; i--) {
        if(task_set->task_list[i].procrast_length[LOW] > task_set->task_list[i+1].procrast_length[LOW]) {
            task_set->task_list[i].procrast_length[LOW] = task_set->task_list[i+1].procrast_length[LOW];
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
    kernel->state = ACTIVE;
    kernel->timer = -1;

    return kernel;

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
    
    return;
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
        Input: {pointer to taskset, current criticality level} 
    
    Purpose of the function: This function finds the time of earliest arriving job. 

    Postconditions:
        Output: {The arrival time of earliest arriving job}

*/
double find_earliest_arrival_job(task_set_struct* task_set, int criticality_level) {

    double min_arrival_time = INT_MAX;

    for(int i=0; i<task_set->total_tasks; i++){
        if(task_set->task_list[i].criticality_lvl >= criticality_level){
            min_arrival_time = min(min_arrival_time, task_set->task_list[i].phase + task_set->task_list[i].period*task_set->task_list[i].job_number);
        }
    }

    return min_arrival_time;
}

/*
    Preconditions: 
        Input: {pointer to taskset, pointer to ready queue, pointer to kernel}

    Purpose of the function: This function will find the next decision point of the kernel. 
                             The decision point will be the minimum of the earliest arrival job, the completion time of currently executing job and the WCET counter of currently executing job.

    Postconditions: 
        Output: {the decision point, decision time}
        Decision point = ARRIVAL | COMPLETION | TIMER_EXPIRE
        
  
*/
decision_struct find_decision_point(task_set_struct* task_set, job* ready_queue, kernel_struct* kernel) {

    double arrival_time = find_earliest_arrival_job(task_set, kernel->curr_crit_level); 
    double completion_time, timer;
    completion_time = INT_MAX;
    timer = INT_MAX;
    decision_struct decision;

    //If ready queue is not null, then update the completion time and WCET counter of the job.
    if(kernel->curr_exec_job != NULL){
        completion_time = kernel->curr_exec_job->completed_job_time;
    }

    //if the kernel is sleeping, update the timer value.
    if(kernel->state == SLEEPING){
        timer = kernel->timer;
    }

    double decision_time = min(min(arrival_time, completion_time), timer);

    //If arrival time = completion time or arrival time = wcet counter, then give preference to COMPLETION or CRIT_CHANGE.
    if(decision_time == completion_time){
        decision.decision_point = COMPLETION;
        decision.decision_time = completion_time;
    }
    else if(decision_time == timer) {
        decision.decision_point = TIMER_EXPIRE;
        decision.decision_time = timer;
    }
    else if(decision_time == arrival_time) {
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
void remove_jobs_from_queue(job_queue_struct** ready_queue, task* tasks_list){
    while((*ready_queue)->num_jobs != 0 && tasks_list[(*ready_queue)->job_list_head->task_number].criticality_lvl == LOW) {
        job* free_job = (*ready_queue)->job_list_head;
        (*ready_queue)->job_list_head = (*ready_queue)->job_list_head->next;
        (*ready_queue)->num_jobs--;
        free(free_job);
    }

    if((*ready_queue)->job_list_head==NULL)
        return;

    job* temp = (*ready_queue)->job_list_head;

    while(temp && temp->next) {
        if(tasks_list[temp->next->task_number].criticality_lvl == LOW) {
            job* free_job = temp->next;
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
void insert_job_in_queue(job_queue_struct** ready_queue, job* new_job) {
    if((*ready_queue)->num_jobs == 0) {
        (*ready_queue)->job_list_head = new_job;
        (*ready_queue)->num_jobs = 1;
    }
    else{

        if(new_job->absolute_deadline < (*ready_queue)->job_list_head->absolute_deadline) {
            new_job->next = (*ready_queue)->job_list_head;
            (*ready_queue)->job_list_head = new_job;
        }
        else{
         
            job* temp = (*ready_queue)->job_list_head;

            while(temp->next!=NULL && temp->next->absolute_deadline <= new_job->absolute_deadline) {     
                temp = temp->next;
            }
            new_job->next = temp->next;
            temp->next = new_job;
        }

        (*ready_queue)->num_jobs ++ ;
    }

    return;
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
        Input: {pointer to job queue, pointer to taskset, pointer to kernel}
                ready_queue!=NULL
                task_set!=NULL
                kernel!=NULL

    Purpose of the function: This function will insert all the jobs which have arrived at the current time unit in the ready queue. The ready queue is sorted according to the deadlines.
                             It will also compute the procrastination length which is the minimum of the procrastination intervals of all newly arrived jobs.
    Postconditions: 
        Output: {Returns the procrastination length to update the kernel timer}
        Result: An updated ready queue with all the newly arrived jobs inserted in their right positions.
*/
double update_job_arrivals(job_queue_struct** ready_queue, task_set_struct* task_set, int curr_crit_level, double arrival_time) {
    int total_tasks = task_set->total_tasks;
    task* tasks_list = task_set->task_list;

    double timer = INT_MAX;

    //If the job has arrived and its criticality level is greater than the criticality level of kernel, then only update the job in the ready queue.
    for(int curr_task = 0; curr_task < total_tasks; curr_task++) {
        double release_time = (tasks_list[curr_task].phase + tasks_list[curr_task].period * tasks_list[curr_task].job_number);        
        int criticality_lvl = tasks_list[curr_task].criticality_lvl;

        if(release_time == arrival_time && criticality_lvl >= curr_crit_level) {
            job* new_job = malloc(sizeof(job));
            find_job_parameters(tasks_list, new_job, curr_task, release_time, curr_crit_level);
            new_job->job_number = tasks_list[curr_task].job_number;

            timer = min(timer, tasks_list[curr_task].procrast_length[LOW]);

            insert_job_in_queue(ready_queue, new_job);
            tasks_list[curr_task].job_number++;

        }

    }

    return timer;
}

/*
    Preconditions: 
        Input: {pointer to taskset, pointer to the ready queue}

    Purpose of the function: Remove the currently completed job from the ready queue.

    Postconditions: 
        Output: void
        Result: The completed job is freed and the ready queue is updated.
*/
void update_job_removal(task_set_struct* taskset, job_queue_struct** ready_queue) {
    //Remove the currently executing job from the ready queue.
    job* completed_job = (*ready_queue)->job_list_head;
    (*ready_queue)->job_list_head = (*ready_queue)->job_list_head->next;
    (*ready_queue)->num_jobs -- ;

    free(completed_job);

    return; 
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
void schedule_new_job(kernel_struct *kernel, job_queue_struct* ready_queue, task_set_struct* task_set) {
    kernel->curr_exec_job = ready_queue->job_list_head;
    ready_queue->job_list_head->scheduled_time = kernel->total_time; 
    ready_queue->job_list_head->completed_job_time = kernel->total_time + (ready_queue->job_list_head->execution_time - ready_queue->job_list_head->actual_execution_time); 
    kernel->WCET_counter = ready_queue->job_list_head->scheduled_time + (ready_queue->job_list_head->WCET_counter - ready_queue->job_list_head->actual_execution_time);

    return;
}

/*
    Preconditions:
        Input: {pointer to the free run time list, the procrastination interval of currently arrived job}

    Purpose of the function: To find the latest procrastination interval of the currently arrived job.

    Postconditions:
        Output: {The max procrastination interval for the currently arrived job}
        Result: The timer of the kernel is updated with the latest value of procrastination interval. 
*/
double find_timer_expire(frt_list_struct** frt_head, double procrast_timer){
    double slack_time = INT_MIN;
    if((*frt_head)){
        slack_time = (*frt_head)->free_run_time;
    }   

    //The timer will be the max of the static procrastination interval and the slack time in the FRT list.
    double timer = max(procrast_timer, slack_time);

    //If we are consuming the slack time, remove the node from the FRT list.
    if(timer == slack_time){
        frt_list_struct* node = (*frt_head);
        (*frt_head) = (*frt_head)->next;
        free(node);
    }

    return timer;
}

/*
    Preconditions: 
        Input: {pointer to the free run time list, the currently completed job, the WCET of the completed job}

    Purpose of the function: Add any slack time generated by the job in the free run time list. 
                             The slack time is generated if the WCET of the completed job is greater than the execution time of the job.
                             The FRT list is prioritised according to the absolute deadline of the jobs.

    Postconditions:
        Output: {void}
        Result: The FRT list is updated with any slack generated by the completed job. This slack can be reclaimed by the next scheduled job. 
*/
void add_free_time_to_list(frt_list_struct** frt_head, job* completed_job, double WCET) {
    double free_time = WCET - completed_job->execution_time;

    if(free_time > 0.00000) {
        //Remove the first dummy node
        if((*frt_head) && (*frt_head)->priority == 0){
            frt_list_struct* dummy = (*frt_head);
            (*frt_head) = (*frt_head)->next;
            free(dummy);
        }    

        frt_list_struct* node = (frt_list_struct*)malloc(sizeof(frt_list_struct));
        node->free_run_time = free_time;
        node->priority = completed_job->absolute_deadline;
        node->next = NULL;

        if((*frt_head) == NULL) {
            (*frt_head) = node;
        }
        else{
            if(node->priority < (*frt_head)->priority) {
                node->next = (*frt_head);
                (*frt_head) = node;
            }
            else{
                frt_list_struct* temp = (*frt_head);

                while(temp->next && temp->next->priority <= node->priority) {
                    temp = temp->next;
                }

                node->next = temp->next;
                temp->next = node;    
            }
        }
        
    }

    return;
}

/*
    Preconditions: 
        Input: {pointer to the free run time list, the time to be subtracted from the slack (curr_time - prev_time)}

    Purpose of the function: Updates the slack time in the FRT list. 
                             The time between the previous decision point and the current decision point is the lost slack time which cannot be reclaimed by the job.

    Postconditions:
        Output: {void}
        Result: The slack time of all nodes in the FRT list is updated. 
                If the slack time becomes <= 0, then the node is removed from the list as the slack time cannot be reclaimed by any job.
*/
void update_free_time_list(frt_list_struct** frt_head, double curr_time, double prev_time) {
    double time_consumed = curr_time - prev_time;
    frt_list_struct* temp;
    temp = (*frt_head);

    while(temp!=NULL) {
        temp->free_run_time -= time_consumed;
        temp = temp->next;
    }

    //Remove all the nodes having slack time as <= 0.
    while((*frt_head)!=NULL && (*frt_head)->free_run_time <= 0){
        temp = (*frt_head);
        (*frt_head) = (*frt_head)->next;
        free(temp);
    }

    return;
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
void schedule_taskset(task_set_struct* task_set, kernel_struct* kernel, FILE* output_file){

    double hyperperiod;

    print_task_list(task_set, output_file);

    int total_tasks  = task_set->total_tasks;
    task* tasks_list = task_set->task_list;

    // Creating a job array of dynamic size. Thus, it is implemented as a linked list. This array will contain only those jobs which have arrived at the current timestamp.
    //The head of the linked list will always contain the job to be executed.
    job_queue_struct* ready_queue = (job_queue_struct*)malloc(sizeof(job_queue_struct)); 
    ready_queue->num_jobs = 0;
    ready_queue->job_list_head = NULL;

    //Initialise the frt list;
    frt_list_struct* frt_head = (frt_list_struct*)malloc(sizeof(frt_list_struct));
    frt_head->priority = 0;
    frt_head->next = NULL;

    //Finding the hyperperiod of all tasks. The kernel will execute for one hyperperiod.
    hyperperiod = find_hyperperiod(task_set);
    fprintf(output_file, "Hyperperiod = %.2lf\n\n", hyperperiod);

    while(kernel->total_time <= (hyperperiod)) {
        //Find the decision point. The decision point will be the minimum of the earliest arrival job, the completion of the currently executing job and the WCET counter for criticality change.
        decision_struct decision = find_decision_point(task_set, ready_queue->job_list_head, kernel);
        int decision_point = decision.decision_point;
        double decision_time = decision.decision_time;

        //Break from loop if total time is equal to hyperperiod.
        if(decision_time == (hyperperiod)) {
            kernel->total_idle_time += (decision_time - kernel->total_time);
            kernel->total_time = decision_time;
            fprintf(output_file, "Hyperperiod completed | Completing Scheduling\n");
            break;
        }
        
        fprintf(output_file, "Decision point: %s, Decision time: %.2lf \n", decision_point == ARRIVAL ? "ARRIVAL" : ((decision_point == COMPLETION) ? "COMPLETION" : "TIMER EXPIRE"), decision_time);
        //If the decision point is due to arrival of a job
        if(decision_point == ARRIVAL) {
            double total_prev_time = kernel->total_time;

            //The total time elapsed will be equal to the arrival time of new job now.
            kernel->total_time = decision_time;

            //Update the newly arrived jobs in the ready queue. Only those jobs whose criticality level is greater than the current criticality level of kernel will be inserted in ready queue.
            double procrast_timer = update_job_arrivals(&ready_queue, task_set, kernel->curr_crit_level, decision_time);

            //If the kernel is active, schedule a job from the ready queue
            if(kernel->state == ACTIVE) {
                //If the currently executing job in the kernel is NULL, then schedule a new job from the ready queue.
                if(kernel->curr_exec_job == NULL) {
                    kernel->total_idle_time += (decision_time - total_prev_time);
                    schedule_new_job(kernel, ready_queue, task_set);
                }

                //Update the time for which the job has executed in the kernel and the WCET counter of the job.
                kernel->curr_exec_job->actual_execution_time = kernel->total_time - kernel->curr_exec_job->scheduled_time;
                kernel->curr_exec_job->WCET_counter -= kernel->curr_exec_job->actual_execution_time;

                fprintf(output_file, "Kernel time = %.2lf | Crit level = %d | ", kernel->total_time, kernel->curr_crit_level);
                //If the currently executing job is not the head of the ready queue, then a job with earlier deadline has arrived.
                //Preempt the current job and schedule the new job for execution.
                if(!compare_jobs(kernel->curr_exec_job, ready_queue->job_list_head)) {
                    fprintf(output_file, "Preempt current job | ");
                    schedule_new_job(kernel, ready_queue, task_set);
                }
            }
            //Else, update the timer of the processor with the latest procrastination interval computed.
            else{
                fprintf(output_file, "Processor in SLEEP. Procrastinating jobs\n\n");
                update_free_time_list(&frt_head, decision_time, total_prev_time);

                //Find the procrastination interval which is the max of the slack time and the static procrastination interval computed.
                double timer = find_timer_expire(&frt_head, procrast_timer);

                timer = kernel->total_time + timer;

                //The kernel timer is the minimum of the current timer and the new timer computed.
                kernel->timer = min(kernel->timer, timer);
                kernel->total_idle_time += (decision_time - total_prev_time);

                continue;
            }
        }

        //If the decision point was due to completion of the currently executing job.
        else if(decision_point == COMPLETION) {
            //Update the total time of kernel.
            kernel->total_time = decision_time;
            kernel->curr_exec_job->actual_execution_time = kernel->total_time - kernel->curr_exec_job->scheduled_time;
            
            //If the job has completed before its WCET, add the free time in the FRT list.
            add_free_time_to_list(&frt_head, kernel->curr_exec_job, task_set->task_list[kernel->curr_exec_job->task_number].WCET[kernel->curr_crit_level]);

            int completed_task = kernel->curr_exec_job->task_number;
            update_job_removal(task_set, &ready_queue);

            fprintf(output_file, "Kernel time = %.2lf | Crit level = %d | ", kernel->total_time, kernel->curr_crit_level);
            fprintf(output_file, "Job %d completed execution | ", completed_task);

            //If ready queue is null, no job is ready for execution. Put the processor to sleep and initialize the timer as INT_MAX.
            if(ready_queue->job_list_head==NULL) {
                fprintf(output_file, "No job to execute | Putting kernel to sleep\n\n");
                kernel->curr_exec_job = NULL;

                kernel->state = SLEEPING;
                kernel->timer = INT_MAX;    
                continue;
            }
            else{
                schedule_new_job(kernel, ready_queue, task_set);
            }

        }

        //If the decision point is due to timer expiry, wakeup the processor and schedule a new job from the ready queue.
        else if(decision_point == TIMER_EXPIRE) {
            double total_time = kernel->total_time;
            kernel->total_time = decision_time;
            //Wakeup the kernel and schedule the high priority process.
            fprintf(output_file, "Kernel time = %.2lf | Crit level = %d | ", kernel->total_time, kernel->curr_crit_level);

            kernel->state = ACTIVE;
            kernel->total_idle_time += (decision_time - total_time);
            fprintf(output_file, "Timer expired. Waking up scheduler | ");

            if(ready_queue->job_list_head != NULL) {
                schedule_new_job(kernel, ready_queue, task_set);
            }
            else{
                continue;
            }
        }

        fprintf(output_file, "Scheduled job: %d  Exec time: %.2lf  Actual exec time: %.2lf  Deadline: %.2lf", kernel->curr_exec_job->task_number, kernel->curr_exec_job->execution_time, kernel->curr_exec_job->actual_execution_time, kernel->curr_exec_job->absolute_deadline);
        // print_utilisation(total_tasks, output_file);
        fprintf(output_file, "\n\n");

    }
    return;
}

/*
    Preconditions: 
        Input: {pointer to taskset, pointer to kernel, pointer to output file}

    Purpose of the function: This function will perform the offline preprocessing phase and the runtime scheduling of edf-vd.
                            If the taskset is not schedulable, it will return after displaying the same message. Else, it will start the runtime scheduling of the tasket.
                        
    Postconditions: 
        Output: {void}
*/
void edf_vd_scheduler(task_set_struct* task_set, kernel_struct* kernel, FILE* output_file) {
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
    
    print_procrast_lengths(task_set, output_file);

    schedule_taskset(task_set, kernel, output_file);
    fprintf(output_file, "Idle time of cpu = %.2lf, busy time = %.2lf\n", kernel->total_idle_time, kernel->total_time - kernel->total_idle_time);

    return; 
}
