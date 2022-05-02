/*
 * server_handler.cc
 *
 *  Created on: 23 gen 2018
 *      Author: peppe
 */

#include "../headers/server_handler.h"

ServerHandler::ServerHandler() { }

ServerHandler::~ServerHandler() { }

// Initialize the servers' queue given the list of servers' ids
void ServerHandler::add_server(std::shared_ptr<Server> server) {
    this->servers.push_back(server);
}

std::shared_ptr<Server> ServerHandler::get_server(int server_id) {
    auto it = find_if(servers.begin(), servers.end(),
            [server_id](std::shared_ptr<Server> server) {
        return server->getTaskServer() == server_id;
    });

    if (it != servers.end())
        return *it;
    else
        return nullptr;
}


