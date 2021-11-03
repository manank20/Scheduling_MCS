#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#define MAX_CRITICALITY_LEVELS 4

int main()
{
    FILE *mcs_file, *rts_file;
    mcs_file = fopen("input_mcs.txt", "w");
    rts_file = fopen("input_rts.txt", "w");

    srand(time(NULL));
    int num_tasks = 12;

    double phase = 0.00;

    fprintf(mcs_file, "%d\n", num_tasks);
    fprintf(rts_file, "%d\n", num_tasks);

    double period[num_tasks];
    double WCET[num_tasks][MAX_CRITICALITY_LEVELS];
    int crit_level[num_tasks];

    double total_util[MAX_CRITICALITY_LEVELS] = {0.00};

    for (int i = 0; i < num_tasks; i++)
    {
        fprintf(mcs_file, "%.2lf ", phase);
        fprintf(rts_file, "%.2lf ", phase);

        period[i] = (double)((double)(rand() % 11) + 5) * 50;
        fprintf(mcs_file, "%.2lf ", period[i]);
        fprintf(rts_file, "%.2lf ", period[i]);

        crit_level[i] = i%4;
        fprintf(mcs_file, "%d ", crit_level[i]);
        fprintf(rts_file, "0 ");

        double min_wcet = period[i] * (0.22);
        double max_wcet = period[i] * 0.60;
        double increments = 5;

        WCET[i][0] = min_wcet;
        total_util[0] += WCET[i][0] / period[i];

        fprintf(mcs_file, "%.2lf ", WCET[i][0]);

        for (int j = 1; j < MAX_CRITICALITY_LEVELS; j++)
        {
            WCET[i][j] = WCET[i][j - 1];
            if (j <= crit_level[i])
                WCET[i][j] += increments;
            fprintf(mcs_file, "%.2lf ", WCET[i][j]);
            total_util[j] += WCET[i][j] / period[i];
        }
        fprintf(rts_file, "%.2lf ", WCET[i][MAX_CRITICALITY_LEVELS-1]);
        
        fprintf(mcs_file, "\n");
        fprintf(rts_file, "\n");
    }

    fclose(mcs_file);
    fclose(rts_file);

    printf("Total util: ");
    for(int i=0; i<MAX_CRITICALITY_LEVELS; i++)
    {
        printf("%.2lf ", total_util[i]);
    }
    printf("\n");

}