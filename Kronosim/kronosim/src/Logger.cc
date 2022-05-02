/*
 * Logger.cc
 *
 *  Created on: Mar 23, 2020
 *      Author: francesco
 */

#include "../headers/Logger.h"

Logger::Logger(int _level) {
    level = _level;
}

Logger::~Logger() { }

void Logger::print(Level _level, std::string msg) {
    // if the set "level" is higher than the one specified for this printing action "_level", then the message is printed.
    // Example: if "level" = Error and "_level" is Warning, the msg won't be printed
    if(level >= _level) {
        switch(_level){
        case PrintNothing: case Debug: case Default: case EveryInfo: case EveryInfoPlus:
            EV << msg << std::endl;
            std::cout << msg << std::endl;
            break;
        case Error:
            EV_ERROR << msg << std::endl;
            std::cerr << msg << std::endl;
            break;
        case Warning:
            EV_WARN << msg << std::endl;
            std::cout << msg << std::endl;
            break;
        case EssentialInfo:
            EV << msg << std::endl;
            std::cout << msg << std::endl;
            break;
        default:
            /* should never reach this point */
            EV_ERROR << "[WARINING] Invalid logger level! Message: " << msg << std::endl;
            std::cerr << "[WARINING] Invalid logger level! Message: " << msg << std::endl;
            break;
        }
    }
}

// Getter methods
int Logger::get_level() {
    return level;
}

// Setter methods
void Logger::set_level(int _level) {
    level = _level;
}

