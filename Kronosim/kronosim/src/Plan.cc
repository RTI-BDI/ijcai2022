/*
 * Plan.cc
 *
 *  Created on: Mar 5, 2020
 *      Author: francesco
 */

#include "../headers/Plan.h"

Plan::Plan(int _id,
        std::string _goal_name,
        double _pref,
        json _preconditions,
        json _context_conditions,
        json _post_conditions,
        std::vector<std::shared_ptr<Effect>> _effects_at_begin,
        std::vector<std::shared_ptr<Effect>> _effects_at_end,
        std::vector<std::shared_ptr<Action>> _body)
{
    id = _id;
    goal_name = _goal_name;
    pref = _pref;
    preconditions = _preconditions;
    cont_conditions = _context_conditions;
    post_conditions = _post_conditions;
    effects_at_begin = _effects_at_begin;
    effects_at_end = _effects_at_end;
    body = _body;
}

Plan::Plan() {
    id = -1;
    goal_name = "";
    pref = -1;
    preconditions = json::array();
    cont_conditions = json::array();
    post_conditions = json::array();
    effects_at_begin = {};
    effects_at_end = {};
    body = {};
}

Plan::~Plan() { }

std::vector<std::shared_ptr<Action>> Plan::make_a_copy_of_body() {
    std::vector<std::shared_ptr<Action>> original_body = this->get_body();
    std::vector<std::shared_ptr<Action>> copy;
    std::shared_ptr<Task> task;
    std::shared_ptr<Goal> goal, action_goal;

    for(int i = 0; i < (int)original_body.size(); i++) {
        if(original_body[i]->get_type() == "task") {
            task = std::dynamic_pointer_cast<Task>(original_body[i]);

            copy.push_back(std::make_shared<Task>(task->getTaskId(), task->getTaskPlanId(), task->getTaskOriginalPlanId(),
                    task->getTaskExecuter(), task->getTaskDemander(), task->getTaskCompTime(),
                    task->getTaskCompTimeRes(), task->getTaskArrivalTime(), task->getTaskArrivalTimeInitial(),
                    task->getTaskDeadline(), task->getTaskFirstActivation(), task->getTaskLastActivation(),
                    task->getTaskServer(), task->getTaskPeriod(),
                    task->getTaskNExec(), task->getTaskNExecInitial(), task->getTaskisBeenActivated(), task->getTaskisPeriodic(),
                    task->getTaskStatus(), task->getTaskExecutionType()));
        }
        else {
            goal = std::dynamic_pointer_cast<Goal>(original_body[i]);

            action_goal = std::make_shared<Goal>(goal->get_id(),
                    id, id, goal->get_goal_name(),
                    goal->get_preconditions(), goal->get_cont_conditions(),
                    goal->get_priority(),
                    goal->get_activation_time(), goal->get_arrival_time(),
                    goal->get_status(), goal->get_DDL(), goal->getGoalExecutionType());
            action_goal->set_priority_formula(goal->get_priority_formula());

            copy.push_back(action_goal);
        }
    }

    return copy;
}

std::shared_ptr<Plan> Plan::make_a_copy() {
    std::shared_ptr<Plan> plan = std::make_shared<Plan>(
            this->id,
            this->goal_name,
            this->pref,
            this->preconditions, this->cont_conditions, this->post_conditions,
            this->effects_at_begin, this->effects_at_end,
            this->make_a_copy_of_body());

    return plan;
}

std::shared_ptr<Plan> Plan::makeACopyOfPlan() {
    std::shared_ptr<Plan> plan;

    plan = std::make_shared<Plan>(*this); // make a copy of the parameters
    plan->set_body(this->make_a_copy_of_body());

    return plan;
}

// Getter -----------------------------------------------------------------------------------------------
int Plan::get_id() {
    return id;
}

std::string Plan::get_goal_name() {
    return goal_name;
}

json Plan::get_preconditions() {
    return preconditions;
}

json Plan::get_cont_conditions() {
    return cont_conditions;
}

json Plan::get_post_conditions() {
    return post_conditions;
}

std::vector<std::shared_ptr<Effect>> Plan::get_effects_at_begin() {
    return effects_at_begin;
}

std::vector<std::shared_ptr<Effect>> Plan::get_effects_at_end() {
    return effects_at_end;
}

std::vector<std::shared_ptr<Action>> Plan::get_body() {
    return body;
}

void Plan::remove_action_from_body(int i) {
    if(body.size() > 0) {
        body.erase(body.begin() + i);
    }
}

double Plan::get_pref() {
    return pref;
}

bool Plan::get_computedDynamically() {
    return isPrefComputedDynamically;
}

json Plan::get_pref_formula() {
    return formula;
}

std::vector<std::shared_ptr<Reference_table>> Plan::get_pref_reference_table() {
    return reference_table;
}

// Setters -----------------------------------------------------------------------------------------------
void Plan::set_computedDynamically(bool cd) {
    isPrefComputedDynamically = cd;
}

void Plan::set_pref_formula(const json _formula) {
    formula = _formula;
}

void Plan::set_pref_reference_table(std::vector<std::shared_ptr<Reference_table>>& r_t) {
    reference_table = r_t;
}

void Plan::set_body(std::vector<std::shared_ptr<Action>> b) {
    body = b;
}

// Add item to body ...
void Plan::add_action_to_body(const std::shared_ptr<Action> action) {
    body.push_back(action);
}

void Plan::insert_action_to_body(const std::shared_ptr<Action> action, int pos) {
    body.insert(body.begin() + pos, action);
}


