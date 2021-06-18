#include "functions.h"

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Invalid parameters. Format for input is ./test INPUT_FILE OUTPUT_FILE\n");
        return 0;
    }

    FILE *statistics_file;

    //get_task_set function - takes input from input file. Pass file pointer to the function.
    task_set_struct *task_set = get_taskset();
    processor_struct *processor = initialize_processor();
    stats_struct* stats = initialize_stats_struct();

    if(task_set == NULL || processor == NULL || stats == NULL)
    {
        printf("Runtime error\n");
        return 0;
    }

    //Set the frequency values.
    frequency[0] = 0.5;
    frequency[1] = 0.6;
    frequency[2] = 0.75;
    frequency[3] = 0.9;
    frequency[4] = 1.00;

    //Open the output file here.
    for (int i = 0; i < processor->total_cores; i++)
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

    statistics_file = fopen("statistics.txt", "w");
    fprintf(statistics_file, "STATISTICS FOR ALL CORES\n");
    for (int i = 0; i < NUM_CORES; i++)
    {
        fprintf(statistics_file, "Core %d\n", i);
        fprintf(statistics_file, "\
            Total active time: %.2lf\n\
            Total idle time: %.2lf\n\
            Total shutdown time: %.2lf\n\
            Total arrival points: %d\n\
            Total completion points: %d\n\
            Total wakeup points: %d\n\
            Total crit change points: %d\n\
            Total context switches: %d\n",
                stats->total_active_energy[i],
                stats->total_idle_energy[i],
                stats->total_shutdown_time[i],
                stats->total_arrival_points[i],
                stats->total_completion_points[i],
                stats->total_wakeup_points[i],
                stats->total_criticality_change_points[i],
                stats->total_context_switches[i]);
        fprintf(statistics_file, "\n");
    }

    fclose(statistics_file);
    for (int i = 0; i < NUM_CORES; i++)
    {
        fclose(output[i]);
    }
    fclose(output_file);
}