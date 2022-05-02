/*
 * Sensor.cc
 *
 *  Created on: Apr 16, 2020
 *      Author: francesco
 */

#include "../headers/Sensor.h"

Sensor::Sensor(int _id, std::string _belief_name, std::variant<int, double, bool, std::string> _value, double _time, Sensor::Mode _mode)
{
    id = _id;
    belief_name = _belief_name;
    value = _value;
    time = _time;
    mode = _mode;

    scheduled = false;
}

Sensor::~Sensor() { }

// Getters:
int Sensor::get_id() {
    return id;
}

std::string Sensor::get_belief_name() {
    return belief_name;
}

std::variant<int, double, bool, std::string> Sensor::get_value() {
    return value;
}

double Sensor::get_time() {
    return time;
}

Sensor::Mode Sensor::get_mode() {
    return mode;
}

std::string Sensor::get_mode_as_string() {
    if(mode == Mode::DECREASE) {
        return "DECREASE";
    }
    else if(mode == Mode::INCREASE) {
        return "INCREASE";
    }
    else if(mode == Mode::SET) {
        return "SET";
    }

    return "undefined";
}

bool Sensor::get_scheduled() {
    return scheduled;
}

// Setters:
void Sensor::set_scheduled(bool val) {
    scheduled = true;
}


