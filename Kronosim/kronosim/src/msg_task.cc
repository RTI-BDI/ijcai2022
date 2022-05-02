/*
 * msg_task.cc
 *
 *  Created on: May 4, 2018
 *      Author: darth
 */

#include "../headers/msg_task.h"

MsgTask::MsgTask(agentMSG* msg, int ag_id, int server, double t_now, const char* type) {
    this->message = msg;
    this->type = type;

    t_id = msg->getId();
    t_Ag_Executer = ag_id;
    t_Ag_Demander = ag_id;
    t_C = DEF_COMP_TIME;
    t_CRes = t_C;
    t_R = t_now;
    t_DDL = DEF_DDL;
    t_FirstActivation = -1;
    t_LastActivation = -1;
    t_Server = server;
    t_Period = 0;
    t_release = -1;
    t_abs_ddl = t_R + t_DDL;
    t_n_exec = 1;
}

MsgTask::~MsgTask() {
    delete message;
    delete type; // free memory of char*
}

bool MsgTask::is_msg_task(){
    return true;
}

const char* MsgTask::getType(){
    return this->type;
}

void MsgTask::setMessage(agentMSG* n_msg){
    this->message = n_msg;
}

agentMSG* MsgTask::getMessage(){
    return this->message;
}
