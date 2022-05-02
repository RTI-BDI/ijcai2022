/*
 * Intention.cc
 *
 *  Created on: Apr 1, 2020
 *      Author: francesco
 */
#include "../headers/Intention.h"

Intention::Intention(int _id, std::shared_ptr<Goal> _goal,
        int _planID, std::shared_ptr<MaximumCompletionTime> _plan,
        const std::vector<std::shared_ptr<Belief>>& beliefset,
        std::vector<std::shared_ptr<Effect>> _effects_at_begin,
        std::vector<std::shared_ptr<Effect>> _effects_at_end,
        json _post_conditions,
        double _startTime, double _firstActivation)
{
    id = _id;
    plandID = _planID;
    goal = std::make_shared<Goal>(*_goal);
    original_plandID = _planID; // default value
    plan = _plan;
    original_plan = makeACopyOfMCT(_plan);
    if(_plan->getInternalGoalsSelectedPlansSize() > 0) {
        actions = _plan->getInternalGoalsSelectedPlans()[0].first->get_body();
    }
    startTime = _startTime;
    firstActivation = _firstActivation;
    lastExecution = _startTime;
    batch_startTime = _startTime;
    effects_at_begin = _effects_at_begin;
    effects_at_end = _effects_at_end;
    post_conditions = _post_conditions;

    current_completion_time = -1;
    scheduled_completion_time = -1;
    waiting_for_completion = false;
}

Intention::~Intention() { }

std::shared_ptr<MaximumCompletionTime> Intention::makeACopyOfMCT(const std::shared_ptr<MaximumCompletionTime> plan) {
    std::shared_ptr<MaximumCompletionTime> plan_copy = std::make_shared<MaximumCompletionTime>();

    // copy MaximumCompletionTime - subplans;
    std::vector<std::pair<std::shared_ptr<Plan>, int>> sub_plans_copy;
    std::vector<std::pair<std::shared_ptr<Plan>, int>> sub_plan = plan->getInternalGoalsSelectedPlans();
    for(int i = 0; i < (int)plan->getInternalGoalsSelectedPlansSize(); i++) {
        sub_plans_copy.push_back(std::make_pair(sub_plan[i].first, sub_plan[i].second));
        plan_copy->add_internal_goal_selectedPlan(sub_plan[i].first, sub_plan[i].second);
    }

    return plan_copy;
}

void Intention::clear_effects_at_begin() {
    effects_at_begin.clear();
}

// Getter methods
int Intention::get_id() {
    return id;
}

int Intention::get_goalID() {
    return goal->get_id();
}

int Intention::get_planID(){
    return plandID;
}

void Intention::set_planID(int id) {
    plandID = id;
}

int Intention::get_original_planID() {
    return original_plandID;
}

void Intention::set_original_planID(int id) {
    original_plandID = id;
}

std::vector<std::shared_ptr<Action>> Intention::get_mainPlanActions(){
    return plan->getInternalGoalsSelectedPlans()[0].first->get_body();
}

std::vector<std::shared_ptr<Action>> Intention::get_Actions() {
    return actions;
}

std::string Intention::get_goal_name() {
    return goal->get_goal_name();
}

double Intention::get_startTime() { // Note: set_startTime is not available. Only the constructor can set it.
    return startTime;
}

double Intention::get_firstActivation() {
    return firstActivation;
}

void Intention::set_firstActivation(double time) {
    firstActivation = time;
}

double Intention::get_lastExecution() {
    return lastExecution;
}

double Intention::get_batch_startTime() {
    return batch_startTime;
}

double Intention::get_current_completion_time() {
    return current_completion_time;
}

double Intention::get_scheduled_completion_time() {
    return scheduled_completion_time;
}

bool Intention::get_waiting_for_completion() {
    return waiting_for_completion;
}

void Intention::set_lastExecution(double time) {
    lastExecution = time;
}

void Intention::set_batch_startTime(double time) {
    batch_startTime = time;
}

void Intention::set_current_scheduled_completion_time(double time) {
    current_completion_time = time;
}

void Intention::set_scheduled_completion_time(double time) {
    scheduled_completion_time = time;
}

void Intention::set_waiting_for_completion(bool val) {
    waiting_for_completion = val;
}

std::vector<std::shared_ptr<Belief>> Intention::get_expression_variables() {
    return expression_variables;
}

std::vector<std::shared_ptr<Belief>> Intention::get_expression_constants() {
    return expression_constants;
}

std::vector<std::shared_ptr<Effect>> Intention::get_effects_at_begin() {
    return effects_at_begin;
}

std::vector<std::shared_ptr<Effect>> Intention::get_effects_at_end() {
    return effects_at_end;
}

json Intention::get_post_conditions() {
    return post_conditions;
}

void Intention::set_startEqualLastExecution(double time) {
    lastExecution = time;
    startTime = time;
}

void Intention::set_expression_variables(std::vector<std::shared_ptr<Belief>>& variables) {
    expression_variables = variables;
}

void Intention::set_expression_constants(std::vector<std::shared_ptr<Belief>>& consts) {
    expression_constants = consts;
}

std::shared_ptr<MaximumCompletionTime> Intention::getMainPlan() {
    return plan;
}

std::shared_ptr<MaximumCompletionTime> Intention::getOriginalMainPlan() {
    return std::make_shared<MaximumCompletionTime>(*original_plan);
}

std::shared_ptr<Plan> Intention::getTopInternalSelectedPlan()
{
    if(plan->getInternalGoalsSelectedPlansSize() == 0) {
        std::cerr << "[ERROR] There are no selected plans for this plan and its internal goals! (There must at least be the Main plan)" << std::endl;
        return nullptr;
    }
    else {
        return plan->getInternalGoalsSelectedPlans()[0].first;
    }
}

int Intention::getInternalGoalsSelectedPlansSize() {
    return plan->getInternalGoalsSelectedPlansSize();
}

// functions to manage internal goal stack **********************************
std::tuple<std::shared_ptr<Goal>, int, int> Intention::get_top_internal_goal(int index) {
    return intention_internal_goals[index].top();
}

std::vector<std::stack<std::tuple<std::shared_ptr<Goal>, int, int>>> Intention::get_internal_goals() {
    return intention_internal_goals;
}

void Intention::pop_Action_and_nothing_more(int i, int plan_index) {
    if((int)actions.size() > i) {
        actions.erase(actions.begin() + i);
        plan->getInternalGoalsSelectedPlans()[plan_index].first->remove_action_from_body(i);
    }
}

std::shared_ptr<Task> Intention::pop_Action(int a_id, int a_plan_id, int a_original_plan_id) {
    std::shared_ptr<Task> ret_task;
    std::shared_ptr<Task> task;

    for(int i = 0; i < (int)actions.size(); i++) {
        if(actions[i]->get_type() == "task") {
            task = std::dynamic_pointer_cast<Task>(actions[i]);
            if(task->getTaskId() == a_id && task->getTaskPlanId() == a_plan_id && task->getTaskOriginalPlanId() == a_original_plan_id) {
                ret_task = std::make_shared<Task>(*task);
                actions.erase(actions.begin() + i);
                break;
            }
        }
    }

    std::vector<std::shared_ptr<Action>> body = plan->getInternalGoalsSelectedPlans()[0].first->get_body();

    for(int i = 0; i < (int)body.size(); i++) {
        if(body[i]->get_type() == "task") {
            task = std::dynamic_pointer_cast<Task>(body[i]);
            if(task->getTaskId() == a_id && task->getTaskPlanId() == a_plan_id && task->getTaskOriginalPlanId() == a_original_plan_id) {
                ret_task = std::make_shared<Task>(*task);
                plan->getInternalGoalsSelectedPlans()[0].first->remove_action_from_body(i);
                break;
            }
        }
    }
    return ret_task;
}

void Intention::pop_internal_goal_plan(int index) {
    plan->pop_internal_goal_plan(index);
}

void Intention::pop_internal_goals(int index){
    if(index < (int)intention_internal_goals.size()) {
        intention_internal_goals[index].pop();

        if(intention_internal_goals[index].empty()) {
            intention_internal_goals.erase(intention_internal_goals.begin() + index);
        }
    }
}

/* add a new stack to the list of internal goals, dedicated to a new internal goal of the main plan
 * If the plan chosen for the internal goal has more sub-sub-plans, their data will be added to the stack of the respective goal */
void Intention::push_internal_goal(std::shared_ptr<Goal> goal, int original_plan_id, int counter) {
    std::stack<std::tuple<std::shared_ptr<Goal>, int, int>> stack;
    stack.push({goal, original_plan_id, counter});
    intention_internal_goals.push_back(stack);
}

void Intention::update_ith_top_internal_goal(int index, int counter) {
    if(index < (int)intention_internal_goals.size()) {
        std::get<2>(intention_internal_goals[index].top()) -= counter;
    }
}


void Intention::update_internal_goal_selectedPlans(std::shared_ptr<MaximumCompletionTime> selectedPlans) {
    plan->update_internal_goal_selectedPlans(selectedPlans);
    original_plan->update_original_plan_internal_goal_selectedPlans(selectedPlans);
}
