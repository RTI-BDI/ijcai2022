/*
 * Metric.cc
 *
 *  Created on: Feb 22, 2021
 *      Author: francesco
 */

#include "../headers/Metric.h"

Metric::Metric(int level) {
    logger = std::make_shared<Logger>(level);
}

Metric::~Metric() { }

void Metric::addActivatedGoal(std::shared_ptr<Goal> goal, simtime_t activationTime, int planid, int original_planid, std::string pref_index) {
    logger->print(Logger::EveryInfo, "addActivatedGoal:[id="+ std::to_string(goal->get_id())
    +"], belief:("+ goal->get_goal_name() +")");
    activatedGoals.push_back(std::make_tuple(goal, activationTime, -1, planid, original_planid, pref_index, "undefined", 0));
}

void Metric::addActivatedIntentions(std::shared_ptr<Intention> intention, simtime_t activationTime) {
    activatedIntentions.push_back(std::make_tuple(intention, activationTime, -1, "undefined"));
}

void Metric::addGeneratedError(simtime_t time, std::string function, std::string msg) {
    generated_errors.push_back(std::make_tuple(time, function, msg));
}

void Metric::addGeneratedWarning(simtime_t time, std::string function, std::string msg) {
    generated_warnings.push_back(std::make_tuple(time, function, msg));
}

void Metric::addGoalCompleted(std::shared_ptr<Intention> intention, simtime_t time, std::string function) {
    completedGoals.push_back(std::make_tuple(intention, time, function));
}

void Metric::addPhiIStats(int active_goals, int active_intentions, simtime_t time) {
    phiI_stats.push_back(std::make_tuple(phiI_stats.size() + 1, active_goals, active_intentions, time));
}

void Metric::addReasoningCycleStats(int active_goals, int active_intentions, simtime_t time) {
    reasoning_cycle_stats.push_back(std::make_tuple(reasoning_cycle_stats.size() + 1, active_goals, active_intentions, time));
}

void Metric::addTaskResponseTime(std::shared_ptr<Task> task, simtime_t finishTime, simtime_t responseTime) {
    tasks_response_time.push_back(std::make_tuple(task, finishTime, responseTime));
}

void Metric::addUnexpectedTaskExecution(std::shared_ptr<Task> task) {
    unexpected_tasks_execution.push_back(std::make_pair(simTime().dbl(), task));
}

void Metric::checkIfUnexpectedTaskExecution(const std::shared_ptr<Task> task, const std::vector<std::shared_ptr<Intention>>& intentions)
{
    bool isUnexpected = true;

    for(int i = 0; i < (int)intentions.size(); i++)
    {
        if(intentions[i]->get_planID() == task->getTaskPlanId()
                && intentions[i]->get_original_planID() == task->getTaskOriginalPlanId())
        {
            isUnexpected = false;
            break;
        }
    }

    if(isUnexpected) {
        logger->print(Logger::Warning, "[WARNING] t: "+ simTime().str()
        +", Unexpected task completion! Task:["+ std::to_string(task->getTaskId())
        +"], plan_id:["+ std::to_string(task->getTaskPlanId())
        +"], original plan_id:["+ std::to_string(task->getTaskOriginalPlanId()) +"]");

        addUnexpectedTaskExecution(task);
    }
}

// Finish() support functions ------------------------------------------------------------
void Metric::finishCrash(bool crashed, std::string folder, std::string user, std::string reason, std::string class_name, std::string function, simtime_t time) {
    if(crashed == true) {
        json main = { { "Simulation_crash", crashed },
                { "Time", time.dbl() },
                { "Class", class_name },
                { "Function", function },
                { "Reason", reason }
        };

        write_metrics(main);
        write_metrics_simpletext("t: "+ time.str() +", Simulation crashed! Class: "+ class_name +", Function: "+ function +", Reason: "+ reason);
    }
    else {
        json main = { { "Simulation_crash", crashed } };
        write_metrics(main);
    }
}

void Metric::finishForceSimulationEnd(bool force_end, std::string folder, std::string user, std::string reason, std::string class_name, std::string function, simtime_t time) {
    if(force_end == true) {
        json main = { { "Forced_simulation_end", force_end },
                { "Time", time.dbl() },
                { "Class", class_name },
                { "Function", function },
                { "Reason", reason }
        };

        write_metrics(main);
        write_metrics_simpletext("t: "+ time.str() +", Force simulation end! Class: "+ class_name +", Function: "+ function +", Reason: "+ reason);
    }
    else {
        json main = { { "Forced_simulation_end", force_end } };
        write_metrics(main);
    }
}

void Metric::finishGeneratedDeadlineMiss(const std::vector<std::shared_ptr<Lateness>>& ag_tasks_lateness,
        int agent_id, int sched_type, std::string sched_type_str, bool phiI_prioritize_internal_goals)
{
    json main, task; std::vector<json> tasks;

    printFinish(" ******************************************** ");
    printFinish(" Simulation run until:["+ simTime().str() +"] using as scheduling algorithm: " + sched_type_str);
    printFinish(" -------------------------------------------- ");
    printFinish(" Generated Deadline Miss for ag["+ std::to_string(agent_id) +"], [tot="+ std::to_string(ag_tasks_lateness.size()) +"]: ");


    double tot_completed_tasks = 0;
    if(tasks_response_time.size() > 0) {
        tot_completed_tasks = tasks_response_time.size();
    }

    if(ag_tasks_lateness.size() > 0)
    {
        double tot_lateness_avg = 0, tasks_miss_percentage = 0, lateness_avg = 0;
        for(std::shared_ptr<Lateness> l : ag_tasks_lateness)
        {
            printFinish(" - t: "+ std::to_string(l->getTaskTime())
            +", task:["+ std::to_string(l->getTaskId())
            +"], plan:["+ std::to_string(l->getTaskPlanId())
            +"], original_plan:["+ std::to_string(l->getTaskOriginalPlanId())
            +"], N_exec:["+ std::to_string(l->getTaskN_exec())
            +"], LATENESS:["+ std::to_string(l->getTaskLateness())
            +"], latenessLog:\""+ (l->getTaskLatenessLog()) +"\" ");
            tot_lateness_avg += l->getTaskLateness();

            task = { { "Time", l->getTaskTime() },
                    { "Id", l->getTaskId() },
                    { "Plan", l->getTaskPlanId() },
                    { "Original_plan", l->getTaskOriginalPlanId() },
                    { "N_exec", l->getTaskN_exec() },
                    { "Lateness", l->getTaskLateness() },
                    { "LatenessLog", l->getTaskLatenessLog()  }
            };

            tasks.push_back(task);
        }

        double tasks_generating_miss = ag_tasks_lateness.size();
        tasks_miss_percentage = (tasks_generating_miss / tot_completed_tasks) * 100.0;
        lateness_avg = tot_lateness_avg / ag_tasks_lateness.size();

        printFinish(" > TOT generating miss: "+ std::to_string(tasks_generating_miss)
        +" - Tot completed tasks: "+ std::to_string(tot_completed_tasks)
        +" - Average Lateness: "+ std::to_string(lateness_avg)
        +" - Tasks Lateness in percent (%): "+ std::to_string(tasks_miss_percentage));

        main = { { "Scheduling_type", sched_type_str },
                { "Deadlinemiss_generated", tasks },
                { "Average_lateness", lateness_avg },
                { "Tot_miss_generated", tasks_generating_miss },
                { "Tot_completed_tasks", tot_completed_tasks }};
    }
    else {
        printFinish(" - No Deadline MISS generated!");
        main = { { "Scheduling_type", sched_type_str },
                { "Deadlinemiss_generated", {} },
                { "Average_lateness", 0 },
                { "Tot_miss_generated", 0 },
                { "Tot_completed_tasks", tot_completed_tasks } };
    }
    printFinish(" -------------------------------------------- ");

    write_metrics(main, agent_id);
}

void Metric::finishGeneratedErrors(int agent_id) {
    printFinish(" GeneratedErrors:[tot="+ std::to_string(generated_errors.size()) +"]: ");
    json main, errors_json, error_json;
    std::vector<json> stats;

    for(int i = 0; i < (int)generated_errors.size(); i++) {
        printFinish(" - Time: "+ std::get<0>(generated_errors[i]).str()
        +", Function: "+ std::get<1>(generated_errors[i])
        +", Cause:["+ std::get<2>(generated_errors[i]) +"]");

        error_json = { { "Time",std::get<0>(generated_errors[i]).dbl() },
                { "Function", std::get<1>(generated_errors[i]) },
                { "Cause", std::get<2>(generated_errors[i]) }
        };
        errors_json.push_back(error_json);
    }

    main = {
            { "Num_of_generated_errors", generated_errors.size() },
            { "Generated_errors", errors_json }
    };

    printFinish(" -------------------------------------------- ");

    write_metrics(main, agent_id);
}

void Metric::finishGeneratedWarnings(int agent_id) {
    printFinish(" GeneratedWarning:[tot="+ std::to_string(generated_warnings.size()) +"]: ");
    json main, warnings_json, warning_json;
    std::vector<json> stats;

    for(int i = 0; i < (int)generated_warnings.size(); i++) {
        printFinish(" - Time: "+ std::get<0>(generated_warnings[i]).str()
        +", Function: "+ std::get<1>(generated_warnings[i])
        +", Cause:["+ std::get<2>(generated_warnings[i]) +"]", Logger::EveryInfo);

        warning_json = { { "Time",std::get<0>(generated_warnings[i]).dbl() },
                { "Function", std::get<1>(generated_warnings[i]) },
                { "Cause", std::get<2>(generated_warnings[i]) }
        };
        warnings_json.push_back(warning_json);
    }

    main = { { "Num_of_generated_warnings", generated_warnings.size() },
            { "Generated_warnings", warnings_json } };
    printFinish(" -------------------------------------------- ");

    write_metrics(main, agent_id);
}

void Metric::finishGoalAchieved(int agent_id) {
    printFinish(" Goal achieved:[tot="+ std::to_string(completedGoals.size()) +"]: ");
    json main, stat;
    std::vector<json> stats;
    std::shared_ptr<Intention> intention;

    if(completedGoals.size() > 0) {
        for(int i = 0; i < (int)completedGoals.size(); i++)
        {
            intention = std::get<0>(completedGoals[i]);

            printFinish(" - Time: "+ std::get<1>(completedGoals[i]).str()
            +", Intention: "+ std::to_string(intention->get_id())
            +", Goal: ("+ intention->get_goal_name()
            +"), due to:["+ std::get<2>(completedGoals[i]) +"]", Logger::EveryInfo);

            stat = { { "Time", std::get<1>(completedGoals[i]).dbl() },
                    { "Intention", intention->get_id() },
                    { "Goal_name", intention->get_goal_name() },
                    { "Reason", std::get<2>(completedGoals[i]) }
            };
            stats.push_back(stat);
        }
    }

    main = { { "Tot_achievedGoals", completedGoals.size() },
            { "Achieved_goals", stats }};
    printFinish(" -------------------------------------------- ");

    write_metrics(main, agent_id);
}

void Metric::finishPhiIStats(int agent_id) {
    printFinish(" PhiI stats:[tot="+ std::to_string(phiI_stats.size()) +"]: ");
    double avg_activeGoals = 0, avg_activeIntentions = 0;
    json main, stat;
    std::vector<json> stats;

    if(phiI_stats.size() > 0) {
        for(int i = 0; i < (int)phiI_stats.size(); i++) {
            printFinish(" - Time: "+ std::get<3>(phiI_stats[i]).str()
            +", Iteration: "+ std::to_string(std::get<0>(phiI_stats[i]))
            +", Active Goals:["+ std::to_string(std::get<1>(phiI_stats[i]))
            +"], Active Intentions:["+ std::to_string(std::get<2>(phiI_stats[i])) +"]", Logger::EveryInfo);

            stat = { { "Time", std::get<3>(phiI_stats[i]).dbl() },
                    { "Iteration", std::get<0>(phiI_stats[i]) },
                    { "Active_goals", std::get<1>(phiI_stats[i]) },
                    { "Active_intentions", std::get<2>(phiI_stats[i]) }
            };
            stats.push_back(stat);

            avg_activeGoals += std::get<1>(phiI_stats[i]);
            avg_activeIntentions += std::get<2>(phiI_stats[i]);
        }
        main = { { "PhiI_stats", "PhiI stats" },
                { "Stats_phiI", stats },
                { "Avg_active_goals_phiI", avg_activeGoals / phiI_stats.size() },
                { "Avg_active_intentions_phiI", avg_activeIntentions / phiI_stats.size() } };

        printFinish(" > AVG Goals: "+ std::to_string(avg_activeGoals / phiI_stats.size())
        +", AVG Intentions: "+ std::to_string(avg_activeIntentions / phiI_stats.size()));
    }
    else {
        main = { { "PhiI_stats", "PhiI stats" },
                { "Stats_phiI", stats },
                { "Avg_active_goals_phiI", 0 },
                { "Avg_active_intentions_phiI", 0 } };
    }
    printFinish(" -------------------------------------------- ");

    write_metrics(main, agent_id);
}

void Metric::finishReasoningCyclesStats(int agent_id) {
    printFinish(" Reasoning cycles stats:[tot="+ std::to_string(reasoning_cycle_stats.size()) +"]:");
    double avg_activeGoals = 0, avg_activeIntentions = 0;
    json main, stat;
    std::vector<json> stats;

    if(reasoning_cycle_stats.size() > 0) {
        for(int i = 0; i < (int)reasoning_cycle_stats.size(); i++) {
            printFinish(" - Time: "+ std::get<3>(reasoning_cycle_stats[i]).str()
            +", Iteration: "+ std::to_string(std::get<0>(reasoning_cycle_stats[i]))
            +", Active Goals:["+ std::to_string(std::get<1>(reasoning_cycle_stats[i]))
            +"], Active Intentions:["+ std::to_string(std::get<2>(reasoning_cycle_stats[i])) +"]", Logger::EveryInfo);

            stat = { { "Time", std::get<3>(reasoning_cycle_stats[i]).dbl() },
                    { "Iteration", std::get<0>(reasoning_cycle_stats[i]) },
                    { "Active_goals", std::get<1>(reasoning_cycle_stats[i]) },
                    { "Active_intentions", std::get<2>(reasoning_cycle_stats[i]) }
            };
            stats.push_back(stat);

            avg_activeGoals += std::get<1>(reasoning_cycle_stats[i]);
            avg_activeIntentions += std::get<2>(reasoning_cycle_stats[i]);
        }
        main = {  { "Reasoningcycle_stats", "Reasoning cycles stats" },
                { "Stats_rc", stats },
                { "Avg_active_goals_rc", avg_activeGoals / reasoning_cycle_stats.size() },
                { "Avg_active_intentions_rc", avg_activeIntentions / reasoning_cycle_stats.size() }};

        printFinish(" > AVG Goals: "+ std::to_string(avg_activeGoals / reasoning_cycle_stats.size())
        +", AVG Intentions: "+ std::to_string(avg_activeIntentions / reasoning_cycle_stats.size()));
    }
    else {
        main = { { "Reasoningcycle_stats", "Reasoning cycles stats" },
                { "Stats_rc", stats },
                { "Avg_active_goals_rc", 0 },
                { "Avg_active_intentions_rc", 0 }};
    }
    printFinish(" -------------------------------------------- ");

    write_metrics(main, agent_id);
}

void Metric::finishResponseTime(int agent_id) {
    printFinish(" Tasks Response Time:[tot="+ std::to_string(tasks_response_time.size()) +"]: ");
    json main, task_json;
    std::vector<json> tasks_json;
    std::shared_ptr<Task> task;
    double tot_responseTime = 0, avg_responseTime = 0;

    for(int i = 0; i < (int)tasks_response_time.size(); i++) {
        task = std::get<0>(tasks_response_time[i]);
        tot_responseTime += std::get<2>(tasks_response_time[i]).dbl();

        printFinish(" - Task:["+ std::to_string(task->getTaskId())
        +"], Plan_id:["+ std::to_string(task->getTaskPlanId())
        +"], Original Plan_id:["+ std::to_string(task->getTaskOriginalPlanId())
        +"], responseTime:["+ std::get<2>(tasks_response_time[i]).str()
        +"], arrivalTime:["+ std::to_string(task->getTaskArrivalTime())
        +"], finishTime:["+ std::get<1>(tasks_response_time[i]).str()
        +"], absolute_deadline:["+ std::to_string(task->getTaskArrivalTime() + task->getTaskDeadline() )
        +"], relative_deadline:["+ std::to_string(task->getTaskDeadline())
        +"], compTime:["+ std::to_string(task->getTaskCompTime())
        +"], firstActivation:["+ std::to_string(task->getTaskFirstActivation())
        +"], lastActivation:["+ std::to_string(task->getTaskLastActivation())
        +"], server:["+ std::to_string(task->getTaskServer()) +"]",
        Logger::EveryInfo);

        task_json = { { "taskId", task->getTaskId() },
                { "taskPlanId", task->getTaskPlanId() },
                { "taskOriginalPlanId", task->getTaskOriginalPlanId() },
                { "absolute_deadline", task->getTaskArrivalTime() + task->getTaskDeadline() },
                { "arrivalTime", task->getTaskArrivalTime() },
                { "arrivalTimeInitial", task->getTaskArrivalTimeInitial() },
                { "compTime", task->getTaskCompTime() },
                { "execution", task->getTaskExecutionType_as_string() },
                { "finishTime", std::get<1>(tasks_response_time[i]).dbl() },
                { "firstActivation", task->getTaskFirstActivation() },
                { "lastActivation", task->getTaskLastActivation() },
                { "n_exec", task->getTaskNExec() },
                { "n_execInitial", task->getTaskNExecInitial() },
                { "relative_deadline", task->getTaskDeadline() },
                { "responseTime", std::get<2>(tasks_response_time[i]).dbl() },
                { "server", task->getTaskServer() }
        };

        tasks_json.push_back(task_json);
    }

    if(tasks_response_time.size() > 0) {
        avg_responseTime = tot_responseTime / tasks_response_time.size();
    }
    printFinish(" > TOT: "+ std::to_string(tasks_response_time.size()) +" - Average Response Time: "+ std::to_string(avg_responseTime));

    main = { { "Num_of_executed_tasks", tasks_response_time.size() },
            { "Response_time", tasks_json },
            { "AVG_response_time", avg_responseTime } };
    printFinish(" -------------------------------------------- ");

    write_metrics(main, agent_id);
}

void Metric::finishSaveToFile(std::string folder, std::string user, std::string sched_type) {
    save_metrics_simpletext(folder, user, sched_type);
    save_metrics(folder, user, sched_type);
}

void Metric::finishStillActiveElements(const std::vector<std::shared_ptr<Task>> release_queue, const std::vector<std::shared_ptr<Task>> ready_queue,
        const std::vector<std::shared_ptr<Intention>>& ag_intention_set, const std::vector<std::shared_ptr<Goal>>& ag_goal_set,
        const std::vector<std::shared_ptr<Belief>>& ag_belief_set, int agent_id, bool simulation_limit_reached, const simtime_t max_simulation_time)
{
    json main; std::vector<json> events;
    int intention_waiting_completion = 0;

    main = { { "Still_active_events", "Still active" } };
    printFinish(" Timeline goals activation:[tot="+ std::to_string(activatedGoals.size()) +"]:");
    events = printActivatedGoals();
    main += { "Goals_activation", events };
    printFinish(" -------------------------------------------- ");
    printFinish(" Timeline intentions execution:[tot="+ std::to_string(activatedIntentions.size()) +"]:");
    events = printActivatedIntentions();
    main += { "Intentions_activation", events };
    printFinish(" -------------------------------------------- ");

    if(simulation_limit_reached) {
        main += { "Simulation_limit_reached", simulation_limit_reached };
        printFinish(" *** Max Simulation Time instant:["+ simTime().str() +"] has been reached! ***");
    }
    else {
        main += { "Simulation_limit_reached", simulation_limit_reached };
    }

    if(ag_intention_set.size() > 0) {
        for(int i = 0; i < (int)ag_intention_set.size(); i++)
        {
            if(ag_intention_set[i]->get_waiting_for_completion() && ag_intention_set[i]->get_scheduled_completion_time() >= max_simulation_time.dbl())
            {
                intention_waiting_completion += 1;
            }
        }

        // printFinish(" There are still '"+ std::to_string(ag_intention_set.size()) +"' active intention(s), '"+ std::to_string(intention_waiting_completion) +"' of them are waiting for completion!!!!!");
        printFinish(" There are still '"+ std::to_string(ag_intention_set.size()) +"' active intention(s)!");

        main += { "Still_active_intentions", ag_intention_set.size() };
        main += { "Still_active_intentions_waiting_completion", intention_waiting_completion };
        printFinish(" -------------------------------------------- ");
    }
    else {
        main += { "Still_active_intentions", 0 };
        main += { "Still_active_intentions_waiting_completion", 0 };
    }

    if(release_queue.size() > 0) {
        printFinish(" There are still '"+ std::to_string(release_queue.size()) +"' task(s) in RELEASE vector!!!!!");
        main += { "Release_vector", release_queue.size() };
        printFinish(" -------------------------------------------- ");
    }
    else {
        main += { "Release_vector", 0 };
    }

    if(ready_queue.size() > 0) {
        printFinish(" There are still '"+ std::to_string(ready_queue.size()) +"' task(s) in READY vector!!!!!");
        main += { "Ready_vector", ready_queue.size() };
        printFinish(" -------------------------------------------- ");
    }
    else {
        main += { "Ready_vector", 0 };
    }

    write_metrics(main, agent_id);
}

void Metric::finishUnexpectedTasksExecution(int agent_id) {
    json main, task;
    std::vector<json> tasks;
    std::vector<std::pair<simtime_t, std::shared_ptr<Task>>> unexpected = getUnexpectedTasksExecution();

    if(unexpected.size() > 0)
    {
        printFinish(" Unexpected task(s) executed during simulation:[tot="+ std::to_string(unexpected.size()) +"]:");

        for(int i = 0; i < (int)unexpected.size(); i++)
        {
            printFinish(" - t: "+ unexpected[i].first.str()
            +" task:["+ std::to_string(unexpected[i].second->getTaskId())
            +"], plan:["+ std::to_string(unexpected[i].second->getTaskPlanId())
            +"], original_plan:["+ std::to_string(unexpected[i].second->getTaskOriginalPlanId())
            +"] has been executed!!", Logger::EveryInfo);

            task = { { "Time", unexpected[i].first.dbl() },
                    { "Task", unexpected[i].second->getTaskId() },
                    { "Plan", unexpected[i].second->getTaskPlanId() },
                    { "Original_plan", unexpected[i].second->getTaskOriginalPlanId()}
            };
            tasks.push_back(task);
        }
        printFinish(" -------------------------------------------- ");
        main = { { "Unexpected_executions", "Unexpected executions" },
                { "Tot_exec", unexpected.size() },
                { "Tasks", tasks } };
    }
    else {
        main = { { "Unexpected_executions", "Unexpected executions" },
                { "Tot_exec", 0 },
                { "Tasks", tasks } };
    }

    write_metrics(main, agent_id);
}

std::string Metric::boolToString(bool b) {
    if(b == true)
        return "true";

    return "false";
}

std::string Metric::variantToString(std::variant<int, double, bool, std::string> variant)
{
    std::variant<int, double, bool, std::string> tmp;
    tmp = variant;

    if(auto val = std::get_if<int>(&tmp))
    { // it's a int
        int& s = *val;
        return std::to_string(s);
    }
    else if(auto val = std::get_if<double>(&tmp))
    { // it's a double
        double& s = *val;
        return std::to_string(s);
    }
    else if(auto val = std::get_if<bool>(&tmp))
    { // it's a bool
        bool& s = *val;
        return (s) ? "true" : "false";
    }
    else if(auto val = std::get_if<std::string>(&tmp))
    {
        std::string& s = *val;
        return s;
    }

    logger->print(Logger::Warning, "[WARNING] Not possible to convert variant to string. Type not supported.");
    return ""; // Type not supported (should not reach this point)
}

std::vector<std::tuple<int, int, int, simtime_t>>& Metric::getPhiIStats(){
    return phiI_stats;
}

std::vector<std::tuple<int, int, int, simtime_t>>& Metric::getReasoningCycleStats() {
    return reasoning_cycle_stats;
}

std::vector<std::tuple<std::shared_ptr<Task>, simtime_t, simtime_t>>& Metric::getTasksResponseTime() {
    return tasks_response_time;
}

std::vector<std::pair<simtime_t, std::shared_ptr<Task>>>& Metric::getUnexpectedTasksExecution() {
    return unexpected_tasks_execution;
}

// Setters -------------------------------------------------------------------------------
void Metric::setActivatedIntentionCompletion(std::shared_ptr<Intention> intention, simtime_t completionTime, std::string complitionReason) {
    std::shared_ptr<Intention> tmp_intention;
    double compTime;
    for(int i = 0; i < (int)activatedIntentions.size(); i++) {
        tmp_intention = std::get<0>(activatedIntentions[i]);
        compTime = std::get<2>(activatedIntentions[i]).dbl();
        if(tmp_intention->get_original_planID() == intention->get_original_planID() && compTime == -1) {
            std::get<2>(activatedIntentions[i]) = completionTime;
            std::get<3>(activatedIntentions[i]) = complitionReason;
        }
    }
}

void Metric::setActivatedGoalCompletion(std::shared_ptr<Goal> goal, simtime_t completionTime, std::string complitionReason) {
    std::shared_ptr<Goal> tmp_goal;
    simtime_t compTime;

    logger->print(Logger::EveryInfo, "setActivatedGoalCompletion: "+ std::to_string(goal->get_id())
    +", belief:("+ goal->get_goal_name() +")");
    logger->print(Logger::EveryInfo, "---------------------------");

    for(int i = 0; i < (int)activatedGoals.size(); i++) {
        tmp_goal = std::get<0>(activatedGoals[i]);
        compTime = std::get<2>(activatedGoals[i]);

        if(tmp_goal->get_id() == goal->get_id() && compTime == -1) {
            std::get<2>(activatedGoals[i]) = completionTime;
            std::get<6>(activatedGoals[i]) = complitionReason;
        }
    }
}

void Metric::setActivatedGoalCompletion(const int goal_id, const std::string goal_name, simtime_t completionTime, std::string complitionReason) {
    std::shared_ptr<Goal> tmp_goal;
    simtime_t compTime;

    logger->print(Logger::EveryInfo, "setActivatedGoalCompletion: "+ std::to_string(goal_id)
    +", belief:("+ goal_name +")");
    logger->print(Logger::EveryInfo, "---------------------------");

    for(int i = 0; i < (int)activatedGoals.size(); i++) {
        tmp_goal = std::get<0>(activatedGoals[i]);
        compTime = std::get<2>(activatedGoals[i]);

        if(tmp_goal->get_id() == goal_id && compTime == -1) {
            std::get<2>(activatedGoals[i]) = completionTime;
            std::get<6>(activatedGoals[i]) = complitionReason;
        }
    }
}

void Metric::setActivatedGoalsSelectedPlan(const std::vector<std::shared_ptr<Goal>>& goalset,
        const std::vector<std::shared_ptr<MaximumCompletionTime>>& selectedPlans,
        const std::vector<int>& pref_index, const std::vector<std::vector<std::shared_ptr<MaximumCompletionTime>>>& feasible_plans)
{
    logger->print(Logger::EveryInfo, " ----setActivatedGoalsSelectedPlan---- ");
    for(int i = 0; i < (int)goalset.size(); i++)
    {
        for(int j = 0; j < (int)activatedGoals.size(); j++)
        {
            logger->print(Logger::EveryInfo, std::to_string(goalset[i]->get_id())
            +" == "+ std::to_string(std::get<0>(activatedGoals[j])->get_id()));
            if(goalset[i]->get_id() == std::get<0>(activatedGoals[j])->get_id()) {

                if(selectedPlans[i]->getInternalGoalsSelectedPlansSize() > 0) {
                    if(std::get<3>(activatedGoals[j]) == -1 && std::get<4>(activatedGoals[j]) == -1) {
                        // update plan_id, original_planid, annidation_level
                        std::get<3>(activatedGoals[j]) = selectedPlans[i]->getInternalGoalsSelectedPlans()[0].first->get_id();
                        std::get<4>(activatedGoals[j]) = selectedPlans[i]->getInternalGoalsSelectedPlans()[0].first->get_id();
                        std::get<5>(activatedGoals[j]) = std::to_string(pref_index[i]);
                    }

                    for(int s = 0; s < (int)feasible_plans[i].size(); s++) {
                        if(selectedPlans[i]->getInternalGoalsSelectedPlans()[0].first->get_id() ==
                                feasible_plans[i][s]->getInternalGoalsSelectedPlans()[0].first->get_id())
                        {
                            // update selected plan position in feasible_plans
                            std::get<7>(activatedGoals[j]) = s;
                        }
                    }
                }
                else {
                    std::get<3>(activatedGoals[j]) = -1;
                    std::get<4>(activatedGoals[j]) = -1;
                    std::get<5>(activatedGoals[j]) = "undefined";
                }
            }
        }
    }
}

std::vector<json> Metric::printActivatedIntentions() {
    std::vector<json> intentions; json intention;
    if(activatedIntentions.size() > 0) {
        for(int i = 0; i < (int)activatedIntentions.size(); i++) {
            printFinish(" - Intention:["+ std::to_string(std::get<0>(activatedIntentions[i])->get_id())
            +"], goal ID:["+ std::to_string(std::get<0>(activatedIntentions[i])->get_goalID())
            +"], goal name:["+ std::get<0>(activatedIntentions[i])->get_goal_name()
            +"], original_plan:["+ std::to_string(std::get<0>(activatedIntentions[i])->get_original_planID())
            +"], activated at:["+ std::get<1>(activatedIntentions[i]).str()
            +"], completed at:["+ std::get<2>(activatedIntentions[i]).str()
            +"], due to:["+ std::get<3>(activatedIntentions[i]) +"]", Logger::EveryInfo);

            intention = { { "Intention_id", std::get<0>(activatedIntentions[i])->get_id() },
                    { "Goal_id", std::get<0>(activatedIntentions[i])->get_goalID() },
                    { "Goal_name", std::get<0>(activatedIntentions[i])->get_goal_name() },
                    { "Original_plan_id", std::get<0>(activatedIntentions[i])->get_original_planID() },
                    { "Activation_time", std::get<1>(activatedIntentions[i]).dbl() },
                    { "Completion_time", std::get<2>(activatedIntentions[i]).dbl() },
                    { "Due_to", std::get<3>(activatedIntentions[i]) }
            };
            intentions.push_back(intention);
        }
    }
    else {
        printFinish(" - No intentions activated!");
    }

    return intentions;
}

std::vector<json> Metric::printActivatedGoals() {
    std::vector<json> goals; json goal;

    if(activatedGoals.size() > 0) {
        for(int i = 0; i < (int)activatedGoals.size(); i++) {
            printFinish(" - Goal:["+ std::to_string(std::get<0>(activatedGoals[i])->get_id())
            +"], belief_name:["+ std::get<0>(activatedGoals[i])->get_goal_name()
            +"], deadline:["+ std::to_string(std::get<0>(activatedGoals[i])->get_DDL())
            +"], activated at:["+ std::get<1>(activatedGoals[i]).str()
            +"], completed at:["+ std::get<2>(activatedGoals[i]).str()
            +"], plan_id:["+ std::to_string(std::get<3>(activatedGoals[i]))
            +"], original_plan_id:["+ std::to_string(std::get<4>(activatedGoals[i]))
            +"], annidation_level:["+ std::get<5>(activatedGoals[i])
            +"], plan index:["+ std::to_string(std::get<7>(activatedGoals[i]))
            +"], due to:["+ std::get<6>(activatedGoals[i]) +"]", Logger::EveryInfo);

            goal = { { "Goal_id", std::get<0>(activatedGoals[i])->get_id() },
                    { "Belief_name", std::get<0>(activatedGoals[i])->get_goal_name() },
                    { "Deadline", std::get<0>(activatedGoals[i])->get_DDL() },
                    { "Activation_time", std::get<1>(activatedGoals[i]).dbl() },
                    { "Completion_time", std::get<2>(activatedGoals[i]).dbl() },
                    { "Plan_id", std::get<3>(activatedGoals[i]) },
                    { "Origina_plan_id", std::get<4>(activatedGoals[i]) },
                    { "Annidation_level", std::get<5>(activatedGoals[i]) },
                    { "Plan_pref_index", std::get<7>(activatedGoals[i]) },
                    { "Due_to", std::get<6>(activatedGoals[i]) }
            };
            goals.push_back(goal);
        }
    }
    else {
        printFinish(" - No goals activated!");
    }
    return goals;
}

void Metric::printAlreadyAnalizedIntention(const std::vector<std::pair<int, bool>>& vec, const std::vector<std::shared_ptr<Intention>>& ag_intention_set) {
    printFinish("Already analyzed intentions: ");

    for(int i = 0; i < (int)ag_intention_set.size(); i++) {
        printFinish(" - Intention["+ std::to_string(vec[i].first)
        +"], original_plan["+ std::to_string(ag_intention_set[i]->get_original_planID())
        +"], analyzed: "+ boolToString(vec[i].second));
    }
}

void Metric::printFinish(std::string line, Logger::Level level) {
    logger->print(level, line);
    write_metrics_simpletext(line);
}
