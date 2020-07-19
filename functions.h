#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include "data_structures.h"
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

//Purpose of the function. Precondition and postcondtions of the function.
//Global and local scope of the functions

extern void schedule_taskset(task* tasks_list, int total_tasks, kernel_struct *kernel, FILE* output_file);
extern int check_schedulability(task* tasks_list, int total_tasks, FILE* output_file);
extern task* get_taskset(FILE* fd, int *total_tasks);
extern kernel_struct* initialize_kernel();

#endif