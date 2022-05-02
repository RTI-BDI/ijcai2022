/*
 * CompareSimulations.h
 *
 *  Created on: Mar 1, 2021
 *      Author: francesco
 */

#ifndef HEADERS_COMPARESIMULATIONS_H_
#define HEADERS_COMPARESIMULATIONS_H_

#include "Belief.h"
#include "Desire.h"
#include "Goal.h"
#include "jsonfile_handler.h"
#include "Logger.h"
#include "Plan.h"
#include "Sensor.h"
#include "server.h"
#include "task.h"
#include "writer.h"

#include <exception>    // in order to do try {} catch (excpetion& e) {}
#include <iostream>     // debug (in order to use std::cout)
#include <memory>       // in order to use smart and unique pointers
#include <time.h>       // to get time and date
#include <vector>
#include <fstream>

#include <omnetpp.h>
using namespace omnetpp;

// --------------------------------------------------------------------------------------------------- //
// ---------------------------------- Data read from json -------------------------------------------- //
// --------------------------------------------------------------------------------------------------- //
class SimulationData {
public:
    SimulationData();

    // Getter methods
    int get_crashed_simulations();
    int get_forced_stop_simulations();
    int get_time_limit_errors();
    int get_num_of_simulations();
    int get_simulation_analyzed();
    std::string get_scheduling_algorithm();
    double get_deadlinemiss_generated();
    double get_tot_completed_tasks();
    double get_tot_miss_generated();
    double get_executed_tasks_tot();
    double get_miss_percentage();
    double get_AVG_lateness();
    double get_AVG_response_time();
    double get_AVG_executed_tasks();
    double get_AVG_goals_completiontime();
    double get_AVG_goals_phi();
    double get_AVG_goals_reasoningcycle();
    double get_AVG_intentions_completiontime();
    double get_AVG_intentions_phi();
    double get_AVG_achieved_goals();
    double get_AVG_intentions_reasoningcycle();
    double get_AVG_still_intentionset();
    double get_AVG_still_ready();
    double get_AVG_still_release();
    double get_AVG_unexpected_events();
    double get_AVG_generated_errors();

    // Setters:
    void set_crashed_simulations(const int val);
    void set_forced_stop_simulations(const int val);
    void set_time_limit_errors(const int val);
    void set_num_of_simulations(const int val);
    void set_simulation_analyzed(const double val);
    void set_scheduling_algorithm(const std::string val);
    void set_deadlinemiss_generated(const double val);
    void set_tot_completed_tasks(const double val);
    void set_tot_miss_generated(const double val);
    void set_executed_tasks_tot(const double val);
    void set_miss_percentage(const double miss, const double tot_tasks);
    void set_AVG_lateness(const double val);
    void set_AVG_response_time(const double val);
    void set_AVG_executed_tasks(const double val);
    void set_AVG_goals_reasoningcycle(const double val);
    void set_AVG_intentions_reasoningcycle(const double val);
    void set_AVG_goals_phi(const double val);
    void set_AVG_intentions_phi(const double val);
    void set_AVG_achieved_goals(const double val);
    void set_AVG_goals_completiontime(const double val);
    void set_AVG_intentions_completiontime(const double val);
    void set_AVG_still_intentionset(const double val);
    void set_AVG_still_ready(const double val);
    void set_AVG_still_release(const double val);
    void set_AVG_unexpected_events(const double val);
    void set_AVG_generated_errors(const double val);

private:
    int crashed_simulations, forced_stop_simulations;
    int time_limit_errors, num_of_simulations, simulation_analyzed;
    std::string scheduling_algorithm;
    double deadlinemiss_generated, AVG_lateness, tot_completed_tasks, tot_miss_generated, miss_percentage;
    double executed_tasks_tot, AVG_response_time, AVG_executed_tasks;
    double AVG_goals_reasoningcycle, AVG_intentions_reasoningcycle;
    double AVG_goals_phi, AVG_intentions_phi;
    double AVG_achieved_goals;
    double AVG_goals_completiontime;
    double AVG_intentions_completiontime;
    double AVG_still_intentionset, AVG_still_ready, AVG_still_release;
    double AVG_unexpected_events;
    double AVG_generated_errors;
};

class CompareSimulations : public cSimpleModule  {
public:
    CompareSimulations();
    virtual ~CompareSimulations();

    virtual void initialize() override;
    virtual void finish() override;

private:
    void saveComparison(std::shared_ptr<Logger> logger, std::string user, std::string folder, const std::vector<std::unique_ptr<SimulationData>>& ,
            const std::string simulation_name, int& debug_simulations_results_index, const std::string output_report_name, const bool print_short_format, const bool single_simulation = true);
    void printComparison_latex(std::shared_ptr<Logger> logger, const std::vector<std::unique_ptr<SimulationData>>& sim_data,
            const std::string simulation_name, const int simulation_number, const bool single_simulation = true);
    std::unique_ptr<SimulationData> read_simulation_analysis_json(std::string user, std::string folder,
            std::string analysis_file, std::string simulation_name, std::string sched_algorithm);
    std::string compareParameterResults(const double analyzed, const double comparison1, const double comparison2, const std::string op = ">");

    const std::string EDF = "EDF";
    const std::string RR = "RR";
    const std::string FCFS = "FCFS";
    const std::vector<std::string> simulations = {
            // BATCH 0: --------------------------------------------------------------
            "corner-cases",
            "_test-batch",
            "test-CalcDDL-consecutive",
            "_Test-goal-operator",
            "test-isschedulable.4",
            "_test-multi-loop5",
            "test-multiplans.1",
            "test-multiplans.1.2",
            "test-multiplans.1.3",
            "_Test-multiple-goal",
            "_test-multiple-plans",
            "test-overlapping-plans-1",
            "test-overlapping-plans-2",
            // BATCH 1: --------------------------------------------------------------
            "test-overlapping-plans-3",
            "test-overlapping-plans-4",
            "_test_post_effects",
            "test-parallelo.1",
            "test-phiI_only_period1.3",
            "test-phiI_only_period1.3.1",
            "test-phiI_only_period1.3_multiplan",
            "test-phiI_only_period1.3_sensors",
            "_test-phiI_only_period-INF.8",
            "_test-phiI_only_period-INF.8.1",
            "_test-phiI_only_period-INF.8.1_cornercaseEDF",
            "_test-shared-goal",
            "_test-shared-goal_DDL",
            // BATCH 2: --------------------------------------------------------------
            "_test-shared-goal-different-result",
            "_test-shared-goal-different-result2",
            "_test-shared-goal-different-result2-operator",
            "_test-shared-goal-different-result2-operator_DDL",
            "_test-shared-goal-different-result-operator",
            "_test-shared-goal-loop2",
            "_test-shared-goal-loop3",
            "_test-shared-goal-loop4",
            "_test-shared-goal_originale",
            "test-trigger-goals.2.1",
            "_thesis_EDF",
            "_thesis_EDF_DDL",
            "_test_expressions"
    };

    const std::vector<std::string> simulations_results = {
            " EDF > | 3263.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 3.230000",
            //                " RR > | 3134.000000 | 1020.000000 | 11408.000000 | 11.184314 | 32.546267 | 3.280000",
            //                " FCFS > | 2649.000000 | 33.000000 | 2204.000000 | 66.787879 | 1.245753 | 2.400000",
            " EDF > | 2929.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 3.340000",
            //                " RR > | 2521.000000 | 282.000000 | 1852.000000 | 6.567376 | 11.186037 | 3.340000",
            //                " FCFS > | 2307.000000 | 43.000000 | 2100.000000 | 48.837209 | 1.863893 | 2.620000",
            " EDF > | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000",
            //                " RR > | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000",
            //                " FCFS > | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000",
            " EDF > | 2330.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 1.155000",
            //                " RR > | 2311.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 1.155000",
            //                " FCFS > | 2091.000000 | 10.000000 | 231.000000 | 23.100000 | 0.478240 | 1.105000",
            " EDF > | 1644.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 2.015000",
            //                " RR > | 1646.000000 | 34.000000 | 115.000000 | 3.382353 | 2.065614 | 2.025000",
            //                " FCFS > | 1624.000000 | 5.000000 | 98.000000 | 19.600000 | 0.307882 | 2.000000",
            // 5 -----------------------------------------------------------------------------------
            " EDF > | 1239.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 1.760000",
            //                " RR > | 1203.000000 | 142.000000 | 585.000000 | 4.119718 | 11.803824 | 1.730000",
            //                " FCFS > | 1210.000000 | 3.000000 | 37.000000 | 12.333333 | 0.247934 | 1.695000",
            " EDF > | 1375.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 1.165000",
            //                " RR > | 1379.000000 | 393.000000 | 4112.000000 | 10.463104 | 28.498912 | 1.215000",
            //                " FCFS > | 879.000000 | 50.000000 | 5156.000000 | 103.120000 | 5.688282 | 0.895000",
            " EDF > | 1784.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 1.225000",
            //                " RR > | 1644.000000 | 414.000000 | 2855.000000 | 6.896135 | 25.182482 | 1.230000",
            //                " FCFS > | 1391.000000 | 60.000000 | 7889.000000 | 131.483333 | 4.313444 | 1.045000",
            " EDF > | 1471.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 1.160000",
            //                " RR > | 1358.000000 | 323.000000 | 1703.000000 | 5.272446 | 23.784978 | 1.200000",
            //                " FCFS > | 1003.000000 | 41.000000 | 3491.000000 | 85.146341 | 4.087737 | 0.965000",
            " EDF > | 2148.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 1.500000",
            //                " RR > | 2205.000000 | 117.000000 | 502.000000 | 4.290598 | 5.306122 | 1.550000",
            //                " FCFS > | 1621.000000 | 61.000000 | 8776.000000 | 143.868852 | 3.763109 | 1.350000",
            // 10 -----------------------------------------------------------------------------------
            " EDF > | 3799.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 3.005000",
            //                " RR > | 3603.000000 | 143.000000 | 696.000000 | 4.867133 | 3.968915 | 3.000000",
            //                " FCFS > | 3335.000000 | 26.000000 | 3634.000000 | 139.769231 | 0.779610 | 2.850000",
            " EDF > | 2396.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 1.980000",
            //                " RR > | 2330.000000 | 135.000000 | 852.000000 | 6.311111 | 5.793991 | 1.970000",
            //                " FCFS > | 1811.000000 | 75.000000 | 10353.000000 | 138.040000 | 4.141358 | 1.745000",
            " EDF > | 619.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 1.150000",
            //                " RR > | 619.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 1.150000",
            //                " FCFS > | 617.000000 | 7.000000 | 131.000000 | 18.714286 | 1.134522 | 1.080000",
            " EDF > | 623.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 1.105000",
            //                " RR > | 623.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 1.105000",
            //                " FCFS > | 636.000000 | 7.000000 | 147.000000 | 21.000000 | 1.100629 | 1.075000",
            " EDF > | 2201.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 2.010000",
            //                " RR > | 2267.000000 | 140.000000 | 703.000000 | 5.021429 | 6.175562 | 1.985000",
            //                " FCFS > | 1754.000000 | 57.000000 | 5891.000000 | 103.350877 | 3.249715 | 1.775000",
            // 15 -----------------------------------------------------------------------------------
            " EDF > | 3154.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 4.600000",
            //                " RR > | 2995.000000 | 582.000000 | 4278.000000 | 7.350515 | 19.432387 | 4.585000",
            //                " FCFS > | 3205.000000 | 144.000000 | 14870.000000 | 103.263889 | 4.492980 | 4.520000",
            " EDF > | 142938.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 1.115000",
            //                " RR > | 142864.000000 | 277.000000 | 1957.000000 | 7.064982 | 0.193891 | 1.170000",
            //                " FCFS > | 142327.000000 | 86.000000 | 2851.000000 | 33.151163 | 0.060424 | 0.760000",
            " EDF > | 34016.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 1.100000",
            //                " RR > | 33864.000000 | 81.000000 | 601.000000 | 7.419753 | 0.239192 | 1.160000",
            //                " FCFS > | 36380.000000 | 17.000000 | 434.000000 | 25.529412 | 0.046729 | 0.540000",
            " EDF > | 33892.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 0.950000",
            //                " RR > | 33867.000000 | 136.000000 | 1394.000000 | 10.250000 | 0.401571 | 1.160000",
            //                " FCFS > | 36436.000000 | 24.000000 | 1070.000000 | 44.583333 | 0.065869 | 0.595000",
            " EDF > | 2312.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 1.970000",
            //                " RR > | 2312.000000 | 131.000000 | 695.000000 | 5.305344 | 5.666090 | 1.965000",
            //                " FCFS > | 1782.000000 | 57.000000 | 6035.000000 | 105.877193 | 3.198653 | 1.760000",
            // 20 -----------------------------------------------------------------------------------
            " EDF > | 33947.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 1.060000",
            //                " RR > | 33864.000000 | 153.000000 | 1617.000000 | 10.568627 | 0.451807 | 1.170000",
            //                " FCFS > | 36436.000000 | 24.000000 | 1070.000000 | 44.583333 | 0.065869 | 0.595000",
            " EDF > | 664.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 0.800000",
            //                " RR > | 693.000000 | 194.000000 | 851.000000 | 4.386598 | 27.994228 | 0.835000",
            //                " FCFS > | 444.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 0.455000",
            " EDF > | 3263.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 3.230000",
            //                " RR > | 3134.000000 | 1020.000000 | 11408.000000 | 11.184314 | 32.546267 | 3.280000",
            //                " FCFS > | 2649.000000 | 33.000000 | 2204.000000 | 66.787879 | 1.245753 | 2.400000",
            " EDF > | 3263.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 3.230000",
            //                " RR > | 3134.000000 | 1020.000000 | 11408.000000 | 11.184314 | 32.546267 | 3.280000",
            //                " FCFS > | 2649.000000 | 33.000000 | 2204.000000 | 66.787879 | 1.245753 | 2.400000",
            " EDF > | 3484.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 3.635000",
            //                " RR > | 3334.000000 | 272.000000 | 1602.000000 | 5.889706 | 8.158368 | 3.555000",
            //                " FCFS > | 3647.000000 | 98.000000 | 11176.000000 | 114.040816 | 2.687140 | 3.475000",
            // 25 -----------------------------------------------------------------------------------
            " EDF > | 3484.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 3.635000",
            //                " RR > | 3334.000000 | 272.000000 | 1602.000000 | 5.889706 | 8.158368 | 3.555000",
            //                " FCFS > | 3647.000000 | 98.000000 | 11176.000000 | 114.040816 | 2.687140 | 3.475000",
            " EDF > | 2892.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 3.675000",
            //                " RR > | 2714.000000 | 906.000000 | 7909.000000 | 8.729581 | 33.382461 | 3.650000",
            //                " FCFS > | 2974.000000 | 95.000000 | 6497.000000 | 68.389474 | 3.194351 | 3.455000",
            " EDF > | 2808.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 3.430000",
            //                " RR > | 2721.000000 | 759.000000 | 5770.000000 | 7.602108 | 27.894157 | 3.450000",
            //                " FCFS > | 2819.000000 | 71.000000 | 7206.000000 | 101.492958 | 2.518624 | 3.230000",
            " EDF > | 2807.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 3.320000",
            //                " RR > | 2638.000000 | 416.000000 | 2963.000000 | 7.122596 | 15.769522 | 3.340000",
            //                " FCFS > | 2808.000000 | 71.000000 | 6781.000000 | 95.507042 | 2.528490 | 3.230000",
            " EDF > | 2771.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 3.295000",
            //                " RR > | 2597.000000 | 408.000000 | 2900.000000 | 7.107843 | 15.710435 | 3.315000",
            //                " FCFS > | 2759.000000 | 71.000000 | 6688.000000 | 94.197183 | 2.573396 | 3.200000",
            // 30 -----------------------------------------------------------------------------------
            " EDF > | 3335.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 3.760000",
            //                " RR > | 2775.000000 | 810.000000 | 6977.000000 | 8.613580 | 29.189189 | 3.685000",
            //                " FCFS > | 3123.000000 | 99.000000 | 6991.000000 | 70.616162 | 3.170029 | 3.615000",
            " EDF > | 1503.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 2.000000",
            //                " RR > | 1483.000000 | 155.000000 | 935.000000 | 6.032258 | 10.451787 | 1.990000",
            //                " FCFS > | 1679.000000 | 60.000000 | 3328.000000 | 55.466667 | 3.573556 | 1.840000",
            " EDF > | 1485.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 2.125000",
            //                " RR > | 1459.000000 | 266.000000 | 1909.000000 | 7.176692 | 18.231666 | 2.135000",
            //                " FCFS > | 1472.000000 | 53.000000 | 1817.000000 | 34.283019 | 3.600543 | 1.980000",
            " EDF > | 842.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 1.270000",
            //                " RR > | 805.000000 | 116.000000 | 357.000000 | 3.077586 | 14.409938 | 1.265000",
            //                " FCFS > | 828.000000 | 3.000000 | 54.000000 | 18.000000 | 0.362319 | 1.270000",
            " EDF > | 2775.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 3.680000",
            //                " RR > | 2714.000000 | 906.000000 | 7909.000000 | 8.729581 | 33.382461 | 3.650000",
            //                " FCFS > | 2974.000000 | 95.000000 | 6792.000000 | 71.494737 | 3.194351 | 3.455000",
            // 35 -----------------------------------------------------------------------------------
            " EDF > | 2080.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 1.960000",
            //                " RR > | 2367.000000 | 983.000000 | 11304.000000 | 11.499491 | 41.529362 | 1.850000",
            //                " FCFS > | 2133.000000 | 131.000000 | 6908.000000 | 52.732824 | 6.141585 | 1.605000",
            " EDF > | 29615.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 1.240000",
            //                " RR > | 29611.000000 | 16.000000 | 16.000000 | 1.000000 | 0.054034 | 1.235000",
            //                " FCFS > | 29593.000000 | 2.000000 | 31.000000 | 15.500000 | 0.006758 | 1.115000",
            " EDF > | 29609.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 1.235000",
            //                " RR > | 29611.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 1.230000",
            //                " FCFS > | 29593.000000 | 2.000000 | 31.000000 | 15.500000 | 0.006758 | 1.115000",
            " EDF > | 227.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 1.135000",
            //                " RR > | 227.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 1.135000",
            //                " FCFS > | 227.000000 | 0.000000 | 0.000000 | 0.000000 | 0.000000 | 1.135000"
            // 39 -----------------------------------------------------------------------------------
    };
};

Define_Module(CompareSimulations);




#endif /* HEADERS_COMPARESIMULATIONS_H_ */
