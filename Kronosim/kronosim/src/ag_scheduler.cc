/*
 * ag_scheduler.cc
 *
 *  Created on: Apr 6, 2017
 *      Author: davide
 */

#include "../headers/ag_scheduler.h"

Ag_Scheduler::Ag_Scheduler(SchedType type, int _logger_level, double quantum) {
    this->type = type;
    this->quantum = quantum;
    this->curr_quantum = quantum;

    logger = std::make_shared<Logger>(_logger_level);
}

Ag_Scheduler::~Ag_Scheduler() { }

bool Ag_Scheduler::ag_task_arrived(std::shared_ptr<Task> server) {

    if (ag_tasks_vector_to_release.size() == 1)
    {
        double curr_time = simTime().dbl();
        double arrivalTime = ag_tasks_vector_to_release.at(0)->getTaskArrivalTime();
        if(curr_time - EPSILON <= arrivalTime && curr_time + EPSILON >= arrivalTime) // basically: curr_time == arrivalTime
        {
            if (server == nullptr || server->queue_length() == 1) {
                ag_tasks_vector_ready.push_back(ag_tasks_vector_to_release[0]);
            }

            ag_tasks_vector_to_release.clear();
        }
    }
    else if (ag_tasks_vector_to_release.size() > 1) {

        if (server == nullptr || server->queue_length() == 1) {
            ag_tasks_vector_ready.push_back(ag_tasks_vector_to_release[0]);
        }

        ag_tasks_vector_to_release.erase(ag_tasks_vector_to_release.begin());
    }

    if (ag_tasks_vector_ready.size() == 1)
        return true;
    else
        return false;
}

// schedulability test
double Ag_Scheduler::ag_sched_test(std::vector<std::shared_ptr<Task>> p_ag_tasks_vector,
        std::shared_ptr<ServerHandler> server_handler)
{
    double test_UF = 0;
    double t_C;
    double t_Period;
    //note residual computation time [ok?] --> si pu� usare CRes per "migliorare" la stima di U, ottenendo un stima pi� ottimistica
    if (!p_ag_tasks_vector.empty()) {
        for (auto task : p_ag_tasks_vector) {
            int server = task->getTaskServer();
            if (server != -1 && server != 100 && server != 200
                    && server_handler != nullptr)
            {
                std::shared_ptr<Server> server = server_handler->get_server(task->getTaskServer());
                if(server != nullptr)
                { // BDI: added (11/05/20)
                    if (!server->is_empty())
                    {
                        test_UF += server->get_bandwidth();
                    }
                }
            }
            else {
                t_C = task->getTaskCompTime();
                if (task->getTaskPeriod() == 0) {
                    t_Period = task->getTaskDeadline();
                }
                else
                    t_Period = task->getTaskPeriod();
                test_UF = test_UF + (t_C / t_Period);
            }
        }
    }

    return test_UF;
}

bool Ag_Scheduler::check_overlap(double task_start, double task_end,
        double window_start, double window_end) {
    bool condition = false;
    if (((window_start <= task_start) && (task_start < window_end))
            || ((window_start < task_end) && (task_end <= window_end))
            || ((task_start <= window_start) && (task_end >= window_end))) {
        condition = true;
    }
    return condition;
}

bool Ag_Scheduler::check_is_task_in_ready(std::shared_ptr<Task> task)
{
    for(int i = 0; i < (int)ag_tasks_vector_ready.size(); i++)
    {
        if(ag_tasks_vector_ready[i]->is_server())
        {
            // loop over the entire queue
            for(int j = 0; j < ag_tasks_vector_ready[i]->queue_length(); j++)
            {
                if(ag_tasks_vector_ready[i]->get_ith_task(j)->getTaskId() == task->getTaskId()
                        && ag_tasks_vector_ready[i]->get_ith_task(j)->getTaskPlanId() == task->getTaskPlanId()
                        && ag_tasks_vector_ready[i]->get_ith_task(j)->getTaskOriginalPlanId() == task->getTaskOriginalPlanId())
                {
                    return true;
                }
            }
        }
        else  if (ag_tasks_vector_ready[i]->getTaskId() == task->getTaskId()
                && ag_tasks_vector_ready[i]->getTaskPlanId() == task->getTaskPlanId()
                && ag_tasks_vector_ready[i]->getTaskOriginalPlanId() == task->getTaskOriginalPlanId())
        {
            return true;
        }
    }

    return false;
}

bool Ag_Scheduler::check_is_task_in_release(std::shared_ptr<Task> task)
{
    for(int i = 0; i < (int)ag_tasks_vector_to_release.size(); i++)
    {
        if(ag_tasks_vector_to_release[i]->is_server())
        {
            // loop over the entire queue
            for(int j = 0; j < ag_tasks_vector_to_release[i]->queue_length(); j++)
            {
                if(ag_tasks_vector_to_release[i]->get_ith_task(j)->getTaskId() == task->getTaskId()
                        && ag_tasks_vector_to_release[i]->get_ith_task(j)->getTaskPlanId() == task->getTaskPlanId()
                        && ag_tasks_vector_to_release[i]->get_ith_task(j)->getTaskOriginalPlanId() == task->getTaskOriginalPlanId())
                {
                    return true;
                }
            }
        }
        else if (ag_tasks_vector_to_release[i]->getTaskId() == task->getTaskId()
                && ag_tasks_vector_to_release[i]->getTaskPlanId() == task->getTaskPlanId()
                && ag_tasks_vector_to_release[i]->getTaskOriginalPlanId() == task->getTaskOriginalPlanId())
        {
            return true;
        }
    }

    return false;
}

double Ag_Scheduler::get_current_util(std::shared_ptr<ServerHandler> server_handler, double msg_util) {
    std::vector<std::shared_ptr<Task>> tmp = eval_current_taskset(0, INT32_MAX);
    return ag_sched_test(tmp, server_handler) + msg_util;
}

/*
 * TASK RELEASE RELATED
 */
void Ag_Scheduler::ag_sort_tasks_arrival() {
    // bubble-sort
    bool sorted;
    std::shared_ptr<Task> temp;
    if(ag_tasks_vector_to_release.size() > 1)
    {
        do {
            sorted = true;
            for(int i = 0; i < (int)ag_tasks_vector_to_release.size() - 1; i++)
            {
                if(ag_tasks_vector_to_release[i]->getTaskArrivalTime() > ag_tasks_vector_to_release[i + 1]->getTaskArrivalTime())
                {
                    temp = ag_tasks_vector_to_release[i];
                    ag_tasks_vector_to_release[i] = ag_tasks_vector_to_release[i+1];
                    ag_tasks_vector_to_release[i+1] = temp;
                    sorted = false;
                }
            }
        }
        while (sorted == false);
    }
}

void Ag_Scheduler::ag_sort_tasks_ddl(double comp_time_head_ready)
{
    logger->print(Logger::EveryInfo, "ag_sort_tasks_ddl("+ std::to_string(comp_time_head_ready) +")");

    /* v2: sort according to task's deadline
     * AND if deadlines are the same, prioritize task w.r.t. servers */
    std::sort(ag_tasks_vector_ready.begin(), ag_tasks_vector_ready.end(), [this](std::shared_ptr<Task> a, std::shared_ptr<Task> b) {
        if(fabs(a->getTaskDeadline() - b->getTaskDeadline()) < this->EPSILON) { // same deadlines
            if(a->getTaskServer() != -1) { // "a" is not a server --> even if "b" is a server -> do nothing, already sorted
                return false;
            }
            else if(b->getTaskServer() != -1) { // "a" is a server, "b" is a task, same ddl -> inverted them
                return true;
            }
            else { // "a" is a server, "b" is server -> do nothing, already sorted
                return false;
            }
        }
        else { // different deadlines
            return (a->getTaskDeadline() < b->getTaskDeadline());
        }
    });
}

void Ag_Scheduler::ag_add_task_in_vector_to_release(std::shared_ptr<Task> pn_task) {
    ag_tasks_vector_to_release.push_back(pn_task);
}

void Ag_Scheduler::ag_add_task_in_ready_tasks_vector(std::shared_ptr<Task> pn_task) {
    ag_tasks_vector_ready.push_back(pn_task);
}

void Ag_Scheduler::ag_remove_head_in_ready_tasks_vector() {
    if (ag_tasks_vector_ready.size() == 1) {
        ag_tasks_vector_ready.clear();
    }
    else {
        ag_tasks_vector_ready.erase(ag_tasks_vector_ready.begin());
    }
}

void Ag_Scheduler::ag_remove_head_in_release_tasks_vector() {
    if (ag_tasks_vector_to_release.size() == 1) {
        ag_tasks_vector_to_release.clear();
    } else {
        ag_tasks_vector_to_release.erase(ag_tasks_vector_to_release.begin());
    }
}

// BDI:
void Ag_Scheduler::ag_remove_task_in_ready_based_on_original_plan_id(int plan_id)
{
    for(int k = 0; k < (int)ag_tasks_vector_ready.size(); k++) {
        if(ag_tasks_vector_ready[k]->is_server()) {
            // loop over the entire queue
            for(int q = 0; q < ag_tasks_vector_ready[k]->queue_length(); q++) {
                if(ag_tasks_vector_ready[k]->get_ith_task(q)->getTaskOriginalPlanId() == plan_id) {
                    ag_tasks_vector_ready[k]->remove_element_from_queue(q);
                    q -= 1;
                }
            }
            if(ag_tasks_vector_ready[k]->queue_length() == 0) {
                ag_tasks_vector_ready.erase(ag_tasks_vector_ready.begin() + k);
                k -= 1;
            }
            else {
                ag_tasks_vector_ready[k]->setTaskId(ag_tasks_vector_ready[k]->get_ith_task(0)->getTaskId());
                ag_tasks_vector_ready[k]->setTaskPlanId(ag_tasks_vector_ready[k]->get_ith_task(0)->getTaskPlanId());// BDI
                ag_tasks_vector_ready[k]->setTaskOriginalPlanId(ag_tasks_vector_ready[k]->get_ith_task(0)->getTaskOriginalPlanId());// BDI
                ag_tasks_vector_ready[k]->setTaskDemander(ag_tasks_vector_ready[k]->get_ith_task(0)->getTaskDemander());
                ag_tasks_vector_ready[k]->setTaskExecuter(ag_tasks_vector_ready[k]->get_ith_task(0)->getTaskExecuter());
                ag_tasks_vector_ready[k]->setTaskReleaseTime(ag_tasks_vector_ready[k]->get_ith_task(0)->getTaskReleaseTime());
                ag_tasks_vector_ready[k]->setTaskStatus(ag_tasks_vector_ready[k]->get_ith_task(0)->getTaskStatus()); // BDI
            }
        }
        else {
            if (ag_tasks_vector_ready[k]->getTaskOriginalPlanId() == plan_id) {
                if (ag_tasks_vector_ready.size() == 1) {
                    ag_tasks_vector_ready.clear();
                } else {
                    ag_tasks_vector_ready.erase(ag_tasks_vector_ready.begin() + k);
                    k -= 1;
                }
            }
        }
    }
}

void Ag_Scheduler::ag_remove_complited_preempted_task_from_ready(std::shared_ptr<Task> task) {
    for(int i = 0; i < (int)ag_tasks_vector_ready.size(); i++) {
        if(ag_tasks_vector_ready[i]->is_server()) {
            // loop over the entire queue
            for(int j = 0; j < ag_tasks_vector_ready[i]->queue_length(); j++) {
                if(ag_tasks_vector_ready[i]->get_ith_task(j)->getTaskId() == task->getTaskId()
                        && ag_tasks_vector_ready[i]->get_ith_task(j)->getTaskPlanId() == task->getTaskPlanId()
                        && ag_tasks_vector_ready[i]->get_ith_task(j)->getTaskOriginalPlanId() == task->getTaskOriginalPlanId())
                {
                    ag_tasks_vector_ready[i]->remove_element_from_queue(j);
                    j -= 1;
                }
            }
            if(ag_tasks_vector_ready[i]->queue_length() == 0) {
                ag_tasks_vector_ready.erase(ag_tasks_vector_ready.begin() + i);
                i -= 1;
            }
            else {
                ag_tasks_vector_ready[i]->setTaskId(ag_tasks_vector_ready[i]->get_ith_task(0)->getTaskId());
                ag_tasks_vector_ready[i]->setTaskPlanId(ag_tasks_vector_ready[i]->get_ith_task(0)->getTaskPlanId());// BDI
                ag_tasks_vector_ready[i]->setTaskOriginalPlanId(ag_tasks_vector_ready[i]->get_ith_task(0)->getTaskOriginalPlanId());// BDI
                ag_tasks_vector_ready[i]->setTaskDemander(ag_tasks_vector_ready[i]->get_ith_task(0)->getTaskDemander());
                ag_tasks_vector_ready[i]->setTaskExecuter(ag_tasks_vector_ready[i]->get_ith_task(0)->getTaskExecuter());
                ag_tasks_vector_ready[i]->setTaskReleaseTime(ag_tasks_vector_ready[i]->get_ith_task(0)->getTaskReleaseTime());
                ag_tasks_vector_ready[i]->setTaskStatus(ag_tasks_vector_ready[i]->get_ith_task(0)->getTaskStatus()); // BDI
            }
        }
        else {
            if(ag_tasks_vector_ready[i]->getTaskId() == task->getTaskId()
                    && ag_tasks_vector_ready[i]->getTaskPlanId() == task->getTaskPlanId()
                    && ag_tasks_vector_ready[i]->getTaskOriginalPlanId() == task->getTaskOriginalPlanId())
            {
                if (ag_tasks_vector_ready.size() == 1) {
                    ag_tasks_vector_ready.clear();
                } else {
                    ag_tasks_vector_ready.erase(ag_tasks_vector_ready.begin() + i);
                    i -= 1;
                }
            }
        }
    }
}

void Ag_Scheduler::ag_remove_preempted_task_from_ready(std::shared_ptr<Task> server)
{   /* Given the server containing that task completed during preemption, remove the entire server from READY
 * It will then be added back to RELEASE (if contains other tasks that needs to be fulfilled)
 * using "Ag_Sched->ag_add_task_in_vector_to_release(...)" */
    for(int i = 0; i < (int)ag_tasks_vector_ready.size(); i++) {
        if(ag_tasks_vector_ready[i]->is_server()) {
            if(ag_tasks_vector_ready[i]->get_server_id() == server->get_server_id()) {
                ag_tasks_vector_ready.erase(ag_tasks_vector_ready.begin() + i);
                break;
            }
        }
    }
}

// BDI
void Ag_Scheduler::ag_remove_task_to_release_based_on_original_plan_id(int plan_id)
{
    for(int k = 0; k < (int)ag_tasks_vector_to_release.size(); k++) {
        if(ag_tasks_vector_to_release[k]->is_server()) {
            // loop over the entire queue...
            bool update_first_item = false;
            Task::Status queue_head_status;

            for(int q = 0; q < ag_tasks_vector_to_release[k]->queue_length(); q++) {
                if(ag_tasks_vector_to_release[k]->get_ith_task(q)->getTaskOriginalPlanId() == plan_id) {
                    if(q == 0 && update_first_item == false) {
                        update_first_item = true;
                        queue_head_status = ag_tasks_vector_to_release[k]->get_ith_task(q)->getTaskStatus();
                    }

                    ag_tasks_vector_to_release[k]->remove_element_from_queue(q);
                    q -= 1;
                }
            }

            if(ag_tasks_vector_to_release[k]->queue_length() == 0) {
                ag_tasks_vector_to_release.erase(ag_tasks_vector_to_release.begin() + k);
                k -= 1;
            }
            else {
                ag_tasks_vector_to_release[k]->setTaskId(ag_tasks_vector_to_release[k]->get_ith_task(0)->getTaskId());
                ag_tasks_vector_to_release[k]->setTaskPlanId(ag_tasks_vector_to_release[k]->get_ith_task(0)->getTaskPlanId());// BDI
                ag_tasks_vector_to_release[k]->setTaskOriginalPlanId(ag_tasks_vector_to_release[k]->get_ith_task(0)->getTaskOriginalPlanId());// BDI
                ag_tasks_vector_to_release[k]->setTaskDemander(ag_tasks_vector_to_release[k]->get_ith_task(0)->getTaskDemander());
                ag_tasks_vector_to_release[k]->setTaskExecuter(ag_tasks_vector_to_release[k]->get_ith_task(0)->getTaskExecuter());
                ag_tasks_vector_to_release[k]->setTaskReleaseTime(ag_tasks_vector_to_release[k]->get_ith_task(0)->getTaskReleaseTime());
                ag_tasks_vector_to_release[k]->setTaskStatus(ag_tasks_vector_to_release[k]->get_ith_task(0)->getTaskStatus()); // BDI

                if(update_first_item) { // ADDED: 07/04/21
                    // scenario: server[0] has state ACTIVE and 4 elements in queue (that are ACTIVE, READY, READY, READY)
                    // if we remove the first task, we must be consistent with the status. Keep ACTIVE for the server, and update the new head task to ACTIVE
                    logger->print(Logger::EveryInfoPlus, "task old STATUS:"+ ag_tasks_vector_to_release[k]->getTaskStatus_as_string());
                    ag_tasks_vector_to_release[k]->setTaskStatus(queue_head_status);
                    ag_tasks_vector_to_release[k]->get_ith_task(0)->setTaskStatus(queue_head_status);
                    logger->print(Logger::EveryInfoPlus, "task updated STATUS:"+ ag_tasks_vector_to_release[k]->getTaskStatus_as_string());
                }
            }
        }
        else {
            if (ag_tasks_vector_to_release[k]->getTaskOriginalPlanId() == plan_id) {
                if (ag_tasks_vector_to_release.size() == 1) {
                    ag_tasks_vector_to_release.clear();
                } else {
                    ag_tasks_vector_to_release.erase(ag_tasks_vector_to_release.begin() + k);
                    k -= 1;
                }
            }
        }
    }
}

void Ag_Scheduler::ag_remove_task_to_release_phiI(std::vector<std::shared_ptr<Task>>& queue, int plan_id)
{
    for(int k = 0; k < (int)queue.size(); k++)
    {
        if(queue[k]->is_server())
        {
            // loop over the entire queue...
            bool update_first_item = false;
            Task::Status queue_head_status;

            for(int q = 0; q < queue[k]->queue_length(); q++) {
                if(queue[k]->get_ith_task(q)->getTaskOriginalPlanId() == plan_id)
                {
                    if(q == 0 && update_first_item == false) {
                        update_first_item = true;
                        queue_head_status = queue[k]->get_ith_task(q)->getTaskStatus();
                    }

                    queue[k]->remove_element_from_queue(q);
                    q -= 1;
                }
            }
            if(queue[k]->queue_length() == 0) {
                queue.erase(queue.begin() + k);
                k -= 1;
            }
            else {
                queue[k]->setTaskId(queue[k]->get_ith_task(0)->getTaskId());
                queue[k]->setTaskPlanId(queue[k]->get_ith_task(0)->getTaskPlanId());// BDI
                queue[k]->setTaskOriginalPlanId(queue[k]->get_ith_task(0)->getTaskOriginalPlanId());// BDI
                queue[k]->setTaskDemander(queue[k]->get_ith_task(0)->getTaskDemander());
                queue[k]->setTaskExecuter(queue[k]->get_ith_task(0)->getTaskExecuter());
                queue[k]->setTaskReleaseTime(queue[k]->get_ith_task(0)->getTaskReleaseTime());
                queue[k]->setTaskStatus(queue[k]->get_ith_task(0)->getTaskStatus()); // BDI

                if(update_first_item) { // ADDED: 07/04/21
                    // scenario: server[0] has state ACTIVE and 4 elements in queue (that are ACTIVE, READY, READY, READY)
                    // if we remove the first task, we must be consistent with the status. Keep ACTIVE for the server, and update the new head task to ACTIVE
                    logger->print(Logger::EveryInfoPlus, "task old STATUS:["+ queue[k]->getTaskStatus_as_string());
                    queue[k]->setTaskStatus(queue_head_status);
                    queue[k]->get_ith_task(0)->setTaskStatus(queue_head_status);
                    logger->print(Logger::EveryInfoPlus, "task updated STATUS:["+ queue[k]->getTaskStatus_as_string());
                }
            }
        }
        else { // simple PERIODIC task
            if (queue[k]->getTaskOriginalPlanId() == plan_id) {
                if (queue.size() == 1)
                {
                    queue.clear();
                } else {
                    queue.erase(queue.begin() + k);
                    k -= 1;
                }
            }
        }
    }
}

bool Ag_Scheduler::ag_vector_ready_contains_server(std::shared_ptr<Task> server) {
    if(server == nullptr) {
        return true; // received server is null
    }
    else {
        for(int i = 0; i < (int)ag_tasks_vector_ready.size(); i++) {
            if(ag_tasks_vector_ready[i]->is_server() && server->is_server()) {
                if(ag_tasks_vector_ready[i]->getTaskServer() == server->getTaskServer()) {
                    return true;
                }
            }
        }
    }
    return false;
}

void Ag_Scheduler::update_active_task_in_sorted_tasks_vector(std::shared_ptr<Task> p_task) {
    std::shared_ptr<Task> first_task = ag_tasks_vector_ready[0];
    if (p_task->getTaskId() == first_task->getTaskId()
            && p_task->getTaskDemander() == first_task->getTaskDemander()
            && p_task->getTaskReleaseTime() == first_task->getTaskReleaseTime())
    {
        ag_tasks_vector_ready[0] = p_task;
    }
}

// Getter methods
SchedType Ag_Scheduler::get_sched_type() {
    return this->type;
}

std::string Ag_Scheduler::get_sched_type_as_string() {
    switch (this->type) {
    case FCFS: return "FCFS"; break;
    case RR: return "RR"; break;
    case EDF: return "EDF"; break;
    default: return "undefined"; break;
    }
}

double Ag_Scheduler::get_quantum() {
    return this->quantum;
}

double Ag_Scheduler::get_curr_quantum() {
    return this->curr_quantum;
}

void Ag_Scheduler::set_curr_quantum(double curr_quantum) {
    this->curr_quantum = curr_quantum;
}

std::vector<std::shared_ptr<Task>> Ag_Scheduler::get_tasks_vector_ready() {
    return ag_tasks_vector_ready;
}

std::vector<std::shared_ptr<Task>> Ag_Scheduler::get_tasks_vector_to_release() {
    return ag_tasks_vector_to_release;
}

//checks if a task has passed the deadline by calculating the residual computation time
double Ag_Scheduler::get_ddl_miss(std::shared_ptr<Task> p_task, int first_task_id) {
    double c_res = -1;
    if (p_task != nullptr) {
        c_res = p_task->getTaskCompTimeRes();

        if ((p_task->getTaskFirstActivation() != -1)
                && (p_task->getTaskId() == first_task_id)) {
            //update c_res
            double last_computation = simTime().dbl() - p_task->getTaskLastActivation();
            c_res -= last_computation;
        }
    }
    return c_res;
}

//checks if a task has passed the deadline by calculating the residual computation time
double Ag_Scheduler::get_ddl_miss(std::shared_ptr<Task> p_task, int first_task_id, int first_task_plan_id) {
    double c_res = -1;
    if (p_task != nullptr) {
        c_res = p_task->getTaskCompTimeRes();

        if (p_task->getTaskFirstActivation() != -1
                && p_task->getTaskId() == first_task_id
                && p_task->getTaskPlanId() == first_task_plan_id)
        {
            //update c_res
            double last_computation = simTime().dbl() - p_task->getTaskLastActivation();
            c_res -= last_computation;
        }
    }
    return c_res;
}

std::shared_ptr<Task> Ag_Scheduler::get_release_task(int task_id, int task_plan_id, int task_origina_plan_id,int task_demander, double releaseTime)
{
    std::shared_ptr<Task> queue_task;
    for (int i = 0; i < (int)ag_tasks_vector_to_release.size(); i++) {
        if(ag_tasks_vector_to_release[i]->is_server()) {
            for (int j = 0; j < ag_tasks_vector_to_release[i]->queue_length(); j++) {
                queue_task = ag_tasks_vector_to_release[i]->get_ith_task(j);
                if (queue_task->getTaskId() == task_id
                        && queue_task->getTaskPlanId() == task_plan_id
                        && queue_task->getTaskOriginalPlanId() == task_origina_plan_id)
                {
                    return queue_task;
                }
            }
        }
        else {
            if (ag_tasks_vector_to_release[i]->getTaskId() == task_id
                    && ag_tasks_vector_to_release[i]->getTaskPlanId() == task_plan_id
                    && ag_tasks_vector_to_release[i]->getTaskOriginalPlanId() == task_origina_plan_id)
            {
                return ag_tasks_vector_to_release[i];
            }
        }
    }
    return nullptr;
}

std::shared_ptr<Task> Ag_Scheduler::get_ready_task(int task_id, int task_plan_id, int task_origina_plan_id,int task_demander, double releaseTime)
{
    std::shared_ptr<Task> queue_task;
    for (int i = 0; i < (int)ag_tasks_vector_ready.size(); i++) {
        if(ag_tasks_vector_ready[i]->is_server()) {
            for (int j = 0; j < ag_tasks_vector_ready[i]->queue_length(); j++) {
                queue_task = ag_tasks_vector_ready[i]->get_ith_task(j);
                if (queue_task->getTaskId() == task_id
                        && queue_task->getTaskPlanId() == task_plan_id
                        && queue_task->getTaskOriginalPlanId() == task_origina_plan_id)
                {
                    return queue_task;
                }
            }
        }
        else {
            if (ag_tasks_vector_ready[i]->getTaskId() == task_id
                    && ag_tasks_vector_ready[i]->getTaskPlanId() == task_plan_id
                    && ag_tasks_vector_ready[i]->getTaskOriginalPlanId() == task_origina_plan_id)
            {
                return ag_tasks_vector_ready[i];
            }
        }
    }
    return nullptr;
}

void Ag_Scheduler::update_tasks_planid_ready(int task_id, int task_plan_id, int task_origina_plan_id, int new_plan_id) {
    std::shared_ptr<Task> task = nullptr;
    for (int i = 0; i < (int)ag_tasks_vector_ready.size(); i++) {
        if(ag_tasks_vector_ready[i]->is_server()) {
            for(int j = 0; j < ag_tasks_vector_ready[i]->queue_length(); j++) {
                task = ag_tasks_vector_ready[i]->get_ith_task(j);

                if (task->getTaskId() == task_id
                        && task->getTaskPlanId() == task_plan_id
                        && task->getTaskOriginalPlanId() == task_origina_plan_id)
                {
                    task->setTaskPlanId(new_plan_id);
                    ag_tasks_vector_ready[i]->set_ith_task(j, task);
                }
            }
        }
        else {
            if (ag_tasks_vector_ready[i]->getTaskId() == task_id
                    && ag_tasks_vector_ready[i]->getTaskPlanId() == task_plan_id
                    && ag_tasks_vector_ready[i]->getTaskOriginalPlanId() == task_origina_plan_id)
            {
                ag_tasks_vector_ready[i]->setTaskPlanId(new_plan_id);
            }
        }
    }
}

void Ag_Scheduler::update_tasks_planid_release(int task_id, int task_plan_id, int task_origina_plan_id, int new_plan_id) {
    std::shared_ptr<Task> task = nullptr;
    for (int i = 0; i < (int)ag_tasks_vector_to_release.size(); i++) {
        if(ag_tasks_vector_to_release[i]->is_server()) {
            for(int j = 0; j < ag_tasks_vector_to_release[i]->queue_length(); j++) {
                task = ag_tasks_vector_to_release[i]->get_ith_task(j);

                if (task->getTaskId() == task_id
                        && task->getTaskPlanId() == task_plan_id
                        && task->getTaskOriginalPlanId() == task_origina_plan_id)
                {
                    task->setTaskPlanId(new_plan_id);
                    ag_tasks_vector_to_release[i]->set_ith_task(j, task);
                }
            }
        }
        else {
            if (ag_tasks_vector_to_release[i]->getTaskId() == task_id
                    && ag_tasks_vector_to_release[i]->getTaskPlanId() == task_plan_id
                    && ag_tasks_vector_to_release[i]->getTaskOriginalPlanId() == task_origina_plan_id)
            {
                ag_tasks_vector_to_release[i]->setTaskPlanId(new_plan_id);
            }
        }
    }
}

// Utility
void Ag_Scheduler::ev_ag_tasks_vector_to_release() {
    logger->print(Logger::Default, "Release queue[tot = "+ std::to_string(ag_tasks_vector_to_release.size()) +"]:");

    for (auto task : ag_tasks_vector_to_release) {
        if (task->is_server()) {
            logger->print(Logger::Default, "- SERVER["+ std::to_string(task->getTaskServer()) +"]: curr_budget:["+ std::to_string(task->get_curr_budget())
            +"], curr_ddl:["+ std::to_string(task->get_curr_ddl())
            +"], queue.size:["+ std::to_string(task->queue_length())
            +"], deadline:["+ std::to_string(task->getTaskDeadline())
            +"], startTime:["+ std::to_string(task->get_startTime())
            +"], arrivalTime:["+ std::to_string(task->getTaskArrivalTime())
            +"], tskCres:["+ std::to_string(task->getTaskCompTimeRes())
            +"], tskStatus:["+ task->getTaskStatus_as_string()
            +"], tskExecType:["+ task->getTaskExecutionType_as_string()
            +"], isBeenAct:["+ boolToString(task->getTaskisBeenActivated())
            +"], task:["+ std::to_string(task->getTaskId())
            +"], plan:["+ std::to_string(task->getTaskPlanId())
            +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId()) +"]");

            for(int i = 0; i < task->queue_length(); i++) {
                std::shared_ptr<Task> tmp = task->get_ith_task(i);

                logger->print(Logger::Default, " -- task:["+ std::to_string(tmp->getTaskId())
                +"], plan:["+ std::to_string(tmp->getTaskPlanId())
                +"], original_plan:["+ std::to_string(tmp->getTaskOriginalPlanId())
                +"], deadline:["+ std::to_string(tmp->getTaskDeadline())
                +"], arrivalTime:["+ std::to_string(tmp->getTaskArrivalTime())
                +"], releaseTime:["+ std::to_string(tmp->getTaskReleaseTime())
                +"], N_exec left:["+ std::to_string(tmp->getTaskNExec())
                +" of "+ std::to_string(tmp->getTaskNExecInitial())
                +"], tskCres:["+ std::to_string(tmp->getTaskCompTimeRes())
                +"], tskStatus:["+ tmp->getTaskStatus_as_string()
                +"], tskExecType:["+ tmp->getTaskExecutionType_as_string()
                +"], isBeenAct:["+ boolToString(tmp->getTaskisBeenActivated()) +"]");
            }
        }
        else {
            logger->print(Logger::Default, "- Task:["+ std::to_string(task->getTaskId())
            +"], plan:["+ std::to_string(task->getTaskPlanId())
            +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId())
            +"], deadline:["+ std::to_string(task->getTaskDeadline())
            +"], arrivalTime:["+ std::to_string(task->getTaskArrivalTime())
            +"], releaseTime:["+ std::to_string(task->getTaskReleaseTime())
            +"], N_exec left:["+ std::to_string(task->getTaskNExec())
            +" of "+ std::to_string(task->getTaskNExecInitial())
            +"], tskCres:["+ std::to_string(task->getTaskCompTimeRes())
            +"], tskStatus:["+ task->getTaskStatus_as_string()
            +"], tskExecType:["+ task->getTaskExecutionType_as_string()
            +"], isBeenAct:["+ boolToString(task->getTaskisBeenActivated()) +"]");
        }
    }
}

void Ag_Scheduler::ev_ag_tasks_vector_ready()
{
    logger->print(Logger::Default, "Ready queue[tot = "+ std::to_string(ag_tasks_vector_ready.size()) +"]:");

    for (auto task : ag_tasks_vector_ready) {
        if (task->is_server()) {
            logger->print(Logger::Default, "- SERVER["+ std::to_string(task->getTaskServer()) +"]: curr_budget:["+ std::to_string(task->get_curr_budget())
            +"], curr_ddl:["+ std::to_string(task->get_curr_ddl())
            +"], queue.size:["+ std::to_string(task->queue_length())
            +"], deadline:["+ std::to_string(task->getTaskDeadline())
            +"], startTime:["+ std::to_string(task->get_startTime())
            +"], arrivalTime:["+ std::to_string(task->getTaskArrivalTime())
            +"], tskCres:["+ std::to_string(task->getTaskCompTimeRes())
            +"], tskStatus:["+ task->getTaskStatus_as_string()
            +"], tskExecType:["+ task->getTaskExecutionType_as_string()
            +"], isBeenAct:["+ boolToString(task->getTaskisBeenActivated())
            +"], task:["+ std::to_string(task->getTaskId())
            +"], plan:["+ std::to_string(task->getTaskPlanId())
            +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId()) +"]");

            for(int i = 0; i < task->queue_length(); i++) {
                std::shared_ptr<Task> tmp = task->get_ith_task(i);

                logger->print(Logger::Default, " -- task:["+ std::to_string(tmp->getTaskId())
                +"], plan:["+ std::to_string(tmp->getTaskPlanId())
                +"], original_plan:["+ std::to_string(tmp->getTaskOriginalPlanId())
                +"], deadline:["+ std::to_string(tmp->getTaskDeadline())
                +"], arrivalTime:["+ std::to_string(tmp->getTaskArrivalTime())
                +"], releaseTime:["+ std::to_string(tmp->getTaskReleaseTime())
                +"], N_exec left:["+ std::to_string(tmp->getTaskNExec())
                +" of "+ std::to_string(tmp->getTaskNExecInitial())
                +"], tskCres:["+ std::to_string(tmp->getTaskCompTimeRes())
                +"], tskStatus:["+ tmp->getTaskStatus_as_string()
                +"], tskExecType:["+ tmp->getTaskExecutionType_as_string()
                +"], isBeenAct:["+ boolToString(tmp->getTaskisBeenActivated()) +"]");
            }
        }
        else {
            logger->print(Logger::Default, "- Task:["+ std::to_string(task->getTaskId())
            +"], plan:["+ std::to_string(task->getTaskPlanId())
            +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId())
            +"], deadline:["+ std::to_string(task->getTaskDeadline())
            +"], arrivalTime:["+ std::to_string(task->getTaskArrivalTime())
            +"], releaseTime:["+ std::to_string(task->getTaskReleaseTime())
            +"], N_exec left:["+ std::to_string(task->getTaskNExec())
            +" of "+ std::to_string(task->getTaskNExecInitial())
            +"], tskCres:["+ std::to_string(task->getTaskCompTimeRes())
            +"], tskStatus:["+ task->getTaskStatus_as_string()
            +"], tskExecType:["+ task->getTaskExecutionType_as_string()
            +"], isBeenAct:["+ boolToString(task->getTaskisBeenActivated()) +"]");
        }
    }
}

void Ag_Scheduler::ag_replace_task_to_release(std::shared_ptr<Task> np_task) {
    ag_tasks_vector_to_release[0] = np_task;
}

std::string Ag_Scheduler::boolToString(const bool val) {
    if(val) {
        return "true";
    }
    return "false";
}


/* *******************************************************************
 * **************    Private Functions    ****************************
 * *******************************************************************/
//Predict taskset to evaluate utilization
std::vector<std::shared_ptr<Task>> Ag_Scheduler::eval_current_taskset(double window_start, double window_end) {
    std::vector<std::shared_ptr<Task>> taskset;
    //eval release vector
    for (auto task : ag_tasks_vector_to_release) {
        int server = task->getTaskServer();
        if (server != 100 && server != 200) {
            if (server != -1) {
                auto it = find_if(taskset.begin(), taskset.end(),
                        [&server](std::shared_ptr<Task> obj) {return obj->getTaskServer() == server;});
                if (it == taskset.end()) {
                    taskset.push_back(task);
                }
            }
            else if (task->getTaskNExec() == -1) {
                int id = task->getTaskId();
                int demander = task->getTaskDemander();
                auto it = find_if(taskset.begin(), taskset.end(),
                        [id, demander](std::shared_ptr<Task> obj)
                        {   return (obj->getTaskId() == id) && (obj->getTaskDemander() == demander);}); // flip the condition below here
                if (it == taskset.end()) { // note: add check -> if task has not been activated and will not be activated in the window (t_now + period > window end), don't consider it!
                    taskset.push_back(task);
                }
            }
            else {
                int id = task->getTaskId();
                int demander = task->getTaskDemander();
                auto it = find_if(taskset.begin(), taskset.end(),
                        [id, demander](std::shared_ptr<Task> obj)
                        {   return (obj->getTaskId() == id) && (obj->getTaskDemander() == demander);});
                if (it == taskset.end()) {
                    int n_exec = task->getTaskNExec();
                    double task_start = task->getTaskArrivalTime();
                    double task_end = task_start + ((n_exec + 1) * task->getTaskPeriod());
                    if (check_overlap(task_start, task_end, window_start, window_end)) {
                        taskset.push_back(task);
                    }
                }
            }
        }
    }

    //eval ready std::vector
    for (auto task : ag_tasks_vector_ready) {
        int server = task->getTaskServer();
        if (server != 100 && server != 200) {
            if (task->getTaskNExec() != -1) {
                int id = task->getTaskId();
                int demander = task->getTaskDemander();
                auto it = find_if(taskset.begin(), taskset.end(),
                        [id, demander](std::shared_ptr<Task> obj)
                        {   return (obj->getTaskId() == id) && (obj->getTaskDemander() == demander);});
                if (it == taskset.end()) {
                    double task_start = task->getTaskArrivalTime();
                    double task_end = task_start + task->getTaskPeriod();
                    if (check_overlap(task_start, task_end, window_start, window_end)) {
                        taskset.push_back(task);
                    }
                }
            }
            else if (server != -1) {
                auto it = find_if(taskset.begin(), taskset.end(),
                        [&server](std::shared_ptr<Task> obj) {return obj->getTaskServer() == server;});
                if (it == taskset.end()) {
                    taskset.push_back(task);
                }
            }
        }
    }
    return taskset;
}

// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
// Note: this function is used in project 'agent_gprt' to perform the 'schdulability test' of a taskset WHEN a task completes!
// In Kronosim we do not use it.
// I kept it as a "reminder" of how 'agent_gprt' threats servers when computing U
//double Ag_Scheduler::ag_sched_test(std::vector<Task*> p_ag_tasks_vector,
//        ServerHandler* server_handler) {
//    double test_UF = 0;
//    double t_C;
//    double t_Period;
//    //note residual computation time [ok?] --> si più usare CRes per "migliorare" la stima di U, ottenendo un stima più ottimistica
//    if (!p_ag_tasks_vector.empty()) {
//        for (auto task : p_ag_tasks_vector) {
//            int server = task->getTaskServer();
//            if (server != -1 && server != 100 && server != 200
//                    && server_handler != nullptr) {
// ********* --------------------------------------------------------------
// **** IDEA:
// **** - funzione chiamata dopo che 'check_task_completion_rt' viene chiamata per controllare il completamento di un task
// **** - analizzo tutti i task ancora in coda
// **** - se PERIODICA, aggiorno la Uf sommando la U del task --> DDL MISS
// **** - se APERIODICA, prendo il server associato al task
// **** -- se ci sono elementi nella coda del server, sommo la banda del server alla U -> banda = budget / period --> DDL MISS
// **** -- se la coda è vuota, il server non ha impatto sulla U
// ********* --------------------------------------------------------------
//                Server* server = server_handler->get_server(
//                        task->getTaskServer());
//                if (!server->is_empty()) {
//                    test_UF += server->get_bandwidth();
//                }
//            } else {
//                t_C = task->getTaskCompTime();
//                if (task->getTaskPeriod() == 0) {
//                    t_Period = task->getTaskDeadLine();
//                } else
//                    t_Period = task->getTaskPeriod();
//                test_UF = test_UF + (t_C / t_Period);
//            }
//        }
//    }
//    //p_ag_tasks_vector.
//    //    double test_UF_trunc = trunc(100 * test_UF) / 100;
//    return test_UF;
//}
// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------
