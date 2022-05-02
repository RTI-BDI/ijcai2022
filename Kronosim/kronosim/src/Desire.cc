/*
 * Desire.cc
 *
 *  Created on: Mar 2, 2020
 *      Author: francesco
 */
#include "../headers/Desire.h"

Desire::Desire(int _id, std::string _goal_name, json _preconditions, json _cont_conditions, double _ddl)
{
    id = _id;
    goal_name = _goal_name;
    preconditions = _preconditions;
    cont_conditions = _cont_conditions;
    priority = 0;
    ddl = _ddl;
}

Desire::~Desire() { }

// Getter methods
int Desire::get_id() {
    return id;
}

std::string Desire::get_goal_name() {
    return goal_name;
}

json Desire::get_preconditions() {
    return preconditions;
}

json Desire::get_cont_conditions() {
    return cont_conditions;
}

double Desire::get_priority(){
    return priority;
}

json Desire::get_priority_formula() {
    return formula;
}

std::vector<std::shared_ptr<Reference_table>> Desire::get_priority_reference_table() {
    return reference_table;
}

bool Desire::get_computedDynamically() {
    return isPriorityComputedDynamically;
}

double Desire::get_DDL() {
    return ddl;
}


// Setters
void Desire::set_DDL(double deadline) {
    ddl = deadline;
}

void Desire::set_computedDynamically(bool cd) {
    isPriorityComputedDynamically = cd;
}

void Desire::set_priority_formula(const json _formula) {
    formula = _formula;
}

void Desire::set_priority(double pr) {
   priority = pr;
}

void Desire::set_priority_reference_table(std::vector<std::shared_ptr<Reference_table>>& r_t) {
    reference_table = r_t;
}



