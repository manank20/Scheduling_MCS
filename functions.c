/*
    Input format in input.txt
    1st line - number of tasks_list
    In each subsequent line, the numbers denote respectively -
    Release time of task
    Relative deadline of task (which is also the period as i have assumed deadline = period)
    Criticality level of task
    Worst Case Execution Time (WCET) for each criticality level. The WCET will be stored as an array indexed with the criticality level.

    The number of criticality levels assumed is 2.


    The struct of Kernel contains :
            Curr exec time = The time scheduled job executes for.
            Curr exec job = Scheduled job
            Total exec time = total time kernel has executed for
            Total idle time = Total time kernel was idle for
            Curr crit level = Current criticality level of kernel.

    The struct of task contains : 
        Release time
        Deadline
        Criticality level of task
        WCET for each criticality level

    We will find the hyperperiod of periods of all the tasks. The simulation of the tasks will be done for exactly one hyperperiod.

*/

#include "functions.h"
#include "data_structures.h"
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>


int gcd(int a, int b);
int min(int a, int b);
void print_task_list(task* task_list, int total_tasks, FILE* output_file);
void print_job_list(job* job_list, int total_jobs, FILE* output_file);
void print_utilisation(double total_utilisation[][MAX_CRITICALITY_LEVELS], FILE* output_file);
int update_job_arrivals(job** job_list, task* tasks_list, int total_jobs, kernel_struct* kernel);
void find_job_parameters(task* tasks_list, job* job_list, int task_number, int release_time, kernel_struct *kernel);
void find_earliest_job(job** job_list_head);
int find_actual_execution_time(int exec_time);
int find_hyperperiod(task* tasks_list, int total_tasks);
void find_utilisation(task* tasks_list, int total_tasks, double total_utilisation[][MAX_CRITICALITY_LEVELS]);
void check_criticality_change(job** job_list_head, task* task_list, kernel_struct** kernel, FILE* output_file);
void assign_absolute_deadlines(job** job_list_head, task* tasks_list);

task* get_taskset(FILE* fd, int* total_tasks) {
    int num_task, criticality_lvl;
    int tasks;

    //Number of tasks_list
    fscanf(fd, "%d", &tasks);
    (*total_tasks) = tasks;

    task *tasks_list;

    tasks_list = (task*)malloc(sizeof(task)*tasks);

    /*
        Reading input from the file.
        The input format of the file is:
        Release time, Deadline, Criticality level, WCET for each criticality level.
    */
    for(num_task=0; num_task<tasks; num_task++){
        fscanf(fd, "%d%d%d", &tasks_list[num_task].phase, &tasks_list[num_task].relative_deadline, &tasks_list[num_task].criticality_lvl);
        
        //As it is an implicit-deadline taskset, period = deadline.
        tasks_list[num_task].period = tasks_list[num_task].relative_deadline;
        tasks_list[num_task].job_number = 0;

        for(criticality_lvl=0; criticality_lvl<MAX_CRITICALITY_LEVELS; criticality_lvl++){
            fscanf(fd, "%d", &tasks_list[num_task].WCET[criticality_lvl]);
        }
    }

    return tasks_list;
}

kernel_struct* initialize_kernel() {
    kernel_struct* kernel;
    kernel = (kernel_struct*)malloc(sizeof(kernel_struct));
    
    /*
        Different parameters of Kernel are as follows: 
        Curr exec time = The time scheduled job executes for.
        Curr exec job = Scheduled job
        Total exec time = total time kernel has executed for
        Total idle time = Total time kernel was idle for
        Curr crit level = Current criticality level of kernel.
    */
    kernel->curr_exec_time = 0;
    kernel->curr_exec_process = -1;
    kernel->total_exec_time = 0;
    kernel->curr_crit_level = 0;
    kernel->total_idle_time = 0;

    return kernel;

}

void schedule_taskset(task* tasks_list, int total_tasks, kernel_struct* kernel, FILE* output_file){

    int hyperperiod;
    job* earliest_job;

    print_task_list(tasks_list, total_tasks, output_file);

    // Creating a job array of dynamic size. Thus, it is implemented as a linked list. This array will contain only those jobs which have arrived at the current timestamp.
    //The head of the linked list will always contain the job to be executed.
    job *job_list_head = NULL;

    //Finding the hyperperiod of all tasks. The kernel will execute for one hyperperiod.
    hyperperiod = find_hyperperiod(tasks_list, total_tasks);
    fprintf(output_file, "Hyperperiod = %d\n\n", hyperperiod);

    while(kernel->total_exec_time < hyperperiod && kernel->curr_crit_level <= threshold_criticality_level) {

        int has_arrived = update_job_arrivals(&job_list_head, tasks_list, total_tasks, kernel);    

        fprintf(output_file, "Kernel Total Exec time: %d | Curr Crit level: %d | ", kernel->total_exec_time, kernel->curr_crit_level);

        //If the job array is NULL, then no jobs have arrived for execution. The kernel will be idle for this time unit.
        if(job_list_head == NULL){
            fprintf(output_file, "Kernel Idle \n");
            kernel->total_idle_time++;
            goto L1;
        }

        // If the remaining execution time of the currently executing job is 0, then remove it from the list and schedule a different job.
        if(job_list_head->remaining_execution_time == 0) {
            //Check for criticality change. Remove all the jobs from the array below the criticality level of kernel.
            check_criticality_change(&job_list_head, tasks_list, &kernel, output_file);

            if(kernel->curr_crit_level > threshold_criticality_level) {
                break;
            }
            
            //Remove the currently executing job from the job array,.
            job* remove_job = job_list_head;
            job_list_head = job_list_head->next;
            remove_job->next = NULL;
            free(remove_job);


            //If this job was the last job in the array, then the array will be empty and the kernel will be idle for this time unit.
            if(job_list_head!=NULL)
                find_earliest_job(&job_list_head);
            else{
                fprintf(output_file, "Kernel Idle \n");
                kernel->total_idle_time++;
                goto L1;
            }
        }

        //If has_arrived is 1, then a job has arrived at the current timestep. This is a decision point and the currently executing job needs to be updated.
        if(has_arrived == 1) {
            find_earliest_job(&job_list_head);
        }

        fprintf(output_file, "Scheduled job: %d  Exec time: %d  Rem exec time: %d  Deadline: %d  ", job_list_head->task_number, job_list_head->execution_time, job_list_head->remaining_execution_time, job_list_head->absolute_deadline);
        job_list_head->remaining_execution_time --;
    
        // print_job_list(job_list_head, total_tasks, output_file);
        fprintf(output_file, "\n");


L1:     kernel->total_exec_time++;

    }

    fprintf(output_file, "\nKernel Total Exec time: %d | Curr Crit level: %d | \n", kernel->total_exec_time, kernel->curr_crit_level);    

    //As the criticality level has crossed the threshold criticality level, we will assign absolute deadlines to all the jobs currently in the array.
    assign_absolute_deadlines(&job_list_head, tasks_list);

    //For the remaining part of scheduling, we will use the absolute deadline of the tasks.
    while(kernel->total_exec_time < hyperperiod) {

        int has_arrived = update_job_arrivals(&job_list_head, tasks_list, total_tasks, kernel);    

        fprintf(output_file, "Kernel Total Exec time: %d | Curr Crit level: %d | ", kernel->total_exec_time, kernel->curr_crit_level);

        //If the job array is NULL, then no jobs have arrived for execution. The kernel will be idle for this time unit.
        if(job_list_head == NULL){
            fprintf(output_file, "Kernel Idle \n");
            kernel->total_idle_time++;
            goto L2;
        }

        // If the remaining execution time of the currently executing job is 0, then remove it from the list and schedule a different job.
        if(job_list_head->remaining_execution_time == 0) {
            //Check for criticality change
            check_criticality_change(&job_list_head, tasks_list, &kernel, output_file);
            
            //Remove the currently executing job from the job array,.
            job* remove_job = job_list_head;
            job_list_head = job_list_head->next;
            remove_job->next = NULL;
            free(remove_job);



            //If this job was the last job in the array, then the array will be empty and the kernel will be idle for this time unit.
            if(job_list_head!=NULL)
                find_earliest_job(&job_list_head);
            else{
                fprintf(output_file, "Kernel Idle \n");
                kernel->total_idle_time++;
                goto L2;
            }
        }

        //If has_arrived is 1, then a job has arrived at the current timestep. This is a decision point and the currently executing job needs to be updated.
        if(has_arrived == 1) {
            find_earliest_job(&job_list_head);
        }

        fprintf(output_file, "Scheduled job: %d  Exec time: %d  Rem exec time: %d  Deadline: %d  ", job_list_head->task_number, job_list_head->execution_time, job_list_head->remaining_execution_time, job_list_head->absolute_deadline);
        job_list_head->remaining_execution_time --;
    
        // print_job_list(job_list_head, total_tasks, output_file);
        fprintf(output_file, "\n");


L2:     kernel->total_exec_time++;

    }

}

void check_criticality_change(job** job_list_head, task* task_list, kernel_struct** kernel, FILE* output_file) {
    int WCET = task_list[(*job_list_head)->task_number].WCET[(*kernel)->curr_crit_level];
    

    //If the worst case execution time of the currently executing job is less than the actual execution time of the job, then the criticality changes.
    if(WCET < (*job_list_head)->execution_time) {
        fprintf(output_file, "Criticality Changed | ");
        (*kernel)->curr_crit_level = min(MAX_CRITICALITY_LEVELS - 1, (*kernel)->curr_crit_level + 1);
    }

    //Remove all the jobs whose criticality level is less than the current criticality level of kernel.
    job* job_temp = (*job_list_head);
    while(job_temp->next) {
        if(task_list[job_temp->next->task_number].criticality_lvl < (*kernel)->curr_crit_level) {
            job* remove_job = job_temp->next;
            job_temp->next->next == NULL ? (job_temp->next = NULL) : (job_temp->next = job_temp->next->next);
            remove_job->next = NULL;
            free(remove_job);
        }
        if(job_temp->next==NULL)
            break;
        job_temp = job_temp->next;
    }      

}

int update_job_arrivals(job** job_list, task* tasks_list, int total_jobs, kernel_struct* kernel) {
    int has_arrived = 0;

    for(int curr_job=0; curr_job<total_jobs; curr_job++) {
        int release_time = (tasks_list[curr_job].phase + tasks_list[curr_job].period * tasks_list[curr_job].job_number);

        //If the job has arrived at this time unit, then insert the job after the head in the job array.
        if(tasks_list[curr_job].criticality_lvl >= kernel->curr_crit_level && kernel->total_exec_time == release_time) {
            // fprintf(output_file, "New job %d has arrived | ", curr_job);

            job* new_job = malloc(sizeof(job));

            find_job_parameters(tasks_list, new_job, curr_job, release_time, kernel);
            
            if((*job_list) == NULL){
                (*job_list) = new_job;
            }
            else{
                new_job->next = (*job_list)->next;
                (*job_list)->next = new_job;
            }
            
            tasks_list[curr_job].job_number++;
            has_arrived = 1;
        }
    }

    return has_arrived;
}

void assign_absolute_deadlines(job** job_list_head, task* tasks_list){
    job* temp = (*job_list_head);
    while(temp!=NULL) {
        temp->absolute_deadline = temp->release_time + tasks_list[temp->task_number].relative_deadline;
        temp = temp->next;
    }
}

void find_job_parameters(task* tasks_list, job* new_job, int task_number, int release_time, kernel_struct *kernel){
    
    new_job->release_time = release_time;

    int actual_exec_time = find_actual_execution_time(tasks_list[task_number].WCET[kernel->curr_crit_level]);
    new_job->execution_time = actual_exec_time;
    new_job->remaining_execution_time = actual_exec_time;
    new_job->task_number = task_number;    
    new_job->next = NULL;

    if(kernel->curr_crit_level > threshold_criticality_level) {
        new_job->absolute_deadline = new_job->release_time + tasks_list[task_number].relative_deadline;
    }
    else{
        new_job->absolute_deadline = new_job->release_time + tasks_list[task_number].virtual_deadline;
    }

}

/*
    Finding the right job to schedule
    Parameters used to schedule the job:
        Release time of job <= Kernel's total execution time
        Criticality level of job >= Kernel's current criticality level
        Earliest absolute deadline of job.

    In this function, we don't need to check the first two cases as the job array will only contain the jobs which have arrived and whose criticality level is greater than the current criticality level of the kernel.
    Absolute deadline is calculated using the virtual deadline of the task.
*/

void find_earliest_job(job** job_list_head){

    job* selected_job = (*job_list_head);
    int min_deadline = (*job_list_head)->absolute_deadline;

    job* curr_job = (*job_list_head);
    job* prev_job = NULL;

    while(curr_job->next != NULL) {
        if(curr_job->next->absolute_deadline < min_deadline) {
            prev_job = curr_job;
            min_deadline = curr_job->next->absolute_deadline;
        }
        curr_job = curr_job->next;
    }   

    if(prev_job != NULL) {
        selected_job = prev_job->next;
        (prev_job->next->next == NULL) ? (prev_job->next = NULL) : (prev_job->next = prev_job->next->next);
        selected_job->next = (*job_list_head);
        (*job_list_head) = selected_job;
    }


}

/*
    Finding the hyperperiod of all tasks. The simulation of Kernel will be done for exactly one hyperperiod.
*/
int find_hyperperiod(task* tasks_list, int total_tasks){
    int lcm = tasks_list[0].period;
    int num_task;

    for(num_task=1; num_task<total_tasks; num_task++){
        lcm = ((tasks_list[num_task].period*lcm)/gcd(lcm, tasks_list[num_task].period));
    }
    
    return lcm;

}

/*
    Finding the utilisation for all criticality levels. 
    The utilisation structure is a 2-D matrix.
*/
void find_utilisation(task* tasks_list, int total_tasks, double total_utilisation[][MAX_CRITICALITY_LEVELS]){

    double utilisation[total_tasks][MAX_CRITICALITY_LEVELS];
    int i, j, l, k;

    for(i=0; i<total_tasks; i++){
        for(j=0; j<MAX_CRITICALITY_LEVELS; j++){
            //Do not need to calculate utilisation for criticality level greater than the task's criticality level.
            utilisation[i][j] = (double)tasks_list[i].WCET[j] / (double)tasks_list[i].relative_deadline;
        }
    }

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
    Check if the given taskset is schedulable or not.

    If the taskset is schedulable, then calculate virtual deadlines for all tasks

*/
int check_schedulability(task* tasks_list, int total_tasks, FILE* output_file){

    //Not creating this array in heap as we don't need this outside of the scope of this function.
    double total_utilisation[MAX_CRITICALITY_LEVELS][MAX_CRITICALITY_LEVELS];

    find_utilisation(tasks_list, total_tasks, total_utilisation);

    double check_utilisation = 0.0;
    int flag = 0;
    int check_feasibility = 1;
    int criticality_lvl, num_task;
    int l, k;
    double left_ratio, right_ratio, left_ratio_div, right_ratio_div;
    double utilisation_1_to_l = 0.0;
    double utilisation_l_to_MAX = 0.0;
    double utilisation_K_to_MAX = 0.0;

    print_utilisation(total_utilisation, output_file);

    //Condition to be checked for feasible tasksets. The condition is given in 2015 Baruah's paper - page 7.
    for(k=0; k<MAX_CRITICALITY_LEVELS; k++){
        check_utilisation = 0.0;
        for(l=k; l<MAX_CRITICALITY_LEVELS; l++){
            check_utilisation += total_utilisation[l][k];
        }  

        //If check_utilisation is greater than 1, the task set is not feasible on unit speed processor. 
        if(check_utilisation > 1){
            check_feasibility = 0;
            break;
        }

    }

    //If check_feasibility is zero, then the taskset is not feasible on a unit speed processor.
    if(check_feasibility == 0)
        return 0;

    check_utilisation = 0.0;
    for(criticality_lvl=0; criticality_lvl<MAX_CRITICALITY_LEVELS; criticality_lvl++){
        check_utilisation += total_utilisation[criticality_lvl][criticality_lvl];
    }

    //If all tasks are able to execute the worst case execution time of their respective criticality level, that is, check_utilisation <= 1
    //then the taskset is schedulable and thus, the virtual deadlines are equal to their relative deadlines.
    if(check_utilisation <= 1){
        threshold_criticality_level = MAX_CRITICALITY_LEVELS - 1;
        //x = 1 for all tasksets.
        for(num_task=0; num_task<total_tasks; num_task++)  
            tasks_list[num_task].virtual_deadline = tasks_list[num_task].relative_deadline;
        x = 1.00;
        return 1;
    }

    //We find the first k for which the required condition is satisfied. Condition given in 2015 Baruah's paper - Page 10, Section 3.2
    for(criticality_lvl=0; criticality_lvl<MAX_CRITICALITY_LEVELS - 1; criticality_lvl++){
        utilisation_1_to_l = 0;
        utilisation_K_to_MAX = 0;
        utilisation_l_to_MAX = 0;
    
        //Calculating the required quantities as specified in paper.
        for(l=0; l<=criticality_lvl; l++){
            utilisation_1_to_l += total_utilisation[l][l];
        }

        for(l=criticality_lvl + 1; l<MAX_CRITICALITY_LEVELS; l++){
            utilisation_l_to_MAX += total_utilisation[l][l];
            utilisation_K_to_MAX += total_utilisation[l][criticality_lvl];
        }

        // fprintf(output_file, "\nutilisation:\n1_to_l: %lf\nl_to_max: %lf\nk_to_max: %lf\n", utilisation_1_to_l, utilisation_l_to_MAX, utilisation_K_to_MAX);

        left_ratio_div = (double)utilisation_K_to_MAX / (double)(1 - utilisation_1_to_l);
        right_ratio_div = (double)(1 - utilisation_l_to_MAX) / (double)utilisation_1_to_l;
  
        //Instead of using division, using multiplication to compare the two ratios.
        left_ratio = utilisation_1_to_l * utilisation_K_to_MAX;
        right_ratio = (1 - utilisation_1_to_l) * (1 - utilisation_l_to_MAX);
        // fprintf(output_file, "left ratio: %lf\nright ratio: %lf\n", left_ratio_div, right_ratio_div);

        //If condition is satisfied, then store that value of criticality level and make the flag as 1.
        if((utilisation_1_to_l < 1) && (left_ratio < right_ratio)){
            threshold_criticality_level = criticality_lvl;
            flag = 1;
            break;
        }

    }

    //If no such k exists, then the taskset is not schedulable.
    if(flag == 0){
        return 0;
    }

    //Otherwise, choose a value of x between the two limits. For simplicity, we are choosing the average value of two limits.
    x = (double)(left_ratio_div + right_ratio_div)/2;

    //Find the virtual deadlines of all tasks_list.
    for(num_task=0; num_task<total_tasks; num_task++){

        //If the task's criticality level is less than the threshold criticality level, then the virtual deadline is the original deadline of the task.
        if(tasks_list[num_task].criticality_lvl <= threshold_criticality_level){
            tasks_list[num_task].virtual_deadline = tasks_list[num_task].relative_deadline;
        }

        //Else, the virtual deadline is x times the relative deadline of the task.
        else{
            tasks_list[num_task].virtual_deadline = x*tasks_list[num_task].relative_deadline;
        }
    }
    return 1;
}

int rand50() 
{ 
    return rand() & 1; 
} 
 
int rand75() 
{ 
    return rand50() | rand50(); 
} 

//Random number generator to generate a number greater than or less than the given number.
//This will generate execution time greater than the worst case execution time 25% of the time and less than the worst case execution time 75% of the time.
int find_actual_execution_time(int exec_time){
    int n = rand()%exec_time;

    if(rand75() == 1){
        exec_time+=n;
    }
    else{
        exec_time-=n;
    }
    return exec_time;

}


void print_task_list(task* task_list, int total_tasks, FILE* output_file){
    int i, j;
    fprintf(output_file, "\nTaskset:\n");
    for (i = 0; i < total_tasks; i++)
    {
        fprintf(output_file, "Task %d, criticality level %d, phase %d, relative deadline %d, virtual deadline %d, ", i, task_list[i].criticality_lvl, task_list[i].phase, task_list[i].relative_deadline, task_list[i].virtual_deadline);
        fprintf(output_file, "WCET ");
        for(j = 0; j < MAX_CRITICALITY_LEVELS; j++) {
            fprintf(output_file, "%d ", task_list[i].WCET[j]);
        }
        fprintf(output_file, "\n");
    }
    fprintf(output_file, "\n");
}

void print_job_list(job* job_list_head, int total_jobs, FILE* output_file){
    job* job_temp = job_list_head;
    fprintf(output_file, "\n");
    while(job_temp!=NULL) {
        fprintf(output_file, "Job:: Task no: %d  Exec time: %d  Deadline: %d\n", job_temp->task_number, job_temp->execution_time, job_temp->absolute_deadline);
        job_temp = job_temp->next;
    }
}

void print_utilisation(double total_utilisation[][MAX_CRITICALITY_LEVELS], FILE* output_file){
    int i, j;
    fprintf(output_file, "\nTotal utilisation:\n");
    for(i=0; i<MAX_CRITICALITY_LEVELS; i++){
        for(j=0; j<MAX_CRITICALITY_LEVELS; j++){
            fprintf(output_file, "%lf  ", total_utilisation[i][j]);
        }
        fprintf(output_file, "\n");
    }
}

int gcd(int a, int b) 
{ 
    if (b == 0) 
        return a; 
    return gcd(b, a % b);  
} 

int min(int a, int b){
    return (a<b)?a:b;
}
