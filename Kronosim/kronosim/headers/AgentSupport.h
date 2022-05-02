/*
 * AgentSupport.h
 *
 *  Created on: Feb 2, 2021
 *      Author: francesco
 */

#ifndef HEADERS_AGENTSUPPORT_H_
#define HEADERS_AGENTSUPPORT_H_

#include <algorithm>
#include <cstdio>
#include <functional>               // in order to use delegates
#include <limits>                   // in order to assign +- infinity to a variable
#include <memory>                   // in order to use smart and unique pointers
#include <stack>                    // stacks are used to manage intentions
#include <typeinfo>
#include <tuple>
#include <vector>
#include <math.h>                   // math functions: pow, sqrt...
#include <stdio.h>
#include <string.h>
#include <omnetpp.h>                    // in order to get the name of the class
#include <boost/algorithm/string.hpp>   // in order to do: boost::to_upper_copy(...)
#include <boost/core/demangle.hpp>      // in order to get the type of a variable without the unique identification numbers

#include "Desire.h"
#include "Goal.h"
#include "Intention.h"
#include "Lateness.h"
#include "Logger.h"
#include "Metric.h"
#include "Operator.h"
#include "Plan.h"
#include "Sensor.h"
#include "Skill.h"

#include "ag_scheduler.h"
#include "server.h"
#include "msg_task.h"
#include "server_handler.h"
#include "writer.h"
#include "json.hpp" // to write to json file
using namespace omnetpp;
using json = nlohmann::json;

class AgentSupport {

private:
    // Use to truncate simTime value to the 3 decimal digits (Milliseconds)
    const SimTimeUnit sim_time_unit = SIMTIME_MS;

    std::shared_ptr<Logger> logger;
    std::unique_ptr<Operator> operator_entity;
    std::pair<simtime_t, std::string> simulation_last_execution = { 0, "" }; // used for debug purposes
    std::pair<simtime_t, std::string> schedule_in_future = { 0, "" };        // used for debug purposes: store the further in future event scheduled using "scheduleAt".

    /** Struct using as return value in functions dealing with expression and formulas */
    struct ExpressionDoubleResult {
        std::string msg = "Unexpected error";
        bool generated_errors = true;
        double value = -1;
    };
    struct ExpressionBoolResult {
        std::string msg = "Unexpected error";
        bool generated_errors = true;
        bool value = false;
    };
    struct ExpressionResult {
        std::string msg = "Unexpected error";
        bool generated_errors = true;
        std::variant<int, double, bool, std::string> value;
    };

    /** Return the current simulation time with MILLISECOND approximation */
    simtime_t sim_current_time();

    /** NOT USED: debug purposes - Recursive function used by "printIntentionsBodyDetailed" */
    void printIntentionsBodyInternalGoals(const std::shared_ptr<MaximumCompletionTime> plan, int& index_plan);

public:
    AgentSupport(int level = Logger::Default);
    virtual ~AgentSupport();

    /** Remove the specified task from the specified vector (ready or release) */
    void ag_Sched_remove_task_in_queue(std::vector<std::shared_ptr<Task>>& queue_vector, int original_plan_id);
    /** Print the specified vector */
    void ag_Sched_ev_ag_tasks_in_queue(std::vector<std::shared_ptr<Task>> queue_vector, std::string name);

    /** Convert bool (0/1) to string ("true"/"false") */
    std::string boolToString(bool b);

    /** Add the plan to the list of plans already analyzed for the current batch */
    bool checkAndAddPlanToPlansInBatch(std::vector<std::shared_ptr<Plan>>& plans_in_batch, std::shared_ptr<Plan> plan);

    /** Function called when an intention completes */
    void checkAndManageIntentionDerivedFromIG(int plan_id, int plan_p, int intention_goal_id,
            const std::vector<std::shared_ptr<Belief>>& beliefset, std::shared_ptr<Goal> achievedGoal,
            std::shared_ptr<Intention> intention);

    /** This function is used to remove a plan dedicated to an internal goal that is already an active intention */
    void checkAndRemovePlanToPlansInBatch(std::vector<std::shared_ptr<Plan>>& plans_in_batch, std::shared_ptr<Plan> plan);

    /** Check if a desire is already active as goal (we can not have two desire linked to the same skill, active at the same time */
    bool checkDesireToAvoidDuplicatedGoals(const std::vector<std::shared_ptr<Goal>>& goalset, const std::shared_ptr<Goal> goal);

    /** Given a belief's name, check if the agent has such belief. If yes, it returns it. Otherwise, returns nullptr */
    std::shared_ptr<Belief> checkIfBeliefExists(const std::vector<std::shared_ptr<Belief>>& beliefset, const std::string name);

    /** Check if desire is already in goalset (similar to checkDesireToAvoidDuplicatedGoals) */
    bool checkIfDesireIsGoal(const std::shared_ptr<Desire> desire, const std::vector<std::shared_ptr<Goal>>& goalset);
    bool checkIfDesireIsLinkedToValidSkill(std::shared_ptr<Desire> desire, const std::vector<std::shared_ptr<Skill>>& skillset);
    /** Returns true if the two parameters are equal (with a given range of accuracy) */
    bool checkIfDoublesAreEqual(const double v1, const double v2, const double tollerance = 0.001); // 0.001 same value of ag_Sched.EPSILON_small

    bool checkIfGoalIsInGoalset(std::shared_ptr<Goal> goal, const std::vector<std::shared_ptr<Goal>>& goalset, bool isFCFSMode = false);
    bool checkIfGoalIsLinkedToIntention(const std::shared_ptr<Goal> goal, const std::vector<std::shared_ptr<Intention>>& intentionset, int& index_intention);
    bool checkIfGoalIsLinkedToValidSkill(std::shared_ptr<Goal> goal, const std::vector<std::shared_ptr<Skill>>& skillset);
    void checkIfGoalWasActivatedInOtherIntentions(std::shared_ptr<Goal> goal, std::vector<std::shared_ptr<Intention>>& intentionset);

    /** Returns true if the plan has already be considered in the current batch. Used to detect loops */
    bool checkIfPlanAlreadyCounted(const std::shared_ptr<Plan> plan, std::vector<std::shared_ptr<Plan>>& contained_plans, std::string& msg, const std::string nesting_spaces);

    /** Given a plan, check if it's already an intention */
    bool checkIfPlanWasAlreadyIntention(const std::shared_ptr<Plan> plan, int& index_intention, const std::vector<std::shared_ptr<Intention>>& ag_intention_set);
    bool checkIfPlanWasAlreadyIntention(const std::shared_ptr<Plan> plan, const std::vector<std::shared_ptr<Intention>>& ag_intention_set);

    bool checkIfPostconditionsAreValid(const std::shared_ptr<Intention> intention, const std::vector<std::shared_ptr<Belief>>& beliefset,
            const std::vector<std::shared_ptr<Belief>>& expression_constants,
            const std::vector<std::shared_ptr<Belief>>& expression_variables);
    bool checkIfPreconditionsAreValid(const std::shared_ptr<Plan> plan, const std::vector<std::shared_ptr<Belief>>& beliefset, const std::string debug = "");

    /** Check if there were waiting for completions intentions among the ones removed by phiF.
    * If there were, restore them (because they do not occupy U) */
    void checkIfRemovedIntentionsWaitingCompletion(const std::vector<std::shared_ptr<Goal>>& goalset, std::vector<std::shared_ptr<Goal>>& goalset_copy,
            std::vector<std::shared_ptr<MaximumCompletionTime>>& selected_plans, const std::vector<std::shared_ptr<MaximumCompletionTime>>& selected_plans_copy,
            std::vector<std::shared_ptr<Intention>>& intentionset, const std::vector<std::shared_ptr<Intention>>& intentionset_copy);

    /** Returns true if the server is already present in ready or release queue */
    bool checkIfServerAlreadyCounted(const std::shared_ptr<ServerHandler> ag_Server_Handler, const std::shared_ptr<Task> task, bool& server_already_counted, std::vector<int>& servers_needed, const Logger::Level logger_level);

    /** If the server is already present in ready or release queue, return its deadline. Returns -2 otherwise. Note: -1 means infinite deadline */
    double checkIfServerIsActiveAndGetDDL(const int server_id, const std::vector<std::shared_ptr<Task>> ready, const std::vector<std::shared_ptr<Task>> release);
    /** Returns the index of the task in the queue of the server. -1 if not present */
    int checkIfTaskAlreadyInServerQueue(std::shared_ptr<Task> server, std::shared_ptr<Task> task);
    /** Returns false if the task belongs to the main plan */
    bool checkIfTaskBelongsToIntentionInternalGoal(const std::shared_ptr<Task> task, const std::shared_ptr<Plan> plan, const std::vector<std::shared_ptr<Intention>>& ag_intention_set);
    /** Used in phiRR -- Returns true if the task is in either ready or release queue */
    bool checkIfTasksInReadyOrRealeaseQueue(const std::shared_ptr<Task> task, const std::vector<std::shared_ptr<Task>> ready, const std::vector<std::shared_ptr<Task>> release);
    /** Returns true if the task is contained in an active intention. It also returns the index of the intention, using variable "index_intention" */
    bool checkIfTaskIsLinkedToIntention(const std::shared_ptr<Task> task, const std::vector<std::shared_ptr<Intention>>& intentionset, int& index_intention);

    bool checkIntentionStartBatchTime(std::shared_ptr<Intention> intention, std::shared_ptr<Ag_Scheduler> ag_Sched);

    /** Returns true if conditions are valid */
    bool checkPlanOrDesireConditions(const std::vector<std::shared_ptr<Belief>>& beliefset,
            const json preconditions, std::string& msg,
            const std::string description, // for debug purposes
            const int id, // for debug purposes
            const std::vector<std::shared_ptr<Belief>>& expression_constants = {},
            const std::vector<std::shared_ptr<Belief>>& expression_variables = {});

    /** Given two beliefs compare they values based on the specified operator:
     * - true if the values are valid
     * - false if they are not, or if they have different types */
    bool compareBeliefValues(std::variant<int, double, bool, std::string> belief_value, std::variant<int, double, bool, std::string> comparison_value, Operator::Type op);

    /** Compute Priority and Preference value either using the specified formula, or using the reference_table specified by the user
     * + update the priority/pref value of the given element (desire or plan) */
    void computeDesirePriority(std::shared_ptr<Desire>& desire, const std::vector<std::shared_ptr<Belief>>& beliefset);
    void computeGoalPriority(std::shared_ptr<Goal>& goal, const std::vector<std::shared_ptr<Belief>>& beliefset);
    double computePlanPref(std::shared_ptr<Plan>& plan, const std::vector<std::shared_ptr<Belief>>& beliefset);

    /** Computes the time needed for the simulation to fully complete */
    double computeSimulationEllapsedTime(const std::chrono::steady_clock::time_point begin_sim_time);

    /** Debug and visualization purposes */
    std::string convertOperatorToString(Operator::Type op);
    std::string convertSchedType_to_string(const int sched_type);

    /** Returns the sum of selected_plans for every goals */
    int countGoalsPlans(const std::vector<std::vector<std::shared_ptr<Plan>>>& goals_plans);

    /* Given a task:
     * - if PERIODIC -> return -1 // not linked to servers
     * - if APERIODIC -> task is a server, return N = (server index in 'servers_needed') if it is contained in the vector; -1 otherwise. */
    int IsServerAlreadyUsed(std::shared_ptr<Task>, std::vector<int>& servers_needed);
    void initializeAlreadyActiveIntention(std::vector<std::pair<int, bool>>& alreadyActive, const std::vector<std::shared_ptr<Intention>>& intentionset);

    void manageForceReasoningCycle(std::vector<std::shared_ptr<Intention>>& intentionset,
            std::vector<std::shared_ptr<Plan>>& planset, const int removedIntentionPlanId);

    /** Manage the ready queue when tasks change due to a reasoning cycle */
    bool manageReadyQueue(std::shared_ptr<Ag_Scheduler> ag_Sched);

    /** Choose the best plan for an goal according to the selected approach */
    std::shared_ptr<MaximumCompletionTime> MCTSelect(const std::shared_ptr<MaximumCompletionTime> MCTint_goal, const std::shared_ptr<MaximumCompletionTime> MCTfp, const bool optimisticApproach = false);

    void orderFeasiblePlansBasedOnDDL(std::vector<std::vector<std::shared_ptr<MaximumCompletionTime>>>& feasible_plans,
            const bool approach, const std::vector<std::shared_ptr<Intention>>& intentionset,
            const std::shared_ptr<Ag_Scheduler> ag_Sched);
    void orderGoalsetBasedOnPriority(std::vector<std::shared_ptr<Goal>>& goalset);
    void orderPlansBasedOnPref(std::vector<std::vector<std::shared_ptr<Plan>>>& plans);

    /*
     * Manage Effects Expressions
     */
    ExpressionResult parseExpression(const json expression,
            const std::vector<std::shared_ptr<Belief>>& beliefset,
            std::vector<std::shared_ptr<Belief>>& expression_variables,
            std::vector<std::shared_ptr<Belief>>& expression_constants);

    ExpressionResult effectEvaluateExpression(const json expression,
            const std::vector<std::shared_ptr<Belief>>& beliefset,
            std::vector<std::shared_ptr<Belief>>& expression_variables,
            std::vector<std::shared_ptr<Belief>>& expression_constants);

    ExpressionDoubleResult effectNumericalExpression(const json expression,
            const std::vector<std::shared_ptr<Belief>>& beliefset,
            const std::vector<std::shared_ptr<Belief>>& expression_variables = {}, // = {} is used to keep the same function and use it for plan.effects, plan.formula, desire.formula
            const std::vector<std::shared_ptr<Belief>>& expression_constants = {}, // = {} is used to keep the same function and use it for plan.effects, plan.formula, desire.formula
            int inner_level = 0);

    ExpressionBoolResult effectLogicExpression(const json expression,
            const std::vector<std::shared_ptr<Belief>>& beliefset,
            const std::vector<std::shared_ptr<Belief>>& expression_variables = {},
            const std::vector<std::shared_ptr<Belief>>& expression_constants = {},
            int inner_level = 0);

    /** Convert from variant to type */
    double checkAndConvertVariantToDouble(std::variant<int, double, bool, std::string> value, bool& correct_type);
    bool checkAndConvertVariantToBool(std::variant<int, double, bool, std::string> value, bool& correct_type);

    ExpressionBoolResult computeComparisonExpression(const json expression,
            const int inner_level, const std::string operator_str, const std::string equation_side,
            const std::vector<std::shared_ptr<Belief>>& beliefset,
            const std::vector<std::shared_ptr<Belief>>& expression_variables,
            const std::vector<std::shared_ptr<Belief>>& expression_constants,
            ExpressionBoolResult& result_bool,
            ExpressionDoubleResult& result_double,
            bool& is_bool_expression);

    /** Compare two numerical values */
    bool compareNumericalValues(const std::string op, const double value_A, const double value_B);
    void convertExpressionBoolToResult(const ExpressionBoolResult result_bool, ExpressionResult& result);
    void convertExpressionDoubleToResult(const ExpressionDoubleResult result_double, ExpressionResult& result);
    void convertExpressionDoubleToBoolResult(const ExpressionDoubleResult result_double, ExpressionBoolResult& result);
    void convertExpressionResultToBool(const ExpressionResult result, ExpressionBoolResult& result_bool);

    /** Convert json expression to string + replace "READ" commands with the actual value */
    std::string dump_expression(json expression, const std::vector<std::shared_ptr<Belief>>& beliefset,
            const std::vector<std::shared_ptr<Belief>>& expression_constants = {},
            const std::vector<std::shared_ptr<Belief>>& expression_variables = {});

    bool updateNumericalResult(std::string op, double& result, const double value);
    bool updateNumericalResult(std::string op, double& result, const int value);
    bool updateLogicalResult(std::string op, bool& result, const bool value);
    bool updateSingleBeliefDueToEffect(const std::string mode,
            std::variant<int, double, bool, std::string>& current_value,
            std::variant<int, double, bool, std::string> effect_value,
            AgentSupport::ExpressionResult& result_parse);
    // -------------------------------------------------------------------------------------------------------------

    /** Manage Plan preference (pref) formula and Desire/Goal priority (pr) formula */
    ExpressionDoubleResult formulaExpression(const json expression, const std::vector<std::shared_ptr<Belief>>& beliefset);
    // -------------------------------------------------------------------------------------------------------

    /** Sort vector chronologically */
    void sortSensorVectorBubbleSort(std::vector<std::shared_ptr<Sensor>>& sensors);
    void sortWaitingIntentionsBubbleSort(std::vector<std::shared_ptr<Intention>>& intentions);

    void removeGoalAndSelectedPlanFromPhiI(std::vector<std::shared_ptr<MaximumCompletionTime>>& ag_selected_plans, std::vector<std::shared_ptr<Goal>>& ag_goal_set, const std::shared_ptr<Intention> intention);
    void removePlanFromCounted(const std::shared_ptr<Plan> plan, std::vector<std::shared_ptr<Plan>>& contained_plans);

    void update_activated_goal_list(const std::vector<std::shared_ptr<Goal>>& ag_goal_set,
            const std::vector<std::shared_ptr<Goal>>& reasoning_cycle_already_active_goals, std::vector<std::string>& reasoning_cycle_new_activated_goals);
    void update_internal_goal_selectedPlans(const std::vector<std::shared_ptr<MaximumCompletionTime>>& ag_selected_plans, std::vector<std::shared_ptr<Intention>>& ag_intention_set);

    /** Update the value of a specific belief:
     * if the two variant have different types, updatedSuccessfully becomes False */
    std::variant<int, double, bool, std::string> updateBeliefValueBasedOnSensorRead(
            std::variant<int, double, bool, std::string> ag_belief_value,
            std::variant<int, double, bool, std::string> sensor_value,
            const Sensor::Mode mode, bool& updatedSuccessfully);

    /** Returns the number of updated beliefs */
    int updateBeliefsetDueToEffect(std::vector<std::shared_ptr<Belief>>& beliefset,
            std::shared_ptr<Intention>& intention,
            const std::vector<std::shared_ptr<Effect>>& effects);

    /** Decrease the n_exec parameter of p_task in the intention that contains it */
    void updatePeriodicTasksInsideIntention(std::shared_ptr<Task> task, std::vector<std::shared_ptr<Intention>>& intentions);

    /** If a reasoning cycle changes the best plan for a goal, change the id of the selectedPlan even in active internal goals */
    void updateSelectedPlans(std::vector<std::shared_ptr<MaximumCompletionTime>>& selectedPlans);

    /** Make 'variant' printable by converting it to string, double, bool */
    std::string variantToString(std::variant<int, double, bool, std::string> variant);
    double variantToDouble(std::variant<int, double, bool, std::string> variant, bool& succ);
    bool variantToBool(std::variant<int, double, bool, std::string> variant, bool& succ);

    // Getters
    /** Given the plan_id of a task, it returns the index of the linked intention in the ag_intention_set */
    int getIntentionIndexGivenPlanId(const int plan_id, const std::vector<std::shared_ptr<Intention>>& intention_set);
    /** Returns the scheduled event further ahead in the future */
    std::pair<simtime_t, std::string> getScheduleInFuture();
    /** Returns the index of the server. If not found, return -1 */
    int getServerIndexInAg_servers(const int server_id, const std::vector<std::shared_ptr<Server>>& servers);
    /** Returns the overall deadline of a periodic task, or the deadline (period) of the budget linked to the aperiodic one */
    double getTaskDeadline_based_on_nexec(const std::shared_ptr<Task>,
            bool& isTaskInAlreadyActiveServer,
            double& server_original_deadline,
            const std::vector<std::shared_ptr<Server>>& servers,
            const std::vector<std::shared_ptr<Task>>& ready,
            const std::vector<std::shared_ptr<Task>>& release);
    /** Returns the type stored in the variant as UPPER-CASE string */
    std::string get_variant_type_as_string(std::variant<int, double, bool, std::string> variant);

    // Setters
    void setLogger_level(int level);
    void setScheduleInFuture(std::string event, simtime_t time = simTime());
    void setSimulationLastExecution(std::string event, simtime_t time = simTime());
    // ---------------------------------------------------------------------------------------

    // Initialize() Support functions --------------------------------------------------------
    void initialize_delete_files(std::string user, std::string folder, int sched_type);
    // Finish() Support functions ------------------------------------------------------------
    void finishSaveToFile(std::string folder, std::string user, std::string sched_type);
    // ---------------------------------------------------------------------------------------

    // PRINT functions -----------------------------------------------------------------------
    void printActions(const std::vector<std::shared_ptr<Action>> actions);
    void printApplicablePlans(const std::vector<std::vector<std::shared_ptr<Plan>>>& applicable_plans, const std::vector<std::shared_ptr<Goal>>& ag_goal_set);
    void printBeliefset(const std::vector<std::shared_ptr<Belief>>& beliefset, std::string list_desc = "List of Beliefs: ");
    void printCompletedIntentions(const std::vector<std::shared_ptr<Intention>>& intentionset, const std::vector<std::shared_ptr<Goal>>& goalset);
    void printDesireset(const std::vector<std::shared_ptr<Desire>>& desireset);
    void printEffects(const std::vector<std::shared_ptr<Effect>>& effects);
    void printFeasiblePlans(const std::vector<std::vector<std::shared_ptr<MaximumCompletionTime>>>& feasible_plans, std::shared_ptr<Metric> ag_metric = nullptr);
    void printGoalset(const std::vector<std::shared_ptr<Goal>>& goalset);
    void printIntentions(const std::vector<std::shared_ptr<Intention>>& intentionset, const std::vector<std::shared_ptr<Goal>>& goalset, const bool printSubPlans = false);
    /* NOT USED: debug purposes */void printIntentionsBodyDetailed(const std::vector<std::shared_ptr<Intention>> intention);
    void printInvalidIntentions(const std::vector<std::shared_ptr<Intention>>& intentionset, const std::vector<std::shared_ptr<Goal>>& goalset);
    void printWaitingCompletionIntentions(const std::vector<std::shared_ptr<Intention>>& intentionset, const std::vector<std::shared_ptr<Goal>>& goalset, const bool printSubPlans = false);
    void printInternalGoalsSelectedPlans(const std::vector<std::pair<std::shared_ptr<Plan>, int>>& plans);
    void printPlanset(const std::vector<std::shared_ptr<Plan>>& planset, const std::vector<std::shared_ptr<Goal>>& goalset);
    void printPlanset(const std::vector<std::shared_ptr<MaximumCompletionTime>>& planset, const std::vector<std::shared_ptr<Goal>>& goalset);
    void printSelectedPlans(const std::vector<std::shared_ptr<MaximumCompletionTime>>& planset);
    void printSelectedPlan(const std::shared_ptr<MaximumCompletionTime> plan);
    void printSensors(const std::vector<std::shared_ptr<Sensor>>& sensors);
    void printServers(const std::vector<std::shared_ptr<Server>>& servers);
    void printSkillset(const std::vector<std::shared_ptr<Skill>>& skillset);
    void printTaskset(const std::vector<std::shared_ptr<Task>>& taskset);
    void printVectorOfString(const std::vector<std::string>& strings);
    void printVectorOfPairString(const std::vector<std::pair<std::string, std::string>>& strings);
    // ---------------------------------------------------------------------------------------
};

#endif /* HEADERS_AGENTSUPPORT_H_ */
