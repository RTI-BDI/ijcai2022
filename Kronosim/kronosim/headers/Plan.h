/*
 * Plan.h
 *
 *  Created on: Mar 5, 2020
 *      Author: francesco
 */

#ifndef HEADERS_PLAN_H_
#define HEADERS_PLAN_H_

#include <memory>
#include <vector>
#include <stack>
#include <boost/core/demangle.hpp>

#include "Action.h"
#include "Belief.h"
#include "Effect.h"
#include "Operator.h"
#include "Reference_table.h"
#include "task.h"
#include "Goal.h"

class Plan {
    // Note: .... variable declared here are considered 'private'

public:
    Plan(int _id, std::string _goal_name,
            double _pref,
            json _preconditions,
            json _context_conditions,
            json _post_conditions,
            std::vector<std::shared_ptr<Effect>> _effects_at_begin,
            std::vector<std::shared_ptr<Effect>> _effects_at_end,
            std::vector<std::shared_ptr<Action>> _body);

    // used by IsSchedulable and computeGoalDDL whenever an already achieved goal/internal goal is found
    Plan();

    virtual ~Plan();
    std::vector<std::shared_ptr<Action>> make_a_copy_of_body();
    std::shared_ptr<Plan> make_a_copy();
    std::shared_ptr<Plan> makeACopyOfPlan();

    // Getter methods
    int get_id();
    std::string get_goal_name();

    json get_preconditions();
    json get_cont_conditions();
    json get_post_conditions();
    std::vector<std::shared_ptr<Effect>> get_effects_at_begin();
    std::vector<std::shared_ptr<Effect>> get_effects_at_end();

    std::vector<std::shared_ptr<Action>> get_body();
    void remove_action_from_body(int i = 0);

    double get_pref();
    bool get_computedDynamically();
    json get_pref_formula();
    std::vector<std::shared_ptr<Reference_table>> get_pref_reference_table();

    // Setter methods
    void set_computedDynamically(bool cd = true); // true: use pref_formula, false: use reference_table
    void set_pref_formula(const json _formula);
    void set_pref_reference_table(std::vector<std::shared_ptr<Reference_table>>& r_t);
    void set_body(std::vector<std::shared_ptr<Action>> b);

    /** Add item to body */
    void add_action_to_body(const std::shared_ptr<Action> action);
    void insert_action_to_body(const std::shared_ptr<Action> action, int pos);

protected:
    int id;
    std::string goal_name;  // name of the SKILL for which this plan is responsible

    double pref;
    bool isPrefComputedDynamically; // TRUE -> use formula; FALSE -> use reference table
    json formula;
    std::vector<std::shared_ptr<Reference_table>> reference_table; // beliefName, belief_threshold_value, pref, operator

    json preconditions;    // plan's preconditions (check only when the planner is looking for which plan to use in order to achieve a given goal)
    json cont_conditions;  // context preconditions (check for those plans that where already intention before the actual reasoning cycle)
    json post_conditions;  // checked when the plan completes, just after "effects_at_end" has been applied

    std::vector<std::shared_ptr<Effect>> effects_at_begin;  // update beliefset when plan becomes intention
    std::vector<std::shared_ptr<Effect>> effects_at_end;    // update beliefset when intention is accomplished

    std::vector<std::shared_ptr<Action>> body;  // list of Actions that define the plan (each element can either be a Goal or a Task)
};

#endif /* HEADERS_PLAN_H_ */
