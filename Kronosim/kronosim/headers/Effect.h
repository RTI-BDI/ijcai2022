/*
 * Effect.h
 *
 *  Created on: Apr 12, 2021
 *      Author: francesco
 */

#ifndef HEADERS_EFFECT_H_
#define HEADERS_EFFECT_H_

#include "json.hpp" // to read from json file
using json = nlohmann::json; // to read from json file

class Effect {
public:
    // Used in file "jsonfile_handler.cc" when reading the effects of a plan "read_planset_from_json(...)"
    Effect(json expression);
    virtual ~Effect();

    // Getters methods
    json get_expression();

private:
    json expression;
};

#endif /* HEADERS_EFFECT_H_ */
