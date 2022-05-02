/*
 * writer.cc
 *
 *  Created on: 28 set 2017
 *      Author: peppe
 */

#include "../../headers/writer.h"

const std::string REPORT_PATH = "/reports";
const std::string ACC_RATIO = "/acc_ratio.json";
const std::string DEBUG_ISSCHEDULABLE = "/_debug_isschedulable.json";
const std::string DDL_CHECKS = "/ddl_checks.json";
const std::string DMR = "/ddl_miss.json";
const std::string LATENESS = "/lateness.json";
const std::string METRICS = "/metrics.json";
const std::string METRICS_simpletext = "/metrics_simpletext.json";
const std::string POT_UTIL = "pot_util.json";
const std::string RESP_PER_TASK = "/resp_per_task.json";
const std::string RESP_TIME = "/resp_time.json";
const std::string SIMULATION_TIMELINE = "/_simulation_timeline.json";
const std::string STATS = "/stats.json";
const std::string UTIL = "/util.json";

json ddl_json;
json lateness_json;
json stats_json;
json resp_time_json;
json resp_time_per_task_json;
json util_json;
json pot_util_json;
json acc_ratio_json;
json ddl_checks_json;
json simulation_timeline_json;  // BDI
json debug_isschedulable_json;  // BDI
json metrics_json;              // BDI
json metrics_simpletext_json;   // BDI
json simulation_analysis_json;  // BDI
// ----------------------------------------------------------------------------

// GETTERS --------------------------------------------------------------------------------
json get_metrics_data() {
    return metrics_json;
}

// *****************************************************************************
// *******                       JSON - REPORTS                          *******
// *****************************************************************************
void write_response_json_report(std::shared_ptr<Task> task, int agent_id, double finish_time, double resp_time) {
    json resp = { { "taskId", task->getTaskId() },
            { "taskPlanId", task->getTaskPlanId() },
            { "taskOriginalPlanId", task->getTaskOriginalPlanId() },
            { "absolute_deadline", task->getTaskAbsDDL() },
            { "arrivalTime", task->getTaskArrivalTime() },
            { "arrivalTimeInitial", task->getTaskArrivalTimeInitial() },
            { "compTime", task->getTaskCompTime() },
            { "deadline", task->getTaskDeadline() },
            { "execution", task->getTaskExecutionType_as_string() },
            { "finishTime", finish_time },
            { "firstActivation", task->getTaskFirstActivation() },
            { "lastActivation", task->getTaskLastActivation() },
            { "n_exec", task->getTaskNExec() },
            { "n_execInitial", task->getTaskNExecInitial() },
            { "responseTime", resp_time },
            { "server", task->getTaskServer() } };

    resp_time_json[agent_id].push_back(resp);
}

void write_util_json_report(std::shared_ptr<Task> task, int agent_id, double time, double utilization, const char* action) {
    json util = { { "action", action },
            { "time", time },
            { "id", task->getTaskId() },
            { "plan_id", task->getTaskPlanId() },
            { "original_plan_id", task->getTaskOriginalPlanId() },
            { "utilization", utilization } };
    util_json[agent_id].push_back(util);
}

void write_lateness_json_report(std::shared_ptr<Task> task, int agent_id, double lateness, double finish_time) {
    json late = { { "id", task->getTaskId() },
            { "plan_id", task->getTaskOriginalPlanId() },
            { "original_plan_id", task->getTaskPlanId() },
            { "executor", task->getTaskExecuter() },
            { "demander", task->getTaskDemander() },
            { "computationTime", task->getTaskCompTime() },
            { "arrivalTime", task->getTaskArrivalTime() },
            { "releaseTime", task->getTaskReleaseTime() },
            { "relativeDeadline", task->getTaskDeadline() },
            { "firstActivation", task->getTaskFirstActivation() },
            { "lastActivation", task->getTaskLastActivation() },
            { "server", task->getTaskServer() },
            { "period", task->getTaskPeriod() },
            { "finishTime", finish_time },
            { "lateness", lateness } };
    lateness_json.push_back(late);
}

void write_ddl_json_report(std::shared_ptr<Task> task, simtime_t cast_time, double c_res,
        double ag_utilization, int num_tasks) {
    json ddl = { { "id", task->getTaskId() },
            { "plan_id", task->getTaskOriginalPlanId() },
            { "original_plan_id", task->getTaskPlanId() },
            { "executor", task->getTaskExecuter() },
            { "demander", task->getTaskDemander() },
            { "computationTime", task->getTaskCompTime() },
            { "arrivalTime", task->getTaskArrivalTime() },
            { "releaseTime", task->getTaskReleaseTime() },
            { "relativeDeadline", task->getTaskDeadline() },
            { "firstActivation", task->getTaskFirstActivation() },
            { "lastActivation", task->getTaskLastActivation() },
            { "server", task->getTaskServer() },
            { "period", task->getTaskPeriod() },
            { "ddlMissTime", cast_time.dbl() },
            { "agUtilization", ag_utilization },
            { "runningTasks", num_tasks } };
    ddl_json.push_back(ddl);
}

void write_simulation_timeline_json_report(std::string line) { // BDI
    simulation_timeline_json.push_back(line);
}

void write_debug_isschedulable_json_report(std::string line) { // BDI
    debug_isschedulable_json.push_back(line);
}

void write_json_resp_per_task(std::shared_ptr<Task> task, int agent_id, double finish_time, double resp_time) {
    json resp = {
            { "arrivalTime", task->getTaskArrivalTime() },
            { "deadline", task->getTaskDeadline() },
            { "finishTime", finish_time },
            { "firstActivation", task->getTaskFirstActivation() },
            { "n_exec", task->getTaskNExec() },
            { "n_execInitial", task->getTaskNExecInitial() },
            { "responseTime", resp_time },
            { "server", task->getTaskServer() }
    };
    std::string task_id = std::to_string(task->getTaskId());
    std::string original_plan_id = std::to_string(task->getTaskOriginalPlanId());
    resp_time_per_task_json[agent_id][original_plan_id][task_id].push_back(resp);
}

void write_pot_util_json(int agent_id, double time, double u_pot) {
    json pot_util = { { "time", time }, { "u_pot", u_pot } };
    pot_util_json[agent_id].push_back(pot_util);
}

void write_ddl_checks(int plan_id, int original_plan_id, int task_id, int agent_id) {
    /* Note: ddl_checks_json structure is:
     * - one main array containing - one array for each agent
     * -- for agent X, N elements, from 0 to the highest original_plan value analyzed
     * -- if there are no tasks with original_plan n (i.e. n = 0), in position n (0 in this case) there will be **null,**
     * -- otherwise, an array containing the list of tasks referring to original_plan
     * --- for each task, it's stored the counter of how many time that task has been checked in the simulation.
     *
     * EXAMPLE:
     * t: 3.000000, COMPLETED tskId:[0], plan:[1], original_plan:[5]
     * t: 33.000000, COMPLETED tskId:[0], plan:[1], original_plan:[5]
     * t: 34.000000, COMPLETED tskId:[0], plan:[1], original_plan:[3]
     * t: 35.000000, COMPLETED tskId:[1], plan:[1], original_plan:[3]
     * t: 36.000000, COMPLETED tskId:[0], plan:[1], original_plan:[6]
     * t: 41.000000, COMPLETED tskId:[0], plan:[1], original_plan:[6]
     * t: 42.000000, COMPLETED tskId:[0], plan:[1], original_plan:[7]
     * t: 52.000000, COMPLETED tskId:[0], plan:[1], original_plan:[7]
     * t: 62.000000, COMPLETED tskId:[0], plan:[1], original_plan:[7]
     * t: 72.000000, COMPLETED tskId:[0], plan:[1], original_plan:[7]
     * t: 73.000000, COMPLETED tskId:[1], plan:[1], original_plan:[7]
     * t: 78.000000, COMPLETED tskId:[1], plan:[1], original_plan:[7]
     * t: 80.000000, COMPLETED tskId:[0], plan:[1], original_plan:[1]
     * t: 81.000000, COMPLETED tskId:[0], plan:[13], original_plan:[13]
     * t: 101.000000, COMPLETED tskId:[0], plan:[13], original_plan:[13]
     * t: 121.000000, COMPLETED tskId:[0], plan:[13], original_plan:[13]
     * t: 141.000000, COMPLETED tskId:[0], plan:[13], original_plan:[13]
     * t: 152.000000, COMPLETED tskId:[0], plan:[10], original_plan:[10]
     * t: 153.000000, COMPLETED tskId:[1], plan:[10], original_plan:[10]
     * t: 154.000000, COMPLETED tskId:[2], plan:[10], original_plan:[10]
     *
     * RESULT:
     *[ // agent 0
     *   null,   // original_plan 0
     *   [       // original_plan 1
     *       1   // task_id: 0
     *   ],
     *   null,   // original_plan 2
     *   [       // original_plan 3
     *       1,  // task_id: 0
     *       1   // task_id: 1
     *   ],
     *   null,   // original_plan 4
     *   [       // original_plan 5
     *       2   // task_id: 0
     *   ],
     *   [       // original_plan 6
     *       2   // task_id: 0
     *   ],
     *   [       // original_plan 7
     *       4,  // task_id: 0
     *       2   // task_id: 1
     *   ],
     *   null,   // original_plan 8
     *   null,   // original_plan 9
     *   [       // original_plan 10
     *       1,  // task_id: 0
     *       1,  // task_id: 1
     *       1   // task_id: 2
     *   ],
     *   null,   // original_plan 11
     *   [       // original_plan 12
     *       1   // task_id: 0
     *   ],
     *   [       // original_plan 13
     *       4   // task_id: 0
     *   ]
     *]
     */
    if (ddl_checks_json[agent_id][original_plan_id][task_id].empty())
        ddl_checks_json[agent_id][original_plan_id][task_id] = 1;
    else {
        int new_check = ddl_checks_json[agent_id][original_plan_id][task_id];
        ddl_checks_json[agent_id][original_plan_id][task_id] = new_check + 1;
    }
}

void write_metrics_simpletext(std::string line) { // BDI
    metrics_simpletext_json.push_back(line);
}

void write_metrics(json line, int agent_id) { // BDI
    metrics_json.push_back(line);
}

void write_simulation_analysis(json line) {
    simulation_analysis_json.push_back(line);
}

/**
 *   Write to file....
 */
bool write_stats_json_report(int agent_id, int ddl_checks, int ddl_miss, std::string folder, std::string user, const std::string sched_type) {
    stats_json[agent_id] = {
            {   "ddlCheckCount", ddl_checks},
            {   "ddlMissCount", ddl_miss}
    };

    if(create_reports_dir(folder, user, sched_type))
    {
        path report_dir("../simulations/" + user + "/" + folder + REPORT_PATH + "/" + sched_type);
        std::ofstream o(report_dir.string() + STATS);
        o << std::setw(4) << stats_json << std::endl;

        return true;
    }
    return false;
}

bool write_acceptance_ratio(int agent_id, double ratio, std::string folder, std::string user, const std::string sched_type) {
    acc_ratio_json[agent_id] = {
            {   "acc_ratio", ratio},
    };

    if(create_reports_dir(folder, user, sched_type))
    {
        path report_dir("../simulations/" + user + "/" + folder + REPORT_PATH + "/" + sched_type);
        std::ofstream o(report_dir.string() + ACC_RATIO);
        o << std::setw(4) << acc_ratio_json << std::endl;

        return true;
    }
    return false;
}


// SAVE FUNCTIONS -----------------------------------------------------------------------
bool save_ddl_checks_json(std::string folder, std::string user, std::string sched_type) {
    if(create_reports_dir(folder, user, sched_type))
    {
        path report_dir("../simulations/" + user + "/" + folder + REPORT_PATH + "/" + sched_type + "/" + sched_type);
        std::ofstream o(report_dir.string() + DDL_CHECKS);
        o << std::setw(4) << ddl_checks_json << std::endl;

        return true;
    }
    return false;
}

bool save_ddl_json(std::string folder, std::string user, std::string sched_type) {
    if(create_reports_dir(folder, user, sched_type))
    {
        path report_dir("../simulations/" + user + "/" + folder + REPORT_PATH + "/" + sched_type);
        std::ofstream o(report_dir.string() + DMR);
        o << std::setw(4) << ddl_json << std::endl;

        return true;
    }
    return false;
}

bool save_debug_isschedulable(std::string folder, std::string user, std::string sched_type) {
    if(create_reports_dir(folder, user, sched_type))
    {
        path report_dir("../simulations/" + user + "/" + folder + REPORT_PATH + "/" + sched_type);
        std::ofstream o(report_dir.string() + DEBUG_ISSCHEDULABLE);
        o << std::setw(4) << debug_isschedulable_json << std::endl;

        return true;
    }
    return false;
}

bool save_metrics(std::string folder, std::string user, std::string sched_type) {
    if(create_reports_dir(folder, user, sched_type))
    {
        path report_dir("../simulations/" + user + "/" + folder + REPORT_PATH + "/" + sched_type);
        std::ofstream o(report_dir.string() + METRICS);
        o << std::setw(4) << metrics_json << std::endl;

        return true;
    }
    return false;
}

bool save_metrics_simpletext(std::string folder, std::string user, std::string sched_type) {
    if(create_reports_dir(folder, user, sched_type))
    {
        path report_dir("../simulations/" + user + "/" + folder + REPORT_PATH + "/" + sched_type);
        std::ofstream o(report_dir.string() + METRICS_simpletext);
        o << std::setw(4) << metrics_simpletext_json << std::endl;

        return true;
    }
    return false;
}

bool save_lateness_json(std::string folder, std::string user, std::string sched_type) {
    if(create_reports_dir(folder, user, sched_type))
    {
        path report_dir("../simulations/" + user + "/" + folder + REPORT_PATH + "/" + sched_type);
        std::ofstream o(report_dir.string() + LATENESS);
        o << std::setw(4) << lateness_json << std::endl;

        return true;
    }
    return false;
}

bool save_pot_util_json(std::string folder, std::string user, std::string sched_type) {
    if(create_reports_dir(folder, user, sched_type))
    {
        path report_dir("../simulations/" + user + "/" + folder + REPORT_PATH + "/" + sched_type);
        std::ofstream o(report_dir.string() + POT_UTIL);
        o << std::setw(4) << pot_util_json << std::endl;

        return true;
    }
    return false;
}

bool save_resp_per_task_json(std::string folder, std::string user, std::string sched_type) {
    if(create_reports_dir(folder, user, sched_type))
    {
        path report_dir("../simulations/" + user + "/" + folder + REPORT_PATH + "/" + sched_type);
        std::ofstream o(report_dir.string() + RESP_PER_TASK);
        o << std::setw(4) << resp_time_per_task_json << std::endl;

        return true;
    }
    return false;
}

bool save_response_json(std::string folder, std::string user, std::string sched_type) {
    if(create_reports_dir(folder, user, sched_type))
    {
        path report_dir("../simulations/" + user + "/" + folder + REPORT_PATH + "/" + sched_type);
        std::ofstream o(report_dir.string() + "/resp_time.json");
        o << std::setw(4) << resp_time_json << std::endl;

        return true;
    }
    return false;
}

bool save_simulation_analysis(std::string fileName, std::string folder, std::string user) {
    if(create_simulation_analysis_dir(folder, user))
    {
        path dir("../simulations/" + user + "/" + folder);
        std::ofstream o(dir.string() +"/"+ fileName);
        o << std::setw(4) << simulation_analysis_json << std::endl;

        return true;
    }
    return false;
}

bool save_simulation_comparison_json(std::string folder, std::string user, std::string simulation_name,
        std::string ofileName, json data)
{
    if(create_comapre_simulation_dir(folder, user, simulation_name))
    {
        path dir("../simulations/" + user + "/" + folder + "/"+ simulation_name);
        std::ofstream o(dir.string() +"/"+ ofileName);
        o << std::setw(4) << data << std::endl;

        return true;
    }
    return false;
}

bool save_simulation_timeline(std::string folder, std::string user, std::string sched_type) {
    if(create_reports_dir(folder, user, sched_type))
    {
        path report_dir("../simulations/" + user + "/" + folder + REPORT_PATH + "/" + sched_type);
        std::ofstream o(report_dir.string() + SIMULATION_TIMELINE);
        o << std::setw(4) << simulation_timeline_json << std::endl;

        return true;
    }
    return false;
}

bool save_util_json(std::string folder, std::string user, std::string sched_type) {
    if(create_reports_dir(folder, user, sched_type))
    {
        path report_dir("../simulations/" + user + "/" + folder + REPORT_PATH + "/" + sched_type + "/" + sched_type);
        std::ofstream o(report_dir.string() + UTIL);
        o << std::setw(4) << util_json << std::endl;

        return true;
    }
    return false;
}

// DELETE REPORTS/... ------------------------------------------------------------------------------------
void delete_simulation_analysis_json_file(std::string fileName, std::string folder, std::string user) {
    // check if report directory exists
    path user_dir("../simulations/" + user);
    if (exists(user_dir))
    {
        path main_dir = user_dir.append(folder);
        if (exists(main_dir)) {
            // check if simulation_analysis.json  exist
            if(remove(main_dir.string() +"/"+ fileName) != 0) {
                EV_ERROR << " File '"<< main_dir.string() +"/"+ fileName <<"' deleted!" << std::endl;
                std::cerr << " File '"<< main_dir.string() +"/"+ fileName <<"' deleted!" << std::endl;
            }
            else {
                EV_ERROR << " Error while deleting File: '"<< main_dir.string() +"/"+ fileName <<"'!" << std::endl;
                std::cerr << " Error while deleting File '"<< main_dir.string() +"/"+ fileName <<"'!" << std::endl;
            }

        }
    }
}

void delete_report_folder_content(std::string folder, std::string user) {
    // check if report directory exists
    path user_dir("../simulations/" + user);
    if (exists(user_dir))
    {
        path main_dir = user_dir.append(folder);
        if (exists(main_dir)) {
            path report_dir = main_dir.append(REPORT_PATH);
            if (exists(report_dir)) {
                if(remove_all(report_dir.string()) != 0) {
                    EV_ERROR << " Folder: '"<< report_dir.string() <<"' deleted!" << std::endl;
                    std::cerr << " Folder: '"<< report_dir.string()  <<"' deleted!" << std::endl;
                }
                else {
                    EV_ERROR << " Error while deleting Folder: '"<< report_dir.string() <<"'!" << std::endl;
                    std::cerr << " Error while deleting Folder: '"<<report_dir.string() <<"'!" << std::endl;
                }
            }
        }
    }
}

void delete_report_sched_type_folder(std::string folder, std::string user, std::string sched_type) {
    // check if report directory exists
    path user_dir("../simulations/" + user);
    if (exists(user_dir))
    {
        path main_dir = user_dir.append(folder);
        if (exists(main_dir))
        {
            path report_dir = main_dir.append(REPORT_PATH);
            if (exists(report_dir))
            {
                path sched_type_dir = main_dir.append("/"+ sched_type);
                if (exists(sched_type_dir)) {
                    if(remove_all(sched_type_dir.string()) != 0) {
                        EV_ERROR << " Folder: '"<< sched_type_dir.string() <<"' deleted!" << std::endl;
                        std::cerr << " Folder: '"<< sched_type_dir.string()  <<"' deleted!" << std::endl;
                    }
                    else {
                        EV_ERROR << " Error while deleting Folder: '"<< sched_type_dir.string() <<"'!" << std::endl;
                        std::cerr << " Error while deleting Folder: '"<< sched_type_dir.string() <<"'!" << std::endl;
                    }
                }
            }
        }
    }
}
// ----------------------------------------------------------------------------------------

// CREATE FUNCTION ------------------------------------------------------------------------
bool create_reports_dir(std::string folder, std::string user, std::string sched_type) {
    try
    {
        path user_dir("../simulations/" + user);
        if (!exists(user_dir)) {
            std::cout << user_dir << " doesn't exist" << endl;
            if (create_directory(user_dir)) {
                std::cout << "....Successfully Created ("<< user_dir <<")" << endl;
                EV << "....Successfully Created ("<< user_dir <<")" << endl;
            }
        }

        path main_dir = user_dir.append(folder);
        if (!exists(main_dir))
        {
            std::cout << main_dir << " doesn't exist" << endl;
            if (create_directory(main_dir)) {
                std::cout << "....Successfully Created ("<< main_dir <<")" << endl;
                EV << "....Successfully Created ("<< main_dir <<")" << endl;
            }
        }

        path report_dir = main_dir.append(REPORT_PATH);
        if (!exists(report_dir)) {
            std::cout << report_dir << " doesn't exist" << endl;
            if (create_directory(report_dir)) {
                std::cout << "....Successfully Created ("<< report_dir <<")" << endl;
                EV << "....Successfully Created ("<< report_dir <<")" << endl;
            }
        }

        if(sched_type == "EDF" || sched_type == "RR" || sched_type == "FCFS") {
            path sched_type_dir = main_dir.append(sched_type);
            if (!exists(sched_type_dir)) {
                std::cout << sched_type_dir << " doesn't exist" << endl;
                if (create_directory(sched_type_dir)) {
                    std::cout << "....Successfully Created ("<< sched_type_dir <<")" << endl;
                    EV << "....Successfully Created ("<< sched_type_dir <<")" << endl;
                }
            }
        }

        return true;
    }
    catch(std::exception& e) {
        std::cout << "[ERROR]: "<< e.what() << std::endl;
        EV_ERROR << "[ERROR]: "<< e.what() << std::endl;
    }
    return false;
}

bool create_comapre_simulation_dir(std::string folder, std::string user, std::string simulation_name) {
    try
    {
        path user_dir("../simulations/" + user);
        if (!exists(user_dir)) {
            std::cout << user_dir << " doesn't exist" << endl;
            if (create_directory(user_dir)) {
                std::cout << "....Successfully Created ("<< user_dir <<")" << endl;
                EV << "....Successfully Created ("<< user_dir <<")" << endl;
            }
        }

        path main_dir = user_dir.append(folder); // e.g., ../simulations/francesco/generated_simulations/
        if (!exists(main_dir)) {
            std::cout << main_dir << " doesn't exist" << endl;
            if (create_directory(main_dir)) {
                std::cout << "....Successfully Created ("<< main_dir <<")" << endl;
                EV << "....Successfully Created ("<< main_dir <<")" << endl;
            }
        }

        path sim_dir = main_dir.append(simulation_name); // e.g., ../simulations/francesco/generated_simulations/simulation_name/
        if (!exists(sim_dir)) {
            std::cout << sim_dir << " doesn't exist" << endl;
            if (create_directory(sim_dir)) {
                std::cout << "....Successfully Created ("<< sim_dir <<")" << endl;
                EV << "....Successfully Created ("<< sim_dir <<")" << endl;
            }
        }
        return true;
    }
    catch(std::exception& e) {
        std::cout << e.what() << std::endl;
        EV_WARN << e.what() << std::endl;
    }
    return false;
}

bool create_simulation_analysis_dir(std::string folder, std::string user) {
    try
    {
        path user_dir("../simulations/" + user);
        if (!exists(user_dir)) {
            std::cout << user_dir << " doesn't exist" << endl;
            if (create_directory(user_dir)) {
                std::cout << "....Successfully Created ("<< user_dir <<")" << endl;
                EV << "....Successfully Created ("<< user_dir <<")" << endl;
            }
        }

        path main_dir = user_dir.append(folder);
        if (!exists(main_dir)) {
            std::cout << main_dir << " doesn't exist" << endl;
            if (create_directory(main_dir)) {
                std::cout << "....Successfully Created ("<< main_dir <<")" << endl;
                EV << "....Successfully Created ("<< main_dir <<")" << endl;
            }
        }
        return true;
    }
    catch(std::exception& e) {
        std::cout << e.what() << std::endl;
        EV_WARN << e.what() << std::endl;
    }
    return false;
}


