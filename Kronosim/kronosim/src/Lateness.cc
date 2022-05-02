/*
 * Lateness.cc
 *
 *  Created on: May 27, 2020
 *      Author: francesco
 */

#include "../headers/Lateness.h"

Lateness::Lateness(double _time, int _task_id, int _plan_id, int _original_plan_id, int _N_exec, double _lateness, std::string _log) {
    time = _time;
    task_id = _task_id;
    plan_id = _plan_id;
    original_plan_id = _original_plan_id;
    N_exec = _N_exec;
    lateness = _lateness;
    latenessLog = _log;
}

Lateness::~Lateness() { }

int Lateness::getTaskId() {
    return task_id;
}

int Lateness::getTaskPlanId() {
    return plan_id;
}

int Lateness::getTaskOriginalPlanId() {
    return original_plan_id;
}

int Lateness::getTaskN_exec() {
    return N_exec;
}

double Lateness::getTaskTime() {
    return time;
}

double Lateness::getTaskLateness() {
    return lateness;
}

std::string Lateness::getTaskLatenessLog() {
    return latenessLog;
}



