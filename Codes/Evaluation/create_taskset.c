#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#define MAX_CRITICALITY_LEVELS 4

int min(int a, int b)
{
    return (a < b) ? a : b;
} 

double max(double a, double b)
{
    return (a > b) ? a : b;
}

int randnum()
{
    int num = 12;
    int val = 0;
    for (int i = 0; i < num; i++)
        val |= (rand() & 1);
    return val;
}

double gcd(double a, double b)
{
    if (a == 0)
        return b;
    if (b == 0)
        return a;

    if (a == b)
        return a;

    if (a > b)
        return gcd(a - b, b);
    return gcd(a, b - a);
}

double find_actual_execution_time(double exec_time, int task_crit_lvl, int core_crit_lvl)
{
    double n = rand() % 3;
    if (task_crit_lvl <= core_crit_lvl)
    {
        exec_time = max(1.00, exec_time - n);
    }
    else
    {
        if (randnum() == 0)
        {
            exec_time += n;
        }
        else
        {
            exec_time = max(1.00, exec_time - n);
        }
    }

    return exec_time;
}

int main()
{
    FILE *mcs_file, *rts_file, *exec_file;
    mcs_file = fopen("input_mcs.txt", "w");
    rts_file = fopen("input_rts.txt", "w");
    exec_file = fopen("input_times.txt", "w");

    srand(time(NULL));
    int num_tasks = rand() % 10 + 10;

    double phase = 0.00;
    double hyperperiod = 1.00;

    fprintf(mcs_file, "%d\n", num_tasks);
    fprintf(rts_file, "%d\n", num_tasks);

    double period[num_tasks];
    double WCET[num_tasks][MAX_CRITICALITY_LEVELS];
    int crit_level[num_tasks];

    for (int i = 0; i < num_tasks; i++)
    {
        fprintf(mcs_file, "%.2lf ", phase);
        fprintf(rts_file, "%.2lf ", phase);

        period[i] = (double)((double)(rand() % 18) + 2) * 5;
        fprintf(mcs_file, "%.2lf ", period[i]);
        fprintf(rts_file, "%.2lf ", period[i]);

        crit_level[i] = rand() % MAX_CRITICALITY_LEVELS;
        fprintf(mcs_file, "%d ", crit_level[i]);
        fprintf(rts_file, "0 ");

        double min_wcet = period[i] * 0.10;
        double max_wcet = period[i] * 0.50;
        double increments = (double)(max_wcet - min_wcet) / (double)MAX_CRITICALITY_LEVELS;

        WCET[i][0] = min_wcet;
        fprintf(mcs_file, "%.2lf ", WCET[i][0]);

        for (int j = 1; j < MAX_CRITICALITY_LEVELS; j++)
        {
            WCET[i][j] = WCET[i][j - 1];
            if (j <= crit_level[i])
                WCET[i][j] += increments;
            fprintf(mcs_file, "%.2lf ", WCET[i][j]);
        }
        fprintf(rts_file, "%.2lf ", WCET[i][MAX_CRITICALITY_LEVELS-1]);
        
        fprintf(mcs_file, "\n");
        fprintf(rts_file, "\n");
    }

    for(int i=0; i<num_tasks; i++)
    {
        hyperperiod = (hyperperiod * period[i]) / gcd(hyperperiod, period[i]);
    }

    fclose(mcs_file);
    fclose(rts_file);

    for(int i=0; i<num_tasks; i++)
    {
        int num_jobs = (hyperperiod / period[i]);
        int curr_crit_level = 0;

        fprintf(exec_file, "%d ", num_jobs);

        for(int j=0; j<num_jobs; j++)
        {
            double exec_time = find_actual_execution_time(WCET[i][curr_crit_level], crit_level[i], curr_crit_level);
            fprintf(exec_file, "%.2lf ", exec_time);
            if(exec_time > WCET[i][curr_crit_level])
                curr_crit_level = min(curr_crit_level+1, MAX_CRITICALITY_LEVELS-1);
        }

        fprintf(exec_file, "\n");
    }

    fclose(exec_file);

}