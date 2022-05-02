/*
 * Metric.h
 *
 *  Created on: Feb 22, 2021
 *      Author: francesco
 */

#ifndef HEADERS_METRIC_H_
#define HEADERS_METRIC_H_

#include <cstdio>
#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include <vector>
#include <algorithm>
#include <typeinfo>                 // in order to get the name of the class
#include <boost/core/demangle.hpp>  // in order to get the type of a variable without the unique identification numbers
#include <memory>                   // in order to use smart and unique pointers
#include <stack>                    // stacks are used to manage intentions
#include <functional>               // in order to use delegates
#include <limits>                   // in order to assign +- infinity to a variable
#include <tuple>

#include "Desire.h"
#include "Goal.h"
#include "Intention.h"
#include "Lateness.h"
#include "Logger.h"
#include "Operator.h"
#include "Plan.h"
#include "Sensor.h"
#include "task.h"
#include "writer.h"
#include "json.hpp" // to write to json file

using namespace omnetpp;
using json = nlohmann::json;

class Metric {

protected:
    // Use to truncate simTime value to the 3 decimal digits (Milliseconds)
    const SimTimeUnit sim_time_unit = SIMTIME_MS;

    std::shared_ptr<Logger> logger;
    std::pair<double, std::string> crash_report; // structure: <time, reason>

    std::vector<std::tuple<std::shared_ptr<Goal>, simtime_t, simtime_t, int, int, std::string, std::string, int>> activatedGoals; // structure: <goal to be satisfied, activation time, completion time, plan, original, pref_index, reason, best_feasible_plan> -> used whenever a desire or an internal goal is activated and completed
    std::vector<std::tuple<std::shared_ptr<Intention>, simtime_t, simtime_t, std::string>> activatedIntentions;     // <intention, activation time, completion time, completion_reason>
    std::vector<std::tuple<std::shared_ptr<Intention>, simtime_t, std::string>> completedGoals;     // structure: <intention, simTime, function>
    std::vector<std::tuple<simtime_t, std::string, std::string>> generated_errors;      // structure: <simTime, function, message>
    std::vector<std::tuple<simtime_t, std::string, std::string>> generated_warnings;    // structure: <simTime, function, message>
    std::vector<std::tuple<int, int, int, simtime_t>> phiI_stats;               // structure: structure: <iteration ith, active goals, active intentions, time>
    std::vector<std::tuple<int, int, int, simtime_t>> reasoning_cycle_stats;    // structure: <iteration ith, active goals, active intentions, time>
    std::vector<std::tuple<std::shared_ptr<Task>, simtime_t, simtime_t>> tasks_response_time;   // structure: <Task, finishTime, responseTime>
    std::vector<std::pair<simtime_t, std::shared_ptr<Task>>> unexpected_tasks_execution;        // structure: <time, executed_task>

public:
    Metric(int logger_level = Logger::Default);
    virtual ~Metric();

    void addActivatedGoal(std::shared_ptr<Goal> goal, simtime_t activationTime, int planid = -1, int original_planid = -1, std::string pref_index = "not set");
    void addActivatedIntentions(std::shared_ptr<Intention> intention, simtime_t activationTime);
    void addGeneratedError(simtime_t time, std::string function, std::string msg);
    void addGeneratedWarning(simtime_t time, std::string function, std::string msg);
    void addGoalCompleted(std::shared_ptr<Intention> intention, simtime_t time, std::string function);

    void addPhiIStats(int active_goals, int active_intentions, simtime_t time);
    void addReasoningCycleStats(int active_goals, int active_intentions, simtime_t time);
    void addTaskResponseTime(std::shared_ptr<Task> task, simtime_t finishTime, simtime_t responseTime);
    void addUnexpectedTaskExecution(std::shared_ptr<Task> task);

    void checkIfUnexpectedTaskExecution(const std::shared_ptr<Task> task, const std::vector<std::shared_ptr<Intention>>& intentions);

    // Finish(): ----------------------------------------------------------------
    void finishCrash(bool crashed, std::string folder, std::string user, std::string reason = "", std::string class_name = "agent", std::string function = "", simtime_t time = -1);
    void finishForceSimulationEnd(bool force_end, std::string folder, std::string user, std::string reason = "", std::string class_name = "agent", std::string function = "", simtime_t time = -1);
    void finishGeneratedDeadlineMiss(const std::vector<std::shared_ptr<Lateness>>& ag_tasks_lateness, int agent_id, int sched_type, std::string sched_type_str, bool phiI_prioritize_internal_goals);
    void finishGeneratedErrors(int agent_id);
    void finishGeneratedWarnings(int agent_id);
    void finishGoalAchieved(int agent_id);
    void finishPhiIStats(int agent_id);
    void finishReasoningCyclesStats(int agent_id);
    void finishResponseTime(int agent_id);
    void finishSaveToFile(std::string folder, std::string user, std::string sched_type);
    void finishStillActiveElements(const std::vector<std::shared_ptr<Task>> release_queue, const std::vector<std::shared_ptr<Task>> ready_queue,
            const std::vector<std::shared_ptr<Intention>>& ag_intention_set, const std::vector<std::shared_ptr<Goal>>& ag_goal_set,
            const std::vector<std::shared_ptr<Belief>>& ag_belief_set, int agent_id, bool simulation_limit_reached, const simtime_t max_simulation_time);
    void finishUnexpectedTasksExecution(int agent_id);
    // --------------------------------------------------------------------------

    // Support: -----------------------------------------------------------------
    std::string boolToString(bool b);
    std::string variantToString(std::variant<int, double, bool, std::string> variant);
    // --------------------------------------------------------------------------

    // Getters: -----------------------------------------------------------------
    std::vector<std::tuple<int, int, int, simtime_t>>& getPhiIStats();
    std::vector<std::tuple<int, int, int, simtime_t>>& getReasoningCycleStats();
    std::vector<std::tuple<std::shared_ptr<Task>, simtime_t, simtime_t>>& getTasksResponseTime();
    std::vector<std::pair<simtime_t, std::shared_ptr<Task>>>& getUnexpectedTasksExecution();
    // --------------------------------------------------------------------------

    // Setters: -----------------------------------------------------------------
    void setActivatedIntentionCompletion(std::shared_ptr<Intention> intention, simtime_t completionTime, std::string complitionReason);
    void setActivatedGoalCompletion(std::shared_ptr<Goal> goal, simtime_t completionTime, std::string complitionReason);
    void setActivatedGoalCompletion(const int goal_id, const std::string goal_name, simtime_t completionTime, std::string complitionReason);
    void setActivatedGoalsSelectedPlan(const std::vector<std::shared_ptr<Goal>>& goalset, const std::vector<std::shared_ptr<MaximumCompletionTime>>& selectedPlans,
            const std::vector<int>& pref_index, const std::vector<std::vector<std::shared_ptr<MaximumCompletionTime>>>& feasible_plans);
    // --------------------------------------------------------------------------

    // Print: -------------------------------------------------------------------
    std::vector<json> printActivatedIntentions();
    std::vector<json> printActivatedGoals();
    void printAlreadyAnalizedIntention(const std::vector<std::pair<int, bool>>& vec, const std::vector<std::shared_ptr<Intention>>& ag_intention_set);
    void printFinish(std::string line, Logger::Level level = Logger::EssentialInfo);
    // --------------------------------------------------------------------------
};


#endif /* HEADERS_METRIC_H_ */
