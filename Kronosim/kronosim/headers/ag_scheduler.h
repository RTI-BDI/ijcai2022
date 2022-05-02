/*
 * ag_Scheduler.h
 *
 *  Created on: 25 set 2017
 *      Author: peppe
 */

#ifndef HEADERS_AG_SCHEDULER_H_
#define HEADERS_AG_SCHEDULER_H_

#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include <algorithm>
#include <vector>

#include "ag_settings.h"
#include "agentMSG_m.h"
#include "Logger.h"
#include "Metric.h"
#include "server.h"
#include "server_handler.h"
#include "task.h"

using namespace omnetpp;

// Custom type that defines what kind of scheduler is in use
// (this is more for code clarity than functionality)
enum SchedType {
    FCFS, RR, EDF
};

class Ag_Scheduler {
public:
    // Release time sensitivity (epsilon between task release time and simulation time)
    const double EPSILON = 0.0001;
    const double DELTA_comparison = 0.00001;

    Ag_Scheduler(SchedType type, int _logger_level, double quantum);
    virtual ~Ag_Scheduler();

    /**
     Implements the schedulability test (not used in non-RT algorithms)
     @param std::vector<std::shared_ptr<Task>> contains the taskset to test
     @return the value of the utilization factor
     */
    double ag_sched_test(std::vector<std::shared_ptr<Task>> ag_tasks_vector, std::shared_ptr<ServerHandler> server_handler = nullptr);

    bool check_overlap(double task_start, double task_end, double window_start, double window_end);

    /** Returns true if the specified task is present */
    bool check_is_task_in_ready(std::shared_ptr<Task> task);
    bool check_is_task_in_release(std::shared_ptr<Task> task);

    /** Sorts the taskset according to the tasks' arrival time */
    void ag_sort_tasks_arrival();

    /** Sorts the taskset according to their deadlines */
    void ag_sort_tasks_ddl(double comp_time_head_ready);

    /** Returns true if a single task has arrived */
    bool ag_task_arrived(std::shared_ptr<Task> server = nullptr);

    /**
     Adds a task in the vector that contains the tasks to be released
     @param std::shared_ptr<Task> pn_task pointer to the task to add
     */
    void ag_add_task_in_vector_to_release(std::shared_ptr<Task> pn_task);

    /** Replace the head of release vector with the specified task (to be used only with servers) */
    void ag_replace_task_to_release(std::shared_ptr<Task> np_task);

    /**
     Adds a task in the vector that contains the ready tasks
     @param std::shared_ptr<Task>pn_task pointer to the task to add
     */
    void ag_add_task_in_ready_tasks_vector(std::shared_ptr<Task> pn_task);

    /** Removes the first element in the vector  */
    void ag_remove_head_in_ready_tasks_vector();
    void ag_remove_head_in_release_tasks_vector();

    /** Removes a task from the specified vector  */
    void ag_remove_task_in_ready_based_on_original_plan_id(int plan_id);
    void ag_remove_complited_preempted_task_from_ready(std::shared_ptr<Task> task);
    void ag_remove_preempted_task_from_ready(std::shared_ptr<Task> server);
    void ag_remove_task_to_release_based_on_original_plan_id(int plan_id);
    void ag_remove_task_to_release_phiI(std::vector<std::shared_ptr<Task>>& queue, int plan_id);

    /** Returns false if the task is a server and if such server is already present in the vector */
    bool ag_vector_ready_contains_server(std::shared_ptr<Task> server);

    /**
     Updates the active task in the vector containing sorted tasks
     @param std::shared_ptr<Task>p_task pointer to the task to update
     */
    void update_active_task_in_sorted_tasks_vector(std::shared_ptr<Task> p_task);

    // Getter methods
    /** Returns the vector containing the tasks to release */
    std::vector<std::shared_ptr<Task>> get_tasks_vector_to_release();

    /** Returns the vector containing the ready tasks */
    std::vector<std::shared_ptr<Task>> get_tasks_vector_ready();

    /** Returns the scheduler's type (e.g., the scheduling algorithm) */
    SchedType get_sched_type();
    std::string get_sched_type_as_string();

    /** Returns the time quantum (set as a constant) */
    double get_quantum();
    double get_curr_quantum();
    void set_curr_quantum(double curr_quantum);

    /**
     Returns the residual computation time when a deadline miss occurs
     @param std::shared_ptr<Task> msg_task pointer to the task that sent the message to check the deadline miss
     @param int first_task_id identifier of the first task in the ready vector
     */
    double get_ddl_miss(std::shared_ptr<Task> msg_task, int first_task_id);
    double get_ddl_miss(std::shared_ptr<Task> msg_task, int first_task_id, int first_task_plan_id);

    /**
     Returns a ready task according to its id and release time
     @param int task_id identifier of the task to find
     @param double task_release contains the task's release time
     */
    std::shared_ptr<Task> get_release_task(int task_id, int task_plan_id, int task_origina_plan_id,int task_demander, double releaseTime);
    std::shared_ptr<Task> get_ready_task(int task_id, int task_plan_id, int task_origina_plan_id,int task_demander, double releaseTime);

    /** Update the plan id reference of the tasks that match the parameters */
    void update_tasks_planid_ready(int task_id, int task_plan_id, int task_origina_plan_id, int new_plan_id);
    void update_tasks_planid_release(int task_id, int task_plan_id, int task_origina_plan_id, int new_plan_id);

    /** Prints the tasks to release */
    void ev_ag_tasks_vector_to_release();

    /** Prints the ready tasks */
    void ev_ag_tasks_vector_ready();

    /** Negotiation related: */
    double get_current_util(std::shared_ptr<ServerHandler> server_handler, double msg_util = 0);

    /** Convert a bool variable (0/1) to string ("true"/"false") */
    std::string boolToString(const bool val);
private:
    SchedType type;
    double quantum;
    double curr_quantum;
    std::shared_ptr<Logger> logger;
    std::vector<std::shared_ptr<Task>> ag_tasks_vector;
    std::vector<std::shared_ptr<Task>> ag_tasks_vector_to_release;
    std::vector<std::shared_ptr<Task>> ag_tasks_vector_ready;

    /** helper methods to estimate future taskset */
    std::vector<std::shared_ptr<Task>> eval_current_taskset(double window_start, double window_end);
};

#endif /* HEADERS_AG_SCHEDULER_H_ */

