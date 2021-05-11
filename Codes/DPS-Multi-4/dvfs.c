#include "functions.h"

void set_execution_times(core_struct* core, int core_no)
{
    job* temp = core->ready_queue->job_list_head;
    while(temp!=NULL)
    {
        temp->execution_time /= (*core).frequency;
        temp->rem_exec_time /= (*core).frequency;
        temp->WCET_counter /= (*core).frequency;
        temp = temp->next;
    }
}

void reset_execution_times(core_struct* core)
{
    job* temp = core->ready_queue->job_list_head;
    while(temp != NULL)
    {
        temp->execution_time *= core->frequency;
        temp->rem_exec_time *= core->frequency;
        temp->WCET_counter *= core->frequency;
        temp = temp->next;
    }
}

void select_frequency(core_struct* core, task_set_struct* task_set, int curr_crit_level, int core_no)
{
    reset_execution_times(core);

    int is_discarded_job = 0;
    job* temp = core->ready_queue->job_list_head;
    while(temp != NULL)
    {
        if(task_set->task_list[temp->task_number].criticality_lvl < curr_crit_level)
        {
            is_discarded_job = 1;
            break;
        }
        temp = temp->next;
    }

    if(is_discarded_job == 1)
    {
        printf("Discarded job present. Setting frequency as MAX\n");
        (*core).frequency = 1.00;
        return;
    }
    
    double curr_time = (*core).total_time;
    double deadline = (*core).ready_queue->job_list_head->absolute_deadline;
    printf("Curr_job %d,%d. Deadline %.2lf, Curr time: %.2lf\n", (*core).ready_queue->job_list_head->task_number, (*core).ready_queue->job_list_head->job_number, deadline, (*core).total_time);

    double U_dyn[MAX_CRITICALITY_LEVELS];
    for(int i=0; i<MAX_CRITICALITY_LEVELS; i++)
        U_dyn[i] = 0.00;

    //First traverse the ready queue and update the utilisation.
    temp = core->ready_queue->job_list_head;
    while(temp != NULL)
    {
        int task_number = temp->task_number;
        int crit_level = task_set->task_list[task_number].criticality_lvl;
        
        printf("Job %d,%d.", temp->task_number, temp->job_number);
        for(int i=curr_crit_level; i<=crit_level; i++)
        {
            double absolute_deadline = temp->absolute_deadline;
            double rem_WCET_time = task_set->task_list[task_number].WCET[i] - (temp->execution_time - temp->rem_exec_time);

            if(i > (*core).threshold_crit_lvl){
                absolute_deadline -= task_set->task_list[task_number].virtual_deadline;
                absolute_deadline += task_set->task_list[task_number].relative_deadline;
            }
            double util = rem_WCET_time / (absolute_deadline - curr_time);
            printf("Deadline: %.2lf Rem WCET: %.2lf, util: %.2lf,    ", absolute_deadline, rem_WCET_time, util);
            U_dyn[i] += util;
        }
        printf("\n");
        temp = temp->next;
    }

    printf("U_dyn from ready queue:\n");
    for(int i=curr_crit_level; i<MAX_CRITICALITY_LEVELS; i++){
        printf("%.2lf ", U_dyn[i]);
    }
    printf("\n");

    for(int i=0; i<task_set->total_tasks; i++)
    {
        if(task_set->task_list[i].core == core_no && task_set->task_list[i].criticality_lvl >= curr_crit_level)
        {
            int curr_jobs = task_set->task_list[i].job_number;
            int crit_level = task_set->task_list[i].criticality_lvl;
            double release_time;

            while((release_time = task_set->task_list[i].phase + task_set->task_list[i].period*curr_jobs) < deadline)
            {
                printf("Task %d. Release time: %.2lf Util: ", i, release_time);
                double task_deadline = release_time + task_set->task_list[i].virtual_deadline;
                double util;
                if(task_deadline > deadline)
                {
                    util = task_set->task_list[i].util[curr_crit_level] * (deadline - release_time) / task_set->task_list[i].period;
                }
                else{
                    util = task_set->task_list[i].util[curr_crit_level];
                }
                U_dyn[curr_crit_level] += util;
                printf("%.2lf ", util);

                for(int j=curr_crit_level+1; j<=crit_level; j++)
                {
                    printf("%.2lf ", task_set->task_list[i].util[j]);
                    U_dyn[j] += task_set->task_list[i].util[j];
                }
                printf("\n");
                curr_jobs++;
            }
        }
    }

    printf("U_dyn from taskset:\n");
    for(int i=curr_crit_level; i<MAX_CRITICALITY_LEVELS; i++){
        printf("%.2lf ", U_dyn[i]);
    }
    printf("\n");

    double selected_frequency = frequency[FREQUENCY_LEVELS - 1];
    printf("Frequency calculation:\n");
    for(int i=0; i<FREQUENCY_LEVELS; i++)
    {
        int flag = 1;
        printf("Frequency: %.2lf\n", frequency[i]);
        for(int l=curr_crit_level; l<MAX_CRITICALITY_LEVELS; l++)
        {
            printf("Level %d, Util: %.2lf\n", l, U_dyn[l]);

            if(U_dyn[l] > frequency[i])
            {
                flag = 0;
                break;
            }
        }

        if(flag == 1){
            selected_frequency = frequency[i];
            break;
        }
    }

    printf("Frequency selected: %.2lf\n", selected_frequency);
    (*core).frequency = selected_frequency;
    set_execution_times(core, core_no);

}