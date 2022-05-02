/*
 * EventsGenerator.h
 *
 *  Created on: Jan 29, 2021
 *      Author: francesco
 */

#ifndef HEADERS_EVENTSGENERATOR_H_
#define HEADERS_EVENTSGENERATOR_H_

#include "agentMSG_m.h"
#include "Belief.h"
#include "ConvertFile.h"
#include "Desire.h"
#include "Goal.h"
#include "jsonfile_handler.h"
#include "Logger.h"
#include "Operator.h"
#include "Plan.h"
#include "Reference_table.h"
#include "Sensor.h"
#include "server.h"
#include "task.h"
#include "writerinputs.h"

#include <cmath>        // in order to use ceil(...)
#include <exception>    // in order to do try {} catch (excpetion& e) {}
#include <iostream>     // debug (in order to use std::cout)
#include <memory>       // in order to use smart and unique pointers
#include <stdlib.h>     // to use "srand", "rand"
#include <time.h>       // to get time and date
#include <vector>
#include <fstream>

#include <omnetpp.h>
using namespace omnetpp;

class EventsGenerator : public cSimpleModule {
public:
    EventsGenerator();
    virtual ~EventsGenerator();

    virtual void initialize() override;
    virtual void finish() override;

private:
    // Use to truncate simTime value to the 3 decimal digits (Milliseconds)
    const SimTimeUnit sim_time_unit = SIMTIME_MS;

    const int agent_id = 0;
    int seed;
    int min_int_value, max_int_value, max_simulation_time;
    bool serversAlreadyRead = false, isBeliefsetSetup = false;
    std::shared_ptr<Logger> logger;
    std::unique_ptr<JsonfileHandler> jsonfileHandler;
    std::unique_ptr<WriterInputs> writer_inputs;
    std::vector<std::shared_ptr<Belief>> beliefset;
    std::vector<std::shared_ptr<Desire>> desireset;
    std::vector<std::shared_ptr<Plan>> planset;
    std::vector<std::shared_ptr<Sensor>> sensors;
    std::vector<std::shared_ptr<Server>> servers;
    std::vector<std::shared_ptr<Skill>> skillset;
    std::vector<std::shared_ptr<Belief>> update_beliefset;

    // Copy file from input to output folder -------------------
    void copyBeliefset(const std::string original_path);
    void copyPlanset(const std::string original_path, const std::string selected_sched_type);
    void copySkillset(const std::string original_path);
    void copyOrUpdateServers(const std::string path, const int min_server_budget, const int max_server_budget,
            const int max_server_period, const bool computeServersDinamically, std::string sched_type);
    // ---------------------------------------------------------

    // Create Files (with updated values) ----------------------
    void createDesireset(const std::string original_path, int delta_deadline);
    void createUpdateBeliefset(const std::vector<std::shared_ptr<Belief>>& beliefset, const int change_bool_items_prob);
    void createSensors(int num_of_sensors, int num_of_beliefs, const int change_bool_items_prob);
    // ---------------------------------------------------------

    // Save Data -----------------------------------------------
    void saveSimulation(std::string user, std::string generated_simulation_folder, std::string simulation_names, std::string destination_folder);
    // ---------------------------------------------------------

    // Support Functions ---------------------------------------
    void randomlyUpdateBelief(std::shared_ptr<Belief>& belief, const int change_bool_items_prob);
    void checkAndCorrectParameters(int& num_of_sensors, int& delta_deadline_percentage, int& max_simulation_time,
            int& min_int_value, int& max_int_value, int& seed);
    void clearElementsForNextIteration();
    // ---------------------------------------------------------
    // DEBUG functions -----------------------------------------
    std::string variantToString(const std::variant<int, double, bool, std::string>& variant);
    // ---------------------------------------------------------
};

Define_Module(EventsGenerator);

#endif /* HEADERS_EVENTSGENERATOR_H_ */
