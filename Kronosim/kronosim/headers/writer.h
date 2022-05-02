/*
 * writer.h
 *
 *  Created on: 28 set 2017
 *      Author: peppe
 */

#ifndef WRITER_H_
#define WRITER_H_

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <cstdlib>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <boost/filesystem.hpp>
#include "json.hpp" // to write to json file
#include "task.h"

using namespace omnetpp; // in order to use EV
using namespace boost::filesystem;
using json = nlohmann::json;

//const std::string default_location = "default_simulation";
//void initialize_json_report();

json get_metrics_data();

void write_ddl_json_report(std::shared_ptr<Task> task, simtime_t cast_time, double c_res, double utilization, int num_tasks);
void write_lateness_json_report(std::shared_ptr<Task> task, int agent_id, double lateness, double finish_time);
void write_response_json_report(std::shared_ptr<Task> task, int agent_id, double finish_time, double resp_time);
void write_json_resp_per_task(std::shared_ptr<Task> task, int agent_id, double finish_time, double resp_time);
void write_util_json_report(std::shared_ptr<Task> task, int agent_id, double time, double utilization, const char*);
void write_pot_util_json(int agent_id, double time, double u_pot);
void write_ddl_checks(int plan_id, int original_plan_id, int task_id, int agent_id);
void write_simulation_timeline_json_report(std::string line); // BDI
void write_debug_isschedulable_json_report(std::string line); // BDI
void write_metrics_simpletext(std::string line);
void write_metrics(json data, int agent_id = -1);
void write_simulation_analysis(json data);

/**
 *   Write to file....
 */
bool write_stats_json_report(int agent_id, int ddl_checks, int ddl_miss, std::string folder, std::string user, const std::string sched_type);
bool write_acceptance_ratio(int agent_id, double ratio, std::string folder, std::string user, const std::string sched_type);

bool save_ddl_json(std::string folder, std::string user, std::string sched_type);
bool save_ddl_checks_json(std::string folder, std::string user, std::string sched_type);
bool save_debug_isschedulable(std::string folder, std::string user, std::string sched_type); // BDI
bool save_lateness_json(std::string folder, std::string user, std::string sched_type);
bool save_metrics(std::string folder, std::string user, std::string sched_type); // BDI
bool save_metrics_simpletext(std::string folder, std::string user, std::string sched_type); // BDI
bool save_pot_util_json(std::string folder, std::string user, std::string sched_type);
bool save_resp_per_task_json(std::string folder, std::string user, std::string sched_type);
bool save_response_json(std::string folder, std::string user, std::string sched_type);
bool save_simulation_comparison_json(std::string folder, std::string user, std::string simulation_name, std::string ofileName, json data);
bool save_simulation_timeline(std::string folder, std::string user, std::string sched_type); // BDI
bool save_util_json(std::string folder, std::string user, std::string sched_type);
bool save_simulation_analysis(std::string fileName, std::string folder, std::string user); // BDI

void delete_simulation_analysis_json_file(std::string fileName, std::string folder, std::string user);
void delete_report_folder_content(std::string folder, std::string user);
void delete_report_sched_type_folder(std::string folder, std::string user, std::string sched_type);

bool create_reports_dir(std::string folder, std::string user, std::string sched_type);
bool create_comapre_simulation_dir(std::string folder, std::string user, std::string simulation_name);
bool create_simulation_analysis_dir(std::string folder, std::string user);
// ----------------------------------------------------------------------------

#endif /* WRITER_H_ */

