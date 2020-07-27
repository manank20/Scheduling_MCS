#include "data_structures.h"
#include "functions.h"
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>

int main(int argc, char* argv[]){

    int total_tasks;
    int num_task;
    int criticality_lvl;
    FILE *task_file, *output_file;

    task_file = fopen(argv[1], "r");

    if(task_file == NULL){
        fprintf(output_file, "Error opening input file\n");
        return 0;
    }

    //declare output file here. Postpone opening output file.
    

    if(task_file == NULL){
        fprintf(output_file, "ERROR: Cannot open input file. Format of execution is ./test input.txt\n");
        return 0;
    }

    //get_task_set function - takes input from input file. Pass file pointer to the function.
    task* tasks_list = get_taskset(task_file, &total_tasks);


    kernel_struct *kernel = initialize_kernel();

    //Open the output file here.
    output_file = fopen(argv[2], "w");
    if(output_file == NULL) {
        fprintf(output_file, "ERROR: Cannot open output file. Make sure right permissions are provided\n");
        return 0;
    }

    int result = check_schedulability(tasks_list, total_tasks, output_file);

    if(result == 0){
        fprintf(output_file, "Not schedulable\n");
        return 0;
    }
    else
    {
        fprintf(output_file, "Schedulable\nThreshold criticality level = %d\nx factor = %lf\n", threshold_criticality_level, x);
    }

    srand(time(0));
    
    schedule_taskset(tasks_list, total_tasks, kernel, output_file);
    fprintf(output_file, "Idle time of cpu = %d, busy time = %d\n", kernel->total_idle_time, kernel->total_exec_time - kernel->total_idle_time);

    fclose(task_file);
    fclose(output_file);
    
}