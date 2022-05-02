/*
 * Reference_table.cc
 *
 *  Created on: Jun 15, 2020
 *      Author: francesco
 */

#include "../headers/Reference_table.h"

Reference_table::Reference_table(std::string _belief_name,
        std::variant<int, double, bool, std::string> _threshold_value,
        double _pr_pref, Operator::Type _op)
{
    belief_name = _belief_name;
    threshold_value = _threshold_value;
    pr_pref = _pr_pref;
    op = _op;
}

Reference_table::~Reference_table() { }

// Getters
std::string Reference_table::get_belief_name() {
    return belief_name;
}

std::variant<int, double, bool, std::string> Reference_table::get_threshold_value() {
    return threshold_value;
}

double Reference_table::get_pr_pref() {
    return pr_pref;
}

Operator::Type Reference_table::get_operator() {
    return op;
}
