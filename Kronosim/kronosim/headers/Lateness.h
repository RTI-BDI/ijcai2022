/*
 * Lateness.h
 *
 *  Created on: May 27, 2020
 *      Author: francesco
 */

#ifndef HEADERS_LATENESS_H_
#define HEADERS_LATENESS_H_

#include <string>

class Lateness {
public:
    /** Called whenever a task is completed after its deadline */
    Lateness(double _time, int _task_id, int _plan_id, int _original_plan_id, int _N_exec, double _lateness, std::string _log);
    virtual ~Lateness();

    // Getters
    int getTaskId();
    int getTaskPlanId();
    int getTaskOriginalPlanId();
    int getTaskN_exec();
    double getTaskTime();
    double getTaskLateness();
    std::string getTaskLatenessLog();

protected:
    int task_id;
    int plan_id;
    int original_plan_id;
    int N_exec;
    double time;
    double lateness;
    std::string latenessLog;
};

#endif /* HEADERS_LATENESS_H_ */
