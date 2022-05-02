/*
 * ag_settings.cc
 *
 *  Created on: 21/apr/2017
 *      Author: iDavide
 */


#include "../headers/ag_settings.h"

Ag_settings::Ag_settings() { }

Ag_settings::~Ag_settings() { }

void Ag_settings::add_ddl_miss() {
    ddl_miss += 1;
}

void Ag_settings::add_ddl_check() {
    ddl_check += 1;
}

int Ag_settings::get_ddl_miss() {
    return ddl_miss;
}

int Ag_settings::get_ddl_check() {
    return ddl_check;
}

