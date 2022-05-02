/*
 * server.cc
 *
 *  Created on: 19 gen 2018
 *      Author: peppe
 */
#include "../headers/server.h"
#include <omnetpp.h>

using namespace std;
using namespace omnetpp;

Server::Server(int server_id) : Task() {
    t_id = server_id;
    t_Ag_Executer = -1;
    t_Ag_Demander = -1;
    t_C = 0;
    t_CRes = 0;
    t_R = 0;
    t_R_initial = 0;
    t_DDL = 0;
    t_FirstActivation = -1;
    t_LastActivation = -1;
    t_Server = server_id;
    t_Period = 0;
    t_release = -1;

    this->id = server_id;
    this->bandwidth = DEF_BANDWIDTH;
    this->budget = DEF_BUDGET;
    this->period = budget / bandwidth;
    this->curr_budget = budget;
    this->curr_ddl = period;
    this->released = false;
    this->t_n_exec = -1;
    this->t_n_exec_initial = -1;
}

Server::Server(int server_id, double n_period, double n_budget,
        ServerType n_type) : Task()
{
    t_id = server_id;
    t_Ag_Executer = -1;
    t_Ag_Demander = -1;
    t_C = 0;
    t_CRes = 0;
    t_R = 0;
    t_R_initial = 0;
    t_DDL = 0;
    t_FirstActivation = -1;
    t_LastActivation = -1;
    t_Server = server_id;
    t_Period = 0;
    t_release = -1;

    this->id = server_id;
    this->type = n_type;
    this->bandwidth = n_budget / n_period;
    this->budget = n_budget;
    this->period = n_period;
    this->curr_budget = budget;
    this->curr_ddl = period;
    this->delay = 0;
    this->released = false;
    this->t_n_exec = -1;
    this->t_n_exec_initial = -1;
}

Server::~Server() { }

int Server::get_server_id() {
    return id; // in order to return the original server id, and not the id of the head task in the queue
}

double Server::get_bandwidth() {
    return bandwidth;
}

double Server::get_budget() {
    return budget;
}

double Server::get_period() {
    return period;
}

double Server::get_curr_budget() {
    return this->curr_budget;
}

double Server::get_curr_ddl() {
    return curr_ddl;
}

int Server::get_server_type() {
    return this->type;
}

void Server::set_budget(double n_budget) {
    budget = n_budget;
}

void Server::set_period(double n_period) {
    period = n_period;
}

void Server::set_curr_budget(double budget) {
    curr_budget = budget;
}

void Server::set_curr_ddl(double ddl) {
    curr_ddl = ddl;
}

void Server::push_back(std::shared_ptr<Task> task) {
    queue.push_back(task);
}

// NEW VERSION: 11/01/2021
// called when the budget reaches 0 or when the curr_ddl of the server is reached
void Server::reset(double t_now, double c_req, bool initialize) {
    if(initialize == false) {
        if (fabs(curr_budget) < ERROR) { // check_task_complition called reset. If the curr_budget is 0, reset it. Otherwise, do nothing
            curr_budget = budget;
            curr_ddl = curr_ddl + period;
        }
    }
    else { // schedule_taskset_rt called reset on a new server ('new' means not already in release or ready)
        curr_budget = budget;
        curr_ddl = t_now + period;
    }
}

void Server::replenish(double amount) {
    curr_budget += amount;
}

void Server::update_budget(double t_comp) {
    curr_budget -= t_comp;
}

void Server::update_ddl() {
    if (!queue.empty()) {
        for (auto &task : queue) {
            task->setTaskDeadLine(curr_ddl);
        }
    }
}

std::shared_ptr<Task> Server::get_head() {
    std::shared_ptr<Task> temp = nullptr;
    if (!queue.empty()) {
        temp = queue.front();
    }
    return temp;
}

std::shared_ptr<Task> Server::get_ith_task(int ith) {
    for(int i = 0; i < (int)queue.size(); i++) {
        if(i == ith) {
            return queue[i];
        }
    }
    return nullptr;
}

void Server::set_ith_task(int ith,std::shared_ptr<Task> task) {
    for(int i = 0; i < (int)queue.size(); i++) {
        if(i == ith) {
            queue[i] = task;
        }
    }
}

void Server::remove_element_from_queue(int i) {
    if(i >= 0 && i < (int)queue.size()) {
        queue.erase(queue.begin() + i);
    }
}

void Server::pop_head() {
    if (queue.size() > 1) {
        queue.erase(queue.begin());
    } else if (queue.size() == 1) {
        queue.clear();
    }
}

bool Server::is_server() {
    return true;
}

bool Server::is_empty() {
    return queue.empty();
}

int Server::queue_length() {
    return queue.size();
}

// BDI
double Server::get_startTime() {
    return startTime;
}

// BDI
void Server::set_startTime(double t) {
    startTime = t;
}
