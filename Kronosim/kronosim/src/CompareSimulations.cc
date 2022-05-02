/*
 * CompareSimulations.cc
 *
 *  Created on: Mar 1, 2021
 *      Author: francesco
 */

#include "../headers/CompareSimulations.h"

CompareSimulations::CompareSimulations() { }

CompareSimulations::~CompareSimulations() { }

void CompareSimulations::initialize() {
    bool single_simulation = par("single_simulation");
    bool print_short_format = par("print_short_format");
    bool print_latex = par("print_latex");
    int logger_level = par("logger_level");
    int debug_simulations_results_index = 0;
    std::string user = par("user");
    std::string folder_path = par("folder_path");
    std::string analysis_file = par("analysis_file");
    std::string simulation_name = par("simulation_name");
    std::string simulation_analysis_result = par("simulation_analysis_result");
    std::string result_string = "";
    std::vector<std::unique_ptr<SimulationData>> simulation_data;
    std::shared_ptr<Logger> logger = std::make_shared<Logger>();
    logger->set_level(logger_level);

    if(single_simulation)
    {
        simulation_data.push_back(read_simulation_analysis_json(user, folder_path, analysis_file, simulation_name, EDF));
        simulation_data.push_back(read_simulation_analysis_json(user, folder_path, analysis_file, simulation_name, RR));
        simulation_data.push_back(read_simulation_analysis_json(user, folder_path, analysis_file, simulation_name, FCFS));

        saveComparison(logger, user, folder_path, simulation_data, simulation_name, debug_simulations_results_index, simulation_analysis_result, print_short_format);
    }
    else {
        if(print_short_format) {
            result_string += " sched_type > | tot_completed_tasks ";
            result_string += " | > tot_miss_generated";
            result_string += " < | > deadlinemiss_generated";
            result_string += " < | > AVG_lateness";
            result_string += " < | > miss_percentage";
            result_string += " < | AVG_achieved_goals";

            logger->print(Logger::EssentialInfo, result_string);
        }

        for(int i = 0; i < (int)simulations.size(); i++)
        {
            simulation_name = simulations[i]; // >>> "simulations" is defined in "CompareSimulations.h" ****
            simulation_data.clear();

            simulation_data.push_back(read_simulation_analysis_json(user, folder_path, analysis_file, simulation_name, EDF));
            // simulation_data.push_back(read_simulation_analysis_json(user, folder_path, analysis_file, simulation_name, RR));
            // simulation_data.push_back(read_simulation_analysis_json(user, folder_path, analysis_file, simulation_name, FCFS));

            if(print_latex == false) {
                logger->print(Logger::EssentialInfo, " *** ["+ std::to_string(i + 1)
                +" of "+ std::to_string(simulations.size())
                +"], Simulation: "+ simulation_name +" *** ");

                saveComparison(logger, user, folder_path, simulation_data, simulation_name, debug_simulations_results_index, simulation_analysis_result, print_short_format, false);
            }
            else {
                printComparison_latex(logger, simulation_data, simulation_name, i + 1, false);
            }
        }
    }
}

void CompareSimulations::finish() { }

void CompareSimulations::saveComparison(std::shared_ptr<Logger> logger, std::string user, std::string folder, const std::vector<std::unique_ptr<SimulationData>>& sim_data,
        const std::string simulation_name, int& debug_simulations_results_index, const std::string output_report_name, const bool print_short_format, const bool single_simulation)
{
    std::string result_string = "";
    if(print_short_format)
    { // ONLY PRINT JOURNAL TABLE DATA -------------------------------------------------------------------------
        result_string = " EDF > | "+ std::to_string(sim_data[0]->get_tot_completed_tasks());
        result_string += " | > "+ std::to_string(sim_data[0]->get_tot_miss_generated());
        result_string += " < | > "+ std::to_string(sim_data[0]->get_deadlinemiss_generated());
        result_string += " < | > "+ std::to_string(sim_data[0]->get_AVG_lateness());
        result_string += " < | > "+ std::to_string(sim_data[0]->get_miss_percentage());
        result_string += " < | "+ std::to_string(sim_data[0]->get_AVG_achieved_goals());
        if(sim_data[0]->get_tot_miss_generated() > 0 || sim_data[0]->get_deadlinemiss_generated() > 0
                || sim_data[0]->get_AVG_lateness() > 0 || sim_data[0]->get_miss_percentage() > 0)
        {
            result_string += " <------- !!!";
        }

        // TODO commented during tests (and because RR and FCFS algorithms not implemented)
        if(result_string.compare(simulations_results[debug_simulations_results_index]) != 0) {
            logger->print(Logger::Warning, "Simulation:["+ std::to_string(debug_simulations_results_index) +"] is different!");
            logger->print(Logger::EssentialInfo, "Computed: "+ result_string);
            logger->print(Logger::EssentialInfo, "Journal: "+ simulations_results[debug_simulations_results_index]);
        }
        debug_simulations_results_index += 1;

        // TODO commented during tests (and because RR and FCFS algorithms not implemented)
        //        // -------------------------------------------------
        //        result_string = " RR > | "+ std::to_string(sim_data[1]->get_tot_completed_tasks())
        //        +" | "+ std::to_string(sim_data[1]->get_tot_miss_generated())
        //        +" | "+ std::to_string(sim_data[1]->get_deadlinemiss_generated())
        //        +" | "+ std::to_string(sim_data[1]->get_AVG_lateness())
        //        +" | "+ std::to_string(sim_data[1]->get_miss_percentage())
        //        +" | "+ std::to_string(sim_data[1]->get_AVG_achieved_goals());
        //        if(result_string.compare(simulations_results[debug_simulations_results_index]) != 0) {
        //            logger->print(Logger::Warning, "Simulation:["+ std::to_string(debug_simulations_results_index) +"] is different!");
        //            logger->print(Logger::EssentialInfo, "Computed: "+ result_string);
        //            logger->print(Logger::EssentialInfo, "Journal: "+ simulations_results[debug_simulations_results_index]);
        //        }
        //        debug_simulations_results_index += 1;
        //
        //        // -------------------------------------------------
        //        result_string = " FCFS > | "+ std::to_string(sim_data[2]->get_tot_completed_tasks())
        //        +" | "+ std::to_string(sim_data[2]->get_tot_miss_generated())
        //        +" | "+ std::to_string(sim_data[2]->get_deadlinemiss_generated())
        //        +" | "+ std::to_string(sim_data[2]->get_AVG_lateness())
        //        +" | "+ std::to_string(sim_data[2]->get_miss_percentage())
        //        +" | "+ std::to_string(sim_data[2]->get_AVG_achieved_goals());
        //        if(result_string.compare(simulations_results[debug_simulations_results_index]) != 0) {
        //            logger->print(Logger::Warning, "Simulation:["+ std::to_string(debug_simulations_results_index) +"] is different!");
        //            logger->print(Logger::EssentialInfo, "Computed: "+ result_string);
        //            logger->print(Logger::EssentialInfo, "Journal: "+ simulations_results[debug_simulations_results_index]);
        //        }
        //        debug_simulations_results_index += 1;
        //        // -------------------------------------------------------------------------------------------------------
    }
    else { // PRINT ALL DATA
        json output_data_json;
        // ---------------------------------------------------------------------------------
        if(single_simulation) {
            logger->print(Logger::EssentialInfo, " *** Simulation: "+ simulation_name +" *** ");
            output_data_json.push_back({ { "Simulation_name", simulation_name} });
        }

        logger->print(Logger::EssentialInfo, " EDF  > Simulations analyzed:["+ std::to_string(sim_data[0]->get_simulation_analyzed())
        +" of "+ std::to_string(sim_data[0]->get_num_of_simulations()) +"]");
        logger->print(Logger::EssentialInfo, " RR   > Simulations analyzed:["+ std::to_string(sim_data[1]->get_simulation_analyzed())
        +" of "+ std::to_string(sim_data[1]->get_num_of_simulations()) +"]");
        logger->print(Logger::EssentialInfo, " FCFS > Simulations analyzed:["+ std::to_string(sim_data[2]->get_simulation_analyzed())
        +" of "+ std::to_string(sim_data[2]->get_num_of_simulations()) +"]");
        logger->print(Logger::EssentialInfo, "-------------------------------------------");

        json simulation_header_json = {
                { "Simulation_analyzed_EDF", sim_data[0]->get_simulation_analyzed() },
                { "Simulation_analyzed_RR", sim_data[1]->get_simulation_analyzed() },
                { "Simulation_analyzed_FCFS", sim_data[2]->get_simulation_analyzed() }
        };
        output_data_json.push_back(simulation_header_json);
        // ---------------------------------------------------------------------------------

        // ---------------------------------------------------------------------------------
        if(single_simulation) {
            std::string msgCrashedSimulations = " EDF  > Crashed simulations:["+ std::to_string(sim_data[0]->get_crashed_simulations())
            +" of "+ std::to_string(sim_data[0]->get_num_of_simulations()) +"]";
            if(sim_data[0]->get_crashed_simulations() > 0) {
                logger->print(Logger::Error, msgCrashedSimulations);
            }
            else {
                logger->print(Logger::EssentialInfo, msgCrashedSimulations);
            }
            msgCrashedSimulations = " RR   > Crashed simulations:["+ std::to_string(sim_data[1]->get_crashed_simulations())
            +" of "+ std::to_string(sim_data[1]->get_num_of_simulations()) +"]";
            if(sim_data[1]->get_crashed_simulations() > 0) {
                logger->print(Logger::Error, msgCrashedSimulations);
            }
            else {
                logger->print(Logger::EssentialInfo, msgCrashedSimulations);
            }
            msgCrashedSimulations = " FCFS > Crashed simulations:["+ std::to_string(sim_data[2]->get_crashed_simulations())
            +" of "+ std::to_string(sim_data[2]->get_num_of_simulations()) +"]";
            if(sim_data[2]->get_crashed_simulations() > 0) {
                logger->print(Logger::Error, msgCrashedSimulations);
            }
            else {
                logger->print(Logger::EssentialInfo, msgCrashedSimulations);
            }
            logger->print(Logger::EssentialInfo, "-------------------------------------------");

            json crashes_json = { { "Num_of_crashed_simulations_EDF", sim_data[0]->get_crashed_simulations() },
                    { "Num_of_crashed_simulations_RR", sim_data[1]->get_crashed_simulations() },
                    { "Num_of_crashed_simulations_FCFS", sim_data[2]->get_crashed_simulations() }
            };
            output_data_json.push_back(crashes_json);
        }
        // ---------------------------------------------------------------------------------

        // ---------------------------------------------------------------------------------
        if(single_simulation) {
            std::string msgForcedStopSimulations = " EDF  > Forced Stop simulations:["+ std::to_string(sim_data[0]->get_forced_stop_simulations())
            +" of "+ std::to_string(sim_data[0]->get_num_of_simulations()) +"]";
            if(sim_data[0]->get_forced_stop_simulations() > 0) {
                logger->print(Logger::Error, msgForcedStopSimulations);
            }
            else {
                logger->print(Logger::EssentialInfo, msgForcedStopSimulations);
            }
            msgForcedStopSimulations = " RR   > Forced Stop simulations:["+ std::to_string(sim_data[1]->get_forced_stop_simulations())
            +" of "+ std::to_string(sim_data[1]->get_num_of_simulations()) +"]";
            if(sim_data[1]->get_forced_stop_simulations() > 0) {
                logger->print(Logger::Error, msgForcedStopSimulations);
            }
            else {
                logger->print(Logger::EssentialInfo, msgForcedStopSimulations);
            }
            msgForcedStopSimulations = " FCFS > Forced Stop simulations:["+ std::to_string(sim_data[2]->get_forced_stop_simulations())
            +" of "+ std::to_string(sim_data[2]->get_num_of_simulations()) +"]";
            if(sim_data[2]->get_forced_stop_simulations() > 0) {
                logger->print(Logger::Error, msgForcedStopSimulations);
            }
            else {
                logger->print(Logger::EssentialInfo, msgForcedStopSimulations);
            }
            logger->print(Logger::EssentialInfo, "-------------------------------------------");

            json stopped_json = { { "Num_of_forced_stop_simulations_EDF", sim_data[0]->get_forced_stop_simulations() },
                    { "Num_of_forced_stop_simulations_RR", sim_data[1]->get_forced_stop_simulations() },
                    { "Num_of_forced_stop_simulations_FCFS", sim_data[2]->get_forced_stop_simulations() }
            };
            output_data_json.push_back(stopped_json);
        }
        // ---------------------------------------------------------------------------------

        // ---------------------------------------------------------------------------------
        if(single_simulation) {
            std::string msgTimeLimitErrors = " EDF  > Time limit Errors:["+ std::to_string(sim_data[0]->get_time_limit_errors())
            +" of "+ std::to_string(sim_data[0]->get_num_of_simulations()) +"]";
            if(sim_data[0]->get_time_limit_errors() > 0) {
                logger->print(Logger::Error, msgTimeLimitErrors);
            }
            else {
                logger->print(Logger::EssentialInfo, msgTimeLimitErrors);
            }
            msgTimeLimitErrors = " RR   > Time limit Errors:["+ std::to_string(sim_data[1]->get_time_limit_errors())
            +" of "+ std::to_string(sim_data[1]->get_num_of_simulations()) +"]";
            if(sim_data[1]->get_time_limit_errors() > 0) {
                logger->print(Logger::Error, msgTimeLimitErrors);
            }
            else {
                logger->print(Logger::EssentialInfo, msgTimeLimitErrors);
            }
            msgTimeLimitErrors = " FCFS > Time limit Errors:["+ std::to_string(sim_data[2]->get_time_limit_errors())
            +" of "+ std::to_string(sim_data[2]->get_num_of_simulations()) +"]";
            if(sim_data[2]->get_time_limit_errors() > 0) {
                logger->print(Logger::Error, msgTimeLimitErrors);
            }
            else {
                logger->print(Logger::EssentialInfo, msgTimeLimitErrors);
            }
            logger->print(Logger::EssentialInfo, "-------------------------------------------");

            json limit_errors_json = { { "Num_of_time_limit_errors_EDF", sim_data[0]->get_time_limit_errors() },
                    { "Num_of_time_limit_errors_RR", sim_data[1]->get_time_limit_errors() },
                    { "Num_of_time_limit_errors_FCFS", sim_data[2]->get_time_limit_errors() }
            };
            output_data_json.push_back(limit_errors_json);
        }
        // ---------------------------------------------------------------------------------

        // ---------------------------------------------------------------------------------
        logger->print(Logger::EssentialInfo, " EDF  > AVG Lateness: "+ std::to_string(sim_data[0]->get_AVG_lateness())
        +", Overall: "+ std::to_string(sim_data[0]->get_deadlinemiss_generated())
        +", Tot Deadline miss tasks: "+ std::to_string(sim_data[0]->get_tot_miss_generated())
        +", Tot Generated tasks: "+ std::to_string(sim_data[0]->get_tot_completed_tasks())
        +", Miss percentage (%): "+ std::to_string(sim_data[0]->get_miss_percentage())
        +" "+ compareParameterResults(sim_data[0]->get_miss_percentage(), sim_data[1]->get_miss_percentage(), sim_data[2]->get_miss_percentage(), "<"));
        logger->print(Logger::EssentialInfo, " RR   > AVG Lateness: "+ std::to_string(sim_data[1]->get_AVG_lateness())
        +", Overall: "+ std::to_string(sim_data[1]->get_deadlinemiss_generated())
        +", Tot Deadline miss tasks: "+ std::to_string(sim_data[1]->get_tot_miss_generated())
        +", Tot Generated tasks: "+ std::to_string(sim_data[1]->get_tot_completed_tasks())
        +", Miss percentage (%): "+ std::to_string(sim_data[1]->get_miss_percentage())
        +" "+ compareParameterResults(sim_data[1]->get_miss_percentage(), sim_data[0]->get_miss_percentage(), sim_data[2]->get_miss_percentage(), "<"));
        logger->print(Logger::EssentialInfo, " FCFS > AVG Lateness: "+ std::to_string(sim_data[2]->get_AVG_lateness())
        +", Overall: "+ std::to_string(sim_data[2]->get_deadlinemiss_generated())
        +", Tot Deadline miss tasks: "+ std::to_string(sim_data[2]->get_tot_miss_generated())
        +", Tot Generated tasks: "+ std::to_string(sim_data[2]->get_tot_completed_tasks())
        +", Miss percentage (%): "+ std::to_string(sim_data[2]->get_miss_percentage())
        +" "+ compareParameterResults(sim_data[2]->get_miss_percentage(), sim_data[0]->get_miss_percentage(), sim_data[1]->get_miss_percentage(), "<"));
        logger->print(Logger::EssentialInfo, "-------------------------------------------");

        json lateness_json = { { "AVG_Lateness_EDF", sim_data[0]->get_AVG_lateness() },
                { "AVG_Lateness_RR", sim_data[1]->get_AVG_lateness() },
                { "AVG_Lateness_FCFS", sim_data[2]->get_AVG_lateness() }
        };
        output_data_json.push_back(lateness_json);
        // ---------------------------------------------------------------------------------

        // ---------------------------------------------------------------------------------
        if(single_simulation) {
            logger->print(Logger::EssentialInfo, " EDF  > Tasks executed: "+ std::to_string(sim_data[0]->get_executed_tasks_tot())
            +", AVG executed: "+ std::to_string(sim_data[0]->get_AVG_executed_tasks())
            +", AVG response time: "+ std::to_string(sim_data[0]->get_AVG_response_time()));
            logger->print(Logger::EssentialInfo, " RR   > Tasks executed: "+ std::to_string(sim_data[1]->get_executed_tasks_tot())
            +", AVG executed: "+ std::to_string(sim_data[1]->get_AVG_executed_tasks())
            +", AVG response time: "+ std::to_string(sim_data[1]->get_AVG_response_time()));
            logger->print(Logger::EssentialInfo, " FCFS > Tasks executed: "+ std::to_string(sim_data[2]->get_executed_tasks_tot())
            +", AVG executed: "+ std::to_string(sim_data[2]->get_AVG_executed_tasks())
            +", AVG response time: "+ std::to_string(sim_data[2]->get_AVG_response_time()));
            logger->print(Logger::EssentialInfo, "-------------------------------------------");

            json response_time_json = { { "AVG_executed_tasks_EDF", sim_data[0]->get_AVG_executed_tasks() },
                    { "AVG_executed_tasks_RR", sim_data[1]->get_AVG_executed_tasks() },
                    { "AVG_executed_tasks_FCFS", sim_data[2]->get_AVG_executed_tasks() },
                    { "AVG_response_time_EDF", sim_data[0]->get_AVG_response_time() },
                    { "AVG_response_time_RR", sim_data[1]->get_AVG_response_time() },
                    { "AVG_response_time_FCFS", sim_data[2]->get_AVG_response_time() },
                    { "Tot_executed_tasks_EDF", sim_data[0]->get_executed_tasks_tot() },
                    { "Tot_executed_tasks_RR", sim_data[1]->get_executed_tasks_tot() },
                    { "Tot_executed_tasks_FCFS", sim_data[2]->get_executed_tasks_tot() }
            };
            output_data_json.push_back(response_time_json);
        }
        // ---------------------------------------------------------------------------------

        // ---------------------------------------------------------------------------------
        if(single_simulation) {
            logger->print(Logger::EssentialInfo, " EDF  > Reasoning cycle > AVG Goals: "+ std::to_string(sim_data[0]->get_AVG_goals_reasoningcycle())
            +", AVG Intentions: "+ std::to_string(sim_data[0]->get_AVG_intentions_reasoningcycle()));
            logger->print(Logger::EssentialInfo, " RR   > Reasoning cycle > AVG Goals: "+ std::to_string(sim_data[1]->get_AVG_goals_reasoningcycle())
            +", AVG Intentions: "+ std::to_string(sim_data[1]->get_AVG_intentions_reasoningcycle()));
            logger->print(Logger::EssentialInfo, " FCFS > Reasoning cycle > AVG Goals: "+ std::to_string(sim_data[2]->get_AVG_goals_reasoningcycle())
            +", AVG Intentions: "+ std::to_string(sim_data[2]->get_AVG_intentions_reasoningcycle()));
            logger->print(Logger::EssentialInfo, "-------------------------------------------");

            json reasoning_cycle_json = {
                    { "AVG_goals_reasoningcycle_EDF", sim_data[0]->get_AVG_goals_reasoningcycle() },
                    { "AVG_goals_reasoningcycle_RR", sim_data[1]->get_AVG_goals_reasoningcycle() },
                    { "AVG_goals_reasoningcycle_FCFS", sim_data[2]->get_AVG_goals_reasoningcycle() },
                    { "AVG_intentions_reasoningcycle_EDF", sim_data[0]->get_AVG_intentions_reasoningcycle() },
                    { "AVG_intentions_reasoningcycle_RR", sim_data[1]->get_AVG_intentions_reasoningcycle() },
                    { "AVG_intentions_reasoningcycle_FCFS", sim_data[2]->get_AVG_intentions_reasoningcycle() }
            };
            output_data_json.push_back(reasoning_cycle_json);
        }
        // ---------------------------------------------------------------------------------

        // ---------------------------------------------------------------------------------
        logger->print(Logger::EssentialInfo, " EDF  > PhiI > AVG Goals: "+ std::to_string(sim_data[0]->get_AVG_goals_phi())
        +" "+ compareParameterResults(sim_data[0]->get_AVG_goals_phi(), sim_data[1]->get_AVG_goals_phi(), sim_data[2]->get_AVG_goals_phi())
        +", AVG Intentions: "+ std::to_string(sim_data[0]->get_AVG_intentions_phi())
        +" "+ compareParameterResults(sim_data[0]->get_AVG_intentions_phi(), sim_data[1]->get_AVG_intentions_phi(), sim_data[2]->get_AVG_intentions_phi()));
        logger->print(Logger::EssentialInfo, " RR   > PhiI > AVG Goals: "+ std::to_string(sim_data[1]->get_AVG_goals_phi())
        +" "+ compareParameterResults(sim_data[1]->get_AVG_goals_phi(), sim_data[0]->get_AVG_goals_phi(), sim_data[2]->get_AVG_goals_phi())
        +", AVG Intentions: "+ std::to_string(sim_data[1]->get_AVG_intentions_phi())
        +" "+ compareParameterResults(sim_data[1]->get_AVG_intentions_phi(), sim_data[0]->get_AVG_intentions_phi(), sim_data[2]->get_AVG_intentions_phi()));
        logger->print(Logger::EssentialInfo, " FCFS > PhiI > AVG Goals: "+ std::to_string(sim_data[2]->get_AVG_goals_phi())
        +" "+ compareParameterResults(sim_data[2]->get_AVG_goals_phi(), sim_data[0]->get_AVG_goals_phi(), sim_data[1]->get_AVG_goals_phi())
        +", AVG Intentions: "+ std::to_string(sim_data[2]->get_AVG_intentions_phi())
        +" "+ compareParameterResults(sim_data[2]->get_AVG_intentions_phi(), sim_data[0]->get_AVG_intentions_phi(), sim_data[1]->get_AVG_intentions_phi()));
        logger->print(Logger::EssentialInfo, "-------------------------------------------");

        json phiI_json = {
                { "AVG_goals_phi_EDF", sim_data[0]->get_AVG_goals_phi() },
                { "AVG_goals_phi_RR", sim_data[1]->get_AVG_goals_phi() },
                { "AVG_goals_phi_FCFS", sim_data[2]->get_AVG_goals_phi() },
                { "AVG_intentions_phi_EDF", sim_data[0]->get_AVG_intentions_phi() },
                { "AVG_intentions_phi_RR", sim_data[1]->get_AVG_intentions_phi() },
                { "AVG_intentions_phi_FCFS", sim_data[2]->get_AVG_intentions_phi() }
        };
        output_data_json.push_back(phiI_json);
        // ---------------------------------------------------------------------------------

        // ---------------------------------------------------------------------------------
        logger->print(Logger::EssentialInfo, " EDF  > AVG Achieved Goals: "+ std::to_string(sim_data[0]->get_AVG_achieved_goals())
        +" "+ compareParameterResults(sim_data[0]->get_AVG_achieved_goals(), sim_data[1]->get_AVG_achieved_goals(), sim_data[2]->get_AVG_achieved_goals()));
        logger->print(Logger::EssentialInfo, " RR   > AVG Achieved Goals: "+ std::to_string(sim_data[1]->get_AVG_achieved_goals())
        +" "+ compareParameterResults(sim_data[1]->get_AVG_achieved_goals(), sim_data[0]->get_AVG_achieved_goals(), sim_data[2]->get_AVG_achieved_goals()));
        logger->print(Logger::EssentialInfo, " FCFS > AVG Achieved Goals: "+ std::to_string(sim_data[2]->get_AVG_achieved_goals())
        +" "+ compareParameterResults(sim_data[2]->get_AVG_achieved_goals(), sim_data[0]->get_AVG_achieved_goals(), sim_data[1]->get_AVG_achieved_goals()));
        logger->print(Logger::EssentialInfo, "-------------------------------------------");

        json achieved_goals_json = {
                { "AVG_achieved_goals_EDF", sim_data[0]->get_AVG_achieved_goals() },
                { "AVG_achieved_goals_RR", sim_data[1]->get_AVG_achieved_goals() },
                { "AVG_achieved_goals_FCFS", sim_data[2]->get_AVG_achieved_goals() }
        };
        output_data_json.push_back(achieved_goals_json);
        // ---------------------------------------------------------------------------------

        // ---------------------------------------------------------------------------------
        logger->print(Logger::EssentialInfo, " EDF  > AVG Goals Completion Time: "+ std::to_string(sim_data[0]->get_AVG_goals_completiontime())
        +" "+ compareParameterResults(sim_data[0]->get_AVG_goals_completiontime(), sim_data[1]->get_AVG_goals_completiontime(), sim_data[2]->get_AVG_goals_completiontime(), "<"));
        logger->print(Logger::EssentialInfo, " RR   > AVG Goals Completion Time: "+ std::to_string(sim_data[1]->get_AVG_goals_completiontime())
        +" "+ compareParameterResults(sim_data[1]->get_AVG_goals_completiontime(), sim_data[0]->get_AVG_goals_completiontime(), sim_data[2]->get_AVG_goals_completiontime(), "<"));
        logger->print(Logger::EssentialInfo, " FCFS > AVG Goals Completion Time: "+ std::to_string(sim_data[2]->get_AVG_goals_completiontime())
        +" "+ compareParameterResults(sim_data[2]->get_AVG_goals_completiontime(), sim_data[0]->get_AVG_goals_completiontime(), sim_data[1]->get_AVG_goals_completiontime(), "<"));
        logger->print(Logger::EssentialInfo, "-------------------------------------------");
        logger->print(Logger::EssentialInfo, " EDF  > AVG Intentions Completion Time: "+ std::to_string(sim_data[0]->get_AVG_intentions_completiontime())
        +" "+ compareParameterResults(sim_data[0]->get_AVG_intentions_completiontime(), sim_data[1]->get_AVG_intentions_completiontime(), sim_data[2]->get_AVG_intentions_completiontime(), "<"));
        logger->print(Logger::EssentialInfo, " RR   > AVG Intentions Completion Time: "+ std::to_string(sim_data[1]->get_AVG_intentions_completiontime())
        +" "+ compareParameterResults(sim_data[1]->get_AVG_intentions_completiontime(), sim_data[0]->get_AVG_intentions_completiontime(), sim_data[2]->get_AVG_intentions_completiontime(), "<"));
        logger->print(Logger::EssentialInfo, " FCFS > AVG Intentions Completion Time: "+ std::to_string(sim_data[2]->get_AVG_intentions_completiontime())
        +" "+ compareParameterResults(sim_data[2]->get_AVG_intentions_completiontime(), sim_data[0]->get_AVG_intentions_completiontime(), sim_data[1]->get_AVG_intentions_completiontime(), "<"));
        logger->print(Logger::EssentialInfo, "-------------------------------------------");

        json completion_time_json = {
                { "AVG_goals_completiontime_EDF", sim_data[0]->get_AVG_goals_completiontime() },
                { "AVG_goals_completiontime_RR", sim_data[1]->get_AVG_goals_completiontime() },
                { "AVG_goals_completiontime_FCFS", sim_data[2]->get_AVG_goals_completiontime() },
                { "AVG_intentions_completiontime_EDF", sim_data[0]->get_AVG_intentions_completiontime() },
                { "AVG_intentions_completiontime_RR", sim_data[1]->get_AVG_intentions_completiontime() },
                { "AVG_intentions_completiontime_FCFS", sim_data[2]->get_AVG_intentions_completiontime() }
        };
        output_data_json.push_back(completion_time_json);
        // ---------------------------------------------------------------------------------

        // ---------------------------------------------------------------------------------
        if(single_simulation) {
            logger->print(Logger::EssentialInfo, " EDF  > AVG Still in Ready: "+ std::to_string(sim_data[0]->get_AVG_still_ready())
            +", Still in Release: "+ std::to_string(sim_data[0]->get_AVG_still_release())
            +", Still in Intentions: "+ std::to_string(sim_data[0]->get_AVG_still_intentionset()));
            logger->print(Logger::EssentialInfo, " RR   > AVG Still in Ready: "+ std::to_string(sim_data[1]->get_AVG_still_ready())
            +", Still in Release: "+ std::to_string(sim_data[1]->get_AVG_still_release())
            +", Still in Intentions: "+ std::to_string(sim_data[1]->get_AVG_still_intentionset()));
            logger->print(Logger::EssentialInfo, " FCFS > AVG Still in Ready: "+ std::to_string(sim_data[2]->get_AVG_still_ready())
            +", Still in Release: "+ std::to_string(sim_data[2]->get_AVG_still_release())
            +", Still in Intentions: "+ std::to_string(sim_data[2]->get_AVG_still_intentionset()));
            logger->print(Logger::EssentialInfo, "-------------------------------------------");

            json still_active_json = {
                    { "AVG_still_intentionset_EDF", sim_data[0]->get_AVG_still_intentionset() },
                    { "AVG_still_intentionset_RR", sim_data[1]->get_AVG_still_intentionset() },
                    { "AVG_still_intentionset_FCFS", sim_data[2]->get_AVG_still_intentionset() },
                    { "AVG_still_ready_EDF", sim_data[0]->get_AVG_still_ready() },
                    { "AVG_still_ready_RR", sim_data[1]->get_AVG_still_ready() },
                    { "AVG_still_ready_FCFS", sim_data[2]->get_AVG_still_ready() },
                    { "AVG_still_release_EDF", sim_data[0]->get_AVG_still_release() },
                    { "AVG_still_release_RR", sim_data[1]->get_AVG_still_release() },
                    { "AVG_still_release_FCFS", sim_data[2]->get_AVG_still_release() }
            };
            output_data_json.push_back(still_active_json);
        }
        // ---------------------------------------------------------------------------------

        // ---------------------------------------------------------------------------------
        if(single_simulation) {
            logger->print(Logger::EssentialInfo, " EDF  > AVG Unexpected Events: "+ std::to_string(sim_data[0]->get_AVG_unexpected_events()));
            logger->print(Logger::EssentialInfo, " RR   > AVG Unexpected Events: "+ std::to_string(sim_data[1]->get_AVG_unexpected_events()));
            logger->print(Logger::EssentialInfo, " FCFS > AVG Unexpected Events: "+ std::to_string(sim_data[2]->get_AVG_unexpected_events()));
            logger->print(Logger::EssentialInfo, "-------------------------------------------");

            json unexpected_json = {
                    { "AVG_unexpected_events_EDF", sim_data[0]->get_AVG_unexpected_events() },
                    { "AVG_unexpected_events_RR", sim_data[1]->get_AVG_unexpected_events() },
                    { "AVG_unexpected_events_FCFS", sim_data[2]->get_AVG_unexpected_events() }
            };
            output_data_json.push_back(unexpected_json);
        }
        // ---------------------------------------------------------------------------------

        // ---------------------------------------------------------------------------------
        if(single_simulation) {
            logger->print(Logger::EssentialInfo, " EDF  > AVG Generated Errors: "+ std::to_string(sim_data[0]->get_AVG_generated_errors()));
            logger->print(Logger::EssentialInfo, " RR   > AVG Generated Errors: "+ std::to_string(sim_data[1]->get_AVG_generated_errors()));
            logger->print(Logger::EssentialInfo, " FCFS > AVG Generated Errors: "+ std::to_string(sim_data[2]->get_AVG_generated_errors()));
            logger->print(Logger::EssentialInfo, "-------------------------------------------");

            json generated_errors_json = {
                    { "AVG_generated_errors_EDF", sim_data[0]->get_AVG_generated_errors() },
                    { "AVG_generated_errors_RR", sim_data[1]->get_AVG_generated_errors() },
                    { "AVG_generated_errors_FCFS", sim_data[2]->get_AVG_generated_errors() }
            };
            output_data_json.push_back(generated_errors_json);
        }
        // ---------------------------------------------------------------------------------

        save_simulation_comparison_json(folder, user, simulation_name, output_report_name, output_data_json);
        logger->print(Logger::EssentialInfo, "File ["+ simulation_name + "/" + output_report_name +"] generated!");
    }
}

void CompareSimulations::printComparison_latex(std::shared_ptr<Logger> logger, const std::vector<std::unique_ptr<SimulationData>>& sim_data,
        const std::string simulation_name, const int simulation_number, const bool single_simulation)
{   // print: \multirow{3}{*}{n} & a & b & c & d & e & f \\ \cline{2-8}

    logger->print(Logger::EssentialInfo, "\\multirow{3}{*}{"+ std::to_string(simulation_number)
    +"} & EDF & "+ std::to_string(sim_data[0]->get_tot_completed_tasks())
    +" & "+ std::to_string(sim_data[0]->get_tot_miss_generated())
    +" & "+ std::to_string(sim_data[0]->get_deadlinemiss_generated())
    +" & "+ std::to_string(sim_data[0]->get_AVG_lateness())
    +" & \\textbf{"+ std::to_string(sim_data[0]->get_miss_percentage())
    +"} & \\textbf{"+ std::to_string(sim_data[0]->get_AVG_achieved_goals())
    +"} \\\\ \\cline{2-8} ");
    logger->print(Logger::EssentialInfo, " & RR & "+ std::to_string(sim_data[1]->get_tot_completed_tasks())
    +" & "+ std::to_string(sim_data[1]->get_tot_miss_generated())
    +" & "+ std::to_string(sim_data[1]->get_deadlinemiss_generated())
    +" & "+ std::to_string(sim_data[1]->get_AVG_lateness())
    +" & \\textbf{"+ std::to_string(sim_data[1]->get_miss_percentage())
    +"} & \\textbf{"+ std::to_string(sim_data[1]->get_AVG_achieved_goals())
    +"} \\\\ \\cline{2-8} ");
    logger->print(Logger::EssentialInfo, " & FCFS & "+ std::to_string(sim_data[2]->get_tot_completed_tasks())
    +" & "+ std::to_string(sim_data[2]->get_tot_miss_generated())
    +" & "+ std::to_string(sim_data[2]->get_deadlinemiss_generated())
    +" & "+ std::to_string(sim_data[2]->get_AVG_lateness())
    +" & \\textbf{"+ std::to_string(sim_data[2]->get_miss_percentage())
    +"} & \\textbf{"+ std::to_string(sim_data[2]->get_AVG_achieved_goals())
    +"} \\\\ \\hline ");
}


std::unique_ptr<SimulationData> CompareSimulations::read_simulation_analysis_json(std::string user, std::string folder,
        std::string analysis_file, std::string simulation_name, std::string sched_algorithm)
{
    std::string path = "../simulations/"+ user +"/"+ folder +"/"+ simulation_name +"/"+ (sched_algorithm +"_"+ analysis_file);
    std::unique_ptr<SimulationData> data = std::make_unique<SimulationData>();
    int set = 0;

    std::cout << path << std::endl;

    std::ifstream i(path.c_str());
    try {
        if (!i.fail()) {
            json data_json, item;
            i >> data_json;

            if(data_json.size() > 0) {
                item = data_json[set];
                data->set_crashed_simulations(item["Num_of_crashed_simulations"]);
                data->set_forced_stop_simulations(item["Num_of_forced_stop_simulations"]);
                data->set_time_limit_errors(item["Num_of_time_limit_errors"]);
                data->set_num_of_simulations(item["Num_of_simulations"]);
                data->set_simulation_analyzed(item["Simulation_analyzed"]);
                data->set_scheduling_algorithm(item["Scheduling_algorithm"]);

                set += 1;
                item = data_json[set];
                data->set_AVG_lateness(item["Average_lateness"]);
                data->set_tot_completed_tasks(item["Tot_completed_tasks"]);
                data->set_deadlinemiss_generated(item["Tot_lateness"]);
                data->set_tot_miss_generated(item["Tot_miss_generated"]);
                data->set_miss_percentage(item["Tot_miss_generated"], item["Tot_completed_tasks"]);

                set += 1;
                item = data_json[set];
                data->set_AVG_executed_tasks(item["AVG_executed_tasks"]);
                data->set_AVG_response_time(item["AVG_response_time"]);
                data->set_executed_tasks_tot(item["Tot_executed_tasks"]);

                set += 1;
                item = data_json[set];
                data->set_AVG_goals_reasoningcycle(item["AVG_goals_reasoningcycle"]);
                data->set_AVG_intentions_reasoningcycle(item["AVG_intentions_reasoningcycle"]);

                set += 1;
                item = data_json[set];
                data->set_AVG_goals_phi(item["AVG_goals_phi"]);
                data->set_AVG_intentions_phi(item["AVG_intentions_phi"]);

                set += 1;
                item = data_json[set];
                data->set_AVG_achieved_goals(item["AVG_achieved_goals"]);

                set += 1;
                item = data_json[set];
                data->set_AVG_goals_completiontime(item["AVG_goals_completiontime"]);

                set += 1;
                item = data_json[set];
                data->set_AVG_intentions_completiontime(item["AVG_intentions_completiontime"]);

                set += 1;
                item = data_json[set];
                data->set_AVG_still_intentionset(item["AVG_still_intentionset"]);
                data->set_AVG_still_ready(item["AVG_still_ready"]);
                data->set_AVG_still_release(item["AVG_still_release"]);

                set += 1;
                item = data_json[set];
                data->set_AVG_unexpected_events(item["AVG_unexpected_events"]);

                set += 1;
                item = data_json[set];
                data->set_AVG_generated_errors(item["Generated_errors_avg"]);
            }
        }
        else {
            std::cerr << "Could not open file '"<< path << "'!" << std::endl;
        }
    }
    catch (std::exception const& ex) {
        throw std::runtime_error("[ERROR] While reading file "+ path + ": " + ex.what());
    }

    return data;
}

std::string CompareSimulations::compareParameterResults(const double analyzed, const double comparison1,
        const double comparison2, const std::string op)
{
    if(op == ">") {
        if(analyzed > comparison2 && analyzed > comparison1) {
            return "<---";
        }
    }
    else if(op == "<") {
        if(analyzed < comparison2 && analyzed < comparison1) {
            return "<---";
        }
    }

    return "";
}

// --------------------------------------------------------------------------------------------------- //
// --------------------------------- SimulationData read from json ----------------------------------- //
// --------------------------------------------------------------------------------------------------- //
SimulationData::SimulationData() {
    crashed_simulations = -1;
    forced_stop_simulations = -1;
    time_limit_errors = -1;
    num_of_simulations = -1;
    simulation_analyzed = -1;
    scheduling_algorithm = "";
    deadlinemiss_generated = -1;
    AVG_lateness = -1;
    tot_completed_tasks = -1;
    tot_miss_generated = -1;
    miss_percentage = -1;
    executed_tasks_tot = -1;
    AVG_response_time = -1;
    AVG_executed_tasks = -1;
    AVG_goals_reasoningcycle = -1;
    AVG_intentions_reasoningcycle = -1;
    AVG_goals_phi = -1;
    AVG_intentions_phi = -1;
    AVG_achieved_goals = -1;
    AVG_goals_completiontime = -1;
    AVG_intentions_completiontime = -1;
    AVG_still_intentionset = -1;
    AVG_still_ready = -1;
    AVG_still_release = -1;
    AVG_unexpected_events = -1;
    AVG_generated_errors = -1;
}

// Getters:
int SimulationData::get_crashed_simulations() {
    return crashed_simulations;
}

int SimulationData::get_forced_stop_simulations() {
    return forced_stop_simulations;
}

int SimulationData::get_time_limit_errors() {
    return time_limit_errors;
}

int SimulationData::get_num_of_simulations() {
    return num_of_simulations;
}

int SimulationData::get_simulation_analyzed() {
    return simulation_analyzed;
}

std::string SimulationData::get_scheduling_algorithm() {
    return scheduling_algorithm;
}

double SimulationData::get_deadlinemiss_generated() {
    return deadlinemiss_generated;
}

double SimulationData::get_tot_completed_tasks() {
    return tot_completed_tasks;
}

double SimulationData::get_tot_miss_generated() {
    return tot_miss_generated;
}

double SimulationData::get_AVG_lateness() {
    return AVG_lateness;
}

double SimulationData::get_executed_tasks_tot() {
    return executed_tasks_tot;
}

double SimulationData::get_miss_percentage() {
    return miss_percentage;
}

double SimulationData::get_AVG_response_time() {
    return AVG_response_time;
}

double SimulationData::get_AVG_executed_tasks() {
    return AVG_executed_tasks;
}

double SimulationData::get_AVG_goals_reasoningcycle() {
    return AVG_goals_reasoningcycle;
}

double SimulationData::get_AVG_intentions_reasoningcycle() {
    return AVG_intentions_reasoningcycle;
}

double SimulationData::get_AVG_goals_phi() {
    return AVG_goals_phi;
}

double SimulationData::get_AVG_intentions_phi() {
    return AVG_intentions_phi;
}

double SimulationData::get_AVG_achieved_goals() {
    return AVG_achieved_goals;
}

double SimulationData::get_AVG_goals_completiontime() {
    return AVG_goals_completiontime;
}

double SimulationData::get_AVG_intentions_completiontime() {
    return AVG_intentions_completiontime;
}

double SimulationData::get_AVG_still_intentionset() {
    return AVG_still_intentionset;
}

double SimulationData::get_AVG_still_ready() {
    return AVG_still_ready;
}

double SimulationData::get_AVG_still_release() {
    return AVG_still_release;
}

double SimulationData::get_AVG_unexpected_events() {
    return AVG_unexpected_events;
}

double SimulationData::get_AVG_generated_errors() {
    return AVG_generated_errors;
}


// Setters:
void SimulationData::set_crashed_simulations(const int val) {
    crashed_simulations = val;
}

void SimulationData::set_forced_stop_simulations(const int val) {
    forced_stop_simulations = val;
}

void SimulationData::set_time_limit_errors(const int val) {
    time_limit_errors = val;
}

void SimulationData::set_num_of_simulations(const int val) {
    num_of_simulations = val;
}

void SimulationData::set_simulation_analyzed(const double val) {
    simulation_analyzed = val;
}

void SimulationData::set_scheduling_algorithm(const std::string val) {
    scheduling_algorithm = val;
}

void SimulationData::set_deadlinemiss_generated(const double val) {
    deadlinemiss_generated = val;
}

void SimulationData::set_tot_completed_tasks(const double val) {
    tot_completed_tasks = val;
}

void SimulationData::set_tot_miss_generated(const double val) {
    tot_miss_generated = val;
}

void SimulationData::set_executed_tasks_tot(const double val) {
    executed_tasks_tot = val;
}

void SimulationData::set_miss_percentage(const double miss, const double tot_tasks) {
    if(tot_tasks > 0) {
        miss_percentage = (miss / tot_tasks) * 100.0;
    }
    else {
        miss_percentage = 0;
    }
}

void SimulationData::set_AVG_lateness(const double val) {
    AVG_lateness = val;
}

void SimulationData::set_AVG_response_time(const double val) {
    AVG_response_time = val;
}

void SimulationData::set_AVG_executed_tasks(const double val) {
    AVG_executed_tasks = val;
}

void SimulationData::set_AVG_goals_reasoningcycle(const double val) {
    AVG_goals_reasoningcycle = val;
}

void SimulationData::set_AVG_intentions_reasoningcycle(const double val) {
    AVG_intentions_reasoningcycle = val;
}

void SimulationData::set_AVG_goals_phi(const double val) {
    AVG_goals_phi = val;
}

void SimulationData::set_AVG_intentions_phi(const double val) {
    AVG_intentions_phi = val;
}

void SimulationData::set_AVG_achieved_goals(const double val) {
    AVG_achieved_goals = val;
}

void SimulationData::set_AVG_goals_completiontime(const double val) {
    AVG_goals_completiontime = val;
}

void SimulationData::set_AVG_intentions_completiontime(const double val) {
    AVG_intentions_completiontime = val;
}

void SimulationData::set_AVG_still_intentionset(const double val) {
    AVG_still_intentionset = val;
}

void SimulationData::set_AVG_still_ready(const double val) {
    AVG_still_ready = val;
}

void SimulationData::set_AVG_still_release(const double val) {
    AVG_still_release = val;
}

void SimulationData::set_AVG_unexpected_events(const double val) {
    AVG_unexpected_events = val;
}

void SimulationData::set_AVG_generated_errors(const double val) {
    AVG_generated_errors = val;
}

