/*
 * server_handler.h
 *
 *  Created on: 23 gen 2018
 *      Author: peppe
 */

#ifndef HEADERS_SERVER_HANDLER_H_
#define HEADERS_SERVER_HANDLER_H_

#include <vector>
#include <iostream>
#include <fstream>
#include "server.h"
#include "json.hpp"

using namespace omnetpp;
using json = nlohmann::json;

class ServerHandler {
private:
    std::vector<std::shared_ptr<Server>> servers;

public:
    ServerHandler();
    virtual ~ServerHandler();

    std::shared_ptr<Server> get_server(int s_id);
    void add_server(std::shared_ptr<Server> server);
};

#endif /* HEADERS_SERVER_HANDLER_H_ */
