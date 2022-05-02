/*
 * json_handler.h
 *
 *  Created on: Mar 9, 2020
 *      Author: francesco
 */

#ifndef HEADERS_JSONFILE_HANDLER_H_
#define HEADERS_JSONFILE_HANDLER_H_

#include <memory>       // in order to use smart and unique pointers
#include <exception>    // in order to do try {} catch (exception& e) {}
#include <iostream>     // debug (in order to use std::cout)
#include <fstream>
#include <omnetpp.h>
#include <math.h>       // in order to use pow(,)
#include <ctype.h>      // allows to use isaplha() and check if char is alphabetic

#include <boost/algorithm/string.hpp>           // in order to do: boost::to_upper_copy(...)
#include <boost/algorithm/string/replace.hpp>   // in order to use boost::replace_all on strings

#include "Belief.h"
#include "Desire.h"
#include "Goal.h"
#include "json.hpp" // to read from json file
#include "Metric.h"
#include "Operator.h"
#include "Plan.h"
#include "Reference_table.h"
#include "Sensor.h"
#include "server.h"
#include "server_handler.h"
#include "Skill.h"
#include "task.h"
#include "writer.h"
using json = nlohmann::json;

using namespace omnetpp; // in order to use EV

class JsonfileHandler {

public:
    // -----------------------------------------------------------------------------------------------
    // Note: if a json parameter is not found in the file (i.e. conditions = desire["conditions"]),
    // the error is: kronosim_dbg: src/../headers/json.hpp:16103: const value_type& nlohmann::basic_json<ObjectType, ArrayType, StringType, BooleanType, NumberIntegerType, NumberUnsignedType, NumberFloatType, AllocatorType, JSONSerializer>::operator[](T*) const [with T = const char; ObjectType = std::map; ArrayType = std::vector; StringType = std::__cxx11::basic_string<char>; BooleanType = bool; NumberIntegerType = long int; NumberUnsignedType = long unsigned int; NumberFloatType = double; AllocatorType = std::allocator; JSONSerializer = nlohmann::adl_serializer; nlohmann::basic_json<ObjectType, ArrayType, StringType, BooleanType, NumberIntegerType, NumberUnsignedType, NumberFloatType, AllocatorType, JSONSerializer>::const_reference = const nlohmann::basic_json<>&; nlohmann::basic_json<ObjectType, ArrayType, StringType, BooleanType, NumberIntegerType, NumberUnsignedType, NumberFloatType, AllocatorType, JSONSerializer>::value_type = nlohmann::basic_json<>]: Assertion `m_value.object->find(key) != m_value.object->end()' failed.
    // -----------------------------------------------------------------------------------------------

    JsonfileHandler();
    JsonfileHandler(std::shared_ptr<Logger> _logger, std::shared_ptr<Metric> _metric);
    virtual ~JsonfileHandler();

    json read_beliefset_from_json(std::string path, std::vector<std::shared_ptr<Belief>>& beliefset, int ag_id, std::string received_content = "", bool read_from_file = true);
    json read_desireset_from_json(std::string path, const std::vector<std::shared_ptr<Belief>>& beliefset,
            const std::vector<std::shared_ptr<Skill>>& skillset, std::vector<std::shared_ptr<Desire>>& desireset, int ag_id, std::string received_content = "", bool read_from_file = true);
    json read_planset_from_json(std::string path, const std::vector<std::shared_ptr<Belief>>& beliefset, const std::vector<std::shared_ptr<Skill>>& skillset,
            std::vector<std::shared_ptr<Plan>>& planset, const std::vector<std::shared_ptr<Server>>& servers, int ag_id, const std::string schedule_type_name, std::string received_content = "", bool read_from_file = true);
    json read_sensors_from_json(std::string path, std::vector<std::shared_ptr<Sensor>>& sensors_time, int ag_id, std::string received_content = "", bool read_from_file = true);
    json read_servers_from_json(std::string path, std::shared_ptr<ServerHandler> ag_Server_Handler, std::vector<std::shared_ptr<Server>>& servers, std::string schedule_type_name, int ag_id, std::string received_content = "", bool read_from_file = true);
    json read_skillset_from_json(const std::string path, const std::vector<std::shared_ptr<Belief>>& beliefset, std::vector<std::shared_ptr<Skill>>& skillset, const int ag_id, std::string received_content = "", bool read_from_file = true);

    json update_beliefset_from_json(std::string path, std::vector<std::shared_ptr<Belief>>& beliefset, int ag_id, std::string received_content = "", bool read_from_file = true);

private:
    // Use to truncate simTime value to the 3 decimal digits (Milliseconds)
    const SimTimeUnit sim_time_unit = SIMTIME_MS;

    std::shared_ptr<Metric> ag_metric;
    std::shared_ptr<Logger> logger;

    int update_iteration;   // ONLY USED IN 'dynamically_update_beliefset_from_json'
    int desire_goal_id;     // use to set both desires and goals IDs
    int sensor_read_id;     // use to set sensor's ID
    int plan_id = 0;        // used to compute IDs dynamically

    bool checkIfBeliefAlreadyExists(const std::vector<std::shared_ptr<Belief>>& beliefset, const std::string name);

    ServerType convertIntToServerType(int value);
    std::string convertJsonValueToVariant(json json_value, std::variant<int, double, bool, std::string>& value);
    json convertVariantToJson( std::variant<int, double, bool, std::string>& value);

    Goal::Status convertStringToGoalStatus(std::string value);
    Task::Execution convertStringToTaskExecutionType(std::string value, std::string scheduler);
    Goal::Execution convertStringToGoalExecutionType(std::string value, std::string scheduler);
    Operator::Type convertStringToOperator(std::string value);
    Sensor::Mode convertStringToSensorMode(std::string value);

    json generate_error_warning_json(std::string type, std::string description);

    /** Returns nullptr if not found */
    std::shared_ptr<Belief> findBeliefWithGivenName(const std::vector<std::shared_ptr<Belief>>& beliefset, std::string prec_belief_name_ref);
    std::shared_ptr<Skill> findSkillWithGivenName(const std::vector<std::shared_ptr<Skill>>& skillset, std::string goal_name);

    /** Returns -1 if the server id does not exist */
    int getServerIndexInAg_servers(const std::vector<std::shared_ptr<Server>>& servers, const int server_id);

    /*  Return the current simulation time with MILLISECOND approximation */
    simtime_t sim_current_time();

    /** Update belief only if original value and new one are of the same type family
     * @return 1: if belief correctly updated
     * @return 2: if belief not found
     * @return 3: if types do not match */
    int updateBeliefIfPresent(std::vector<std::shared_ptr<Belief>>& beliefset,
            std::string b_name, std::variant<int, double, bool, std::string> new_value, std::string& message);
};

#endif /* HEADERS_JSONFILE_HANDLER_H_ */
