/*
 * Goal.cc
 *
 *  Created on: Mar 3, 2020
 *      Author: francesco
 */
#include "../headers/Goal.h"

Goal::Goal(int _id, int _plan_id, int _original_plan_id,
        std::string _goal_name,
        json _preconditions, json _cont_conditions,
        double _priority, double activation, double _arrivalTime,
        Goal::Status _status, double _ddl, Goal::Execution _exec_type)
{
    id = _id;
    plan_id = _plan_id;
    original_plan_id = _original_plan_id;
    goal_name = _goal_name;

    preconditions = _preconditions;
    cont_conditions = _cont_conditions;
    priority = _priority;
    status = _status;
    ddl = _ddl;
    execution_type = _exec_type;
    activation_time = activation;
    arrivalTime = _arrivalTime;
    absolute_arrivalTime = -1; // it will be set by phiI

    isBeenActivated = false;
    forcedReasoningCycle = false;

    selected_plan_id = -1; // update only for internal goals
}

Goal::~Goal() { }

int Goal::is_desire_in_goalset(const std::vector<std::shared_ptr<Goal>>& goalset) {
    for(int i = 0; i < (int)goalset.size(); i++) {
        if(this->get_id() == goalset[i]->get_id()
                && this->get_goal_name() == goalset[i]->get_goal_name())
        {
            return i;
        }
    }

    return -1;
}

// Getter methods
int Goal::get_id() {
    return id;
}

int Goal::get_plan_id() {
    return plan_id;
}

int Goal::get_original_plan_id() {
    return original_plan_id;
}

int Goal::get_selected_plan_id() {
     return selected_plan_id;
}

std::string Goal::get_goal_name() {
    return goal_name;
}

json Goal::get_preconditions() {
    return preconditions;
}

json Goal::get_cont_conditions() {
    return cont_conditions;
}

double Goal::get_priority() {
    return priority;
}

Goal::Status Goal::get_status(){
    return status;
}

double Goal::get_DDL() {
    return ddl;
}

Goal::Execution Goal::getGoalExecutionType() {
    return execution_type;
}

std::string Goal::getGoalExecutionType_as_string() {
    switch(execution_type) {
    case Goal::EXEC_NONE: return "DESIRE-NONE";
    case Goal::EXEC_SEQUENTIAL: return "SEQUENTIAL";
    case Goal::EXEC_PARALLEL: return "PARALLEL";
    default:
        return "not-valid";
    }
}

std::string Goal::getGoalStatus_as_string() { // BDI
    switch(status) {
    case Goal::ACTIVE: return "ACTIVE";
    case Goal::IDLE: return "IDLE";
    case Goal::INACTIVE: return "INACTIVE";
    default:
        return "not-valid";
    }
}

bool Goal::getGoalisBeenActivated() {
    return isBeenActivated;
}

bool Goal::getGoalforceReasoningCycle() {
    return forcedReasoningCycle;
}

json Goal::get_priority_formula() {
    return priority_formula;
}

double Goal::get_activation_time() {
    return activation_time;
}

double Goal::get_arrival_time() {
    return arrivalTime;
}

double Goal::get_absolute_arrival_time() {
    return absolute_arrivalTime;
}

std::string Goal::get_type() {
    return "goal";
}

// Setters
void Goal::set_plan_id(int id) {
    plan_id = id;
    original_plan_id = id;
}

void Goal::set_priority_formula(json _formula) {
    priority_formula = _formula;
}

void Goal::set_priority(double pr) {
    priority = pr;
}

void Goal::setGoalExecutionType(Goal::Execution _exec_type) {
    execution_type = _exec_type;
}

void Goal::setGoalisBeenActivated(bool activated) {
    isBeenActivated = activated;
}

void Goal::setGoalforceReasoningCycle() {
    forcedReasoningCycle = true;
}

void Goal::setActivation_time(double t)  {
    activation_time = t;
}

void Goal::set_absolute_arrival_time(double abs_time) {
    absolute_arrivalTime = abs_time;
}

void Goal::set_selected_plan_id(int id) {
    selected_plan_id = id;
}







