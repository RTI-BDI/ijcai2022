/*
 * task.cc
 *
 *  Created on: Feb 21, 2017
 *      Author: davide
 */

#include "../headers/task.h"

Task::Task(int nt_id, int nt_plan_id, int original_plan_id, int nt_Ag_Executer, int nt_Ag_Demander, double nt_C,
        double nt_CRes, double nt_R, double nt_R_initial, double nt_DDL, double nt_FirstActivation,
        double nt_LastActivation, int nt_Server,
        double nt_Period, int n_exec, int n_exec_initial, bool nt_isBeenActivated, bool nt_isPeriodic, Task::Status _status, Task::Execution _exec_type)
{
    t_id = nt_id;
    t_plan_id = nt_plan_id;
    t_original_id = original_plan_id;
    t_Ag_Executer = nt_Ag_Executer;
    t_Ag_Demander = nt_Ag_Demander;
    t_C = nt_C;
    t_CRes = nt_CRes;
    t_R = nt_R;
    t_R_initial = nt_R_initial;
    t_DDL = nt_DDL;
    t_FirstActivation = nt_FirstActivation;
    t_LastActivation = nt_LastActivation;
    t_Server = nt_Server;
    t_Period = nt_Period;
    t_release = -1;
    t_abs_ddl = t_R + t_DDL;
    t_n_exec = n_exec;
    t_n_exec_initial = n_exec_initial;
    t_isBeenActivated = nt_isBeenActivated;
    t_status = _status;
    t_exec_type = _exec_type;
    t_isPeriodic = nt_isPeriodic;
}

Task::Task() { }

Task::~Task() { }

// setter methods
void Task::setTaskId(int nt_id) {
    t_id = nt_id;
}

void Task::setTaskPlanId(int nt_plan_id) {
    t_plan_id = nt_plan_id;
}

void Task::setTaskOriginalPlanId(int nt_plan_id) {
    t_original_id = nt_plan_id;
}

void Task::setTaskExecuter(int nt_Ag_Executer) {
    t_Ag_Executer = nt_Ag_Executer;
}
void Task::setTaskDemander(int nt_Ag_Demander) {
    t_Ag_Demander = nt_Ag_Demander;
}
void Task::setTaskCompTime(double nt_C) {
    t_C = nt_C;
}

void Task::setTaskCompTimeRes(double nt_CRes) {
    t_CRes = nt_CRes;
}

void Task::setTaskArrivalTime(double nt_R) {
    t_R = nt_R;
}

void Task::setTaskArrivalTimeInitial(double nt_R_initial) {
    t_R_initial = nt_R_initial;
}
void Task::setTaskDeadLine(double nt_DDL) {
    t_DDL = nt_DDL;
}
void Task::setTaskAbsDDL(double nt_DDL) {
    t_abs_ddl = nt_DDL;
}

void Task::setTaskFirstActivation(double nt_FirstActivation) {
    t_FirstActivation = nt_FirstActivation;
}

void Task::setTaskLastActivation(double nt_LastActivation) {
    t_LastActivation = nt_LastActivation;
}

void Task::setTaskReleaseTime(double nt_release) {
    t_release = nt_release;
}

void Task::setTaskServer(int nt_Server) {
    t_Server = nt_Server;
}
void Task::setTaskPeriod(double nt_Period) {
    t_Period = nt_Period;
}

void Task::setTaskNExec(int n_exec) {
    t_n_exec = n_exec;
}

void Task::setTaskisBeenActivated(bool nt_isBeenActivated) {
    t_isBeenActivated = nt_isBeenActivated;
}

void Task::setTaskStatus(Task::Status status) {
    t_status = status;
}

void Task::setTaskExecutionType(Task::Execution exec_type) {
    t_exec_type = exec_type;
}

// Getter methods
int Task::getTaskId() {
    return t_id;
}

int Task::getTaskPlanId() {
    return t_plan_id;
}

int Task::getTaskOriginalPlanId() {
    return t_original_id;
}

int Task::getTaskExecuter() {
    return t_Ag_Executer;
}

int Task::getTaskDemander() {
    return t_Ag_Demander;
}

double Task::getTaskCompTime() {
    return t_C;
}

double Task::getTaskCompTimeRes() {
    return t_CRes;
}

double Task::getTaskArrivalTime() {
    return t_R;
}

double Task::getTaskArrivalTimeInitial() {
    return t_R_initial;
}

double Task::getTaskDeadline() {
    return t_DDL;
}

double Task::getTaskAbsDDL() {
    return t_abs_ddl;
}

double Task::getTaskFirstActivation() {
    return t_FirstActivation;
}

double Task::getTaskLastActivation() {
    return t_LastActivation;
}

double Task::getTaskReleaseTime() {
    return t_release;
}

int Task::getTaskServer() const {
    return t_Server;
}
double Task::getTaskPeriod() {
    return t_Period;
}

int Task::getTaskNExec() {
    return t_n_exec;
}

int Task::getTaskNExecInitial() const {
    return t_n_exec_initial;
}

// BDI
bool Task::getTaskisBeenActivated() {
    return t_isBeenActivated;
}

// BDI
bool Task::getTaskisPeriodic() {
    return t_isPeriodic;
}

// BDI
Task::Status Task::getTaskStatus() {
    return t_status;
}

// BDI
std::string Task::getTaskStatus_as_string() {
    switch(t_status) {
    case Task::TSK_IDLE: return "IDLE";
    case Task::TSK_ACTIVE: return "ACTIVE";
    case Task::TSK_READY: return "READY";
    case Task::TSK_EXEC: return "EXEC";
    case Task::TSK_COMPLETED: return "COMPLETED";
    default:
        return "not-valid";
    }
}

Task::Execution Task::getTaskExecutionType() { // BDI
    return t_exec_type;
}

std::string Task::getTaskExecutionType_as_string() { // BDI
    switch(t_exec_type) {
    case Task::EXEC_SEQUENTIAL: return "SEQUENTIAL";
    case Task::EXEC_PARALLEL: return "PARALLEL";
    default:
        return "not-valid";
    }
}

// BDI
std::string Task::get_type() {
    return "task";
}

//Binding dinamico - Server
int Task::get_server_id() {
    return -1;
}

double Task::get_bandwidth() {
    return 0;
}

double Task::get_budget() {
    return -1;
}

double Task::get_period() {
    return -1;
}

double Task::get_curr_budget() {
    return -1;
}

double Task::get_curr_ddl() {
    return 0;
}

// BDI
double Task::get_startTime() {
    return -1;
}

int Task::get_server_type() {
    return -1;
}

void Task::set_budget(double n_budget) { }

void Task::set_period(double n_period) { }

void Task::set_curr_budget(double budget) { }

void Task::set_curr_ddl(double ddl) { }

// BDI
void Task::set_startTime(double t) { }

void Task::push_back(std::shared_ptr<Task> task) { }

void Task::reset(double t_now, double c_req, bool initialize) { }

void Task::update_budget(double t_comp) { }

void Task::update_ddl() { }

void Task::pop_head() { }

std::shared_ptr<Task> Task::get_head() {
    return nullptr;
}

std::shared_ptr<Task> Task::get_ith_task(int i) {
    return nullptr;
}

void Task::remove_element_from_queue(int i) { }

void Task::set_ith_task(int i, std::shared_ptr<Task> task) { }

bool Task::is_server() {
    return false;
}

bool Task::is_empty() {
    return false;
}

int Task::queue_length() {
    return -1;
}

void Task::replenish(double amount) { }

// BDI:
std::shared_ptr<Task> Task::makeCopyOfTask() {
    return std::make_shared<Task>(*this); // make a completely new copy of the pointer
}

std::shared_ptr<Task> Task::cloneTask() {
    std::shared_ptr<Task> n_task = std::make_shared<Task>(*this);
    n_task->setTaskAbsDDL(this->getTaskAbsDDL());

    return n_task;
}

