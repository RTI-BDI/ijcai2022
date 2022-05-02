/*
 * Operator.h
 *
 *  Created on: Mar 5, 2020
 *      Author: francesco
 */

#ifndef HEADERS_OPERATOR_H_
#define HEADERS_OPERATOR_H_

#include <string>

class Operator {
public:
    enum Type { EQUAL, GREATER, GREATER_EQUAL, LESS, LESS_EQUAL };

    Operator();
    virtual ~Operator();

    std::string convertOperatorToString(Type op);
    std::string convertOperatorToSymbol(Type op); /* NOT USED */
};

#endif /* HEADERS_OPERATOR_H_ */
