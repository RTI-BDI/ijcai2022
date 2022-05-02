/*
 * Logger.h
 *
 *  Created on: Mar 23, 2020
 *      Author: francesco
 */

#ifndef HEADERS_LOGGER_H_
#define HEADERS_LOGGER_H_

#include <string>
#include <omnetpp.h>

using namespace omnetpp; // in order to use EV

class Logger {
    // Note: .... variable declared here are considered 'private'

public:
    /* !! LevelDefault: Should be used only inside functions that are called based on another Level.
     * For instance, the function printGoalset sometimes is called if the level is LevelEssentialInfo,
     * other times if it's LevelEveryInfoPlus.
     * All the logging inside the function are thus set to Default in order to be always printed
     * when the function is called. */
    enum Level {
        PrintNothing = 0, // used for batch of simulations
        Error,
        Debug,          // only for print that are going to be removed or changed
        Default,        // should only be used inside 'if(level >= some_level) {}'. The idea is that if "some_level", the developer does not need to also change the print within the function
        Warning,
        EssentialInfo,  // logs info about core functions and those variable that are necessary to follow the behavior of the code
        EveryInfo,      // logs all the variables, and for structured data, it print every parameter
        EveryInfoPlus,  // logs functions steps
    };

    Logger(int _level = Default);
    virtual ~Logger();

    void print(Level _level, std::string msg);

    // Getter methods
    int get_level();

    // Setter methods
    void set_level(int _level);

private:
    int level;
};

#endif /* HEADERS_LOGGER_H_ */
