/*
 * agent_thread_functions.cc
 *
 *  Created on: Jul 28, 2021
 *      Author: stornace
 */

#include "../headers/agent.h"

bool agent::send_msg_to_client(json data)
{
    bool ret_val = false;
    std::string msg = data.dump();
    char msg_char[SIZE_MAX_CHAR_ARRAY] = { 0 };

    if(msg.size() > sizeof(msg_char)) {
        std::string error_msg = "Can not copy string:["+ std::to_string(msg.size());
        error_msg += "] to char array:["+ std::to_string(sizeof(msg_char)) +"] that has smaller size!";

        ag_metric->addGeneratedWarning(sim_current_time().dbl(), "agent::send_msg_to_client", error_msg);
        logger->print(Logger::Error, error_msg);

        // generate an error response message that can be sent to the client
        data = generate_error_server_response("NOT_SET", "[ERROR] Generated server response is too long.");
        msg = data.dump();
    }

    // convert string to array of char
    strcpy(msg_char, msg.c_str());

    if(write(socket_id, msg_char, strlen(msg_char)) == -1) // v1: unistd.h --- send completion message to the client
    {
        logger->print(Logger::Error, " > Error while sending message to the client!");
        ret_val = false;
    }
    else {
        logger->print(Logger::EssentialInfo, " > Successfully sent message to the client!");

        ret_val = true;
        sent_msg_cnt += 1;
        logger->print(Logger::EveryInfo, " > received_msg_cnt:["+ std::to_string(received_msg_cnt) +"], sent_msg_cnt:["+ std::to_string(sent_msg_cnt) +"]");
    }

    is_client_still_waiting_response = false; // response sent to the client

    // this thread can resume its execution...
    resume_background_thread = true;
    thread_cv.notify_one();

    return ret_val;
}

// ACK Messages:
json agent::ack_error_post_conditions(const std::vector<std::string>& stopped_actions, const std::vector<std::string>& activated_actions,
        const std::vector<std::string>& completed_goals, const std::vector<std::pair<std::string, std::string>>& invalid_goals)
{
    json response, goals;

    for(int i = 0; i < (int)invalid_goals.size(); i++)
    {
        goals.push_back({
            { "goal", invalid_goals[i].first },
            { "post_conditions", invalid_goals[i].second }
        });
    }

    response = {
            { "command", "ERR_RUN" },
            { "errors", goals },
            { "stopped_actions", stopped_actions }, // list of goals removed during the reasoning cycle
            { "new_actions", activated_actions }, // list of goals activated during the reasoning cycle
            { "completed_goals", completed_goals } // list of achieved DESIRE during the reasoning cycle (internal goals are not relevant (20/07/21 - discussione con Andrea - dice che non gli interessano)
    };

    return response;
}

json agent::ack_new_solution(const std::vector<std::string>& stopped_actions, const std::vector<std::string>& activated_actions,
        const std::vector<std::string>& completed_goals, const std::vector<std::pair<std::string, std::string>>& invalid_post_cond)
{
    json response = {
            { "command", "NEW_SOLUTION" },
            { "stopped_actions", stopped_actions }, // list of goals removed during the reasoning cycle
            { "new_actions", activated_actions }, // list of goals activated during the reasoning cycle
            { "completed_goals", completed_goals },  // list of achieved desire and internal goals during the reasoning cycle
            { "invalid_post_cond", invalid_post_cond }  // list of "completed" intention that have invalid post_conditions
    };

    return response;
}

json agent::ack_no_solution(const std::vector<std::string>& stopped_actions, const std::vector<std::string>& activated_actions,
        const std::vector<std::string>& completed_goals, const std::vector<std::pair<std::string, std::string>>& invalid_post_cond)
{
    std::string print_msg = "Background thread: No solutions found in reasoning cycle...";

    json response = {
            { "command", "NO_SOLUTION" },
            { "stopped_actions", stopped_actions }, // list of goals removed during the reasoning cycle
            { "new_actions", activated_actions },   // list of goals activated during the reasoning cycle
            { "completed_goals", completed_goals },  // list of achieved desire and internal goals during the reasoning cycle
            { "invalid_post_cond", invalid_post_cond }  // list of "completed" intention that have invalid post_conditions
    };

    return response;
}

json agent::ack_poke_command()
{
    logger->print(Logger::EveryInfo, "Background thread: Checking if agent is performing a reasoning cycle...");
    json response;

    if(performing_reasoning_cycle)
    {
        response = {
                { "command", "WAIT" },
                { "description", "Performing reasoning cycle" }
        };
    }
    else {
        response = {
                { "command", "ACK_POKE" },
                { "description", "Simulation running, not in reasoning cycle" }
        };
    }

    return response;
}

json agent::ack_run_until(std::vector<std::string>& activated_actions)
{
    json response = {
            { "command", "ACK_RUN" },
            { "new_actions", activated_actions }, // list of goals activated when run_until gets called
            { "now", sim_current_time().dbl() }
    };

    // clear the list of activated actions, in order to be re-used
    activated_actions.clear();

    return response;
}

json agent::ack_simulation_completed(const double time, const bool sim_limit_reached, json metrics) {
    std::string print_msg = "Background thread: simulation completed";
    json reports;

    reports.push_back({
        { "name", "metrics" },
        { "content", metrics }
    });

    json response = {
            { "command", "ACK_END_SIM" },
            { "completion_time", time },
            { "time_limit_reached", sim_limit_reached }
    };

    return response;
}

json agent::ack_update_command(const std::string file_name, const std::string file_content, const std::string status, const json error_log) {
    std::string print_msg = "Background thread: acknowledge update command";
    json response = {
            { "command", "ACK_UPDATE" },
            { "file", file_name },
            { "status", status },
            { "error_log", error_log }
    };

    return response;
}

json agent::ack_util_command(const std::string request, const std::string status) {
    std::string print_msg = "Background thread: acknowledge util command";
    json response = {
            { "command", "ACK_UTIL" },
            { "request", request },
            { "status", status }
    };

    return response;
}

json agent::ack_util_initialize_command(const std::string file_name, const std::string file_content,
        const std::string request, const std::string status, const json error_log) {
    std::string print_msg = "Background thread: acknowledge util initialize command";
    json response = {
            { "command", "ACK_UTIL" },
            { "request", request },
            { "file", file_name },
            { "status", status },
            { "error_log", error_log }
    };

    logger->print(Logger::EveryInfo, response.dump());

    return response;
}

json agent::ack_client_connected(std::string key) {
    std::string print_msg = "Background thread: new client connection";
    json response = {
            { "command", "ACK_CONNECTION" },
            { "key", key }
    };

    return response;
}

json agent::generate_server_response(std::string command, std::string msg, std::string status, Logger::Level logger_level)
{
    logger->print(logger_level, msg);

    json response = {
            { "command", command },
            { "response", msg },
            { "status", status },
    };

    return response;
}


json agent::generate_error_server_response(std::string request, std::string msg)
{
    logger->print(Logger::Error, msg);

    json response = {
            { "command", "UTIL" },
            { "request", request },
            { "response", msg },
            { "status", "error" },
    };

    return response;
}

void agent::execute_run_until(const simtime_t until_time)
{
    // ********************************************
    resume_background_thread = false; // false until send_msg_to_client() is called. Force the thread to wait...
    thread_cv.notify_one();
    // ********************************************

    run_until = until_time;
    proceed_simulation = true;

    logger->print(Logger::EssentialInfo, "Background thread: Run until t: "+ run_until.str());

    /* -----------------------------------------------------------------------------------------------------------------
     * Example:
     * if(((proceed_simulation && sim_current_time() < run_until) || exit_simulation) == false) {
     *      logger->print(Logger::Error, " execute_run_until: "+ ag_support->boolToString((proceed_simulation && sim_current_time() < run_until) || exit_simulation));
     * }
     * ----------------------------------------------------------------------------------------------------------------- */
    /* Note: UNLOCK 'cond_var' can be done ONLY AFTER every debug PRINT in the function.
     * Otherwise, the main thread starts before the prints, leading to a 'malloc()'
     * From testing, it emerged that is line: 'logger->print(Logger::Error, " execute_run_until... ); that throws the error */
    cond_var.notify_one();
}

json agent::manage_update(json incoming_obj, bool initialize_sets)
{   /* Used both from UPDATE and UTIL. Even though the two request have different JSON structure,
 * the only things important is that they must both contain parameters "file" and "content" in the main level of the JSON */

    json response_obj = json::object();
    std::pair<bool, json> read_function_result;
    std::string file_name = "", file_content = "";
    std::string response_status = "";
    const bool read_from_file = false;
    const bool initialization_phase = false;

    if(incoming_obj.find("file") == incoming_obj.end()) {
        response_status = "file_not_found";
    }
    else
    {
        // valid_file_name = false;
        file_name = boost::to_lower_copy(incoming_obj["file"].get<std::string>());

        if(incoming_obj.find("content") == incoming_obj.end()) {
            response_status = "file_not_found";
        }
        else
        {
            file_content = incoming_obj["content"].dump();
            if(file_name.compare("beliefset.json") == 0)
            {
                read_function_result = this->set_ag_beliefset(initialization_phase, read_from_file, file_content);
                if(read_function_result.first == false) {
                    response_status = "error";
                }
                else
                {
                    response_status = "success";

                    if(reasoning_cycle_scheduled_after_update == false)
                    {
                        reasoning_cycle_scheduled_after_update = true;
                        if(initialized_beliefset == true || initialize_sets == false)
                        {
                            // apply a new reasoning cycle only when this function is called with command UPDATE. For INITIALIZE, agent:initialize will take care of scheduling the reasoning cycle
                            this->scheduleAt(sim_current_time(), set_reasoning_cycle());
                            scheduled_msg += 1;
                            this->ag_support->setScheduleInFuture("AG_REASONING_CYCLE - new reasoning cycle after command UPDATE"); // METRICS purposes
                        }
                    }
                    initialized_beliefset = true;
                }
            }
            else if(file_name.compare("desireset.json") == 0)
            {
                read_function_result = this->set_ag_desireset(initialization_phase, read_from_file, file_content);
                if(read_function_result.first == false) {
                    response_status = "error";
                }
                else
                {
                    response_status = "success";

                    if(reasoning_cycle_scheduled_after_update == false)
                    {
                        reasoning_cycle_scheduled_after_update = true;
                        if(initialized_desireset == true || initialize_sets == false)
                        {   // apply a new reasoning cycle only when this function is called with command UPDATE. For INITIALIZE, agent:initialize will take care of scheduling the reasoning cycle
                            this->scheduleAt(sim_current_time(), set_reasoning_cycle());
                            scheduled_msg += 1;
                            this->ag_support->setScheduleInFuture("AG_REASONING_CYCLE - new reasoning cycle after command UPDATE DESIRESET"); // METRICS purposes
                        }
                    }
                    initialized_desireset = true;
                }
            }
            else if(file_name.compare("planset.json") == 0)
            {
                read_function_result = this->set_ag_planset(initialization_phase, read_from_file, file_content);
                if(read_function_result.first == false) {
                    response_status = "error";
                }
                else
                {
                    response_status = "success";

                    if(reasoning_cycle_scheduled_after_update == false)
                    {
                        reasoning_cycle_scheduled_after_update = true;
                        if(initialized_planset == true || initialize_sets == false)
                        {   // apply a new reasoning cycle only when this function is called with command UPDATE. For INITIALIZE, agent:initialize will take care of scheduling the reasoning cycle
                            this->scheduleAt(sim_current_time(), set_reasoning_cycle());
                            scheduled_msg += 1;
                            this->ag_support->setScheduleInFuture("AG_REASONING_CYCLE - new reasoning cycle after command UPDATE PLANSET"); // METRICS purposes
                        }
                    }
                    initialized_planset = true;
                }
            }
            else if(file_name.compare("sensors.json") == 0)
            {
                read_function_result = this->set_ag_sensors(initialization_phase, read_from_file, file_content);
                if(read_function_result.first == false) {
                    response_status = "error";
                }
                else
                {
                    response_status = "success";
                    initialized_sensors = true;
                }
            }
            else if(file_name.compare("servers.json") == 0)
            {
                read_function_result = this->set_ag_servers(initialization_phase, read_from_file, file_content);
                if(read_function_result.first == false) {
                    response_status = "error";
                }
                else
                {
                    response_status = "success";

                    if(reasoning_cycle_scheduled_after_update == false)
                    {
                        reasoning_cycle_scheduled_after_update = true;
                        if(initialized_servers == true || initialize_sets == false)
                        {   // apply a new reasoning cycle only when this function is called with command UPDATE. For INITIALIZE, agent:initialize will take care of scheduling the reasoning cycle
                            this->scheduleAt(sim_current_time(), set_reasoning_cycle());
                            scheduled_msg += 1;
                            this->ag_support->setScheduleInFuture("AG_REASONING_CYCLE - new reasoning cycle after command UPDATE SERVERS"); // METRICS purposes
                        }
                    }
                    // ----------------------------------------------------
                    initialized_servers = true;
                }
            }
            else if(file_name.compare("skillset.json") == 0)
            {
                read_function_result = this->set_ag_skillset(initialization_phase, read_from_file, file_content);
                if(read_function_result.first == false) {
                    response_status = "error";
                }
                else
                {
                    response_status = "success";

                    if(reasoning_cycle_scheduled_after_update == false)
                    {
                        reasoning_cycle_scheduled_after_update = true;
                        if(initialized_skillset == true || initialize_sets == false)
                        {   // apply a new reasoning cycle only when this function is called with command UPDATE. For INITIALIZE, agent:initialize will take care of scheduling the reasoning cycle
                            this->scheduleAt(sim_current_time(), set_reasoning_cycle());
                            scheduled_msg += 1;
                            this->ag_support->setScheduleInFuture("AG_REASONING_CYCLE - new reasoning cycle after command UPDATE SKILLSET"); // METRICS purposes
                        }
                    }
                    initialized_skillset = true;
                }
            }
            else if(file_name.compare("update-beliefset.json") == 0)
            {
                read_function_result = this->set_ag_update_beliefset(initialization_phase, read_from_file, file_content);
                if(read_function_result.first == false)
                {
                    response_status = "error";
                }
                else {
                    response_status = "success";

                    if(reasoning_cycle_scheduled_after_update == false)
                    {
                        reasoning_cycle_scheduled_after_update = true;
                        if(initialized_beliefset == true || initialize_sets == false)
                        {
                            // apply a new reasoning cycle only when this function is called with command UPDATE. For INITIALIZE, agent:initialize will take care of scheduling the reasoning cycle
                            this->scheduleAt(sim_current_time(), set_reasoning_cycle());
                            scheduled_msg += 1;
                            this->ag_support->setScheduleInFuture("AG_REASONING_CYCLE - new reasoning cycle after command UPDATE"); // METRICS purposes
                        }
                    }
                    initialized_update_beliefset = true;
                }
            }
            else {
                response_status = "file_not_found";
            }
        }
    }

    if(initialize_sets)
    { // function called due to command UTIL.INITIALIZE
        response_obj = ack_util_initialize_command(file_name, file_content, "INITIALIZE", response_status, read_function_result.second);
    }
    else { // function called due to command UPDATE
        response_obj = ack_update_command(file_name, file_content, response_status, read_function_result.second);
    }

    return response_obj;
}

int agent::read_all_buffer(int client_socket, char* buffer, ControlStatus& status)
{
    logger->print(Logger::EveryInfo, "\n----read_all_buffer----");

    char data[SIZE_MAX_CHAR_ARRAY] = { 0 };
    int bytes_read = 0, tot_read = 0;
    int read_chars = SIZE_MAX_CHAR_ARRAY;
    bool keep_loop = false;
    std::string read_str = "", full_msg = "";
    std::string msg_end = "";
    status = ControlStatus::OK;

    do {
        keep_loop = false;

        // read(...) returns the number of bytes read, -1 for errors or 0 for EOF. Function "read" is contained in socket.h
        bytes_read = read(client_socket, data, read_chars);
        logger->print(Logger::EssentialInfo, "\n ---------------------------");
        logger->print(Logger::EssentialInfo, " Reading buffer: bytes_read=["+ std::to_string(bytes_read) +"]");

        if(bytes_read > 0)
        {
            read_str = data; // convert array of char to string in order to use function 'substr'

            // update the received content to the rest of the message...
            for(int i = tot_read; i < (tot_read + bytes_read); i++) {
                full_msg += data[i - tot_read];
            }

            if(full_msg.size() >= 8) // 8 is the length of keyword "end_json"
            {
                msg_end = full_msg.substr(full_msg.size() - 8, full_msg.size()); // extract the last part of the received data

                if(msg_end.compare("end_json") != 0)
                {
                    logger->print(Logger::Error, " [ERROR] Received message does not contain end of file keyword ('end_json')! --> waiting...");
                    keep_loop = true;
                }
                else { // the end of the message contains the keyword "end_json"str
                    bytes_read -= 8; // in order to NOT copy 'end_json' in the buffer that will be converted to json
                    keep_loop = false;
                }
            }

            // increase control variable...
            tot_read += bytes_read;
        }
        else {
            status = ControlStatus::ERROR;
            tot_read = bytes_read;
        }

        memset(&data[0], 0, sizeof(data)); // clear buffer before the next read
    }
    while(keep_loop);

    // copy the received message in the buffer
    // If there is still space in the server's buffer, store the incoming data
    if(tot_read <= SIZE_MAX_CHAR_ARRAY)
    {   // store the read bytes into the buffer array
        for(int i = 0; i < tot_read; i++) {
            buffer[i] = full_msg[i]; // concatenate arrays of chars
        }
    }
    else { // overflow of the buffer
        status = ControlStatus::ERROR;
    }

    logger->print(Logger::EveryInfo, "----read_all_buffer ended----");
    return tot_read;
}

