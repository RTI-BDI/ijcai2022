/*
 * MaximumCompletionTime.cc
 *
 *  Created on: Jun 11, 2020
 *      Author: francesco
 */

#include "../headers/MaximumCompletionTime.h"

MaximumCompletionTime::MaximumCompletionTime() {
    MCTvalue = 0;
}

MaximumCompletionTime::~MaximumCompletionTime() { }

// Getters
double MaximumCompletionTime::getMCTValue() {
    return MCTvalue;
}

int MaximumCompletionTime::getInternalGoalsSelectedPlansSize() {
    // Apparently vector.size returns an unsigned int value.
    // The problem is that unsigned int >= negative number is always false.
    // i.e., vector.size = 5 >= -1 is FALSE.
    // Casting unsigned int solve the problem.

    return (int) subplans.size();
}

std::vector<std::shared_ptr<Task>>& MaximumCompletionTime::getTaskSet() {
    return task_set;
}

std::vector<std::pair<std::shared_ptr<Plan>, int>>& MaximumCompletionTime::getInternalGoalsSelectedPlans() {
    return subplans;
}

// Setters
void MaximumCompletionTime::setMCTValue(double val) {
    MCTvalue = val;
}

// Methods
void MaximumCompletionTime::taskSet_push_back(std::vector<std::shared_ptr<Task>> tasks) {
    for(int t = 0; t < (int)tasks.size(); t++) {
        task_set.push_back(tasks[t]);
    }
}

void MaximumCompletionTime::taskSet_push_back(std::shared_ptr<Task> task) {
    task_set.push_back(task);
}

void MaximumCompletionTime::taskSet_clear() {
    task_set.clear();
}

void MaximumCompletionTime::pop_internal_goal_plan(int index) {
    // [0] it's always the main plan, from [1] to [n] are the subplans for the internal goals
    if((int)subplans.size() > index) {
        subplans.erase(subplans.begin() + index);
    }
}

void MaximumCompletionTime::add_internal_goal_selectedPlan(std::shared_ptr<Plan> plan, int annidation_level) {
    subplans.push_back(std::make_pair(plan->makeACopyOfPlan(), annidation_level));
}

void MaximumCompletionTime::add_internal_goals_selectedPlans(std::shared_ptr<MaximumCompletionTime>& plans) {
    std::vector<std::pair<std::shared_ptr<Plan>, int>> selectedPlans = plans->getInternalGoalsSelectedPlans();

    for(int i = 0; i < (int)selectedPlans.size(); i++) {
        subplans.push_back(selectedPlans[i]);
    }
}

void MaximumCompletionTime::update_internal_goal_selectedPlans(std::shared_ptr<MaximumCompletionTime> selectedPlans) {
    subplans.clear();
    std::vector<std::pair<std::shared_ptr<Plan>, int>> tmp = selectedPlans->getInternalGoalsSelectedPlans();

    for(int i = 0; i < (int)tmp.size(); i++) {
        subplans.push_back(std::make_pair(tmp[i].first->make_a_copy(), tmp[i].second));
    }
}

void MaximumCompletionTime::update_original_plan_internal_goal_selectedPlans(std::shared_ptr<MaximumCompletionTime> selectedPlans) {
    std::vector<std::pair<std::shared_ptr<Plan>, int>> selected = selectedPlans->getInternalGoalsSelectedPlans();
    std::vector<std::pair<std::shared_ptr<Plan>, int>> original = subplans;
    subplans.clear();

    bool usedOriginalPlan = false;
    for(int i = 0; i < (int)selected.size(); i++)
    {
        if(selected[i].first != nullptr)
        {
            usedOriginalPlan = false;
            for(int j = 0; j < (int)original.size(); j++)
            {
                if(original[j].first != nullptr)
                {
                    if(selected[i].second == original[j].second && selected[i].first->get_id() == original[j].first->get_id())
                    {
                        subplans.push_back(std::make_pair(original[j].first->make_a_copy(), original[j].second));
                        usedOriginalPlan = true;
                        break;
                    }
                }
            }

            if(usedOriginalPlan == false) {
                subplans.push_back(std::make_pair(selected[i].first->make_a_copy(), selected[i].second));
            }
        }
    }
}
