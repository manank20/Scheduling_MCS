#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include "data_structures.h"

/*
    Preconditions: 
        Input: {File pointer to input file}
        fd!=NULL

    Purpose of the function: Takes input from the file and returns a structure of the task set. 

    Postconditions:
        Output: {Pointer to the structure of taskset created}
        task_set!=NULL
    
*/
extern task_set_struct *get_taskset(FILE *fd);

/*
    Preconditions: 
        Input: {pointer to taskset, pointer to kernel, pointer to output file}

    Purpose of the function: This function will perform the offline preprocessing phase and the runtime scheduling of edf-vd.
                             If the taskset is not schedulable, it will return after displaying the same message. Else, it will start the runtime scheduling of the tasket.
                        
    Postconditions: 
        Output: {void}
*/
extern void runtime_scheduler(task_set_struct *task_set, processor_struct *processor);

#endif