#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#define MAX_CRITICALITY_LEVELS 4

int main(){
    srand(time(NULL));
    int num_tasks = rand()%10 + 10;

    double phase = 0.00, period, WCET;
    int crit_level;

    FILE* fd = fopen("input.txt", "w");
    fprintf(fd, "%d\n", num_tasks);
    for(int i=0; i<num_tasks; i++) 
    {
        fprintf(fd, "%.2lf ", phase);

        period = (double)((double)(rand()%18) + 2)*10;
        fprintf(fd, "%.2lf ", period);

        crit_level = rand()%MAX_CRITICALITY_LEVELS;
        fprintf(fd, "%d ", crit_level);
        
        double min_wcet = period*0.10;
        double max_wcet = period*0.25;
        double increments = (double)(max_wcet - min_wcet) / (double)MAX_CRITICALITY_LEVELS;
        WCET = min_wcet;    
        
        for(int j=0; j<MAX_CRITICALITY_LEVELS; j++) {
            fprintf(fd, "%.2lf ", WCET);
            if(j < crit_level)
                WCET += increments;
        }
        fprintf(fd, "\n");
    }
}