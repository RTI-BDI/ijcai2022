/*
 * ag_msg_handler.h
 *
 *  Created on: 21/apr/2017
 *      Author: iDavide
 */

#ifndef HEADERS_AG_SETTINGS_H_
#define HEADERS_AG_SETTINGS_H_

#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include <vector>

#include "../headers/task.h"

using namespace omnetpp;

class Ag_settings {

private:
    int ddl_miss = 0;
    int ddl_check = 0;

public:
    Ag_settings();
    virtual ~Ag_settings();

    // Setters
    void add_ddl_miss();
    void add_ddl_check();

    // Getters
    int get_ddl_miss();
    int get_ddl_check();
};



#endif /* HEADERS_AG_SETTINGS_H_ */
