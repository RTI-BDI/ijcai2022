/*
 * Action.h
 *
 *  Created on: Mar 5, 2020
 *      Author: francesco
 */

#ifndef HEADERS_ACTION_H_
#define HEADERS_ACTION_H_

#include <iostream>
#include <memory>

class Action {

public:
    virtual std::string get_type() = 0; // Note "virtual" combined with "= 0" (means it must be implemented in the children classes)
};

#endif /* HEADERS_ACTION_H_ */
