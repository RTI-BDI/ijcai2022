/**
 * agent.h
 *
 *  Created on: Apr 3, 2020
 *      Author: francesco
 */

#ifndef HEADERS_AGENT_H_
#define HEADERS_AGENT_H_

#include <atomic> // std::atomic
#include <chrono> // to compute time needed to complete the simulation
#include <cstdio>
#include <exception> // in order to do try {} catch (exception& e) {}
#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include <vector>
#include <algorithm> // to perform max
#include <typeinfo> // in order to get the name of the class
#include <boost/core/demangle.hpp> // in order to get the type of a variable without the unique identification numbers
#include <memory> // in order to use smart and unique pointers
#include <stack> // stacks are used to manage intentions
#include <functional> // in order to use delegates
#include <limits> // in order to assign +- infinity to a variable
#include <tuple>

// Condition variable --------------------------------
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
// ---------------------------------------------------
// TCP SERVER --------------------------------------
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#define PORT 8080 // port used by the background server thread to listen for messages
// ---------------------------------------------------

#include "AgentSupport.h"
#include "Desire.h"
#include "Goal.h"
#include "Intention.h"
#include "jsonfile_handler.h"
#include "Logger.h"
#include "MaximumCompletionTime.h"
#include "Operator.h"
#include "Plan.h"
#include "Lateness.h"
#include "Metric.h"

#include "ag_scheduler.h"
#include "ag_settings.h"
#include "agentMSG_m.h"
#include "server.h"
#include "msg_task.h"
#include "server_handler.h"
#include "writer.h"

using namespace omnetpp;

//custom triggers that are sent as informative in agMsg
enum Triggers {
    SCHEDULE,               // schedule_taskset
    ACTIVATE_TASK,          // activate task
    CHECK_TASK_TERMINATED,  // check task completion
    PREEMPT,                // manage preemption between tasks

    SET_BELIEFSET,          // used to schedule the reading of the beliefset.json file
    SET_DESIRESET,          // used to schedule the reading of the desireset.json file
    SET_UPDATE_BELIEF,      // used to schedule the reading of the beliefset.json file
    SET_PLANSET,            // used to schedule the reading of the planset.json file
    SET_SERVERS,            // used to schedule the reading of the servers.json file
    SET_SENSORS,            // used to schedule the reading of the sensors.json file
    SET_SKILLSET,           // used to schedule the reading of the skilset.json file
    UPDATE_BELIEF_TIME,     // [these reading are time dependent!] used to schedule the reading of a value from a sensor and that triggers the change of a belief.
    AG_TASK_SELECTION_PHII, // used by phiI to decide which task to execute next, among the ones at the top of each intention's stack
    AG_REASONING_CYCLE,         // schedule a new reasoning cycle
    ADD_TASK_TO_AG_TASKSET,     // used when the agent has to execute an intention's task
    INTENTION_TASK_COMPLETED,   // whenever an aperiodic task reaches n_exec = 0, this message gets triggered
    INTERNAL_GOAL_ARRIVAL,      // used whenever phiI needs to manage an internal goal with "arrivalTime" greater than 0
    INTENTION_COMPLETED,        // when the last action of an intention gets completed, schedule the intention completion process
    RUN_UNTIL_T                 // used to correctly pause the simulation at time 't'
};

class agent: public cSimpleModule {

public:
    agent();
    virtual ~agent();

private:
    // Background thread that simulates a SOCKET SERVER ----------------------------------
    std::unique_lock<std::mutex> lk, lock_thread; // lock and mutex are used to pause/resume the simulation of a thread according to the boolean value of an expression
    inline static std::mutex my_mutex;
    inline static std::mutex thread_mutex;
    bool function_finish_already_performed = false; // true when the ::finish() function gets called
    // -----------------------------------------------------------------------------------

    // Manage background threads and sent messages ---------------------------------------
    std::shared_ptr<std::thread> background_thread;

    /** [!!!][IMPORTANT][!!!]
     * TRUE, the knowledge-base of the agent is initialized using the simulation files (the background server is not activated)
     * FALSE, use the data received from the client through the background thread */
    const bool read_from_file = false;

    /** When the reasoning cycle completes, set the variable to true.
     * Then, call phiI. When phiI completes, if "send_new_solution_msg" = true, send command NEW_SOLUTION to client */
    bool send_new_solution_msg = false;

    /** True when the reasoning cycle does not generate any valid solution.
     * Reset at the beginning of each reasoning cycle
     * -> if at t: x we have more than one "ag_reasoning_cycle", we return NO_SOLUTIONS only if the last cycle does not have solution
     * -> if at t: x the first cycle does not have solution, but the last one does, then NO_SOLUTIONS gets discarded */
    bool no_solution_found = false;

    std::chrono::steady_clock::time_point begin_sim_time;   // debug purposes
    int scheduled_msg = 0, executed_msg = 0;                // debug purposes
    // -----------------------------------------------------------------------------------

    // Scheduling algorithm used in the simulation
    SchedType selected_sched_type;
    // -----------------------------------------------------------------------------------

    // Path containing the knowledge-base files
    std::string glob_path = "";
    std::string glob_simulation_folder = "";
    std::string glob_user = "";
    // -----------------------------------------------------------------------------------

    /**********************************************************************************************
     ****************               BDI global variables              *****************************
     **********************************************************************************************/
    /** Identifies the maximum time reachable by the simulation.
     * At time MAX_SIMULATION_TIME + 0.1 the simulation automatically finishes */
    const double MAX_SIMULATION_TIME = 10000;
    const double infinite_ddl = MAX_SIMULATION_TIME;
    // ------------------------------------------------------------------------------

    /** Self-Message header: name parameter */
    const char msg_name_ag_sensors_time[45] = { "Update Belief" };
    const char msg_name_ag_scheduler[45] = { "Setting Scheduler" };
    const char msg_name_check_task_complete[45] = { "Checking task completion" };
    const char msg_name_intention_task_completed[45] = { "Task completed" };
    const char msg_name_intention_completion[45] = { "Schedule intention completion" };
    const char msg_name_internal_goal_arrival[45] = { "Schedule internal goal arrival" };
    const char msg_name_next_task_arrival[45] = { "Setting next task arrival" };
    const char msg_name_reasoning_cycle[45] = { "Start Reasoning Cycle" };
    const char msg_name_select_next_intentions_task[45] = { "Select next task from intention" };
    const char msg_name_run_until_t[45] = { "Setting Run Until" };
    const char msg_name_task_activation[45] = { "Setting next activation" };
    // ------------------------------------------------------------------------------

    /** Variables containing the Knowledge base of the agent: */
    std::vector<std::shared_ptr<Belief>> ag_belief_set;
    std::vector<std::shared_ptr<Desire>> ag_desire_set;
    std::vector<std::shared_ptr<Goal>> ag_goal_set;
    std::vector<std::shared_ptr<Intention>> ag_intention_set;
    std::vector<std::shared_ptr<Plan>> ag_plan_set;
    std::vector<std::shared_ptr<MaximumCompletionTime>> ag_selected_plans;
    std::vector<std::shared_ptr<Sensor>> ag_sensors_time;
    std::vector<std::shared_ptr<Server>> ag_servers;
    std::vector<std::shared_ptr<Skill>> ag_skill_set;
    // -----------------------------------------------------------------------------------

    /** Pointers to external classes */
    std::shared_ptr<Ag_Scheduler> ag_Sched;
    std::shared_ptr<Ag_settings> ag_settings;
    std::shared_ptr<ServerHandler> ag_Server_Handler; // CBS: Server handler - it also contains the list of servers available for aperiodic tasks

    std::shared_ptr<AgentSupport> ag_support;
    std::shared_ptr<Metric> ag_metric;
    std::shared_ptr<Logger> logger;
    std::unique_ptr<Operator> operator_entity;
    std::unique_ptr<JsonfileHandler> jsonfileHandler;
    // -----------------------------------------------------------------------------------

    // List of completed, activated, stopped and already active goals, all used to compose the ACK for the client
    std::vector<std::shared_ptr<Goal>> reasoning_cycle_already_active_goals; // list of goals already active when the reasoning cycle begins
    std::vector<std::string> reasoning_cycle_new_activated_goals; // list of goals activated during the current reasoning cycle
    std::vector<std::string> reasoning_cycle_stopped_goals; // list of goals stopped during the current reasoning cycle
    std::vector<std::string> main_goals_completed; // list of desires or internal goals correctly completed during the current period of time
    std::vector<std::pair<std::string, std::string>> goals_with_invalid_post_cond; // list of completed goals that have invalid (false) post conditions - structure: <goal name, false post cond>
    // -----------------------------------------------------------------------------------
    // List computed during execution and containing of computed information
    std::vector<std::shared_ptr<Lateness>> ag_tasks_lateness;
    std::vector<std::tuple<std::shared_ptr<Goal>, int, int>> goals_forcing_reasoning_cycle; // <goal, plan_id, intention_id>
    // -----------------------------------------------------------------------------------
    // used in phiI. Contains the list of tasks to be activated for each intention. At the end of phiI, they will be copied in the release queue
    std::vector<std::shared_ptr<Task>> tasks_to_be_release_phiI;
    // -----------------------------------------------------------------------------------

    /** Use to truncate simTime value to the 3 decimal digits (Milliseconds) */
    const SimTimeUnit sim_time_unit = SIMTIME_MS;
    /** reset every time the agent sent a message to the client */
    bool completed_intention_invalid_postconditions = false;
    /** used in 'ag_reasoning_cycle' to clear vectors and variables used during the previous "RUN_UNTIL" */
    simtime_t lastComputed_reasoning_cycle = -1;
    /** indicate the time 't' when the last "check_task_completion_rt" has returned RESULT: true, meaning that a task was completed! */
    double lastCompletedTaskTime = 0.0;
    int phiIFCFS_index = 0;
    // FCFS -------------------------------------
    /** used in function computeGoalDDL when FCFS is used.
     * Only the plans linked to the first goal can be linked to other active intentions.
     * Furthermore, we check the intentionset only if the first task is an internal goal.
     * For internal goals, get deep in other intentions as long as the first action is a goal linked to an active intention or a task is found. */
    bool check_intentions_fcfs = true;
    std::vector<std::shared_ptr<Intention>> intentions_fcfs; /** TODO da descrivere decentemente: List of intentions linked to the internal goal in the first goal's intention */
    // ------------------------------------------

    /** Used in IsSchedulable to define the interval created by the analyzed tasks */
    struct schedulable_interval {
        double start;
        double end;
        double U;
    };

    /** Used in function 'agent::read_all_buffer()' to tell the thread whether the received message is valid or not */
    enum ControlStatus {
        OK, // used when 'end_json' is received and the length of the message is lower then the buffer size
        ERROR // when the received message is too big and can not fit in the buffer (this also includes case when 'end_json' is not stored in the buffer)
    };

protected:
    /** Initializes the agent */
    virtual void initialize() override;
    virtual void finish() override;
    void supportFinish();
    void throwRuntimeException(bool crashed, std::string folder, std::string user, std::string reason, std::string class_name, std::string function, double time);
    bool simulation_limit_reached();

    /*********************************************************************************************
     **************                     BDI functions                       **********************
     *********************************************************************************************/
    /** If a new belief satisfies the preconditions of a desire, an external goal is instantiated.
     *  If this function is called after a belief gets updated by the completion of an internal goal, or due to a sensor reading,
     *  or even because an intention gets completed, we should also check if the ag_selected_plans and the ag_intention_set still
     *  contain valid plans. It may happen that updating a belief makes a plan not achievable
     *  or even that a goal dosn't need to be achieved anymore and thus it'd be wrong to keep execution the task of the relative intention */
    int activateGoalBRF(const std::vector<std::shared_ptr<Belief>>& beliefset, const std::vector<std::shared_ptr<Desire>>& desireset, std::vector<std::shared_ptr<Goal>>& goalset);

    /** Given a set of objectives and feasible plans, it returns the best combination such that the agent is able to complete them while ensuring compliance with the deadlines */
    std::pair<std::vector<std::shared_ptr<MaximumCompletionTime>>, std::vector<int>> phiF(std::vector<std::shared_ptr<Goal>>& goalset,
            std::vector<std::vector<std::shared_ptr<MaximumCompletionTime>>>& feasible_plans);

    /** Used in phiF to compute the computational cost of executing a plan alongside already selected ones and active intentions */
    bool IsSchedulable_v2(std::shared_ptr<MaximumCompletionTime>& intention, const std::vector<std::shared_ptr<MaximumCompletionTime>>& selectedPlans);

    /** Scheduling intervals generated by the tasks analyzed in 'IsSchedulable_v2' */
    schedulable_interval define_struct(double _s, double _e, double _U = 0);
    double manage_new_schedulable_interval(std::vector<schedulable_interval>& intervals, schedulable_interval new_interval);
    double compute_intervals_maxU(const std::vector<schedulable_interval>& intervals, const double t_start, const double t_end);
    void print_schedulable_interval(const std::vector<schedulable_interval>& intervals);
    // --------------------------------------------------------------------------------------------

    /** Used by 'IsSchedulable_v2' to compute the deadline of internal goals, selected plans already analyzed in the current reasoning cycle, and already active intentions */
    double computeIntentionU_v3(const std::shared_ptr<MaximumCompletionTime> MCT_plan,
            std::vector<schedulable_interval>& intervals_U,
            int& internal_goal_index, std::vector<int>& servers_needed,
            double start_batch, double end_batch, double first_batch_delta_time,
            double end_prev_batch, std::vector<std::pair<int, bool>>& alreadyAnalizedIntentions,
            const int analyzed_plan_id, const int fatherPlanBatch, std::vector<std::shared_ptr<Plan>>& plans_in_batch,
            bool skip_intentionset = false, double goal_arrvalTime = 0);
    // --------------------------------------------------------------------------------------------

    /** Returns the computational cost of the task */
    double computeTaskU(const std::shared_ptr<Task> task);

    /** Given a feasible plan for a goal, compute its overall deadline.     *
     * Parameter description:
     * - verify_intention: if true, set function inner logger level to EveryInfo; otherwise, use EssentialInfo
     * - skip_intentionset: false - the goal or internal goal for which this function has been called,
     *                              will check if the goal's plan is linked to an intention. If yes, use intention body to compute DDL.
     *                      true - always use the plan picked for the goal and do not compare it with the intention-set.
     *                      Used for internal goals that are not in the first batch
     * - annidation_level: debug variable, used to print different strings according to the nesting level of each sub-sub-plan */
    std::shared_ptr<MaximumCompletionTime> computeGoalDDL(const std::shared_ptr<Plan> plan,
            std::vector<std::shared_ptr<Plan>>& contained_plans, bool& loop_detected, double first_batch_delta_time,
            double activation_delay, bool verify_intention, bool skip_intentionset, int annidation_level = 0);

    /** For each active intention, pick the tasks that need to be activated and add them to the release queue */
    void phiI(agentMSG* agMsg);
    void phiI_FCFS(agentMSG* agMsg);
    std::vector<std::shared_ptr<Task>> recursiveFCFSPhiI(int index_intention, int goal_id, bool& updated_beliefset);
    void phiI_RR(agentMSG* agMsg);
    // --------------------------------------------------------------------------------------------

    /** When the last task of an intention gets completed, call this function in order to
     * perform the steps needed to remove the intention, update the beliefset and apply a new reasoning cycle.
     * Parameter: 'scheduleNewReasoningCycle = false' only when 'intention_has_been_completed' calls itself */
    int intention_has_been_completed(int intention_goal_id, int intention_plan_id,
            int intention_original_plan_id,
            int index_intention = -1,
            int nested_level = 0,
            bool scheduleNewReasoningCycle = true);
    /** Used for internal goals that have arrivalTime greater than 0. Called when the message scheduled for the activation of the goal arrives */
    void wrapper_intention_has_been_completed(agentMSG* msg);

    /** Called whenever a plan has invalid cont_conditions or post_conditions */
    bool checkAndManageIntentionFalseConditions(std::shared_ptr<Plan> plan,
            std::vector<std::shared_ptr<MaximumCompletionTime>>& selected_plans,
            std::vector<std::shared_ptr<Goal>>& goal_set,
            std::vector<std::shared_ptr<Intention>>& intention_set,
            const std::vector<std::shared_ptr<Belief>>& beliefset,
            std::vector<std::shared_ptr<Task>>& release_queue);
    /** Works exactly as "checkAndManageIntentionFalseConditions". The only difference is that modifies vector "checkNextActionIntention_ith" used only in phiI_RR */
    bool checkAndManageIntentionFalseConditions_RR(std::shared_ptr<Plan> plan,
            std::vector<std::shared_ptr<MaximumCompletionTime>>& selected_plans,
            std::vector<std::shared_ptr<Goal>>& goal_set,
            std::vector<std::shared_ptr<Intention>>& intention_set,
            const std::vector<std::shared_ptr<Belief>>& beliefset,
            std::vector<std::shared_ptr<Task>>& release_queue,
            std::vector<std::tuple<bool, int, int>>& checkNextActionIntention_ith);

    /** Given a set of goals and the library of plans, extract those plans that are linked to the goals.
     * + Group the plans according to the goal they satisfy */
    void unifyGoals(const std::vector<std::shared_ptr<Goal>>& goalset, std::vector<std::shared_ptr<Plan>>& plans,
            const std::vector<std::shared_ptr<Belief>>& beliefset, std::vector<std::vector<std::shared_ptr<Plan>>>& applicable_plans);
    /** Given a set of goals and applicable plans, remove those that have invalid preconditions */
    void unifyPreconditions(std::vector<std::shared_ptr<Goal>>& goalset, std::vector<std::vector<std::shared_ptr<Plan>>>& goals_plan);

    /** *****************************************************************
     * Support Functions for BDI
     * *****************************************************************/
    /** Returns true if all self-scheduled messages are in the further in future w.r.t. the time when the function is called */
    bool checkIfAllMessagesAreInFuture(const simtime_t present);

    /** The selected desire can not be transformed in goal due to its invalid preconditions.
     * We need to check if it was a goal before BRF got called:
     * If yes, remove it from the ag_goal_set, it's not achievable anymore.
     * + if it had a selectedPlan, remove it
     * + if it was already an intention, remove it
     * If no, do nothing here, it's already been managed. */
    bool checkIfDesireWasAGoalAndRemoveIt(const std::shared_ptr<Desire> desire);

    /** If the desire has valid preconditions, and it was already present in the goal set,
     * we have to check if the associated plan is still valid by analyzing its context_conditions.
     * If they are valid as well, goal, plan, intention are valid.
     * If they are not, we have to remove the intention and the plan, BUT leave the goal, allowing the reasoning cycle to check if there is another plan that can satisfy the goal. */
    bool checkIfGoalsPlanIsStillValid(const std::shared_ptr<Desire> desire, const std::shared_ptr<Goal> goal);
    /** If the removed goal was linked to active intentions with first batch already in release/ready,
     * and batch contains an activated internal goals -> update internal goals planID. */
    void checkIfIntentionHadActiveInternalGoals(const int removed_intention_id, std::vector<std::shared_ptr<Intention>>& intentionset);

    /** If conditions are false, remove the intention linked to the goal */
    void checkIfContconditionsAreValid(std::vector<std::shared_ptr<Goal>>& goal,
            std::vector<std::shared_ptr<MaximumCompletionTime>>& selected_plans,
            const std::vector<std::shared_ptr<Belief>>& beliefset,
            std::vector<std::shared_ptr<Intention>>& intentionset);
    /** ------------------------------------------------------------------------------------------------------------ */

    /** Used when performing "write_util_json_report" */
    double compute_MsgUtil_sever();

    /** Initialize the knowledge-base of the agent by reading the files (beliefset, desireset...) from the file-system */
    void initialize_knowledgbase();

    // !!! this function does a % b, and it also works if 'a' is negative
    int mod(int a, int b);

    /**  Returns the current simulation time with MILLISECOND approximation */
    simtime_t sim_current_time();
    /**  Convert double to simtime_t in order to exploit simtime_t precision */
    simtime_t convert_double_to_simtime_t(double time);
    /** ------------------------------------------------------------------------------------------------------------ */

    /** Given a task, if ready-vector or to-be-released vector contains it, then remove it.
     * It's called by the reasoning cycle when a plan is not valid anymore, and it was already an intention.
     * Thus, we have to remove its tasks from execution. */
    void removeFCFSInvalidTasksFromSchedulerVectors(const std::vector<std::shared_ptr<Intention>>& intentions);
    /** When an intention is removed, remove its tasks from release and ready vectors */
    void removeInternalgoalTasksFromSchedulerVectors(const int original_planID);
    /** Remove from Intentions every plan that is not the best choice for the active goals, or that it's not linked to any goal */
    void removeInvalidIntentions(std::vector<std::shared_ptr<Goal>>& goal_set, const std::vector<std::shared_ptr<MaximumCompletionTime>>& selected_plans);
    /** ------------------------------------------------------------------------------------------------------------ */

    /** This function is triggered when the agent receive a self-message that simulate a sensor's reading at specific time */
    void update_belief_set_time(agentMSG* agMsg);
    /** This function returns
     * - true if at least one belief has been modified due to "effects_at_begin"
     * - false otherwise (and it's used to avoid repeating ag_reasoning cycle when not needed) */
    bool updateIntentionset(const std::vector<std::shared_ptr<MaximumCompletionTime>>& selectedPlans,
            std::vector<std::shared_ptr<Belief>>& beliefset,
            std::vector<std::shared_ptr<Intention>>& intentionset);

    /** This function is needed when a mainPlan is removed from execution because not schedulable with the other active intentions.
     * When this happens, if we have intentions derived from internal goals belonging to the mainPlan,
     * we must update their planID and originalplanID reference */
    void updateIntentionsContent(std::vector<std::shared_ptr<Intention>>& intentionset);
    /** ------------------------------------------------------------------------------------------------------------ */

    /** Check if function 'initialize_knowledgbase' has correctly initialized each set */
    void validate_knowledgebase(const std::pair<bool, json> status);
    /** ------------------------------------------------------------------------------------------------------------ */

    /** *****************************************************************
     * Initializes the agent's core sets
     * *****************************************************************/
    /** Read the specified file and store the data into the agent's memory  */
    std::pair<bool, json> set_ag_beliefset(const bool initilization_phase = true, const bool read_from_file = true, std::string file_content = "");
    std::pair<bool, json> set_ag_desireset(const bool initilization_phase = true, const bool read_from_file = true, std::string file_content = "");
    std::pair<bool, json> set_ag_planset(const bool initilization_phase = true, const bool read_from_file = true, std::string file_content = "");
    std::pair<bool, json> set_ag_sensors(const bool initilization_phase = true, const bool read_from_file = true, std::string file_content = "");
    std::pair<bool, json> set_ag_servers(const bool initilization_phase = true, const bool read_from_file = true, std::string file_content = "");
    std::pair<bool, json> set_ag_skillset(const bool initilization_phase = true, const bool read_from_file = true, std::string file_content = "");
    std::pair<bool, json> set_ag_update_beliefset(const bool initilization_phase = true, const bool read_from_file = true, std::string file_content = "");

    /** Whenever a task completes and it has no more execution left (n_exec = 0),
     * this function is called to remove the task from the intention and to move forward the simulation */
    void ag_intention_task_completed(std::shared_ptr<agentMSG> msg);

    /** Apply the reasoning cycle in order to satisfy agent's goal and activate new ones (if needed) */
    void ag_reasoning_cycle(agentMSG* msg);

    /** Called when "INTERNAL_GOAL_ARRIVAL" gets triggered.
     * Used whenever phiI needs to manage an internal goal with "arrivalTime" greater than 0 */
    void ag_internal_goal_activation(agentMSG* msg);
    /******************************************************************/

    /** Write json report file */
    void ddl_json_report(std::shared_ptr<Ag_Scheduler>  ag_Sched, std::shared_ptr<Task> task, double c_res);
    /** ------------------------------------------------------------------------------------------------------------ */

    /***************************************************************/
    /**** Agent functions ******************************************/
    /***************************************************************/
    /** This function is called when the agent receives a message
     * @param *msg the pointer to the message received (can be a self-message) */
    virtual void handleMessage(cMessage* msg) override;
    virtual void handleMessageGP(cMessage* msg);
    virtual void handleMessageRT(cMessage* msg);

    /** Used to pause simulation at time t (value received from the client)
     * Actually this function does nothing, is the message scheduled to active the function that does all the work.
     * The aim to schedule a message at time t just to guarantee that there is going to be at least one message scheduled for that instant of time.
     * The result is that the simulation will stop at time t, either because this is the only message at t and all the others are in the future,
     * or because it is one of the messages scheduled at t.
     * The simulation stops in function handleMessage(...) when cond_var{ return ((proceed_simulation && now < run_until) || exit_simulation); }
     * results false. */
    void manage_ag_run_until_t(agentMSG* msg);

    /** Sets the scheduler for the tasks this function is also called whenever an intention's task is added to the to-be-released-vector */
    virtual void schedule_taskset_rt(agentMSG* msg);
    virtual void schedule_taskset(agentMSG* msg);

    /** Manage the case where a new incoming task has a higher priority w.r.t. the currently performed task */
    virtual void preempt(agentMSG* agMsg);

    /** Used to decide whether or not preemption is needed
     * Note: use "std::shared_ptr<Task>&" if you want to modify the shared_ptr and keep the modified value even when the function completes */
    bool is_preemption_needed(std::shared_ptr<Ag_Scheduler> ag_Sched, const double c_res_head_ready, std::shared_ptr<Task>& sorted_first_task, std::shared_ptr<Task>& initial_first_task);

    /** When preemption is applied, check if the task that we are "removing" from the head of the queue is completed
     * It may happen that a task completes exactly when another task (with higher priority) arrives.
     * When this happens, the function 'check_task_completion_rt' will not detect it because the function can only analyze the head of the queue.
     * This function guarantees a correct management of the task, even if it is not the head anymore */
    void checkIfPreemptedTaskIsCompleted(std::shared_ptr<Task> , std::shared_ptr<Ag_Scheduler> ag_Sched);

    /** Activates the first ready task */
    virtual void activate_task(agentMSG* msg);

    /** Checks if the task has completed */
    virtual void check_task_completion_rt(agentMSG* msg);
    virtual void check_task_completion(agentMSG* msg);

    /** Sends a message which pause the simulation at time t */
    agentMSG* set_run_until_t();

    /** Sends a message which triggers the scheduler's setting */
    virtual agentMSG* set_ag_scheduler();

    /** Sends a message which triggers a task's arrival */
    agentMSG* set_next_task_arrival();

    /** Sends a message which triggers a task's activation */
    agentMSG* set_task_activation();

    /** Sends a message which triggers the check for the task's completion */
    agentMSG* set_check_task_complete(std::shared_ptr<Task>, double);

    /** Sends a message which triggers preemption */
    agentMSG* set_task_preemption(std::shared_ptr<Task>, std::shared_ptr<Task>);

    /** Schedule internal goal activation */
    agentMSG* set_internal_goal_arrival(std::shared_ptr<Goal> goal, int intention_original_plan_id);

    /************************************************************************
     ***************      BDI messages      *********************************
     ************************************************************************/
    /** For each sensor that has activation time equal to the current simulation time,
     * send a dedicated message in order to modify the beliefset */
    void set_ag_sensors_time(std::vector<std::shared_ptr<Sensor>>& sensors_time);

    /**  Activate whenever an aperiodic task reaches n_exec=0 (no more execution left).
     *  Sends a message which triggers the removal of the task from its intention */
    std::shared_ptr<agentMSG> set_intention_task_completed(std::shared_ptr<Task> task, double absolute_ddl);

    /** Set a new reasoning cycle. 'apply_reasoning_cycle_again' = true used when the agent needs to force a new cycle */
    agentMSG* set_reasoning_cycle(bool apply_reasoning_cycle_again = false);

    /** Message that allows the agent to apply phiI and select the next task to execute */
    agentMSG* set_select_next_intentions_task();

    /** Message that allows to schedule the actual completion of an intention.
     * Called when the deadline of the last completed action in the body is equal to the simulation time. */
    agentMSG* set_intention_completion(const int intention_goal_id, const int intention_plan_id,
            const int intention_original_plan_id, const int index_intention, const std::string intention_goal_name, const bool scheduleNewReasoningCycle);
    /**********************************************************************************************/

    /** Cancel all scheduled messages except for "end of simulation". Used to clear the memory when the simulation completes */
    void cancel_all_messages();
    /** Cancel all messages with specified parameter(s).
     * Should be followed by the function that schedule the same message again.
     * The aim is to only apply such operation only once at the time */
    void cancel_next_redundant_msg(const char* name);
    void cancel_next_redundant_msg(const char* name, const int task_id, const int plan_id, const int original_plan_id);
    void cancel_next_redundant_msg(const char* name, const int server_id);
    /** Cancel next scheduled reasoning cycle, if activation time is equal to the given parameter */
    void cancel_next_reasoning_cycle_msg(const char* name, const simtime_t time);
    /** Cancel the activation of an internal goal with arrivalTime greater than 0. Used when the intention containing the goal is removed. */
    void cancel_message_internal_goal_arrival(const char* name, const int plan_id, const int original_plan_id);

    void cancel_message_intention_completion(const char* name, const std::string intention_goal_name, const bool intention_waiting_for_completion,
            const int intention_goal_id, const int intention_plan_id, const int intention_original_plan_id);
    void cancel_message_removed_task(const int planID, bool isPlanId = true);

    /** Returns the number of scheduled messages waiting to be triggered.
     * @param count_end_of_sim = true --> count "end of simulation" control message
     * By default, count_end_of_sim = false because we did not scheduled it, so it is not count in 'scheduled_msg' */
    int pending_scheduled_messages(bool count_end_of_sim = false);

    /** DEBUG: Print the list of scheduled messages */
    void print_simulation_msgs();

    // *****************************************************************************
    // *******                 BACKGROUND THREAD SERVER                      *******
    // *****************************************************************************
    bool exit_simulation = false;               // true makes the main thread call: "agent::finish(...)"
    bool is_client_still_waiting_response = false; // true between the reception of new command and the moment when the server response to the client
    bool performing_reasoning_cycle = false;    // true when the agent is within function ag_reasoning_cycle. false otherwise. Used with command POKE
    bool proceed_simulation = false;            // true when the server receives "run_until_t" and the specified value is greater than the current simulation time
    bool resume_background_thread = false;      // true when the simulation time reaches the "run_until_t" set value or the server sends the ack message to the client

    simtime_t run_until;
    std::atomic_bool stop_background_thread = false; // used by the server to stay in the loop that reads incoming messages from the client. When true, the thread stops.
    std::condition_variable cond_var; // variable used to pause the execution of the main thread (agent)
    std::condition_variable thread_cv; // variable used to pause the execution of the background server thread (agent_thread)

    int socket_id = -1;
    int sent_msg_cnt = 0; // debug purposes
    int received_msg_cnt = 0; // debug purposes
    const int SIZE_MAX_CHAR_ARRAY = 1024 * 100; // max number of char read by the server
    std::vector<std::string> activated_actions; // list of goals activated during the reasoning cycle. Used to compose the ack message

    bool is_threadcreated = false; // true when the background server is activated. Use to only create 1 thread at the time
    bool reasoning_cycle_scheduled_after_update = false; // used to schedule a new reasoning cycle when the server receives an UPDATE knowledge-base-set, and not during initialization

    // Are the files been initialized -----------------------------------------------------
    bool initialized_beliefset = false;           // true -> after class agent has read the beliefset
    bool initialized_desireset = false;           // true -> after class agent has read the desireset
    bool initialized_planset = false;             // true -> after class agent has read the planset
    bool initialized_sensors = false;             // true -> after class agent has read the sensors
    bool initialized_servers = false;             // true -> after class agent has read the servers
    bool initialized_skillset = false;            // true -> after class agent has read the skillset
    bool initialized_update_beliefset = false;    // true -> after class agent has read the update-beliefset
    std::string file_content = "";  // contains the content of the set currently analyzed
    // ------------------------------------------------------------------------------

    /** *********************************************************
     * ******** Implemented in file 'agent_thread.cc' **********
     * *********************************************************/
    bool start_server(); // initialize the background server thread
    void listening_server(); // function used by the server to listen for messages
    // ------------------------------------------------------------------------------

    /** *********************************************************
     * **** Implemented in file 'agent_thread_functions.cc' ****
     * *********************************************************/
    /** Returns true if the message was successfully sent. False otherwise */
    bool send_msg_to_client(json data);

    /** ACK Messages: -------------------------------------------------------------- */
    json ack_error_post_conditions(const std::vector<std::string>& stopped_actions, const std::vector<std::string>& activated_actions,
            const std::vector<std::string>& completed_goals, const std::vector<std::pair<std::string, std::string>>& invalid_goals);
    json ack_new_solution(const std::vector<std::string>& stopped_actions, const std::vector<std::string>& activated_actions,
            const std::vector<std::string>& completed_goals, const std::vector<std::pair<std::string, std::string>>& invalid_post_cond);
    json ack_no_solution(const std::vector<std::string>& stopped_actions, const std::vector<std::string>& activated_actions,
            const std::vector<std::string>& completed_goals, const std::vector<std::pair<std::string, std::string>>& invalid_post_cond);
    json ack_poke_command();
    json ack_run_until(std::vector<std::string>& activated_actions);
    json ack_simulation_completed(const double time, const bool sim_limit_reached, json metrics);
    json ack_update_command(const std::string file_name, const std::string file_content, const std::string status, const json error_log = json::array());
    json ack_util_command(const std::string request, const std::string status);
    json ack_util_initialize_command(const std::string file_name, const std::string file_content, const std::string request, const std::string status, const json error_log = json::array());
    json ack_client_connected(std::string key);
    // -----------------------------------------------------------------------------

    /** Returns a json object that contains the specified parameters */
    json generate_server_response(std::string command, std::string msg, std::string status, Logger::Level logger_level = Logger::EveryInfo);
    /** Called when:
     * - the response generated by the server is too large to fit in the buffer,
     * - the incoming message is too large
     * - the json is not correctly formatted
     * - there are missing parameters in the json (i.e., "command" not specified) */
    json generate_error_server_response(std::string request, std::string msg);
    /** Update the variables used by the condition_variable in order to move the simulation forward */
    void execute_run_until(const simtime_t until_time);
    /** Manage command UPDATE and UTIL.initialize */
    json manage_update(json incoming_obj, bool initialize_sets = false);
    /** Reads the buffer containing the messages received by the client until keyword 'end_json' is found. (The keyword must be at the end of the message) */
    int read_all_buffer(int client_socket, char* data, ControlStatus& status);
};

Define_Module(agent);

#endif /* HEADERS_AGENT_H_ */
