/*
 * read_task.h
 *
 *  Created on: May 4, 2018
 *      Author: darth
 */

#ifndef HEADERS_MSG_TASK_H_
#define HEADERS_MSG_TASK_H_

#include "task.h"

class MsgTask : public Task {
private:
    const double DEF_COMP_TIME = 1;
    const double DEF_DDL = 10000;

    const char* type;
    agentMSG* message;

public:
    MsgTask(agentMSG* msg, int ag_id, int server, double t_now, const char* type);
    virtual ~MsgTask();

    bool is_msg_task();
    agentMSG* getMessage();
    void setMessage(agentMSG* n_msg);
    const char* getType();
};


#endif /* HEADERS_MSG_TASK_H_ */
