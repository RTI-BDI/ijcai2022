/*
 * Intention.h
 *
 *  Created on: Apr 1, 2020
 *      Author: francesco
 */

#ifndef HEADERS_INTENTION_H_
#define HEADERS_INTENTION_H_

#include <stack>
#include <tuple>
#include <variant>

#include "Action.h"
#include "Goal.h"
#include "task.h"
#include "MaximumCompletionTime.h"

class Intention {
public:
    Intention(int id, std::shared_ptr<Goal> _goal, int planId,
            std::shared_ptr<MaximumCompletionTime> plan,
            const std::vector<std::shared_ptr<Belief>>& beliefset,
            std::vector<std::shared_ptr<Effect>> effects_at_begin,
            std::vector<std::shared_ptr<Effect>> effects_at_end,
            json post_conditions,
            double startTime, double firstActivation = -1);
    virtual ~Intention();

    std::shared_ptr<MaximumCompletionTime> makeACopyOfMCT(const std::shared_ptr<MaximumCompletionTime> plan);
    void clear_effects_at_begin();

    // Getter methods
    int get_id();
    int get_goalID();
    int get_planID();
    int get_original_planID();
    std::vector<std::shared_ptr<Action>> get_mainPlanActions();
    std::vector<std::shared_ptr<Action>> get_Actions();
    std::string get_goal_name();
    double get_startTime();
    double get_firstActivation();
    double get_lastExecution();
    double get_batch_startTime();
    double get_current_completion_time();
    double get_scheduled_completion_time();
    bool get_waiting_for_completion();
    std::vector<std::shared_ptr<Belief>> get_expression_variables();
    std::vector<std::shared_ptr<Belief>> get_expression_constants();
    std::vector<std::shared_ptr<Effect>> get_effects_at_begin();
    std::vector<std::shared_ptr<Effect>> get_effects_at_end();
    json get_post_conditions();
    std::shared_ptr<MaximumCompletionTime> getMainPlan();
    std::shared_ptr<MaximumCompletionTime> getOriginalMainPlan();
    std::shared_ptr<Plan> getTopInternalSelectedPlan();
    int getInternalGoalsSelectedPlansSize();

    /** Read the top element in the stack, without modify it */
    std::tuple<std::shared_ptr<Goal>, int, int> get_top_internal_goal(int index);
    /** Get internal goals in this intention */
    std::vector<std::stack<std::tuple<std::shared_ptr<Goal>, int, int>>> get_internal_goals();

    // Setters:
    void set_planID(int id);
    void set_original_planID(int id);
    void set_firstActivation(double time);
    void set_lastExecution(double time);
    void set_batch_startTime(double time);
    void set_current_scheduled_completion_time(double time);
    void set_scheduled_completion_time(double time);
    void set_waiting_for_completion(bool val);
    /* Whenever the "lastExecution" gets update, we also update the start of the intention.
     * We update "lastExecution" whenever the last action in the first batch gets completed.
     * At this point, updating the startTime of the intention means that we are aligning it with the batch execution   */
    void set_startEqualLastExecution(double time);
    void set_expression_variables(std::vector<std::shared_ptr<Belief>>& expression_vars);
    void set_expression_constants(std::vector<std::shared_ptr<Belief>>& expression_consts);

    /** Remove the specified action from the stack */
    void pop_Action_and_nothing_more(int index, int plan_index);
    std::shared_ptr<Task> pop_Action(int action_id, int action_plan_id, int a_original_plan_id);
    void pop_internal_goals(int index);
    void pop_internal_goal_plan(int index = 0);

    /** Push the received action at the top of the stack */
    void push_internal_goal(std::shared_ptr<Goal> goal, int original_plan_id, int counter);

    /** Update internal goal's data */
    void update_ith_top_internal_goal(int index, int counter = 1);
    void update_internal_goal_selectedPlans(std::shared_ptr<MaximumCompletionTime> selectedPlans);

protected:
    int id;
    int plandID;            // Id of the plan that this intention is referring to
    int original_plandID;   // Id of the original plan from which this intention has been derived from
    std::vector<std::shared_ptr<Action>> actions; // list of action of the plan from which we derived the intention
    std::shared_ptr<MaximumCompletionTime> plan;
    std::shared_ptr<MaximumCompletionTime> original_plan;
    double startTime;           // when the intention is instantiated by 'updateIntentionset(...)'
    double firstActivation;     // time t when the first action of the intention has been activated // -1 if not activated yet
    double lastExecution;       // time t when the latest task has been completed
    double batch_startTime;     // time t when the current batch is activated
    double relativeDDL;         // computed when the intention gets instantiated, based on the main plan and the ones selected for the internal goals
    double scheduled_completion_time;   // computed when the last task of the intention gets completed
    double current_completion_time;     // updated every time a task completes. Used to compute the absolute overall deadline of the intention
    std::shared_ptr<Goal> goal;

    /* true when the last action gets completed; false by default.
    * Used to correctly apply effects_at_end at t: deadline of the last action instead of when the task completes the compTime_res */
    bool waiting_for_completion;

    // Save the variables and constants declared in both effects_at_begin and at_end.
    std::vector<std::shared_ptr<Belief>> expression_variables;
    std::vector<std::shared_ptr<Belief>> expression_constants;

    std::vector<std::shared_ptr<Effect>> effects_at_begin;  // update beliefset when plan becomes intention
    std::vector<std::shared_ptr<Effect>> effects_at_end;    // update beliefset when intention is accomplished
    json post_conditions;   // checked when the intention completes, just after "effects_at_end" has been applied

    /* When the top element of the intention is an internal goal,
     * we push such value in this stack, paired with an integer value.
     * The int value will be used as a counter. It will be initialized to:
     * N = number of actions of the best plan that satisfy the goal.
     *
     * Every time the schedule completes the execution of a task, it decreases the
     * counter. When it reaches 0, the internal goal is achieved. We can remove it
     * from the top of the stack and update the relative belief.
     *
     * If the plan for a goal has an internal goal itself, it will be pushed on top of the stack
     * and its execution will be prioritize.
     * Example: if the stack has two elements, the top one will be the one to be executed.
     * When it gets completed, it gets removed and we resume the execution of the next one in the stack.
     * ---------------------------------
     * Note: access tuple parameters using: std::get<index of the parameter>(intention_internal_goals[0]) */
    std::vector<std::stack<std::tuple<std::shared_ptr<Goal>, int, int>>> intention_internal_goals;
};

#endif /* HEADERS_INTENTION_H_ */
