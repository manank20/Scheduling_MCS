#include "functions.h"

void select_frequency(core_struct *core, task_set_struct *task_set, int curr_crit_level, int core_no)
{
    int is_discarded_job = 0;
    job *temp = core->ready_queue->job_list_head;
    while (temp != NULL)
    {
        if (task_set->task_list[temp->task_number].criticality_lvl < curr_crit_level)
        {
            is_discarded_job = 1;
            break;
        }
        temp = temp->next;
    }

    if (is_discarded_job == 1)
    {
        fprintf(output[core_no], "Discarded job present. Setting frequency as MAX\n");
        (*core).frequency = 1.00;
        return;
    }

    double curr_time = (*core).total_time;
    double deadline = (*core).ready_queue->job_list_head->absolute_deadline;
    int threshold_crit_lvl = (*core).threshold_crit_lvl;
    fprintf(output[core_no], "Curr_job %d,%d. Deadline %.5lf, Curr time: %.5lf\n", (*core).ready_queue->job_list_head->task_number, (*core).ready_queue->job_list_head->job_number, deadline, (*core).total_time);

    double U_dyn[MAX_CRITICALITY_LEVELS];
    for (int i = 0; i < MAX_CRITICALITY_LEVELS; i++)
        U_dyn[i] = 0.00;

    //First traverse the ready queue and update the utilisation.
    temp = core->ready_queue->job_list_head;
    while (temp != NULL)
    {
        int task_number = temp->task_number;
        int crit_level = task_set->task_list[task_number].criticality_lvl;

        fprintf(output[core_no], "Job %d,%d.", temp->task_number, temp->job_number);
        for (int i = curr_crit_level; i <= crit_level; i++)
        {
            double absolute_deadline = temp->absolute_deadline;
            double rem_WCET_time = task_set->task_list[task_number].WCET[i] - (temp->execution_time - temp->rem_exec_time);

            if (i > threshold_crit_lvl)
            {
                absolute_deadline -= task_set->task_list[task_number].virtual_deadline;
                absolute_deadline += task_set->task_list[task_number].relative_deadline;
            }
            double util = rem_WCET_time / (absolute_deadline - curr_time);
            fprintf(output[core_no], "Deadline: %.5lf Rem WCET: %.5lf, util: %.5lf,    ", absolute_deadline, rem_WCET_time, util);
            U_dyn[i] += util;
        }
        fprintf(output[core_no], "\n");
        temp = temp->next;
    }

    //Now traverse the taskset and check for jobs arriving before current deadline.
    for (int i = 0; i < task_set->total_tasks; i++)
    {
        if (task_set->task_list[i].core == core_no && task_set->task_list[i].criticality_lvl >= curr_crit_level)
        {
            int curr_jobs = task_set->task_list[i].job_number;
            int crit_level = task_set->task_list[i].criticality_lvl;
            double release_time;

            while ((release_time = task_set->task_list[i].phase + task_set->task_list[i].period * curr_jobs) < deadline)
            {
                fprintf(output[core_no], "Task %d. Release time: %.5lf Util: ", i, release_time);

                for (int j = curr_crit_level; j <= crit_level; j++)
                {
                    double task_deadline, util;
                    if (j > threshold_crit_lvl)
                    {
                        task_deadline = release_time + task_set->task_list[i].relative_deadline;
                    }
                    else
                    {
                        task_deadline = release_time + task_set->task_list[i].virtual_deadline;
                    }

                    if (task_deadline > deadline)
                    {
                        util = task_set->task_list[i].util[j] * (deadline - release_time) / task_set->task_list[i].period;
                    }
                    else
                    {
                        util = task_set->task_list[i].util[j];
                    }

                    fprintf(output[core_no], "%.5lf ", util);
                    U_dyn[j] += util;
                }
                fprintf(output[core_no], "\n");
                curr_jobs++;
            }
        }
    }

    fprintf(output[core_no], "U_dyn from taskset:\n");
    for (int i = curr_crit_level; i < MAX_CRITICALITY_LEVELS; i++)
    {
        fprintf(output[core_no], "%.5lf ", U_dyn[i]);
    }
    fprintf(output[core_no], "\n");

    double selected_frequency = frequency[FREQUENCY_LEVELS - 1];
    fprintf(output[core_no], "Frequency calculation:\n");
    for (int i = 0; i < FREQUENCY_LEVELS; i++)
    {
        int flag = 1;
        fprintf(output[core_no], "Frequency: %.5lf\n", frequency[i]);
        for (int l = curr_crit_level; l < MAX_CRITICALITY_LEVELS; l++)
        {
            fprintf(output[core_no], "Level %d, Util: %.5lf\n", l, U_dyn[l]);

            if (U_dyn[l] > frequency[i])
            {
                flag = 0;
                break;
            }
        }

        if (flag == 1)
        {
            selected_frequency = frequency[i];
            break;
        }
    }

    fprintf(output[core_no], "Frequency selected: %.5lf\n", selected_frequency);
    (*core).frequency = selected_frequency;
}