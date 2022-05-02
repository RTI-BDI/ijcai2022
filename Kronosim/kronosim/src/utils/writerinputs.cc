/*
 * writerinputs.cc
 *
 *  Created on: Jan 25, 2021
 *      Author: francesco
 */

#include "../../headers/writerinputs.h"

WriterInputs::WriterInputs() {
    logger = std::make_shared<Logger>(Logger::EssentialInfo);
}

WriterInputs::~WriterInputs() { }

// *****************************************************************************
// *******                       JSON - INPUTS                           *******
// *****************************************************************************
void WriterInputs::write_beliefset_json(const std::shared_ptr<Belief> belief) {
    std::variant<int, double, bool, std::string> tmp;
    tmp = belief->get_value();
    json belief_json;

    belief_json = convertBeliefToJson(belief);
    if(belief != nullptr) {
        beliefset_json[agent_id].push_back(belief_json);
    }
    else {
        logger->print(Logger::Error, "[ERROR] (write_beliefset_json) Belief:["+ belief->get_name() +"]'s type is NOT supported!");
    }
}

void WriterInputs::write_desireset_json(const std::shared_ptr<Desire> desire)
{
    // Setup belief -------------------------------------------------
    std::string goal_name = desire->get_goal_name();
    // --------------------------------------------------------------

    // Setup conditions ------------------------------------------
    json preconditions = desire->get_preconditions();
    json cont_conditions = desire->get_cont_conditions();
    // --------------------------------------------------------------

    // Setup reference table ----------------------------------------
    std::vector<std::shared_ptr<Reference_table>> reference_table = desire->get_priority_reference_table();
    std::vector<json> json_reference_table;
    json json_record;
    for(int i = 0; i < (int)reference_table.size(); i++) {
        json_record = convertReferenceTableRecordToJson(reference_table[i], false);
        if(json_record != nullptr) {
            json_reference_table.push_back(json_record);
        }
        else {
            logger->print(Logger::Error, "[ERROR] (writedesireset_json) Reference_table record:["+ reference_table[i]->get_belief_name() +"]'s type is NOT supported!");
        }
    }
    // --------------------------------------------------------------

    // Setup priority -------------------------------------------------
    json priority_json = {
            { "computedDynamically", true },
            { "formula", desire->get_priority_formula() },
            { "reference_table", json_reference_table }
    };
    // --------------------------------------------------------------

    // Setup desire -------------------------------------------------
    json desire_json = {
            { "goal_name", goal_name },
            { "preconditions", preconditions },
            { "cont_conditions", cont_conditions },
            { "deadline", desire->get_DDL() },
            { "priority", priority_json }
    };
    // --------------------------------------------------------------
    desireset_json[agent_id].push_back(desire_json);
}

void WriterInputs::write_planset_json(const std::shared_ptr<Plan> plan, const int debug_index)
{
    std::vector<std::shared_ptr<Effect>> effects_at_begin, effects_at_end;

    // Setup goal_name -------------------------------------------------
    std::string goal_name = plan->get_goal_name();
    // --------------------------------------------------------------

    // Setup conditions ------------------------------------------
    json preconditions = plan->get_preconditions();
    json cont_conditions = plan->get_cont_conditions();
    json post_conditions = plan->get_post_conditions();
    // --------------------------------------------------------------

    // Setup effects_at_begin ---------------------------------------
    std::vector<json> effects_at_begin_json;
    effects_at_begin = plan->get_effects_at_begin();
    for(int i = 0; i < (int)effects_at_begin.size(); i++) {
        effects_at_begin_json.push_back(effects_at_begin[i]->get_expression());
    }
    // --------------------------------------------------------------
    // Setup effects_at_end -----------------------------------------
    std::vector<json> effects_at_end_json;
    effects_at_end = plan->get_effects_at_end();
    for(int i = 0; i < (int)effects_at_end.size(); i++) {
        effects_at_end_json.push_back(effects_at_end[i]->get_expression());
    }
    // --------------------------------------------------------------

    // Setup body ---------------------------------------------------
    std::shared_ptr<Task> tmp_t;
    std::shared_ptr<Goal> tmp_g;
    std::vector<json> body_json;
    json action, task, goal, priority;
    std::vector<std::shared_ptr<Action>> actions = plan->get_body();
    for(int i = 0; i < (int)actions.size(); i++) {
        priority.clear();

        if(actions[i]->get_type() == "task") {
            tmp_t = std::dynamic_pointer_cast<Task>(actions[i]);

            task = { { "server", tmp_t->getTaskServer() },
                    { "n_exec", tmp_t->getTaskNExec() },
                    { "computationTime", tmp_t->getTaskCompTime() },
                    { "period", tmp_t->getTaskPeriod() },
                    { "relativeDeadline", tmp_t->getTaskPeriod() }
            };

            action = { { "action_type", "TASK" },
                    { "action", task },
                    { "execution", tmp_t->getTaskExecutionType_as_string() }
            };

            body_json.push_back(action);
        }
        else if(actions[i]->get_type() == "goal") {
            tmp_g = std::dynamic_pointer_cast<Goal>(actions[i]);
            priority = tmp_g->get_priority_formula(); // Note: use this line if "random_events_generator_network" is using seeds to generate new versions of the current one

            goal = {
                    { "goal_name", tmp_g->get_goal_name() },
                    { "arrivalTime", tmp_g->get_arrival_time() },
                    { "deadline", tmp_g->get_DDL() },
                    { "priority", priority }
            };

            if(goal != nullptr) {
                action = { { "action_type", "GOAL" },
                        { "action", goal },
                        { "execution", tmp_g->getGoalExecutionType_as_string() }
                };

                body_json.push_back(action);
            }
            else {
                logger->print(Logger::Error, "[ERROR] (writeplanset_json) Belief type CAN NOT be converted! Precondition ignored! (Plan:["+ std::to_string(plan->get_id())
                +"], action:["+ std::to_string(i) +"]");
            }
        }
    }
    // --------------------------------------------------------------

    // Setup reference table ----------------------------------------
    std::vector<std::shared_ptr<Reference_table>> reference_table = plan->get_pref_reference_table();
    std::vector<json> json_reference_table;
    json json_record;
    for(int i = 0; i < (int)reference_table.size(); i++) {
        json_record = convertReferenceTableRecordToJson(reference_table[i], true);
        if(json_record != nullptr) {
            json_reference_table.push_back(json_record);
        }
        else {
            logger->print(Logger::Error, "[ERROR] (write_desireset_json) Reference_table record:["+ reference_table[i]->get_belief_name() +"]'s type is NOT supported!");
        }
    }
    // --------------------------------------------------------------

    // Setup compute preference -------------------------------------
    json preference_json = {
            { "computedDynamically", true },
            { "formula", plan->get_pref_formula() },
            { "reference_table", json_reference_table }
    };
    // --------------------------------------------------------------

    // Finally, combine all data into a plan
    json plan_json = {
            { "_debug_index", debug_index },
            { "goal_name", goal_name },
            { "preference", preference_json },
            { "preconditions", preconditions },
            { "cont_conditions", cont_conditions },
            { "post_conditions", post_conditions },
            { "effects_at_begin", effects_at_begin_json },
            { "effects_at_end", effects_at_end_json },
            { "body", body_json }
    };
    // --------------------------------------------------------------
    planset_json[agent_id].push_back(plan_json);
}

void WriterInputs::write_sensors_json(const std::shared_ptr<Sensor> sensor) {
    json sensor_json = convertSensorToJson(sensor);

    if(sensor_json != nullptr)
    {
        sensors_json[agent_id].push_back(sensor_json);
    }
    else {
        logger->print(Logger::Error, "[ERROR] (write_sensors_json) Sensor:["+ std::to_string(sensor->get_id()) +"]'s type is NOT supported!");
    }
}

void WriterInputs::write_servers_json(const std::shared_ptr<Server> server)
{
    json server_json = {
            { "id", server->getTaskId() },
            { "period", server->get_period() },
            { "budget", server->get_budget() }
    };

    servers_json[agent_id].push_back(server_json);
}

void WriterInputs::write_skillset_json(const std::shared_ptr<Skill> skill)
{
    json skill_json = {
            { "goal_name", skill->get_goal_name() }
    };

    skillset_json[agent_id].push_back(skill_json);
}

void WriterInputs::write_update_beliefset_json(const std::shared_ptr<Belief> belief) {
    json belief_json = convertBeliefToJson(belief);

    if(belief_json != nullptr) {
        update_beliefset_json[agent_id].push_back(belief_json);
    }
    else {
        logger->print(Logger::Error, "[ERROR] (write_update_beliefset_json) Belief:["+ belief->get_name()+ "] type NOT supported!");
    }
}

void WriterInputs::write_execution_command(std::string command) {
    execution_commands.push_back(command);
}

// ---------------------------------------------------------------------------------------
void WriterInputs::save_beliefset_json(std::string user, std::string simulation_folder, std::string simulation_name, const std::string oFolder) {
    create_reports_dir(user, simulation_folder, oFolder, simulation_name);
    path inputs_dir("../simulations/" + user + oFolder + simulation_folder + simulation_name + INPUTS_PATH);
    std::ofstream o(inputs_dir.string() + BELIEFSET);
    o << std::setw(4) << beliefset_json << std::endl;
}

void WriterInputs::save_desireset_json(std::string user, std::string simulation_folder, std::string simulation_name, const std::string oFolder) {
    create_reports_dir(user, simulation_folder, oFolder, simulation_name);
    path inputs_dir("../simulations/" + user + oFolder + simulation_folder + simulation_name + INPUTS_PATH);
    std::ofstream o(inputs_dir.string() + DESIRESET);
    o << std::setw(4) << desireset_json << std::endl;
}

void WriterInputs::save_planset_json(std::string user, std::string simulation_folder, std::string simulation_name, const std::string oFolder) {
    create_reports_dir(user, simulation_folder, oFolder, simulation_name);
    path inputs_dir("../simulations/" + user + oFolder + simulation_folder + simulation_name + INPUTS_PATH);
    std::ofstream o(inputs_dir.string() + PLANSET);
    o << std::setw(4) << planset_json << std::endl;
}

void WriterInputs::save_sensors_json(std::string user, std::string simulation_folder, std::string simulation_name, const std::string oFolder) {
    create_reports_dir(user, simulation_folder, oFolder, simulation_name);
    path inputs_dir("../simulations/" + user + oFolder + simulation_folder + simulation_name + INPUTS_PATH);
    std::ofstream o(inputs_dir.string() + SENSORS);
    o << std::setw(4) << sensors_json << std::endl;
}

void WriterInputs::save_servers_json(std::string user, std::string simulation_folder, std::string simulation_name, const std::string oFolder) {
    create_reports_dir(user, simulation_folder, oFolder, simulation_name);
    path inputs_dir("../simulations/" + user + oFolder + simulation_folder + simulation_name + INPUTS_PATH);
    std::ofstream o(inputs_dir.string() + SERVERS);
    o << std::setw(4) << servers_json << std::endl;
}

void WriterInputs::save_skillset_json(std::string user, std::string simulation_folder, std::string simulation_name, const std::string oFolder) {
    create_reports_dir(user, simulation_folder, oFolder, simulation_name);
    path inputs_dir("../simulations/" + user + oFolder + simulation_folder + simulation_name + INPUTS_PATH);
    std::ofstream o(inputs_dir.string() + SKILLS);
    o << std::setw(4) << skillset_json << std::endl;
}

void WriterInputs::save_update_beliefset_json(std::string user, std::string simulation_folder, std::string simulation_name, const std::string oFolder) {
    create_reports_dir(user, simulation_folder, oFolder, simulation_name);
    path inputs_dir("../simulations/" + user + oFolder + simulation_folder + simulation_name + INPUTS_PATH);
    std::ofstream o(inputs_dir.string() + UPDATE_BELIEFSET);
    o << std::setw(4) << update_beliefset_json << std::endl;
}

void WriterInputs::save_exection_commands(std::string user, std::string simulation_folder, std::string fast_ofileName,
        std::string command, std::string command_sched_type, std::string command_log_level, std::string cmd_analyze_simulation_command, const std::string oFolder)
{
    create_reports_dir(user, simulation_folder, oFolder, fast_ofileName, false);
    path inputs_dir("../simulations/" + user + oFolder + simulation_folder); // <- "- simulation_folder": save the .sh in the main folder. Use "+ simulation_folder" to save it in the simulation folder + cd must be: ../../../../kronosim/
    std::ofstream fast_o(inputs_dir.string() + fast_ofileName);

    fast_o << "#!/bin/bash" << std::endl; // set initial line of the .sh document.
    fast_o << "# Run this file using the command: 'bash fast_execution_cmd.sh'" << std::endl;
    fast_o << "# Move from this folder, to the one containing the simulator: '../../../../kronosim/'" << std::endl;
    fast_o << "cd ../../../../kronosim/"<< std::endl;

    for(int i = 0; i < (int)execution_commands.size(); i++) {
        fast_o << "echo \" ****** START Running Simulation: "<< execution_commands[i] << " ****** \"" << std::endl;
        fast_o << command << "'\"" << oFolder << simulation_folder << execution_commands[i] << "\"'" << command_sched_type << command_log_level << std::endl;
        fast_o << "echo \" ****** ENDED Simulation: "<< execution_commands[i] << " ****** \"" << std::endl;
    }

    fast_o << "echo \" ****** START Analyze simulation: ****** \"" << std::endl;
    fast_o << cmd_analyze_simulation_command << std::endl;
    fast_o << "echo \" ****** ENDED Analyze simulation ****** \"" << std::endl;
}

// Support Functions ---------------------------------------------------------------------
void WriterInputs::create_reports_dir(std::string user, std::string simulation_folder, std::string oFolder, std::string simulation_name, bool full_check) {
    path user_dir("../simulations/" + user);
    if (!exists(user_dir)) {
        std::cout << "(user_dir) " << user_dir << " doesn't exist..."<< std::endl;
        EV << "(user_dir) " << user_dir << " doesn't exist..."<< std::endl;
        if (create_directory(user_dir))
            logger->print(Logger::EssentialInfo, "(create_reports_dir) ....Successfully Created !");
    }

    path generated_sim_dir = user_dir.append(oFolder);
    if (!exists(generated_sim_dir)) {
        std::cout << "(generated_sim_dir) " << generated_sim_dir << " doesn't exist..."<< std::endl;
        EV << "(generated_sim_dir) " << generated_sim_dir << " doesn't exist..."<< std::endl;
        if (create_directory(generated_sim_dir))
            logger->print(Logger::EssentialInfo, "(create_reports_dir) ....Successfully Created !");
    }

    if(full_check == true) {
        path sim_folder = user_dir.append(simulation_folder);
        if (!exists(sim_folder)) {
            std::cout << "(simulation_folder) " << sim_folder << " doesn't exist..."<< std::endl;
            EV << "(simulation_folder) " << sim_folder << " doesn't exist..."<< std::endl;
            if (create_directory(sim_folder))
                logger->print(Logger::EssentialInfo, "(create_reports_dir) ....Successfully Created !");
        }

        path main_dir = user_dir.append(simulation_name);
        if (!exists(main_dir)) {
            std::cout << "(main_dir) " << main_dir << " doesn't exist..."<< std::endl;
            EV << "(main_dir) " << main_dir << " doesn't exist..."<< std::endl;
            if (create_directory(main_dir))
                logger->print(Logger::EssentialInfo, "(create_reports_dir) ....Successfully Created !");
        }

        path input_dir = main_dir.append(INPUTS_PATH);
        if (!exists(input_dir)) {
            std::cout << "(input_dir) " << input_dir << " doesn't exist..."<< std::endl;
            EV << "(input_dir) " << input_dir << " doesn't exist..."<< std::endl;
            if (create_directory(input_dir))
                logger->print(Logger::EssentialInfo, "(create_reports_dir) ....Successfully Created !");
        }
    }
}

void WriterInputs::clear_json() {
    beliefset_json.clear();
    desireset_json.clear();
    planset_json.clear();
    sensors_json.clear();
    servers_json.clear();
    skillset_json.clear();
    update_beliefset_json.clear();
}

json WriterInputs::convertBeliefToJson(const std::shared_ptr<Belief> belief) {
    std::variant<int, double, bool, std::string> value;
    value = belief->get_value();
    json belief_json = nullptr;

    if(auto val = std::get_if<int>(&value))
    { // it's a int
        int& s = *val;
        belief_json = {
                { "name", belief->get_name() },
                { "value", s }
        };
    }
    else if(auto val = std::get_if<double>(&value))
    { // it's a double
        double& s = *val;
        belief_json = {
                { "name", belief->get_name() },
                { "value", s }
        };
    }
    else if(auto val = std::get_if<bool>(&value))
    { // it's a bool
        bool& s = *val;
        belief_json = {
                { "name", belief->get_name() },
                { "value", s }
        };
    }
    else if(auto val = std::get_if<std::string>(&value))
    {
        std::string& s = *val;
        belief_json = {
                { "name", belief->get_name() },
                { "value", s }
        };
    }
    return belief_json;
}

std::string WriterInputs::convertOperatorToString(Operator::Type op) {
    switch(op) {
    case 0: return "EQUAL";
    case 1: return "GREATER";
    case 2: return "GREATER_EQUAL";
    case 3: return "LESS";
    case 4: return "LESS_EQUAL";
    default: return "EQUAL";
    }
}

json WriterInputs::convertReferenceTableRecordToJson(const std::shared_ptr<Reference_table> record, const bool isPlansetTable)
{
    std::variant<int, double, bool, std::string> value;
    value = record->get_threshold_value();
    json json_rec = nullptr;
    std::string pref_pr = "priority";
    std::unique_ptr<Operator> op = std::make_unique<Operator>();

    if(isPlansetTable) {
        pref_pr = "pref";
    }

    if(auto val = std::get_if<int>(&value))
    { // it's a int
        int& s = *val;
        json_rec = {
                { "belief_name", record->get_belief_name() },
                { "op", op->convertOperatorToString(record->get_operator()) },
                { pref_pr, record->get_pr_pref() },
                { "threshold", s }
        };
    }
    else if(auto val = std::get_if<double>(&value))
    { // it's a double
        double& s = *val;
        json_rec = {
                { "belief_name", record->get_belief_name() },
                { "op", op->convertOperatorToString(record->get_operator()) },
                { pref_pr, record->get_pr_pref() },
                { "threshold", s }
        };
    }
    else if(auto val = std::get_if<bool>(&value))
    { // it's a bool
        bool& s = *val;
        json_rec = {
                { "belief_name", record->get_belief_name() },
                { "op", op->convertOperatorToString(record->get_operator()) },
                { pref_pr, record->get_pr_pref() },
                { "threshold", s }
        };
    }
    else if(auto val = std::get_if<std::string>(&value))
    {
        std::string& s = *val;
        json_rec = {
                { "belief_name", record->get_belief_name() },
                { "op", op->convertOperatorToString(record->get_operator()) },
                { pref_pr, record->get_pr_pref() },
                { "threshold", s }
        };
    }
    return json_rec;
}

json WriterInputs::convertSensorToJson(const std::shared_ptr<Sensor> sensor) {
    std::variant<int, double, bool, std::string> value;
    value = sensor->get_value();
    json sensor_json = nullptr;

    if(auto val = std::get_if<int>(&value))
    { // it's a int
        int& s = *val;
        sensor_json = {
                { "belief_name", sensor->get_belief_name() },
                { "value", s },
                { "time", sensor->get_time() },
                { "mode", convertSensorModeToString(sensor) }
        };
    }
    else if(auto val = std::get_if<double>(&value))
    { // it's a double
        double& s = *val;
        sensor_json = {
                { "belief_name", sensor->get_belief_name() },
                { "value", s },
                { "time", sensor->get_time() },
                { "mode", convertSensorModeToString(sensor) }
        };
    }
    else if(auto val = std::get_if<bool>(&value))
    { // it's a bool
        bool& s = *val;
        sensor_json = {
                { "belief_name", sensor->get_belief_name() },
                { "value", s },
                { "time", sensor->get_time() },
                { "mode", convertSensorModeToString(sensor) }
        };
    }
    else if(auto val = std::get_if<std::string>(&value))
    {
        std::string& s = *val;
        sensor_json = {
                { "belief_name", sensor->get_belief_name() },
                { "value", s },
                { "time", sensor->get_time() },
                { "mode", convertSensorModeToString(sensor) }
        };
    }
    return sensor_json;
}

std::string WriterInputs::convertSensorModeToString(const std::shared_ptr<Sensor> sensor)
{
    Sensor::Mode mode = sensor->get_mode();
    if(mode == Sensor::INCREASE) {
        return "INCREASE";
    }
    else if(mode == Sensor::DECREASE) {
        return "DECREASE";
    }
    else {
        // set the default value...
        return "SET";
    }
}

// DEBUG functions ----------------------------------------------------------------------
std::string WriterInputs::variantToString(const std::variant<int, double, bool, std::string>& variant)
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
        return s ? "true" : "false";
    }
    else if(auto val = std::get_if<std::string>(&tmp))
    {
        std::string& s = *val;
        return s;
    }

    logger->print(Logger::Warning, "[WARNING] Not possible to convert variant to string. Type not supported.");
    return ""; // Type not supported (should not reach this point)
}

