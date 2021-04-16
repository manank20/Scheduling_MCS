#include "data_structures.h"
#include "scheduler_functions.h"
#include "allocation_functions.h"

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Invalid parameters. Format for input is ./test INPUT_FILE OUTPUT_FILE\n");
        return 0;
    }

    FILE *task_file;
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
    for(int i=0; i<NUM_CORES; i++)
    {
        char filename[15] = "output_";
        char ext[2];
        sprintf(ext, "%d", i);
        strcat(filename, ext);
        strcat(filename, ".txt");

        output[i] = fopen(filename, "w");

        fprintf(output[i], "Schedule for core %d\n", i);
    }
    output_file = fopen(argv[2], "w");
    if (output_file == NULL)
    {
        printf("ERROR: Cannot open output file. Make sure right permissions are provided\n");
        return 0;
    }

    runtime_scheduler(task_set, processor);

    fclose(task_file);
    for(int i=0; i<NUM_CORES; i++)
    {
        fclose(output[i]);
    }
    fclose(output_file);
}