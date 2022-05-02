/*
 * Sensor.h
 *
 *  Created on: Apr 16, 2020
 *      Author: francesco
 */

#ifndef HEADERS_SENSOR_H_
#define HEADERS_SENSOR_H_

#include <string>
#include <variant>
#include <vector>

class Sensor {
public:
    enum Mode {
        DECREASE,   // the value 'x' read by the sensor will be subtracted to the actual value of the belief
        INCREASE,   // the value 'x' read by the sensor will be added to the actual value of the belief
        SET         // the value 'x' read by the sensor will replace the actual value of the belief. If the belief does not exists, it creates a new one
    };

    Sensor(int id, std::string belief_name, std::variant<int, double, bool, std::string> value, double time, Mode mode = SET);
    virtual ~Sensor();

    // Getter methods
    int get_id();
    std::string get_belief_name();
    std::variant<int, double, bool, std::string> get_value();
    double get_time();
    Mode get_mode();
    std::string get_mode_as_string();
    bool get_scheduled();

    // Setter methods
    void set_scheduled(bool val = true);

private:
    int id;                     // reading ID: used to check that function "agent.update_belief_set_time(...)" is simulating the expected sensor
    std::string belief_name;    // name of the belief it will be affected by the reading
    std::variant<int, double, bool, std::string> value; // sensor output value. (could be: INT, DOUBLE, BOOL).

    /* When the reading will be “performed”.
     * -1: in this case, the reading is not performed at a given time, but after the specified plan. */
    double time;

    Mode mode; // how the read data from the sensor will be used to update the belief's value

    /* true when the agent actually schedule the message for the sensor's activation.
    * This parameter is used to avoid scheduling multiple time the same sensor.
    * Example: t: 0 - schedule sensor A to time 10; at t: 5 - client sends a new set of sensors. We only want to schedule the new ones, not the one at t: 10 */
    bool scheduled = false;
};

#endif /* HEADERS_SENSOR_H_ */
