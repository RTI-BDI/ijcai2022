/*
 * Desire.h
 *
 *  Created on: Mar 2, 2020
 *      Author: francesco
 */

#ifndef HEADERS_DESIRE_H_
#define HEADERS_DESIRE_H_

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "Belief.h"
#include "Operator.h"
#include "Reference_table.h"

#include "json.hpp" // to read from json file
using json = nlohmann::json; // to read from json file

class Desire {
public:
    Desire(int _id, std::string _goal_name, json _preconditions, json _cont_conditions, double _ddl = -1);
    virtual ~Desire();

    // Getter methods
    int get_id();
    std::string get_goal_name();
    json get_preconditions();
    json get_cont_conditions();
    json get_priority_formula();
    double get_DDL();
    double get_priority();
    bool get_computedDynamically();
    std::vector<std::shared_ptr<Reference_table>> get_priority_reference_table();

    // Setter methods
    void set_computedDynamically(bool cd = true); // true: use priority_formula, false: use reference_table
    void set_DDL(double deadline);
    void set_priority_formula(const json _formula);
    void set_priority(double pr);
    void set_priority_reference_table(std::vector<std::shared_ptr<Reference_table>>& r_t);

protected:
    int id;
    std::string goal_name; // name of the SKILL from which this desire has been derived
    json formula;
    json preconditions;
    json cont_conditions;
    double priority;
    double ddl;
    bool isPriorityComputedDynamically; // true: use "formula"; false: use "reference_table"
    std::vector<std::shared_ptr<Reference_table>> reference_table; // beliefName, belief_threshold_value, priority, operator
};

#endif /* HEADERS_DESIRE_H_ */
