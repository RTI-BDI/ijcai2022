/*
 * Skill.cc
 *
 *  Created on: May 4, 2021
 *      Author: francesco
 */

#include "../headers/Skill.h"

Skill::Skill(std::string _goal_name) {
    goal_name = _goal_name;
}

Skill::~Skill() { }

// Getters
std::string Skill::get_goal_name() {
    return goal_name;
}
