#include "functions.h"

double find_total_exec_time(job* ready_queue, task_set_struct* task_set, int core_no, int curr_crit_level, int threshold_crit_level, double curr_time)
{
    job* temp = ready_queue;
    double exec_time = 0.00;
    double WCET_times[MAX_CRITICALITY_LEVELS] = {0.00};
    int earliest_task = ready_queue->task_number;

    int max_crit_level = task_set->task_list[ready_queue->task_number].criticality_lvl;

    fprintf(output[core_no], "Ready queue: \n");
    while (temp != NULL)
    {
        int task_number = temp->task_number;
        int crit_level = max_int(min_int(task_set->task_list[task_number].criticality_lvl, max_crit_level), curr_crit_level);

        for(int k=curr_crit_level; k<=crit_level; k++)
        {
            double job_deadline = temp->absolute_deadline;
            double earliest_deadline;
            double rem_WCET_time, relative_deadline;
            
            earliest_deadline = ready_queue->absolute_deadline;

            rem_WCET_time = task_set->task_list[task_number].WCET[k] - (temp->execution_time - temp->rem_exec_time);

            if(k > threshold_crit_level)
            {
                relative_deadline = task_set->task_list[task_number].relative_deadline;

                earliest_deadline -= task_set->task_list[earliest_task].virtual_deadline;
                earliest_deadline += task_set->task_list[earliest_task].relative_deadline;

                job_deadline -= task_set->task_list[task_number].virtual_deadline;
                job_deadline += task_set->task_list[task_number].relative_deadline;

            }
            else
            {
                relative_deadline = task_set->task_list[task_number].virtual_deadline;
            }

            if(job_deadline > earliest_deadline)
            {
                rem_WCET_time *= (earliest_deadline - curr_time) / (relative_deadline);
            }
            fprintf(output[core_no], "Job %d,%d. K: %d, WCET: %.2lf, Deadline: %.2lf\n", temp->task_number, temp->job_number, k, rem_WCET_time, job_deadline);
            WCET_times[k] += rem_WCET_time;

        }
        // exec_time += rem_WCET_time;
        

        temp = temp->next;
    }

    for (int i = 0; i < task_set->total_tasks; i++)
    {
        if (task_set->task_list[i].core == core_no && task_set->task_list[i].criticality_lvl >= curr_crit_level)
        {
            int crit_level = min_int(task_set->task_list[i].criticality_lvl, max_crit_level);
            
            for(int k=curr_crit_level; k<=crit_level; k++){
                int curr_jobs = task_set->task_list[i].job_number;
                double earliest_deadline = ready_queue->absolute_deadline;
                if(k > threshold_crit_level)
                {
                    earliest_deadline -= task_set->task_list[ready_queue->task_number].virtual_deadline;
                    earliest_deadline += task_set->task_list[ready_queue->task_number].relative_deadline;
                }

                double release_time;

                while ((release_time = task_set->task_list[i].phase + task_set->task_list[i].period * curr_jobs) < earliest_deadline)
                {
                    fprintf(output[core_no], "Task %d. Release time: %.5lf Util: ", i, release_time);
                    double WCET = task_set->task_list[i].WCET[k];
                    double task_deadline = release_time + ((k > threshold_crit_level) ? (task_set->task_list[i].relative_deadline) : (task_set->task_list[i].virtual_deadline));
                    if(task_deadline > earliest_deadline)
                    {
                        WCET *= (earliest_deadline - release_time) / task_deadline; 
                    }
                    // exec_time += WCET;
                    WCET_times[k] += WCET;

                    fprintf(output[core_no], "Task %d. K: %d, Release time: %.2lf, Deadline: %.2lf, WCET: %.2lf\n", i, k, release_time, task_deadline, WCET);
                    
                    curr_jobs++;
                }
            }
        }
    }

    fprintf(output[core_no], "WCET array:\n");
    double selected_frequency = 0.00;

    for(int k=0; k<MAX_CRITICALITY_LEVELS; k++)
    {
        double freq = frequency[FREQUENCY_LEVELS - 1];

        double earliest_deadline;
        earliest_deadline = ready_queue->absolute_deadline;
        if(k > threshold_crit_level)
        {
            earliest_deadline -= task_set->task_list[earliest_task].virtual_deadline;
            earliest_deadline += task_set->task_list[earliest_task].relative_deadline;
        }

        double max_slack = earliest_deadline - curr_time;
        double freq_ratio = WCET_times[k] / max_slack;

        for(int i=0; i<FREQUENCY_LEVELS; i++)
        {
            if(frequency[i] > freq_ratio)
            {
                freq = frequency[i];
                break;
            }
        }

        selected_frequency = max(selected_frequency, freq);
    
        fprintf(output[core_no], "K: %d, Exec time: %.2lf, Total slack: %.2lf, %s\n", k, WCET_times[k], max_slack, (exec_time > max_slack ? "yes" : "no"));
        fprintf(output[core_no], "Frequency selected: %.5lf\n", selected_frequency);
    }
    
    return selected_frequency;

}

void select_frequency(core_struct *core, task_set_struct *task_set, int curr_crit_level, int core_no)
{
    fprintf(output[core_no], "FREQUENCY CALCULATIONS:\n");

    double curr_time = (*core).total_time;
    job* earliest_job = (*core).ready_queue->job_list_head;
    double earliest_deadline = earliest_job->absolute_deadline;
    int earliest_task = earliest_job->task_number;
    int max_crit_level = task_set->task_list[earliest_task].criticality_lvl;

    int threshold_crit_lvl = (*core).threshold_crit_lvl;

    fprintf(output[core_no], "Curr_job %d,%d. Deadline %.5lf, Curr time: %.5lf\n", (*core).ready_queue->job_list_head->task_number, (*core).ready_queue->job_list_head->job_number, earliest_deadline, (*core).total_time);

    double U_dyn[MAX_CRITICALITY_LEVELS] = {0.00};

    //First traverse the ready queue and update the utilisation.
    job *temp = core->ready_queue->job_list_head;
    while (temp != NULL)
    {
        int task_number = temp->task_number;
        int crit_level = max_int(min_int(task_set->task_list[task_number].criticality_lvl, max_crit_level), curr_crit_level);

        fprintf(output[core_no], "Job %d,%d. Crit level: %d ", temp->task_number, temp->job_number, crit_level);
        for (int i = curr_crit_level; i <= crit_level; i++)
        {
            earliest_deadline = earliest_job->absolute_deadline;
            double absolute_deadline = temp->absolute_deadline;
            double relative_deadline;

            double rem_WCET_time = task_set->task_list[task_number].WCET[i] - (temp->execution_time - temp->rem_exec_time);

            if (i > threshold_crit_lvl)
            {
                absolute_deadline -= task_set->task_list[task_number].virtual_deadline;
                absolute_deadline += task_set->task_list[task_number].relative_deadline;
                relative_deadline = task_set->task_list[task_number].relative_deadline;

                earliest_deadline -= task_set->task_list[earliest_task].virtual_deadline;
                earliest_deadline += task_set->task_list[earliest_task].relative_deadline;
            }
            else
            {
                relative_deadline = task_set->task_list[task_number].virtual_deadline;
            }

            double util;
            // if(absolute_deadline > earliest_deadline)
            // {
            //     util = rem_WCET_time / relative_deadline;
            // }
            // else
            // {
                util = rem_WCET_time / (absolute_deadline - curr_time);
            // }

            fprintf(output[core_no], "Deadline: %.5lf Rem WCET: %.5lf, util: %.5lf,    ", absolute_deadline, rem_WCET_time, util);
            U_dyn[i] += util;
        }
        fprintf(output[core_no], "\n");
        temp = temp->next;
    }

    //Now traverse the taskset and check for jobs arriving before current earliest_deadline.
    for (int i = 0; i < task_set->total_tasks; i++)
    {
        if (task_set->task_list[i].core == core_no && task_set->task_list[i].criticality_lvl >= curr_crit_level)
        {
            int crit_level = max_int(min_int(task_set->task_list[i].criticality_lvl, max_crit_level), curr_crit_level);
            for (int j = curr_crit_level; j <= crit_level; j++)
            {
                double release_time;
                int curr_jobs = task_set->task_list[i].job_number;
                earliest_deadline = earliest_job->absolute_deadline;

                if(j > threshold_crit_lvl)
                {
                    earliest_deadline -= task_set->task_list[earliest_task].virtual_deadline;
                    earliest_deadline += task_set->task_list[earliest_task].relative_deadline;
                }

                while ((release_time = task_set->task_list[i].phase + task_set->task_list[i].period * curr_jobs) < earliest_deadline)
                {
                    fprintf(output[core_no], "Task: %d, Crit level: %d. ", i, crit_level);
                    fprintf(output[core_no], "K: %d Release time: %.5lf ", j, release_time);

                    double task_deadline, util, relative_deadline;

                    relative_deadline = (j > threshold_crit_lvl) ? task_set->task_list[i].relative_deadline : task_set->task_list[i].virtual_deadline;
                    task_deadline = release_time + relative_deadline;

                    util = task_set->task_list[i].AET[j] / (earliest_deadline - curr_time);

                    if (task_deadline > earliest_deadline)
                    {
                        util *= ((earliest_deadline - release_time) / relative_deadline);
                    }
                    fprintf(output[core_no], "Util %.5lf\n", util);

                    U_dyn[j] += util;
                    curr_jobs++;
                }
            }
        }
    }


    fprintf(output[core_no], "U_dyn:\n");
    for (int i = curr_crit_level; i < MAX_CRITICALITY_LEVELS; i++)
    {
        fprintf(output[core_no], "%lf: %s ", U_dyn[i], (U_dyn[i] <= (double)1 ? "no": "yes"));
    }
    fprintf(output[core_no], "\n");

    //Select frequency such that U_dyn[i] <= fi/fmax for all i. 
    double selected_frequency = frequency[FREQUENCY_LEVELS - 1];
    for (int i = 0; i < FREQUENCY_LEVELS; i++)
    {
        int flag = 1;
        for (int l = curr_crit_level; l < MAX_CRITICALITY_LEVELS; l++)
        {
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
    // (*core).frequency = find_total_exec_time((*core).ready_queue->job_list_head, task_set, core_no, curr_crit_level, threshold_crit_lvl, curr_time);
    (*core).frequency = selected_frequency;
}

int present_in_ready_queue(int task_number, job* ready_queue)
{
    job* temp = ready_queue;
    while(temp != NULL)
    {
        if(temp->task_number == task_number)
            return 1;
        temp = temp->next;
    }
    return 0;
}

int update_task_set(int task_number, job* ready_queue, task_set_struct* task_set, task* new_task, la_edf_struct* temp_task_set, int crit_level, int curr_crit_level, int threshold_crit_level, int core_no)
{
    double deadline = 0.00, rem_exec_time = 0.00;
    int flag = 0;
    if(crit_level == curr_crit_level && present_in_ready_queue(task_number, ready_queue) == 1)
    {
        fprintf(output[core_no], "In RQ | ");
        job* temp = ready_queue;
        while(temp != NULL)
        {   
            if(temp->task_number == task_number)
            {
                deadline = temp->absolute_deadline;

                if(crit_level > threshold_crit_level)
                {
                    deadline -= (task_set->task_list[task_number].virtual_deadline);
                    deadline += (task_set->task_list[task_number].relative_deadline);
                }

                rem_exec_time = task_set->task_list[task_number].WCET[crit_level] - (temp->execution_time - temp->rem_exec_time);
                flag = 1;
                break;
            }
            temp = temp->next;
        }
    }
    else if(new_task->core == core_no && new_task->criticality_lvl >= crit_level)
    {
        fprintf(output[core_no], "In taskset | ");
        deadline = new_task->phase + new_task->period * new_task->job_number + (crit_level > threshold_crit_level ? new_task->relative_deadline : new_task->virtual_deadline);
        rem_exec_time = new_task->AET[crit_level];
        flag = 1;
    }

    if(flag == 1){
        fprintf(output[core_no], "Task: %d, Deadline: %.2lf, Exec time: %.2lf\n", task_number, deadline, rem_exec_time);
        (*temp_task_set).deadline = deadline;
        (*temp_task_set).exec_time = rem_exec_time;
        (*temp_task_set).task_number = task_number;
    }
    return flag;
}

/*Custom comparator for sorting the task list*/
int deadline_comparator(const void *p, const void *q)
{
    double l = ((la_edf_struct *)p)->deadline;
    double r = ((la_edf_struct *)q)->deadline;

    return (r - l);
}

void la_edf(core_struct* core, task_set_struct* task_set, int curr_crit_level, int core_no)
{
    job* earliest_job = (*core).ready_queue->job_list_head;
    int max_crit_level = max_int(task_set->task_list[earliest_job->task_number].criticality_lvl, curr_crit_level);
    int threshold_crit_level = (*core).threshold_crit_lvl;
    int i;

    fprintf(output[core_no], "-------------FREQUENCY CALCULATIONS:-------------\n");

    double selected_frequency = frequency[0];
    print_job_list(core_no, (*core).ready_queue->job_list_head);
    for(int crit_level=curr_crit_level; crit_level<=max_crit_level; crit_level++)
    {
        fprintf(output[core_no], "------------------Crit Level: %d------------------\n", crit_level);
        double utilisation = 0.00;
        double discarded_job_exec_time = 0.00;
        int flag = 0;
        la_edf_struct temp_task_set[task_set->total_tasks];
        double earliest_deadline = INT_MAX, latest_deadline = INT_MIN;

        for(i=0; i<task_set->total_tasks; i++)
        {
            temp_task_set[i].task_number = -1;
            temp_task_set[i].deadline = 0.00;

            // if((task_set->task_list[i].criticality_lvl < curr_crit_level && crit_level == curr_crit_level) || (task_set->task_list[i].core == core_no && task_set->task_list[i].criticality_lvl >= crit_level))
            // {
                int flag = update_task_set(i, (*core).ready_queue->job_list_head, task_set, &(task_set->task_list[i]), &(temp_task_set[i]), crit_level, curr_crit_level, threshold_crit_level, core_no);
            
                if(flag == 1){
                    earliest_deadline = min(earliest_deadline, temp_task_set[i].deadline);
                    latest_deadline = max(latest_deadline, temp_task_set[i].deadline);
                }
            // }
        }

        qsort((void*)temp_task_set, task_set->total_tasks, sizeof(temp_task_set[0]), deadline_comparator);

        fprintf(output[core_no], "Earliest deadline: %.2lf, Latest deadline: %.2lf\n", earliest_deadline, latest_deadline);

        fprintf(output[core_no], "Task list for LA-EDF:\n");
        for(i=0; i<task_set->total_tasks && temp_task_set[i].task_number != -1; i++)
        {
            fprintf(output[core_no], "Task: %d, Deadline: %.2lf, Exec time: %.2lf, ", temp_task_set[i].task_number, temp_task_set[i].deadline, temp_task_set[i].exec_time);
            if(task_set->task_list[temp_task_set[i].task_number].criticality_lvl < curr_crit_level)
            {
                // utilisation += (temp_task_set[i].exec_time / (latest_deadline - (*core).total_time));
                discarded_job_exec_time += temp_task_set[i].exec_time;
                utilisation += (temp_task_set[i].exec_time / (temp_task_set[i].deadline - (*core).total_time));
                // utilisation += task_set->task_list[temp_task_set[i].task_number].util[crit_level];
            }
            else
            {
                utilisation += task_set->task_list[temp_task_set[i].task_number].util[crit_level];
            }
            fprintf(output[core_no], "Util: %.2lf\n", utilisation);
        }

        fprintf(output[core_no], "Calculating total cycles:\n");
        double total_cycles = 0;
        for(i=0; i<task_set->total_tasks && temp_task_set[i].task_number != -1; i++)
        {
            int task_number = temp_task_set[i].task_number;
            double deadline = temp_task_set[i].deadline;
            double exec_time = temp_task_set[i].exec_time;
            
            fprintf(output[core_no], "Task: %d, Deadline: %.2lf, Exec time: %.2lf, Util: %.2lf | ", task_number, deadline, exec_time, task_set->task_list[task_number].util[crit_level]);
            if(task_set->task_list[task_number].criticality_lvl < curr_crit_level && crit_level == curr_crit_level)
            {
                // utilisation -= task_set->task_list[task_number].util[crit_level];
                // utilisation -= (exec_time / (latest_deadline - (*core).total_time));
                utilisation -= (exec_time / (deadline - (*core).total_time));
                discarded_job_exec_time -= exec_time;
            }
            else if(task_set->task_list[task_number].criticality_lvl >= crit_level)
            {
                utilisation -= task_set->task_list[task_number].util[crit_level];
            }

            fprintf(output[core_no], "Utilisation before: %.2lf | ", utilisation);

            if(utilisation > 1){
                flag = 1;
                fprintf(output[core_no], "Yes\n");
                break;
            }
            
            double x = max((double)0, exec_time - ((deadline - earliest_deadline) * (1 - utilisation)));
            fprintf(output[core_no], "x: %.2lf | ", x);

            if(task_set->task_list[task_number].criticality_lvl < curr_crit_level && crit_level == curr_crit_level)
            {
                // if(deadline != earliest_deadline){
                    utilisation += ((exec_time - x) / (deadline - earliest_deadline));
                    // discarded_job_exec_time += (exec_time - x);
                // }
            }
            else if(task_set->task_list[task_number].criticality_lvl >= crit_level)
            {
                utilisation += ((exec_time - x) / (deadline - earliest_deadline));
            }

            fprintf(output[core_no], "Utilisation after: %.2lf | ", utilisation);

            total_cycles += x;
            fprintf(output[core_no], "Total cycles: %.2lf\n", total_cycles);
        }

        double freq_ratio;

        if(flag == 1)
            freq_ratio = 1.00;
        else
            freq_ratio = total_cycles / (earliest_deadline - (*core).total_time);

        fprintf(output[core_no], "Freq ratio: %.5lf | ", freq_ratio);
        
        if(freq_ratio > (double)1)
        {
            fprintf(output[core_no], "Yes\n");
        }
        
        double freq = frequency[FREQUENCY_LEVELS - 1];
        for(i=0; i<FREQUENCY_LEVELS; i++)
        {
            if(frequency[i] > freq_ratio)
            {
                freq = frequency[i];
                break;
            }
        }

        selected_frequency = max(selected_frequency, freq);

    }

    fprintf(output[core_no], "Selected Frequency: %.2lf\n", selected_frequency);
    (*core).frequency = selected_frequency;
    // (*core).frequency = 1.00;

}