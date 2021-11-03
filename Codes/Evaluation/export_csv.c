#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include "common/data_structures.h"

int main() {
    // FILE *fd;
    FILE *algo[4];
    FILE *cores, *energy, *discarded, *jobs, *shutdown, *static_file, *dynamic_file;

    cores = fopen("total_times.csv", "w");
    energy = fopen("total_energy.csv", "w");
    static_file = fopen("total_static_energy.csv", "w");
    dynamic_file = fopen("total_dynamic_energy.csv", "w");
    discarded = fopen("total_discarded_time.csv", "w");
    jobs = fopen("total_discarded_jobs.csv", "w");
    shutdown = fopen("total_shutdown_time.csv", "w");

    algo[0] = fopen("statistics_DPS.txt", "r");
    algo[1] = fopen("statistics_EDF-VD.txt", "r");
    algo[2] = fopen("statistics_EDF-VD-DJ.txt", "r");
    algo[3] = fopen("statistics_EDF.txt", "r");

    double total_at[4] = {0.00}, total_it[4] = {0.00}, total_st[4] = {0.00}, total_djt_exec[4] = {0.00}, total_djt_avail[4] = {0.00};
    int total_dj[4] = {0.00}, total_jobs[4] = {0.00};

    for(int i=0; i<5; i++)
    {
        double at, it, st, djt, djt_avail;
        int dj, jobs;

        char dirname[30] = "../final6/sample_taskset_";
        char ext[2];
        sprintf(ext, "%d", i+1);
        strcat(dirname, ext);

        char file1[50];
        strcpy(file1, dirname);
        strcat(file1, "/statistics_DPS.txt");
        algo[0] = fopen(file1, "r");

        char file2[50];
        strcpy(file2, dirname);
        strcat(file2, "/statistics_EDF-VD.txt");
        algo[1] = fopen(file2, "r");

        char file3[60];
        strcpy(file3, dirname);
        strcat(file3, "/statistics_EDF-VD-DJ.txt");
        algo[2] = fopen(file3, "r");

        char file4[50];
        strcpy(file4, dirname);
        strcat(file4, "/statistics_EDF.txt");
        algo[3] = fopen(file4, "r");

        for(int j=0; j<4; j++)
        {
            for(int k=0; k<NUM_CORES; k++){
                fscanf(algo[j], "%lf%lf%lf%d%d%lf%lf", &at, &it, &st, &dj, &jobs, &djt, &djt_avail);
                printf("%.2lf %.2lf %.2lf %d %d %.2lf\n", at, it, st, dj, jobs, djt);

                total_at[j] += at;
                total_it[j] += it;
                total_st[j] += st;
                total_dj[j] += dj;
                total_jobs[j] += jobs;
                total_djt_exec[j] += djt;
                total_djt_avail[j] += djt_avail;

            }
            printf("\n");
        }
        printf("--------\n");

    }

    // for(int i=0; i<NUM_CORES; i++)
    // {
    //     char filename[15] = "core_";
    //     char ext[2];
    //     sprintf(ext, "%d", i);
    //     strcat(filename, ext);
    //     strcat(filename, ".csv");

    //     fd = fopen(filename, "w");

    //     fprintf(fd, "active time,idle time,shutdown time,discarded jobs\n");

    //     double at, it, st, djt;
    //     int dj;

    //     for(int j=0; j<4; j++)
    //     {
    //         fscanf(algo[j], "%lf%lf%lf%d%lf", &at, &it, &st, &dj, &djt);
    //         fprintf(fd, "%.2lf,%.2lf,%.2lf,%d\n", at, it, st, dj);

    //         total_at[j] += at;
    //         total_it[j] += it;
    //         total_st[j] += st;
    //         total_dj[j] += dj;
    //         total_djt[i] += djt;
    //     }
    // }

    // fprintf(cores, "active time,idle time,shutdown time,discarded jobs\n");
    // for(int i=0; i<4; i++)
    // {
    //     fprintf(cores, "%.2lf,%.2lf,%.2lf,%d\n", total_at[i], total_it[i], total_st[i], total_dj[i]);        
    // }

    double static_energy[4] = {0.00};
    double dynamic_energy[4] = {0.00};
    double total_energy[4] = {0.00};
    for(int i=0; i<4; i++)
    {
        dynamic_energy[i] += (total_at[i] * 1.00) + (total_it[i] * 0.5);
        // total_energy[i] /= (double)5;
        dynamic_energy[i] /= total_at[i];
     
        static_energy[i] += (total_at[i] + total_it[i]) * 0.2 / total_at[i];

        total_energy[i] = dynamic_energy[i] + static_energy[i];

        printf("AT: %.2lf, IT: %.2lf, ST: %.2lf, Energy: %.2lf, jobs: %.2lf, total: %.2lf\n", total_at[i], total_it[i], total_st[i], total_energy[i], total_djt_exec[i], total_djt_avail[i]);
     
        fprintf(static_file, "%.2lf\n", static_energy[i]);
        fprintf(dynamic_file, "%.2lf\n", dynamic_energy[i]);
        fprintf(energy, "%.2lf\n", total_energy[i]);
        fprintf(jobs, "%.2lf\n", ((double)total_djt_exec[i] / (double)total_djt_avail[i]));
        fprintf(discarded, "%.2lf\n", total_djt_exec[i]);
        fprintf(shutdown, "%.2lf\n", total_st[i]);
    }


    // fclose(fd);
    for(int i=0; i<4; i++)
    {
        fclose(algo[i]);
    }
    fclose(cores);
    fclose(energy);
    fclose(static_file);
    fclose(dynamic_file);
    fclose(discarded);
    fclose(jobs);
}