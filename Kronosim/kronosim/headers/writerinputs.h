/*
 * writerinputs.h
 *
 *  Created on: Jan 25, 2021
 *      Author: francesco
 */

#ifndef HEADERS_WRITERINPUTS_H_
#define HEADERS_WRITERINPUTS_H_

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <boost/filesystem.hpp>
#include <cstdlib>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <variant>
#include <vector>

#include "Belief.h"
#include "Desire.h"
#include "Goal.h"
#include "json.hpp"      // to write to json file
#include "Logger.h"
#include "Operator.h"
#include "Plan.h"
#include "Sensor.h"
#include "server.h"
#include "Skill.h"
#include "task.h"

using namespace omnetpp; // in order to use EV
using namespace boost::filesystem;
using namespace std::chrono;
using json = nlohmann::json;

class WriterInputs {
public:
    WriterInputs();
    virtual ~WriterInputs();

    // INPUTS ---------------------------------------------------------------------
    void write_beliefset_json(const std::shared_ptr<Belief> belief);
    void write_desireset_json(const std::shared_ptr<Desire> desire);
    void write_planset_json(const std::shared_ptr<Plan> plan, const int debug_index);
    void write_sensors_json(const std::shared_ptr<Sensor> sensor);
    void write_servers_json(const std::shared_ptr<Server> server);
    void write_skillset_json(const std::shared_ptr<Skill> skill);
    void write_update_beliefset_json(const std::shared_ptr<Belief> belief);
    void write_execution_command(std::string command);

    void save_beliefset_json(std::string user, std::string simulation_folder, std::string simulation_name, const std::string oFolder);
    void save_desireset_json(std::string user, std::string simulation_folder, std::string simulation_name, const std::string oFolder);
    void save_planset_json(std::string user, std::string simulation_folder, std::string simulation_name, const std::string oFolder);
    void save_sensors_json(std::string user, std::string simulation_folder, std::string simulation_name, const std::string oFolder);
    void save_servers_json(std::string user, std::string simulation_folder, std::string simulation_name, const std::string oFolder);
    void save_skillset_json(std::string user, std::string simulation_folder, std::string simulation_name, const std::string oFolder);
    void save_update_beliefset_json(std::string user, std::string simulation_folder, std::string simulation_name, const std::string oFolder);
    void save_exection_commands(std::string user, std::string simulation_folder, std::string fast_ofileName, std::string path, std::string command_sched_type,
            std::string command_log_level, std::string cmd_analyze_simulation_command, const std::string oFolder);

    // Support functions -----------------------------------------------------------
    void create_reports_dir(std::string user, std::string simulation_folder, std::string oFolder, std::string simulation_name, bool full_check = true);
    void clear_json();
    json convertBeliefToJson(const std::shared_ptr<Belief> belief);
    std::string convertOperatorToString(Operator::Type op);
    json convertReferenceTableRecordToJson(const std::shared_ptr<Reference_table> record, const bool isPlansetTable);
    json convertSensorToJson(const std::shared_ptr<Sensor> sensor);
    std::string convertSensorModeToString(const std::shared_ptr<Sensor> sensor);
    // ----------------------------------------------------------------------------

    // DEBUG: ---------------------------------------------------------------------
    std::string variantToString(const std::variant<int, double, bool, std::string>& variant);
    // ----------------------------------------------------------------------------

protected:
    // INPUTS ---------------------------------------------------------------------
    // Complete PATH: ../simulations/francesco/generated_simulations/{simulation_name}/{simulation_name_seed_N}/inputs/{files JSON}
    const std::string INPUTS_PATH = "/inputs/";
    const std::string BELIEFSET = "/beliefset.json";
    const std::string DESIRESET = "/desireset.json";
    const std::string PLANSET = "/planset.json";
    const std::string SENSORS = "/sensors.json";
    const std::string SERVERS = "/servers.json";
    const std::string SKILLS = "/skillset.json";
    const std::string UPDATE_BELIEFSET = "/update-beliefset.json";

    const std::string agent_id = "0";
    std::vector<std::string> execution_commands;
    json beliefset_json;
    json desireset_json;
    json planset_json;
    json sensors_json;
    json servers_json;
    json skillset_json;
    json update_beliefset_json;
    json comparison_report_json;

    // ----------------------------------------------------------------------------
    Operator op;
    std::shared_ptr<Logger> logger;
    // ----------------------------------------------------------------------------
};


#endif /* WRITER_H_ */

