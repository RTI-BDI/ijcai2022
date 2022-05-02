/*
 * agent_thread.cc
 *
 *  Created on: Jul 15, 2021
 *      Author: stornace
 */

#include "../headers/agent.h"

bool agent::start_server()
{
    try
    {
        if(is_threadcreated == false)
        {
            // activate background thread...
            // using approach: https://stackoverflow.com/questions/26043744/access-class-variable-from-stdthread
            background_thread = std::make_shared<std::thread>(&agent::listening_server, this); // [!] listening_server CAN NOT be STATIC if we want to pass 'this'
            background_thread->detach();
            is_threadcreated = true;
            // -------------------------------------------------------------------

            logger->print(Logger::EssentialInfo, "Execution of the main thread is moving forward...");
            return true;
        }
        else {
            logger->print(Logger::EssentialInfo, " - Background thread already activated!");
            return false;
        }
    }
    catch (std::exception &e) {
        std::string exception_msg(e.what());
        logger->print(Logger::Error, "[ERROR] "+ exception_msg);
        return false;
    }
}

void agent::listening_server()
{ // Tutorial: https://www.geeksforgeeks.org/socket-programming-cc/

    struct sockaddr_in address;
    int server_fd;
    int valread;
    int opt = 1;
    int addrlen = sizeof(address);
    simtime_t now, received_time;
    char buffer[SIZE_MAX_CHAR_ARRAY];
    ControlStatus status_buffer;

    std::string command, print_msg, file_name;
    std::string util_request;
    json incoming_obj = json::object(); // initialized to empty object
    json response_obj = json::object(); // initialized to empty object

    // Setup server and wait for client connection ------------------------------------------------
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("Error while attaching socket to port");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bonding socket to port failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) // 3 = up to 3 request at a time
    {
        perror("Error while performing function 'listen' on server");
        exit(EXIT_FAILURE);
    }
    if ((socket_id = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0)
    {
        perror("Error on function 'accept'");
        exit(EXIT_FAILURE);
    }
    logger->print(Logger::EssentialInfo, " Client connected! Socket id:["+ std::to_string(socket_id) +"]");
    // --------------------------------------------------------------------------------------------

    do
    {
        try
        {
            is_client_still_waiting_response = true; // the client sent a message to the server. The server has not yet answered
            command = "NOT_SET";
            print_msg = "";
            incoming_obj.clear(); // clear json object
            response_obj.clear(); // clear json object

            memset(&buffer[0], 0, sizeof(buffer)); // clear buffer

            // RECEIVE ALL DATA ---------------------------------
            valread = read_all_buffer(socket_id, buffer, status_buffer);
            // ----------------------------------------------------

            if(stop_background_thread == false)
            {
                if(valread == 0) // read "returned End Of File (EOF)" -> called when the thread is not active anymore (the client is disconnected)
                {
                    stop_background_thread = true;  // in order to stop the thread
                }
                else if(valread == -1) // error in "read" or empty content
                {
                    response_obj = generate_error_server_response("NOT_SET", "[ERROR] Received message is too large to be concatenated to previous one!");
                    send_msg_to_client(response_obj);
                }
                else if(status_buffer == ControlStatus::ERROR)
                {
                    response_obj = generate_error_server_response("NOT_SET", "[ERROR] Incoming message length is too big!");
                    send_msg_to_client(response_obj);
                }
                else
                {
                    incoming_obj = json::parse(buffer);
                    now = sim_current_time();

                    if(incoming_obj.find("command") == incoming_obj.end())
                    {
                        print_msg = "[ERROR] The received value is not in json format";

                        response_obj = generate_error_server_response("NOT_SET", print_msg);
                        send_msg_to_client(response_obj);
                    }
                    else {
                        received_msg_cnt += 1;

                        command = boost::to_upper_copy(incoming_obj["command"].get<std::string>());

                        if(command.compare("POKE") == 0)
                        {
                            response_obj = ack_poke_command();
                            send_msg_to_client(response_obj);
                        }
                        else if(command.compare("RUN") == 0 || command.compare("RUNALL") == 0)
                        {
                            if(initialized_beliefset && initialized_desireset && initialized_planset
                                    && initialized_sensors && initialized_servers && initialized_skillset) // they must all be true before been able to start the simulation
                            {
                                if(incoming_obj.find("time") != incoming_obj.end())
                                {
                                    if(incoming_obj["time"].type() == json::value_t::number_float
                                            || incoming_obj["time"].type() == json::value_t::number_integer
                                            || incoming_obj["time"].type() == json::value_t::number_unsigned)
                                    {
                                        received_time = convert_double_to_simtime_t(incoming_obj["time"].get<double>()); // convert double to simtime_t in order to use simtime_t precision

                                        if(received_time == -1) {
                                            received_time = convert_double_to_simtime_t(MAX_SIMULATION_TIME + 1); // run simulation until completion (+1 in order to make omnetpp call function 'finish()'
                                        }

                                        if(now < received_time)
                                        {   /* then the "received_time" is in the future
                                         *  [!] response for command "RUN" is managed and sent to the client in call "agent" */

                                            reasoning_cycle_scheduled_after_update = false; // reset to default value
                                            agentMSG* tmp_msg = this->set_run_until_t();
                                            this->scheduleAt(received_time, tmp_msg); // schedule RUN_UNTIL_T message (in order to pause the simulation at time t)
                                            scheduled_msg += 1;

                                            execute_run_until(received_time);
                                        }
                                        else
                                        {   /* it may happen that the run_until's value is set to X, the execution runs until X
                                         * BUT then stops in function 'agent::handleMessage' and the simTime value is bigger than X.
                                         * es. RUN 1, execute everything until t: 1. Then it exploits the next message to pause the execution.
                                         * if the used message was 'activate task t: 3', then in 'handleMessage' simTime would return 3, and between 1 and 3 no events occur. */

                                            print_msg = "No events in interval:["+ std::to_string(incoming_obj["time"].get<double>());
                                            print_msg += ", "+ now.str() +"]!";

                                            response_obj = generate_server_response(command, print_msg, "warning", Logger::Warning);
                                            send_msg_to_client(response_obj);
                                        }
                                    }
                                    else {
                                        response_obj = generate_error_server_response(command, "[ERROR] Parameter 'time' must be a number!");
                                        send_msg_to_client(response_obj);
                                    }
                                }
                                else {
                                    response_obj = generate_error_server_response(command, "[ERROR] Parameter 'time' not found in command 'RUN'!");
                                    send_msg_to_client(response_obj);
                                }
                            }
                            else {
                                response_obj = generate_error_server_response(command, "[ERROR] Command 'RUN' can not be executed until 'SETUP_COMPLETED' is correctly performed!");
                                send_msg_to_client(response_obj);
                            }
                        }
                        else if(command.compare("SETUP_COMPLETED") == 0)
                        {
                            response_obj = generate_server_response("ACK_"+ command, "Background thread: knowledge-base files initialized! Setup completed", "success");
                            send_msg_to_client(response_obj);
                        }
                        else if(command.compare("UPDATE") == 0)
                        {
                            response_obj = manage_update(incoming_obj); // manage_update also update 'server_response'
                            send_msg_to_client(response_obj);
                        }
                        else if(command.compare("UTIL") == 0)
                        {
                            if(incoming_obj.find("request") != incoming_obj.end())
                            {
                                if(incoming_obj["request"].type() == json::value_t::string)
                                {
                                    util_request = incoming_obj["request"].get<std::string>();
                                    if(util_request.compare("INITIALIZE") == 0)
                                    { /* [!] in order to be correctly initialized, the order among files is important!
                                     * The correct sequence is: beliefset, skillset, desireset, servers, planset, sensors */

                                        // in order to only allow INITIALIZE command at the begin of the simulation
                                        if(((initialized_beliefset && initialized_desireset && initialized_planset
                                                && initialized_sensors && initialized_servers && initialized_skillset && initialized_update_beliefset) == false))
                                        {
                                            response_obj = manage_update(incoming_obj, true); // manage_update also update 'server_response'
                                            send_msg_to_client(response_obj);
                                        }
                                        else {
                                            response_obj = ack_util_command(util_request, "[ERROR] Parameter 'INITIALIZE' already used!");
                                            send_msg_to_client(response_obj);
                                        }
                                    }
                                    else if(util_request.compare("EXIT") == 0)
                                    {
                                        response_obj = ack_util_command(util_request, "Background thread: EXIT");
                                        send_msg_to_client(response_obj);
                                        stop_background_thread = true; // simulation ended, stop the thread
                                    }
                                    else if(command.compare("PAUSE") == 0)
                                    {
                                        proceed_simulation = false;
                                        cond_var.notify_one(); // notify the conditions variable that a value has changed

                                        response_obj = generate_server_response(command, "Background thread: Simulation paused", "success");
                                        send_msg_to_client(response_obj);
                                    }
                                    else {
                                        response_obj = ack_util_command(util_request, "[ERROR] Parameter 'request' has invalid value:["+ util_request +"]!");
                                        send_msg_to_client(response_obj);
                                    }
                                }
                                else {
                                    response_obj = ack_util_command(util_request, "[ERROR] Parameter 'request' in command 'UTIL' must be a string!");
                                    send_msg_to_client(response_obj);
                                }
                            }
                            else {
                                response_obj = ack_util_command(util_request, "[ERROR] Parameter 'request' not found in command 'UTIL'!");
                                send_msg_to_client(response_obj);
                            }
                        }
                        else {
                            response_obj = generate_error_server_response(command, "[ERROR] Background thread: Invalid command");
                            send_msg_to_client(response_obj);
                        }
                    }
                }
            }
        }
        catch (json::parse_error& ex) {
            print_msg = "[ERROR] Unexpected error while reading json object: ";
            print_msg += ex.what();

            response_obj = generate_error_server_response(command, print_msg);
            send_msg_to_client(response_obj);
        }

        /********************************************************/
        logger->print(Logger::EveryInfo, " *-*-*- > thread_cv run before: "+ ag_support->boolToString(resume_background_thread));

        // wait until the condition is true before moving on with the code execution
        thread_cv.wait(lock_thread, [this] {
            return resume_background_thread;
        });

        logger->print(Logger::EveryInfo, " *-*-*- > thread_cv run after: "+ ag_support->boolToString(resume_background_thread));
        /********************************************************/
    }
    while(stop_background_thread == false);

    response_obj = ack_util_command(util_request, "Background thread: EXIT");
    send_msg_to_client(response_obj);
    logger->print(Logger::EssentialInfo, "-----------------------\n Simulation completed\n-----------------------");

    close(socket_id);
    logger->print(Logger::EssentialInfo, "-----Thread Closed!---- ");

    // - exit_simulation = true makes the main thread call: "finish(...)"
    exit_simulation = true;
    cond_var.notify_one(); // notify the conditions variable with the new value
}
