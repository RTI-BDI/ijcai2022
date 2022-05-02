/*
 * EventsGenerator.cc
 *
 *  Created on: Jan 29, 2021
 *      Author: francesco
 */

#include "../headers/EventsGenerator.h"

EventsGenerator::EventsGenerator() {
    writer_inputs = std::make_unique<WriterInputs>();
}

EventsGenerator::~EventsGenerator() { }

void EventsGenerator::initialize()
{
    std::string selected_sched_type;
    std::string inputs = "/inputs/";
    std::string user = par("user");
    std::string destination_folder = par("destination_folder");
    std::string folder_path = par("folder_path"); // path containing the already generated files that the designer needs for the simulation
    std::string original_simulation_name = par("simulation_name");
    std::string original_path = "../simulations/"+ user +"/"+ folder_path +"/"+ original_simulation_name + inputs;
    std::string generated_simulation_name, generated_simulation_folder;
    std::string fast_outputFileName = par("fast_outputFileName");
    std::string cmd_command = par("cmd_command");
    std::string cmd_sched_type = par("cmd_sched_type");
    std::string cmd_log_level = par("cmd_log_level");
    std::string cmd_analyze_simulation = par("cmd_analyze_simulation");
    std::string cmd_analyze_simulation_log_level = par("cmd_analyze_simulation_log_level");
    std::string cmd_analyze_simulation_sched_type = par("cmd_analyze_simulation_sched_type");
    std::string cmd_analyze_simulation_folder_path = par("cmd_analyze_simulation_folder_path");
    int delta_deadline_percentage = par("delta_deadline_percentage");
    int initial_seed = par("initial_seed");
    int max_server_budget = par("max_server_budget");
    int max_server_period = par("max_server_period");
    int min_server_budget = par("min_server_budget");
    int num_of_sensors = par("num_of_sensors");
    int num_of_simulations = par("num_of_simulations");
    int sched_type = par("sched_type");
    int change_bool_items_prob = par("change_bool_items_prob");
    bool computeServersDinamically = par("computeServersDinamically");
    bool convertFile = par("convertFile");
    max_simulation_time = par("max_simulation_time");
    min_int_value = (int) par("min_numeric_value");
    max_int_value = (int) par("max_numeric_value");
    logger = std::make_shared<Logger>((int) par("logger_level"));
    jsonfileHandler = std::make_unique<JsonfileHandler>(logger, std::make_shared<Metric>());

    seed = initial_seed;
    cmd_sched_type = cmd_sched_type + std::to_string(sched_type);
    switch (sched_type) {
    case 0: selected_sched_type = "FCFS"; cmd_analyze_simulation_sched_type += "'\"FCFS\"'"; break;
    case 1: selected_sched_type = "RR"; cmd_analyze_simulation_sched_type += "'\"RR\"'"; break;
    default: selected_sched_type = "EDF"; cmd_analyze_simulation_sched_type += "'\"EDF\"'"; break;
    }

    cmd_analyze_simulation += "'\""+ original_simulation_name +"\"' ";
    cmd_analyze_simulation += cmd_analyze_simulation_sched_type;
    cmd_analyze_simulation += cmd_analyze_simulation_folder_path +"'\""+ destination_folder +"\"' ";
    cmd_analyze_simulation += cmd_analyze_simulation_log_level;

    if(convertFile) {
        std::unique_ptr<ConvertFile> convertFiles = std::make_unique<ConvertFile>(user, original_path, original_simulation_name, destination_folder);
    }
    else {
         logger->print(Logger::EveryInfo, " Original Folder:["+ original_path +"]");
         logger->print(Logger::EveryInfo, " Destination Folder:["+ destination_folder +"]");
        for(int i = initial_seed; i <= num_of_simulations; i++) {
            generated_simulation_folder = original_simulation_name + "/";
            generated_simulation_name = original_simulation_name + "_seed_"+ std::to_string(seed);

            srand(seed); // set seed: different numbers give different numerical distributions
            seed += 1;
            clearElementsForNextIteration();
            writer_inputs->clear_json();
            // -------------------------------------------------------------------------------
            logger->print(Logger::EssentialInfo, "...Generating files for simulation:["+ generated_simulation_name +"]");

            checkAndCorrectParameters(num_of_sensors, delta_deadline_percentage, max_simulation_time,
                    min_int_value, max_int_value, seed);

            copyBeliefset(original_path);
            copySkillset(original_path);
            copyOrUpdateServers(original_path, min_server_budget, max_server_budget, max_server_period, computeServersDinamically, selected_sched_type);
            copyPlanset(original_path, selected_sched_type);

            createDesireset(original_path, delta_deadline_percentage);
            createUpdateBeliefset(beliefset, change_bool_items_prob);
            createSensors(num_of_sensors, beliefset.size(), change_bool_items_prob);

            // store command for executing simulation i-th
            writer_inputs->write_execution_command(generated_simulation_name);

            saveSimulation(user, generated_simulation_folder, generated_simulation_name, destination_folder);
        }

        // save to file the list of commands for the generated simulations
        logger->print(Logger::EssentialInfo, "...Saving execution file:["+ fast_outputFileName +"]");
        logger->print(Logger::EveryInfo, "cmd_analyze_simulation: " + cmd_analyze_simulation);
        writer_inputs->save_exection_commands(user, generated_simulation_folder, fast_outputFileName, cmd_command, cmd_sched_type, cmd_log_level,
                cmd_analyze_simulation, destination_folder);
    }
}

void EventsGenerator::finish() { }

// Copy Files --------------------------------------------------
void EventsGenerator::copyBeliefset(const std::string original_path)
{
    if(isBeliefsetSetup == false) {
        jsonfileHandler->read_beliefset_from_json(original_path + "beliefset.json", beliefset, agent_id);
        logger->print(Logger::EveryInfo, "[copyBeliefset] Beliefset.size: "+ std::to_string(beliefset.size()));

        for(int i = 0; i < (int)beliefset.size(); i++) {
            writer_inputs->write_beliefset_json(beliefset[i]);
        }

        isBeliefsetSetup = true;
    }
}

void EventsGenerator::copyPlanset(const std::string original_path, const std::string selected_sched_type)
{
    try {
        jsonfileHandler->read_planset_from_json(original_path + "planset.json", beliefset, skillset, planset, servers, agent_id, "EventsGenerator");
    } catch (std::exception const& ex) {
        logger->print(Logger::Error, ex.what());
        // ag_metric->finishCrash(true, glob_path, glob_user, ex.what(), "JsonfileHandler", "read_planset_from_json", sim_current_time().dbl());
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < (int)planset.size(); i++) {
        writer_inputs->write_planset_json(planset[i], i);
    }
}

void EventsGenerator::copySkillset(const std::string original_path) {
    try {
        jsonfileHandler->read_skillset_from_json(original_path + "skillset.json", beliefset, skillset, agent_id);
    } catch (std::exception const& ex) {
        logger->print(Logger::Error, ex.what());
        // ag_metric->finishCrash(true, glob_path, glob_user, ex.what(), "JsonfileHandler", "read_skillset_from_json", sim_current_time().dbl());
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < (int)skillset.size(); i++) {
        writer_inputs->write_skillset_json(skillset[i]);
    }
}

void EventsGenerator::copyOrUpdateServers(const std::string original_path, const int min_server_budget, const int max_server_budget,
        const int max_server_period, const bool computeServersDinamically, std::string sched_type)
{
    double budget, period;

    if(serversAlreadyRead == false) {
        jsonfileHandler->read_servers_from_json(original_path + "servers.json", nullptr, servers, "EventsGenerator", agent_id);
        for(int i = 0; i < (int)servers.size(); i++) {
            if(computeServersDinamically == true) {
                budget = min_server_budget + (rand() % max_server_budget);
                period = budget * (1 + rand() % max_server_period); // period will be equal to: min = budget, max = budget * max_server_period

                servers[i]->set_budget(budget);
                servers[i]->set_curr_budget(budget);
                servers[i]->setTaskPeriod(period);
            }

            writer_inputs->write_servers_json(servers[i]);
        }
        serversAlreadyRead = true;
    }
}
// -------------------------------------------------------------

// Create Files ------------------------------------------------
void EventsGenerator::createDesireset(const std::string original_path, int delta_deadline) {
    double deadline;
    int max_ddl, min_ddl;
    if(isBeliefsetSetup == false) {
        copyBeliefset(original_path);
    }

    jsonfileHandler->read_desireset_from_json(original_path + "desireset.json", beliefset, skillset, desireset, agent_id);
    for(int i = 0; i < (int)desireset.size(); i++) {
        // get the original DDL
        deadline = desireset[i]->get_DDL();

        // compute the range in which the DDL will be computed:
        // -> delta_deadline is between 1 and 100 -> add/remove from deadline the percentage express in delta_deadline
        min_ddl = ceil(deadline - ((delta_deadline * 0.01) * deadline));
        max_ddl = ceil(deadline + ((delta_deadline * 0.01) * deadline));

        // compute the new deadline
        deadline = min_ddl + (rand() % max_ddl);

        if(deadline > 0) {
            // update the deadline of the desire
            desireset[i]->set_DDL(deadline);
        }

        // store the data
        writer_inputs->write_desireset_json(desireset[i]);
    }
}

void EventsGenerator::createUpdateBeliefset(const std::vector<std::shared_ptr<Belief>>& beliefset,
        const int change_bool_items_prob)
{
    std::shared_ptr<Belief> update_belief;
    std::variant<int, double, bool, std::string> tmp;

    for(int i = 0; i < (int)beliefset.size(); i++) {
        // 1. make a copy of the belief (in order to avoid changing the original belief's value)
        update_belief = std::make_shared<Belief>(beliefset[i]->get_name(), beliefset[i]->get_value());

        tmp = update_belief->get_value();

        // 2. update the new belief's value
        randomlyUpdateBelief(update_belief, change_bool_items_prob);

        // 3. store it
        writer_inputs->write_update_beliefset_json(update_belief);
    }
}

void EventsGenerator::createSensors(int num_of_sensors, int num_of_beliefs, const int change_bool_items_prob)
{ // Note: sensors are always created with MODE::CHANGE!!!!
    try {
        std::shared_ptr<Belief> belief;
        std::shared_ptr<Sensor> sensor;
        std::variant<int, double, bool, std::string> new_value;
        std::string belief_name;
        int rndBeliefIndex;
        double time;

        for(int i = 0; i < (int)num_of_sensors; i++) {
            // 1. select with belief to CHANGE
            rndBeliefIndex = rand() % num_of_beliefs;

            // 2. make a copy of the belief before updating its value
            belief = std::make_shared<Belief>(beliefset[rndBeliefIndex]->get_name(), beliefset[rndBeliefIndex]->get_value());

            // 3. get belief's name
            belief_name = belief->get_name();

            // 4. randomly update it value (based on its type)
            randomlyUpdateBelief(belief, change_bool_items_prob);
            new_value = belief->get_value();

            // 5. choose the instant in time when the CHANGE will be triggered
            time = (double) (rand() % max_simulation_time);

            sensor = std::make_shared<Sensor>(i, belief_name, new_value, time);

            writer_inputs->write_sensors_json(sensor);
        }
    }
    catch (std::exception const& ex) {
        throw std::runtime_error("[ERROR] While generating 'Sensors'");
    }
}
// -------------------------------------------------------------

// Save Data ---------------------------------------------------
void EventsGenerator::saveSimulation(std::string user, std::string generated_simulation_folder, std::string generated_simulation_name, std::string destination_folder) {
    writer_inputs->save_beliefset_json(user, generated_simulation_folder, generated_simulation_name, destination_folder);
    writer_inputs->save_desireset_json(user, generated_simulation_folder, generated_simulation_name, destination_folder);
    writer_inputs->save_planset_json(user, generated_simulation_folder, generated_simulation_name, destination_folder);
    writer_inputs->save_sensors_json(user, generated_simulation_folder, generated_simulation_name, destination_folder);
    writer_inputs->save_servers_json(user, generated_simulation_folder, generated_simulation_name, destination_folder);
    writer_inputs->save_skillset_json(user, generated_simulation_folder, generated_simulation_name, destination_folder);
    writer_inputs->save_update_beliefset_json(user, generated_simulation_folder, generated_simulation_name, destination_folder);
}
// -------------------------------------------------------------

// -------------------------------------------------------------
// ----------------------- Support functions -------------------
// -------------------------------------------------------------
void EventsGenerator::randomlyUpdateBelief(std::shared_ptr<Belief>& belief, const int change_bool_items_prob)
{
    std::variant<int, double, bool, std::string> tmp;

    if(belief != nullptr) {
        tmp = belief->get_value();

        if(std::get_if<int>(&tmp))
        { // it's a int
            // belief current value: int& s = *val;
            int new_val = min_int_value + (rand() % max_int_value);

            belief->set_value(new_val);
        }
        else if(std::get_if<double>(&tmp))
        { // it's a flodoubleat
            // belief current value: double& s = *val;
            double new_val = (double) min_int_value + (rand() % max_int_value);
            belief->set_value(new_val);
        }
        else if(auto val = std::get_if<bool>(&tmp))
        { // it's a bool
            bool& s = *val;
            int deb = (rand() % change_bool_items_prob); // generate an in number between 0 and change_bool_items_prob
            if(deb < 3) { // update belief's value (3 / change_bool_items_prob)% of the times
                belief->set_value(!s);
            }
        }
        else if(auto val = std::get_if<std::string>(&tmp))
        {
            logger->print(Logger::Warning, "[WARNING] (update_belief) Belief with value 'string' are actually not supported. Belief's value unchanged!");
            std::string& s = *val;
            belief->set_value(s);
        }
    }
}

void EventsGenerator::checkAndCorrectParameters(int& num_of_sensors, int& delta_deadline_percentage,
        int& max_simulation_time, int& min_int_value, int& max_int_value, int& seed)
{
    if(num_of_sensors < 0) {
        num_of_sensors = 0;
    }
    // -----------------------------------------------

    if(delta_deadline_percentage < 1) {
        delta_deadline_percentage = 1;
    }
    if(delta_deadline_percentage > 101) {
        delta_deadline_percentage = 101;
    }
    // -----------------------------------------------

    if(max_simulation_time < 0) {
        max_simulation_time = 0;
    }
    // -----------------------------------------------

    if(min_int_value < 0) {
        min_int_value = 0;
    }
    if(max_int_value > 101) {
        max_int_value = 101;
    }
    if(max_int_value < min_int_value) {
        int tmp = max_int_value;
        max_int_value = min_int_value;
        min_int_value = tmp;
    }
    // -----------------------------------------------

    if(seed < 1) {
        seed = 1;
    }
    // -----------------------------------------------
}

void EventsGenerator::clearElementsForNextIteration() {
    serversAlreadyRead = false;
    isBeliefsetSetup = false;
    beliefset.clear();
    desireset.clear();
    planset.clear();
    sensors.clear();
    servers.clear();
    skillset.clear();
    update_beliefset.clear();
}

// DEBUG functions ----------------------------------------------------------------------
std::string EventsGenerator::variantToString(const std::variant<int, double, bool, std::string>& variant)
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
