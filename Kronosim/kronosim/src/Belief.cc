/*
 * Belief.cc
 *
 *  Created on: Mar 2, 2020
 *      Author: francesco
 */
#include "../headers/Belief.h"

Belief::Belief(std::string _name, std::variant<int, double, bool, std::string> _value) {
    name = _name;
    value = _value;
}

Belief::~Belief(){ }

std::string Belief::get_name() {
    return name;
}

std::variant<int, double, bool, std::string> Belief::get_value() {
    return value;
}

// Setter methods
void Belief::set_value(std::variant<int, double, bool, std::string> _value) {
    value = _value;
}
