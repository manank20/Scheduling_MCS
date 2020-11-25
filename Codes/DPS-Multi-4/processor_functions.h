#ifndef __PROCESSOR_FUNCTIONS_H_
#define __PROCESSOR_FUNCTIONS_H_

#include "data_structures.h"

/*
    Preconditions:
        Input: {void}
    
    Purpose of the function: It is used to initialize the processors and create the necessary number of cores.

    Postconditions:
        Output: {Pointer to the processor structure}
                processor!=NULL
*/
extern processor_struct *initialize_processor();

/*
    Preconditions:
        Input: {Pointer to taskset, pointer to processor}

    Purpose of the function: It is used to allocate the tasks to each core.
                             Priority will be given to high criticality tasks. Each core will be alloted high criticality tasks having a utilization of MAX_HIGH_UTIL (defined in data_structures.h)
                            
    Postconditions: 
        Output: If the number of cores is sufficient and all the tasks were allocated to the cores, then it will return 1 to indicate success.
                Else it will return 0.
*/
extern int allocate_tasks_to_cores(task_set_struct *task_set, processor_struct *processor, FILE *output_file);

#endif