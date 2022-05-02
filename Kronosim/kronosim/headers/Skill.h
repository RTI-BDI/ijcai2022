/*
 * Skill.h
 *
 *  Created on: May 4, 2021
 *      Author: francesco
 */

#ifndef HEADERS_SKILL_H_
#define HEADERS_SKILL_H_

#include <iostream> // debug (per poter usare std::cout)
#include <omnetpp.h>
#include <string>

using namespace omnetpp; // in order to use EV

class Skill {
public:
    Skill(std::string _goal_name);
    virtual ~Skill();

    // Getter methods
    std::string get_goal_name();

private:
    std::string goal_name;
};

#endif /* HEADERS_SKILL_H_ */
