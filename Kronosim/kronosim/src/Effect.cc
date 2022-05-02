/*
 * Effect.cc
 *
 *  Created on: Apr 12, 2021
 *      Author: francesco
 */

#include "../headers/Effect.h"

Effect::Effect(json _expression)
{
    expression = _expression;
}

Effect::~Effect() { }

json Effect::get_expression() {
    return expression;
}
