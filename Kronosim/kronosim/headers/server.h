/*
 * CBS.h
 *
 *  Created on: 19 gen 2018
 *      Author: peppe
 */

#ifndef HEADERS_SERVER_H_
#define HEADERS_SERVER_H_

#include "task.h"
#include <vector>
#include <stdio.h>
#include <algorithm>

enum ServerType { CBS = 0 };

class Server : public Task {
protected:
    ServerType type;

    // Time value sensitivity (epsilon)
    const double ERROR = 0.005;
    const double DEF_BANDWIDTH = 1.0/6.0;
    const double DEF_BUDGET = 2.0;

    std::vector<std::shared_ptr<Task>> queue;
    int id;
    double bandwidth;
    double budget;
    double period;
    double curr_budget;
    double curr_ddl;
    double delay;
    double startTime; // BDI: when the server is activated for the first time, store here the time
    bool released;

public:
    Server(int);
    Server(int, double, double, ServerType st = CBS);

    virtual int get_server_id() override;

    virtual int get_server_type() override;
    virtual double get_bandwidth() override;
    virtual double get_budget() override;
    virtual double get_period() override;
    virtual double get_curr_budget() override;
    virtual double get_curr_ddl() override;
    virtual void set_budget(double n_budget) override;
    virtual void set_period(double n_period) override;
    virtual void set_curr_budget(double budget) override;
    virtual void set_curr_ddl(double ddl) override;
    virtual void reset(double t_now, double c_req, bool initialize = false) override; // BDI
    virtual void update_budget(double t_comp) override;
    virtual void update_ddl() override;
    virtual void replenish(double amount) override;
    virtual void push_back(std::shared_ptr<Task> task) override;
    virtual std::shared_ptr<Task> get_head() override;
    virtual std::shared_ptr<Task> get_ith_task(int ith = 0) override;
    virtual void set_ith_task(int,std::shared_ptr<Task> task) override;
    virtual void remove_element_from_queue(int i) override;
    virtual void pop_head() override;
    virtual bool is_server() override;
    virtual bool is_empty() override;
    virtual int queue_length() override;
    virtual double get_startTime() override;        // BDI
    virtual void set_startTime(double t) override;  // BDI
    virtual ~Server();
};

#endif /* HEADERS_SERVER_H_ */
