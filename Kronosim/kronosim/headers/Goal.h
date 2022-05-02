/*
 * Goal.h
 *
 *  Created on: Mar 3, 2020
 *      Author: francesco
 */

#ifndef HEADERS_GOAL_H_
#define HEADERS_GOAL_H_

#include <memory>
#include "Action.h"
#include "Belief.h"
#include "Reference_table.h"

#include "json.hpp"             // to read json file
using json = nlohmann::json;    // to read json file

class Goal : public Action {
public:
    enum Status { ACTIVE, IDLE, INACTIVE };

    enum Execution {
        /* EXEC_NONE: used as default value in the goal constructor.
         * Must be used when we convert desires to goals.
         * It means that the goal was a desire and that will not directly be executed.
         * INTERNAL GOALS must all be either SEQ or PAR! */
        EXEC_NONE,
        EXEC_SEQUENTIAL,
        EXEC_PARALLEL
    };

    Goal(int _id, int _plan_id, int _original_plan_id,
            std::string _goal_name,
            json _preconditions, json _cont_conditions,
            double _priority, double activation, double arrivalTime,
            Status _status = IDLE, double _ddl = -1.0,
            Execution _exec_type = EXEC_NONE);
    virtual ~Goal();

    /** Returns: -1 if the desire is not in goalset; otherwise, the position of the goal in the goalset */
    int is_desire_in_goalset(const std::vector<std::shared_ptr<Goal>>& goalset);

    // Getter methods
    std::string get_type() override;
    int get_id();
    int get_plan_id();
    int get_original_plan_id();
    int get_selected_plan_id();
    std::string get_goal_name();

    json get_preconditions();
    json get_cont_conditions();

    double get_priority();
    json get_priority_formula();

    Status get_status();
    double get_DDL();
    Execution getGoalExecutionType();
    std::string getGoalExecutionType_as_string();
    std::string getGoalStatus_as_string();
    bool getGoalisBeenActivated();
    bool getGoalforceReasoningCycle();
    double get_activation_time();
    double get_arrival_time();
    double get_absolute_arrival_time();

    // Setter methods
    void set_plan_id(int id);
    void set_priority_formula(json _formula);
    void set_priority(double pr);
    void setGoalExecutionType(Execution _exec_type);
    void setGoalisBeenActivated(bool activated);
    void setGoalforceReasoningCycle();
    void setActivation_time(double t);
    void set_absolute_arrival_time(double abs_time);
    void set_selected_plan_id(int id);

protected:
    int id;
    int plan_id;
    int original_plan_id;
    std::string goal_name;

    json preconditions;
    json cont_conditions;

    double priority;
    json priority_formula;

    Goal::Status status;
    Goal::Execution execution_type;

    /* Optional parameter that defines the deadline of the desire. Default = -1 = inf
    * Note: not needed for internal goals */
    double ddl;
    double activation_time;         // set when the goal is activated and becomes and intention
    double arrivalTime;             // delay between when it's activated and when it gets analyzed
    double absolute_arrivalTime;    // computed when the internal goal gets analyzed in phiI

    bool isBeenActivated;       // true when it gets selected from phiI and it's of type SEQ (and thus it's left on top of the main intention)
    bool forcedReasoningCycle;  // true when the goal is an internal goal and it lead to force reasoning cycle

    int selected_plan_id;   // id of the plan selected for accomplish the goal
};

#endif /* HEADERS_GOAL_H_ */
