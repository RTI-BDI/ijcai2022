/*
 * AnalyzeSimulations.h
 *
 *  Created on: Feb 11, 2021
 *      Author: francesco
 */

#ifndef HEADERS_ANALYZESIMULATIONS_H_
#define HEADERS_ANALYZESIMULATIONS_H_

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
#include <iostream>     // debug (per poter usare std::cout)
#include <memory>       // in order to use smart and unique pointers
#include <time.h>       // to get time and date
#include <vector>
#include <fstream>

#include <omnetpp.h>
using namespace omnetpp;

// ----------------------------------------------------------------------------------------------------------- //
// ------------------------------------- AnalyzeSimulations_Deadlinemiss ------------------------------------- //
// ----------------------------------------------------------------------------------------------------------- //
class AnalyzeSimulations_Deadlinemiss {
public:
    AnalyzeSimulations_Deadlinemiss(std::string sim_name, double time, int id, double lateness, std::string log, int n_exec, int origianal_planid, int planid);

    // Getters:
    int getId();
    int getNExec();
    int getPlanId();
    int getOriginalPlanId();
    double getTime();
    double getLateness();
    std::string getSimName();
    std::string getLog();

private:
    int id, n_exec, planid, origianal_planid;
    double time, lateness;
    std::string sim_name, log;

};

// ----------------------------------------------------------------------------------------------------------- //
// ----------------------------------------- AnalyzeSimulations_Stats ---------------------------------------- //
// ----------------------------------------------------------------------------------------------------------- //
class AnalyzeSimulations_Stats {
public:
    AnalyzeSimulations_Stats(int active_goals, int active_intentions, int iteration, double time);

    // Getters:
    int getActiveGoals();
    int getActiveIntentions();
    int getIteration();
    double getTime();

private:
    int active_goals, active_intentions, iteration;
    double time;
};


// ----------------------------------------------------------------------------------------------------------- //
// --------------------------------- AnalyzeSimulations_Achieved_Goals --------------------------------------- //
// ----------------------------------------------------------------------------------------------------------- //
class AnalyzeSimulations_Achieved_Goals {
public:
    AnalyzeSimulations_Achieved_Goals(std::string name,  std::string value,
            double completion_time, std::string completion_reason);

private:
    std::string name;
    std::string value;
    std::string completion_reason;
    double completion_time;
};


// ----------------------------------------------------------------------------------------------------------- //
// --------------------------------- AnalyzeSimulations_Goals_activation ------------------------------------- //
// ----------------------------------------------------------------------------------------------------------- //
class AnalyzeSimulations_Goals_activation {
public:
    AnalyzeSimulations_Goals_activation(std::string belief_name,
            double activation_time, double completion_time, double deadline, std::string completion_reason,
            int goal_id, int plan_id, int original_plan_id, std::string annidation_level, int pref_index);

private:
    std::string belief_name;
    std::string completion_reason;
    std::string annidation_level;
    double activation_time;
    double completion_time;
    double deadline;
    int goal_id;
    int plan_id;
    int original_plan_id;
    int pref_index;
};


// ----------------------------------------------------------------------------------------------------------- //
// ------------------------------ AnalyzeSimulations_Intentions_activation ----------------------------------- //
// ----------------------------------------------------------------------------------------------------------- //
class AnalyzeSimulations_Intentions_activation {
public:
    AnalyzeSimulations_Intentions_activation(double activation_time, double completion_time,
            std::string completion_reason, int goal_id, int original_plan_id, int intention_id);

private:
    std::string completion_reason;
    double activation_time;
    double completion_time;
    int goal_id;
    int original_plan_id;
    int intention_id;
};


class AnalyzeSimulations : public cSimpleModule {
public:
    AnalyzeSimulations();
    virtual ~AnalyzeSimulations();

    virtual void initialize() override;
    virtual void finish() override;

private:
    std::shared_ptr<Logger> logger;

    // Read the relevant data from the seed of the specified simulation and prepare them to be used by "computeAvg..." functions
    void analyzeGeneratedSimulations(std::string user, bool continuousMode, std::string original_path, std::string write_folder_path, std::string simulation_name, std::string inputfile_name, std::string final_output_filename, std::string scheduling_algorithm, int initial_seed, int num_of_simulations);

    // Functions used to combine the metrics of the simulations_seed ----------------------------------------
    void computeAvgAchievedGoals(double tot_achieved_goals, double simulation_analyzed);
    void computeAvgGeneratedErrors(double tot_generated_errors, double simulation_analyzed);
    void computeAvgGeneratedWarnings(double tot_generated_warnings, double simulation_analyzed);
    void computeAvgDeadlinemiss(std::vector<std::shared_ptr<AnalyzeSimulations_Deadlinemiss>>& gen_deadlinemiss, json gen_ddlmiss_sim_names, double total, double tot_completed_tasks, double tot_deadline_miss_tasks, double simulations);
    void computeAvgReasoningcycle(double records, double tot_goals, double tot_intentions);
    void computeAvgResponseTime(double tot_executed_tasks, double tot_response_time, double tot_simulations);
    void computeAvgPhiI(double records, double tot_goals, double tot_intentions);
    void computeAvgCompletedGoals(double overall_needed_time_to_goal_completion, double tot_completed_goals);
    void computeAvgCompletedIntentions(double overall_needed_time_to_intention_completion, double tot_completed_intentions);
    void computeAvgStillActive(double still_ready, double still_release, double still_intentions, double waiting_completion_intentions, double simulations);
    void computeAvgUnexpected(double overall_unexpected_executions, double simulations);
    // ------------------------------------------------------------------------------------------------------

    // Create the json that will be saved to the output file ------------------------------------------------
    void compute_simulation_analysis_header(std::string simulation_name, std::string mainfolder_path,
            int simulation_analyzed, std::string scheduling_algorithm,
            int num_of_simulations, std::vector<std::string> crashed_simulations, std::vector<std::string> forced_stop_simulations,
            std::vector<std::string> simulation_time_reached, std::vector<std::string> simulation_time_reached_errors);
    // ------------------------------------------------------------------------------------------------------

    std::string boolToString(const bool val);
};

Define_Module(AnalyzeSimulations);

#endif /* HEADERS_ANALYZESIMULATIONS_H_ */
