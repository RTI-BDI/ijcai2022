/*
 * task.h
 *
 *  Created on: Feb 21, 2017
 *      Author: davide
 */

#ifndef HEADERS_TASK_H_
#define HEADERS_TASK_H_

#include <cstdint>
#include <stdio.h>
#include <string.h>
#include "agentMSG_m.h"

#include "Action.h"

class Task : public Action {

public:
    enum Status {
        TSK_IDLE = 0,       // when gets created
        TSK_ACTIVE,         // when it's added to '-to-be-released', and it's waiting to be selected in order to become Ready
        TSK_READY,          // when moved from '-to-be-released' to 'ready' vector in order to be executed
        TSK_EXEC,           // when it's ACTIVATED and COMPLETION is scheduled
        TSK_COMPLETED       // task compTime is been performed and the task is between COMPLETION and DDL_check (when it waits the end of its period)
    };

    enum Execution {
        EXEC_SEQUENTIAL,
        EXEC_PARALLEL
    };

    Task(int nt_id, int nt_plan_id, int original_plan_id, int nt_Ag_Executer, int nt_Ag_Demander, double nt_C,
            double nt_CRes, double nt_R, double nt_R_initial, double nt_DDL,
            double nt_FirstActivation, double nt_LastActivation,
            int nt_Server, double nt_Period, int n_exec, int n_exec_initial, bool t_isBeenActivated,
            bool nt_isPeriodic, Status _status = TSK_IDLE, Execution _exec_type = EXEC_SEQUENTIAL);

    Task();
    virtual ~Task();

    std::string get_type() override; // override father class 'action'

    // Setter methods
    void setTaskId(int nt_id);
    void setTaskPlanId(int nt_plan_id);
    void setTaskOriginalPlanId(int nt_plan_id);
    void setTaskExecuter(int nt_Ag_Executer);
    void setTaskDemander(int nt_Ag_Demander);
    void setTaskCompTime(double nt_C);
    void setTaskCompTimeRes(double nt_CRes);
    void setTaskArrivalTime(double nt_R);
    void setTaskArrivalTimeInitial(double nt_R_initial); // BDI
    void setTaskDeadLine(double nt_DDL);
    void setTaskAbsDDL(double nt_DDL);
    void setTaskFirstActivation(double nt_FirstActivation);
    void setTaskLastActivation(double nt_LastActivation);
    void setTaskReleaseTime(double nt_release);
    void setTaskServer(int nt_Server);
    void setTaskPeriod(double nt_Period);
    void setTaskNExec(int n_exec);
    void setTaskisBeenActivated(bool nt_isBeenActivated); // BDI
    void setTaskStatus(Status status); // BDI
    void setTaskExecutionType(Execution exec_type); // BDI

    // Getter methods
    int getTaskId();
    int getTaskPlanId();
    int getTaskOriginalPlanId();
    int getTaskExecuter();
    int getTaskDemander();
    double getTaskCompTime();
    double getTaskCompTimeRes();
    double getTaskArrivalTime();
    double getTaskArrivalTimeInitial();
    double getTaskDeadline();
    double getTaskFirstActivation();
    double getTaskLastActivation();
    double getTaskReleaseTime();
    double getTaskAbsDDL();
    int getTaskServer() const;
    double getTaskPeriod();
    int getTaskNExec();
    int getTaskNExecInitial() const;
    bool getTaskisBeenActivated(); // BDI
    bool getTaskisPeriodic(); // BDI
    Status getTaskStatus(); // BDI
    std::string getTaskStatus_as_string(); // BDI
    Execution getTaskExecutionType(); // BDI
    std::string getTaskExecutionType_as_string(); // BDI

    //For the dynamic binding - Server
    virtual int get_server_id();
    virtual double get_bandwidth();
    virtual double get_budget();
    virtual double get_period();
    virtual double get_curr_budget();
    virtual double get_curr_ddl();
    virtual double get_startTime(); // BDI
    virtual int get_server_type();
    virtual void set_budget(double n_budget);
    virtual void set_period(double n_period);
    virtual void set_curr_budget(double budget);
    virtual void set_curr_ddl(double ddl);
    virtual void set_startTime(double t); // BDI
    virtual void push_back(std::shared_ptr<Task> task);
    virtual void reset(double t_now, double c_req = 0, bool initialize = false);
    virtual void update_budget(double t_comp);
    virtual void update_ddl();
    virtual void pop_head();
    virtual std::shared_ptr<Task> get_head();
    virtual std::shared_ptr<Task> get_ith_task(int ith = 0);
    virtual void remove_element_from_queue(int i);
    virtual void set_ith_task(int i, std::shared_ptr<Task> task);
    virtual bool is_server();
    virtual bool is_empty();
    virtual int queue_length();
    virtual void replenish(double amount);

    // BDI:
    std::shared_ptr<Task> makeCopyOfTask();
    std::shared_ptr<Task> cloneTask();

protected:
    int t_id;
    int t_plan_id;          // each task is identified by it's ID (unique within a plan) and the ID of the plan it's linked to
    int t_original_id;      // for each task derived from an internal goal will store here the original id of the choosen plan
    int t_Ag_Executer;      // agent executing the task
    int t_Ag_Demander;      // agent demanding the task
    double t_C;                // task computational time
    double t_CRes;             // residual task computational time
    double t_R;                // task arrival ~ release time
    double t_R_initial;        // original task arrival ~ release time // BDI
    double t_DDL;              // task deadline (from its server)
    double t_abs_ddl;          // task absolute deadline
    double t_FirstActivation;  // first time the C of a task is computed
    double t_LastActivation;   // computed C if preempted
    double t_release;          // time when the task is first release
    int t_Server;              // associated server
    double t_Period;           // possible period if required, 0 if not
    int t_n_exec;              // number of executions of the task (jobs) updated during simulation
    int t_n_exec_initial;      // number of executions of the task (jobs) value read from file and CONSTANT
    bool t_isBeenActivated;    // BDI: default: False. It becomes True as soon as phiI selects it from the 'top' of the stack of an intention
    Task::Status t_status;     // BDI
    Task::Execution t_exec_type;    // BDI
    bool t_isPeriodic;              // BDI: false if n_exec = 1, true otherwise. Needed in order to make function 'checkIfTaskIsPeriodicAndGetDeadline' work correctly
};

#endif /* HEADERS_TASK_H_ */
