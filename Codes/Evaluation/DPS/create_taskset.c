#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#define MAX_CRITICALITY_LEVELS 4

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

int main()
{
    srand(time(NULL));
    int num_tasks = rand() % 10 + 10;

    double phase = 0.00, hyperperiod = 1.00;
    int crit_level;

    FILE *fd = fopen("input.txt", "w");
    fprintf(fd, "%d\n", num_tasks);
    double period[num_tasks];
    double WCET[num_tasks][MAX_CRITICALITY_LEVELS];

    for (int i = 0; i < num_tasks; i++)
    {
        fprintf(fd, "%.2lf ", phase);

        period[i] = (double)((double)(rand() % 18) + 2) * 5;
        fprintf(fd, "%.2lf ", period[i]);

        crit_level = rand() % MAX_CRITICALITY_LEVELS;
        fprintf(fd, "%d ", crit_level);

        double min_wcet = period[i] * 0.10;
        double max_wcet = period[i] * 0.50;
        double increments = (double)(max_wcet - min_wcet) / (double)MAX_CRITICALITY_LEVELS;

        WCET[i][0] = min_wcet;
        fprintf(fd, "%.2lf ", WCET[i][0]);

        for (int j = 1; j < MAX_CRITICALITY_LEVELS; j++)
        {
            WCET[i][j] = WCET[i][j - 1];
            if (j <= crit_level)
                WCET[i][j] += increments;
            fprintf(fd, "%.2lf ", WCET[i][j]);
        }
        fprintf(fd, "\n");
    }

    fclose(fd);
    // for(int i=0; i<num_tasks; i++)
    // {
    //     int job_number = 0;
    //     while()
    // }
}