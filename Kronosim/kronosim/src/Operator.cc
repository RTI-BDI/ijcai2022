/*
 * Operator.cc
 *
 *  Created on: Mar 3, 2020
 *      Author: francesco
 */
#include "../headers/Operator.h"

Operator::Operator() {
}

Operator::~Operator() { }

std::string Operator::convertOperatorToString(Type op){
    switch(op) {
        case EQUAL: return "EQUAL";
        case LESS: return "LESS";
        case LESS_EQUAL: return "LESS_EQUAL";
        case GREATER: return "GREATER";
        case GREATER_EQUAL: return "GREATER_EQUAL";
        default: return "Operator not valid!"; // should never reach this point
    }
}

std::string Operator::convertOperatorToSymbol(Type op){
    switch(op) {
        case EQUAL: return "=";
        case LESS: return "<";
        case LESS_EQUAL: return "<=";
        case GREATER: return ">";
        case GREATER_EQUAL: return ">=";
        default: return "Operator not valid!"; // should never reach this point
    }
}
