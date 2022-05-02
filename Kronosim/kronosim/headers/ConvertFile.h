/*
 * ConvertFile.h
 *
 *  Created on: Mar 8, 2021
 *      Author: francesco
 */

#ifndef SRC_CONVERTFILE_H_
#define SRC_CONVERTFILE_H_

#include <memory>       // in order to use smart and unique pointers
#include <exception>    // in order to do try {} catch (exception& e) {}
#include <iostream>     // debug (in order to use std::cout)
#include <fstream>
#include <omnetpp.h>
#include <math.h>       // in order to use function pow(,)
#include <ctype.h>      // allows to use isaplha() and check if char is alphabetic
#include <boost/algorithm/string.hpp>           // in order to do: boost::to_upper_copy(...)
#include <boost/algorithm/string/replace.hpp>   // in order to use boost::replace_all on strings

#include "Belief.h"
#include "Desire.h"
#include "Goal.h"
#include "json.hpp" // to read from json file
#include "Operator.h"
#include "Plan.h"
#include "Reference_table.h"
#include "Sensor.h"
#include "server.h"
#include "Skill.h"
#include "task.h"
#include "writerinputs.h"
using json = nlohmann::json;

using namespace omnetpp; // in order to use EV

class ConvertFile {
public:
    ConvertFile(std::string user, std::string path, std::string original_simulation_name, std::string destination_folder);
    virtual ~ConvertFile();

    /** Create "skillset.json" automatically from "desireset.json" and "planset.json" */
    void read_desireset_from_json(std::string user, std::string path, std::vector<std::shared_ptr<Belief>>& ag_beliefset,
            std::vector<std::shared_ptr<Skill>>& ag_skillset, std::vector<std::shared_ptr<Desire>>& ag_desireset, int agent_id);
    void read_planset_from_json(std::string user, std::string path, std::vector<std::shared_ptr<Belief>>& ag_beliefset,
            std::vector<std::shared_ptr<Skill>>& ag_skillset, std::vector<std::shared_ptr<Plan>>& ag_planset, int agent_id);
    bool compareBeliefValues(std::variant<int, double, bool, std::string> belief_value,
            std::variant<int, double, bool, std::string> comparison_value);
    void create_skillset_json(std::string user, std::string path, std::string destination_folder, std::string ofileName,
            std::vector<std::shared_ptr<Skill>>& ag_skills, int ag_id);
    /* ----------------------------------------------------------------------------- */

    void convert_beliefset_from_json(std::string user, std::string path, std::string destination_folder, std::string ofileName,
            std::vector<std::shared_ptr<Belief>>& ag_beliefset, int ag_id);
    void convert_desireset_from_json(std::string user, std::string path, std::string destination_folder, std::string ofileName,
            const std::vector<std::shared_ptr<Belief>>& ag_beliefset,
            const std::vector<std::shared_ptr<Skill>>& ag_skillset, std::vector<std::shared_ptr<Desire>>& ag_desireset, int ag_id);

    void convert_planset_from_json(std::string user, std::string path, std::string destination_folder, std::string ofileName,
            const std::vector<std::shared_ptr<Belief>>& ag_beliefset,
            const std::vector<std::shared_ptr<Server>>& servers, const std::vector<std::shared_ptr<Skill>>& ag_skillset,
            std::vector<std::shared_ptr<Plan>>& ag_plans, int ag_id, std::string schedule_type_name);

    void convert_sensors_from_json(std::string user, std::string path, std::string destination_folder, std::string ofileName, std::vector<std::shared_ptr<Sensor>>& sensors_time,
            std::vector<std::shared_ptr<Sensor>>& ag_sensors, int ag_id);

    void convert_servers_from_json(std::string user, std::string path, std::string destination_folder, std::string ofileName, std::vector<std::shared_ptr<Server>>& ag_servers, int ag_id);
    void convert_skillset_from_json(std::string user, std::string path, std::string destination_folder, std::string ofileName, std::vector<std::shared_ptr<Skill>>& ag_skills, int ag_id);
    void convert_update_beliefset_from_json(std::string user, std::string path, std::string destination_folder, std::string ofileName, std::vector<std::shared_ptr<Belief>>& ag_beliefset, int ag_id);

private:
    std::string checkIfFormulaIsSet(const std::string formula, const std::string msg);
    bool checkIfGoalHasCorrectBeliefName(const std::vector<std::shared_ptr<Belief>>& beliefset, const std::shared_ptr<Belief> action_belief);

    ServerType convertIntToServerType(int value);
    std::string convertJsonValueToVariant(json json_value, std::variant<int, double, bool, std::string>& value);

    Goal::Status convertStringToGoalStatus(std::string value);
    Task::Execution convertStringToTaskExecutionType(std::string value, std::string scheduler);
    Goal::Execution convertStringToGoalExecutionType(std::string value, std::string scheduler);
    Operator::Type convertStringToOperator(std::string value);
    Sensor::Mode convertStringToSensorMode(std::string value);

    std::shared_ptr<Belief> findBeliefWithGivenName(const std::vector<std::shared_ptr<Belief>>& beliefset, std::string belief_name_ref);
    int getServerIndexInAg_servers(const std::vector<std::shared_ptr<Server>>& servers, const int server_id);
    std::string getSkillValueType_asString(std::variant<int, double, bool, std::string>& value);

    const int agent_id = 0;
    std::unique_ptr<WriterInputs> writer_inputs;
    std::vector<std::shared_ptr<Belief>> beliefset;
    std::vector<std::shared_ptr<Desire>> desireset;
    std::vector<std::shared_ptr<Plan>> planset;
    std::vector<std::shared_ptr<Sensor>> sensors_time;
    std::vector<std::shared_ptr<Sensor>> sensors_plans;
    std::vector<std::shared_ptr<Server>> servers;
    std::vector<std::shared_ptr<Skill>> skillset;
    std::vector<std::shared_ptr<Belief>> update_beliefset;
};


#endif /* SRC_CONVERTFILE_H_ */
