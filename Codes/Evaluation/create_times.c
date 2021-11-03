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
    int num = 13;
    int val = 0;
    for (int i = 0; i < num; i++)
        val |= (rand() & 1);
    return val;
}

double find_actual_execution_time(double exec_time, int task_crit_lvl, int core_crit_lvl)
{
    // double n = rand() % 3;
    if (task_crit_lvl <= core_crit_lvl)
    {
        exec_time = max(1.00, exec_time - 1);
    }
    else
    {
        if (randnum() == 0)
        {
            exec_time += 1;
        }
        else
        {
            exec_time = max(1.00, exec_time - 1);
        }
    }

    return exec_time;
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

int main() {
    FILE* fd = fopen("input_mcs.txt", "r");
    FILE* exec_file = fopen("input_times.txt", "w");

    srand(time(NULL));

    int num_tasks;
    fscanf(fd, "%d", &num_tasks);

    double phase[num_tasks], period[num_tasks], WCET[num_tasks][MAX_CRITICALITY_LEVELS];
    double *exec_times[num_tasks];
    int crit_level[num_tasks], num_jobs[num_tasks];
    int max_jobs = 0;

    for(int i=0; i<num_tasks; i++)
    {
        fscanf(fd, "%lf%lf%d", &phase[i], &period[i], &crit_level[i]);
        for(int j=0; j<MAX_CRITICALITY_LEVELS; j++)
        {
            fscanf(fd, "%lf", &WCET[i][j]);
        }
    }

    double hyperperiod = 1;
    for(int i=0; i<num_tasks; i++)
    {
        hyperperiod = (hyperperiod * period[i]) / gcd(hyperperiod, period[i]);
    }

    for(int i=0; i<num_tasks; i++)
    {
        num_jobs[i] = hyperperiod / period[i];
        exec_times[i] = malloc(sizeof(double) * num_jobs[i]);
        max_jobs = (max_jobs > num_jobs[i] ? max_jobs : num_jobs[i]);
    }

    int curr_crit_level = 0;
    for(int i=0; i<max_jobs; i++)
    {
        int crit_change = 0;
        for(int j=0; j<num_tasks; j++)
        {
            if(i < num_jobs[j]) {
                double exec_time = find_actual_execution_time(WCET[j][curr_crit_level], crit_level[j], curr_crit_level);
                exec_times[j][i] = exec_time;
                if(exec_time > WCET[j][curr_crit_level] && crit_change == 0){
                    curr_crit_level = min(curr_crit_level+1, MAX_CRITICALITY_LEVELS-1);
                    crit_change = 1;
                }
            }
        }

    }
    
    for(int i=0; i<num_tasks; i++)
    {
        fprintf(exec_file, "%d ", num_jobs[i]);
        for(int j=0; j<num_jobs[i]; j++)
        {
            fprintf(exec_file, "%.2lf ", exec_times[i][j]);
        }
        fprintf(exec_file, "\n");
    }

    fclose(exec_file);

}