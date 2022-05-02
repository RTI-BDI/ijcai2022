/*
 * Belief.h
 *
 *  Created on: Mar 2, 2020
 *      Author: francesco
 *
 * Note: #ifndef unique_name is a technique to avoid loading the same .h multiple times
 * (it'd lead to 'method already defined' error).
 */
#ifndef HEADERS_BELIEF_H_
#define HEADERS_BELIEF_H_

#include <string>
#include <variant>
#include <vector>

class Belief {
protected:
    std::string name;
    std::variant<int, double, bool, std::string> value;

public:
    Belief(std::string _name, std::variant<int, double, bool, std::string> _value);
    virtual ~Belief();

    // Getter methods
    std::string get_name();
    std::variant<int, double, bool, std::string> get_value();

    // Setter methods
    void set_value(std::variant<int, double, bool, std::string> value);
};

#endif /* HEADERS_BELIEF_H_ */
