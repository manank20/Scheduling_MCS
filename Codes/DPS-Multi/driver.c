#include "data_structures.h"
#include "scheduler_functions.h"
#include "processor_functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Invalid parameters. Format for input is ./test INPUT_FILE OUTPUT_FILE\n");
        return 0;
    }

    FILE *task_file, *output_file;
    task_file = fopen(argv[1], "r");

    if (task_file == NULL)
    {
        printf("ERROR: Cannot open input file. Format of execution is ./test input.txt\n");
        return 0;
    }

    //get_task_set function - takes input from input file. Pass file pointer to the function.
    task_set_struct *task_set = get_taskset(task_file);
    processor_struct *processor = initialize_processor();

    //Open the output file here.
    output_file = fopen(argv[2], "w");
    if (output_file == NULL)
    {
        printf("ERROR: Cannot open output file. Make sure right permissions are provided\n");
        return 0;
    }

    runtime_scheduler(task_set, processor, output_file);

    fclose(task_file);
    fclose(output_file);
}