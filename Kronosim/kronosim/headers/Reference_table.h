/*
 * Referencetable.h
 *
 *  Created on: Jun 15, 2020
 *      Author: francesco
 */

#ifndef HEADERS_REFERENCE_TABLE_H_
#define HEADERS_REFERENCE_TABLE_H_

#include <variant>
#include <string>

#include "Belief.h"
#include "Operator.h"

class Reference_table {
public:
    Reference_table(std::string belief_name, std::variant<int, double, bool, std::string> threshold_value, double pr_pref, Operator::Type op);
    virtual ~Reference_table();

    // Getters
    std::string get_belief_name();
    std::variant<int, double, bool, std::string> get_threshold_value();
    double get_pr_pref();
    Operator::Type get_operator();

protected:
    std::string belief_name;
    std::variant<int, double, bool, std::string> threshold_value;
    double pr_pref;         // for goals, use as priority. for plans, use as preference
    Operator::Type op;      // operator (EQUAL, LESS ...)
};

#endif /* HEADERS_REFERENCE_TABLE_H_ */
