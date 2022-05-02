/*
 * MaximumCompletionTime.h
 *
 *  Created on: Jun 11, 2020
 *      Author: francesco
 */

#ifndef HEADERS_MAXIMUMCOMPLETIONTIME_H_
#define HEADERS_MAXIMUMCOMPLETIONTIME_H_

#include "task.h"
#include "Plan.h"

class MaximumCompletionTime {
public:
    MaximumCompletionTime();
    virtual ~MaximumCompletionTime();

    // Getter methods
    double getMCTValue();
    int getInternalGoalsSelectedPlansSize();
    std::vector<std::shared_ptr<Task>>& getTaskSet();
    std::vector<std::pair<std::shared_ptr<Plan>, int>>& getInternalGoalsSelectedPlans();

    // Setter methods
    void setMCTValue(double);

    // Methods 'subplans' the reference to the plan chosen for the specific internal goal.
    void taskSet_push_back(std::vector<std::shared_ptr<Task>>);     // used by CalcDDL and computeGoalDDL
    void taskSet_push_back(std::shared_ptr<Task>);                  // used by CalcDDL and computeGoalDDL
    void taskSet_clear();            // used by CalcDDL and computeGoalDDL
    void taskSet_remove(int i);      // used by pop_Action in Intention.cc

    /** When an internal goal completes, remove it's plan from the list of sub-plans of the main goal */
    void pop_internal_goal_plan(int index);

    void add_internal_goal_selectedPlan(std::shared_ptr<Plan> plan, int annidation_level);

    /** Add the plan selected for the internal goal of main plan
    * + if the body contains internal goals (internal-internal goals), add them in the correct positions */
    void add_internal_goals_selectedPlans(std::shared_ptr<MaximumCompletionTime>& plans);
    void update_internal_goal_selectedPlans(std::shared_ptr<MaximumCompletionTime> selectedPlans);
    void update_original_plan_internal_goal_selectedPlans(std::shared_ptr<MaximumCompletionTime> selectedPlans);

protected:
    double MCTvalue; // overall deadline of the plan
    std::vector<std::shared_ptr<Task>> task_set; // set of tasks picked for the main and the subgols plans

    /* Vector of sub-plans selected to accomplish the internal goals of the main plan.
     * structure: [0] main plan
     *            [1]...[n] plans for the N sub-goals (and sub-sub-goals) 
     *                      that will derive from the main plan */
    std::vector<std::pair<std::shared_ptr<Plan>, int>> subplans;
};

#endif /* HEADERS_MAXIMUMCOMPLETIONTIME_H_ */
