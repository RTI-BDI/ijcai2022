/*
 * AnalyzeSimulations.cc
 *
 *  Created on: Feb 11, 2021
 *      Author: francesco
 */
#include "../headers/AnalyzeSimulations.h"

AnalyzeSimulations::AnalyzeSimulations() { }

AnalyzeSimulations::~AnalyzeSimulations() { }

void AnalyzeSimulations::initialize() {
    bool continuousMode = par("continuousMode");
    std::string user = par("user");
    std::string folder_path = par("folder_path"); // path containing the already generated files that the designer needs for the simulation
    std::string reports = "/reports/";
    std::string simulation_name = par("simulation_name");
    std::string mainfolder_path = "../simulations/"+ user +"/"+ folder_path +"/"+ simulation_name;
    std::string write_folder_path = folder_path +"/"+ simulation_name;
    std::string input_filename = par("inputFileName");
    std::string output_filename = par("outputFileName");
    std::string scheduling_algorithm = par("sched_type");
    int initial_seed = par("initial_seed");
    int logger_level = par("logger_level");
    int num_of_simulations = par("num_of_simulations");

    logger = std::make_shared<Logger>(logger_level);
    std::string final_output_filename = scheduling_algorithm + "_" + output_filename;

    delete_simulation_analysis_json_file(final_output_filename, write_folder_path, user);

    if(initial_seed > -1 && num_of_simulations >= initial_seed)
    {
        analyzeGeneratedSimulations(user, continuousMode, mainfolder_path, write_folder_path,
                simulation_name, input_filename, final_output_filename, scheduling_algorithm,
                initial_seed, num_of_simulations);
    }
}

void AnalyzeSimulations::finish() { }

void AnalyzeSimulations::analyzeGeneratedSimulations(std::string user, bool continuousMode,
        std::string mainfolder_path, std::string write_folder_path,
        std::string simulation_name, std::string inputfile_name,
        std::string final_output_filename, std::string scheduling_algorithm, int initial_seed, int num_of_simulations)
{
    logger->print(Logger::EssentialInfo, " ----analyze_simulations---- ");
    logger->print(Logger::EssentialInfo, " Analyzing: "+ mainfolder_path);

    std::string partial_folderpath;
    std::string current_folderpath, current_simulation_name;
    std::string file_path;
    std::vector<std::shared_ptr<AnalyzeSimulations_Deadlinemiss>> gen_deadlinemiss;
    std::vector<std::shared_ptr<AnalyzeSimulations_Stats>> reasoningcycle_stats;
    std::vector<std::shared_ptr<AnalyzeSimulations_Stats>> phiI_stats;
    std::vector<std::string> simulation_time_reached, simulation_time_reached_errors, crashed_simulations, forced_stop_simulations;
    json gen_ddlmiss_sim_names;
    bool check_next_simulation = true, printSimulationName = true;
    double simulation_analyzed = 0;
    double overall_tot_gen_deadlinemiss = 0, overall_tot_response_time = 0;
    double overall_tot_executed_tasks = 0;
    double overall_active_goals_reasoningcycle = 0, overall_active_intentions_reasoningcycle = 0;
    double overall_active_goals_phiI = 0, overall_active_intentions_phiI = 0;
    double overall_generated_errors = 0;
    double overall_generated_warnings = 0;
    double overall_still_in_ready = 0, overall_still_in_release = 0, overall_still_in_intentions = 0, overall_still_in_intentions_waiting_completion = 0;
    double overall_unexpected_executions = 0;
    double overall_needed_time_to_goal_completion = 0, tot_completed_goals = 0;
    double overall_needed_time_to_intention_completion = 0, tot_completed_intentions = 0;
    double tot_achieved_goals = 0;
    double tot_completed_tasks = 0, tot_deadline_miss_tasks = 0;
    int index_metric = 0;

    for(int i = initial_seed; i <= num_of_simulations && check_next_simulation; i++) {
        current_simulation_name = simulation_name +"_seed_"+ std::to_string(i);
        partial_folderpath = mainfolder_path + "/" + current_simulation_name;
        current_folderpath = partial_folderpath  + "/reports/";
        file_path = current_folderpath + scheduling_algorithm + "/" + inputfile_name;
        printSimulationName = true;
        index_metric = 0;

        std::ifstream is(file_path.c_str());
        try {
            if (!is.fail()) {
                json metrics, metric;
                is >> metrics;
                if(metrics.size() > 0)
                {
                    // Check if simulation crashed -----------------------------------------------------------------------
                    metric = metrics[index_metric];
                    bool isSimulationCrashed = metric["Simulation_crash"].get<bool>();
                    // ---------------------------------------------------------------------------------------------------
                    if(isSimulationCrashed == true)
                    {
                        logger->print(Logger::Error, " Simulation: "+ current_simulation_name
                                +" CRASHED at t: "+ std::to_string(metric["Time"].get<double>())
                                +", due to: "+ metric["Reason"].get<std::string>());
                        crashed_simulations.push_back(current_simulation_name);
                    }
                    else if(metrics.size() >= 9)
                    {
                        // check if simulation has ended due to message "EXIT" from client -----------------------------------
                        index_metric += 1;
                        metric = metrics[index_metric];
                        if(metric.find("Forced_simulation_end") != metric.end())
                        {
                            if(metric["Forced_simulation_end"].get<bool>() == true)
                            {
                                logger->print(Logger::Error, " Simulation: "+ current_simulation_name
                                        +" has been STOPPED BEFORE COMPLETION at t: "+ std::to_string(metric["Time"].get<double>())
                                        +", due to: "+ metric["Reason"].get<std::string>());
                                forced_stop_simulations.push_back(current_simulation_name);
                            }
                        }
                        else {
                            index_metric -= 1;
                        }
                        // ---------------------------------------------------------------------------------------------------

                        // Deadline miss and Lateness ------------------------------------------------------------------------
                        index_metric += 1;
                        metric = metrics[index_metric];
                        if(metric["Scheduling_type"].get<std::string>() == scheduling_algorithm) {
                            scheduling_algorithm = metric["Scheduling_type"].get<std::string>();
                        }
                        else {
                            logger->print(Logger::Error, " Analyzing simulation: "+ current_simulation_name +" has a different scheduling algorithm: "+ metric["Scheduling_type"].get<std::string>());
                            scheduling_algorithm = "Simulations with different scheduling algorithm!";
                        }

                        for(json element : metric["Deadlinemiss_generated"]) {
                            gen_deadlinemiss.push_back(std::make_shared<AnalyzeSimulations_Deadlinemiss>(
                                    current_simulation_name,
                                    element["Time"], element["Id"],
                                    element["Lateness"], element["LatenessLog"],
                                    element["N_exec"], element["Original_plan"], element["Plan"]));
                            overall_tot_gen_deadlinemiss += element["Lateness"].get<double>();
                        }
                        tot_deadline_miss_tasks += metric["Tot_miss_generated"].get<double>();
                        tot_completed_tasks += metric["Tot_completed_tasks"].get<double>();
                        if(metric["Tot_miss_generated"].get<double>() > 0) {
                            gen_ddlmiss_sim_names.push_back({
                                {"simulation:", current_simulation_name},
                                {"miss", metric["Tot_miss_generated"].get<double>()}
                            });
                        }
                        // ---------------------------------------------------------------------------------------------------

                        // Tasks Response Time -------------------------------------------------------------------------------
                        index_metric += 1;
                        metric = metrics[index_metric];
                        overall_tot_executed_tasks += metric["Num_of_executed_tasks"].get<int>();
                        for(json element : metric["Response_time"]) {
                            overall_tot_response_time += element["responseTime"].get<double>();
                        }
                        // ---------------------------------------------------------------------------------------------------

                        // Reasoning cycle Stats -----------------------------------------------------------------------------
                        index_metric += 1;
                        metric = metrics[index_metric];
                        for(json element : metric["Stats_rc"]) {
                            reasoningcycle_stats.push_back(std::make_shared<AnalyzeSimulations_Stats>(element["Active_goals"],
                                    element["Active_intentions"], element["Iteration"], element["Time"].get<double>()));
                            overall_active_goals_reasoningcycle += element["Active_goals"].get<int>();
                            overall_active_intentions_reasoningcycle += element["Active_intentions"].get<int>();
                        }
                        // ---------------------------------------------------------------------------------------------------

                        // PhiI executions Stats -----------------------------------------------------------------------------
                        index_metric += 1;
                        metric = metrics[index_metric];
                        for(json element : metric["Stats_phiI"]) {
                            phiI_stats.push_back(std::make_shared<AnalyzeSimulations_Stats>(element["Active_goals"],
                                    element["Active_intentions"], element["Iteration"], element["Time"].get<double>()));
                            overall_active_goals_phiI += element["Active_goals"].get<int>();
                            overall_active_intentions_phiI += element["Active_intentions"].get<int>();
                        }
                        // ---------------------------------------------------------------------------------------------------

                        // Still active elements -----------------------------------------------------------------------------
                        index_metric += 1; // INDEX = 5
                        metric = metrics[index_metric];
                        bool is_simulation_limit_reached = metric["Simulation_limit_reached"].get<bool>();
                        overall_still_in_ready += metric["Ready_vector"].get<int>();
                        overall_still_in_release += metric["Release_vector"].get<int>();
                        overall_still_in_intentions += metric["Still_active_intentions"].get<int>();
                        overall_still_in_intentions_waiting_completion += metric["Still_active_intentions_waiting_completion"].get<int>();

                        if(is_simulation_limit_reached && printSimulationName)
                        {
                            simulation_time_reached.push_back(current_simulation_name);
                        }

                        if((metric["Ready_vector"].get<int>() > 0 || metric["Release_vector"].get<int>() > 0
                                || (metric["Still_active_intentions"].get<int>() - metric["Still_active_intentions_waiting_completion"].get<int>() > 0))
                                && printSimulationName)
                        {
                            logger->print(Logger::EssentialInfo, " Analyzing simulation: "+ current_simulation_name);
                            logger->print(Logger::EssentialInfo, " ** Simulation time reached: "+ boolToString(is_simulation_limit_reached) +" ** ");
                            if(is_simulation_limit_reached == false) {
                                simulation_time_reached_errors.push_back(current_simulation_name);
                            }
                            printSimulationName = false;
                        }

                        if(metric["Ready_vector"].get<int>() > 0) {
                            logger->print(Logger::Warning, " - Tasks still in READY: "+ std::to_string(metric["Ready_vector"].get<int>()));
                        }
                        if(metric["Release_vector"].get<int>() > 0) {
                            logger->print(Logger::Warning, " - Tasks still in RELEASE: "+ std::to_string(metric["Release_vector"].get<int>()));
                        }
                        if(metric["Still_active_intentions"].get<int>() > 0) {
                            logger->print(Logger::Warning, " - Still in INTENTIONS: "+ std::to_string(metric["Still_active_intentions"].get<int>()));
                        }
                        if(metric["Still_active_intentions_waiting_completion"].get<int>() > 0) {
                            logger->print(Logger::Warning, " - Waiting for completion: "+ std::to_string(metric["Still_active_intentions_waiting_completion"].get<int>()));
                        }
                        // --------------------------------------------------------------------------

                        // Activated Goals ------------------------------------------------------------------------------------
                        metric = metrics[index_metric]; // INDEX = 5
                        std::shared_ptr<AnalyzeSimulations_Goals_activation> activated_goal;
                        std::vector<std::shared_ptr<AnalyzeSimulations_Goals_activation>> activated_goals;
                        for(json element : metric["Goals_activation"]) {
                            activated_goal = std::make_shared<AnalyzeSimulations_Goals_activation>(
                                    element["Belief_name"].get<std::string>(),
                                    element["Activation_time"].get<double>(), element["Completion_time"].get<double>(),
                                    element["Deadline"].get<double>(), element["Due_to"].get<std::string>(),
                                    element["Goal_id"].get<int>(), element["Plan_id"].get<int>(),
                                    element["Origina_plan_id"].get<int>(), element["Annidation_level"].get<std::string>(),
                                    element["Plan_pref_index"].get<int>()
                            );
                            activated_goals.push_back(activated_goal);
                            if(element["Completion_time"].get<double>() >= 0 && element["Activation_time"].get<double>() >= 0) {
                                overall_needed_time_to_goal_completion += (element["Completion_time"].get<double>() - element["Activation_time"].get<double>());
                            }
                            tot_completed_goals += 1;
                        }
                        // ---------------------------------------------------------------------------------------------------

                        // Activated Intentions ------------------------------------------------------------------------------
                        metric = metrics[index_metric]; // INDEX = 5
                        std::shared_ptr<AnalyzeSimulations_Intentions_activation> activated_intention;
                        std::vector<std::shared_ptr<AnalyzeSimulations_Intentions_activation>> activated_intentions;
                        for(json element : metric["Intentions_activation"]) {
                            activated_intention = std::make_shared<AnalyzeSimulations_Intentions_activation>(
                                    element["Activation_time"].get<double>(), element["Completion_time"].get<double>(),
                                    element["Due_to"].get<std::string>(), element["Goal_id"].get<int>(),
                                    element["Original_plan_id"].get<int>(), element["Intention_id"].get<int>()
                            );
                            activated_intentions.push_back(activated_intention);
                            if(element["Completion_time"].get<double>() >= 0 && element["Activation_time"].get<double>() >= 0) {
                                overall_needed_time_to_intention_completion += (element["Completion_time"].get<double>() - element["Activation_time"].get<double>());
                            }
                            tot_completed_intentions += 1;
                        }
                        // ---------------------------------------------------------------------------------------------------

                        // Achieved Goals ------------------------------------------------------------------------------------
                        index_metric += 1;
                        metric = metrics[index_metric]; // INDEX = 6
                        tot_achieved_goals += metric["Tot_achievedGoals"].get<double>();
                        // --------------------------------------------------------------------------

                        // Unexpected executions -----------------------------------------------------------------------------
                        index_metric += 1;
                        metric = metrics[index_metric];
                        overall_unexpected_executions += metric["Tot_exec"].get<int>();
                        if(metric["Tot_exec"].get<int>() > 0 && printSimulationName) {
                            logger->print(Logger::EssentialInfo, " Analyzing simulation: "+ current_simulation_name);
                            printSimulationName = false;
                        }
                        if(metric["Tot_exec"].get<int>() > 0) {
                            logger->print(Logger::Error, " - Unexpected events: "+ std::to_string(metric["Tot_exec"].get<int>()));
                        }
                        // ---------------------------------------------------------------------------------------------------

                        // Generated Errors -----------------------------------------------------------------------------
                        index_metric += 1;
                        metric = metrics[index_metric];
                        double gen_errs = metric["Num_of_generated_errors"].get<double>();
                        overall_generated_errors += gen_errs;
                        if(gen_errs > 0) {
                            logger->print(Logger::Error, " - Generated Errors: "+ std::to_string(gen_errs));
                        }
                        // ---------------------------------------------------------------------------------------------------

                        // Generated Warnings -----------------------------------------------------------------------------
                        // Note (implementation choice): Warnings are not printed in the file "metrics.json"
                        // if(metrics.size() >= 10)
                        // {
                        //    index_metric += 1; // INDEX = 9
                        //    metric = metrics[index_metric];
                        //    double gen_warns = metric["Num_of_generated_warnings"].get<double>();
                        //    overall_generated_warnings += gen_warns;
                        //    if(gen_warns > 0) {
                        //        logger->print(Logger::Error, " - Generated Warnings: "+ std::to_string(gen_warns));
                        //    }
                        // }
                        // ---------------------------------------------------------------------------------------------------
                    }
                    simulation_analyzed += 1;
                }
            }
            else {
                check_next_simulation = continuousMode;
                logger->print(Logger::Error, "Could not open file: "+ file_path);
            }
        }
        catch (std::exception const& ex) {
            check_next_simulation = continuousMode;
            logger->print(Logger::Error, "File:["+ file_path +"] generated exception: "+ ex.what());
            throw std::runtime_error("File:["+ file_path +"] generated exception: "+ ex.what());
        }
    }

    if(simulation_analyzed == 0) {
        logger->print(Logger::EssentialInfo, "\n No simulation analyzed!");
    }
    else {
        compute_simulation_analysis_header(simulation_name, mainfolder_path, simulation_analyzed,
                scheduling_algorithm, num_of_simulations, crashed_simulations, forced_stop_simulations,
                simulation_time_reached, simulation_time_reached_errors);

        computeAvgDeadlinemiss(gen_deadlinemiss, gen_ddlmiss_sim_names, overall_tot_gen_deadlinemiss, tot_completed_tasks, tot_deadline_miss_tasks, simulation_analyzed);
        computeAvgResponseTime(overall_tot_executed_tasks, overall_tot_response_time, simulation_analyzed);
        computeAvgReasoningcycle(reasoningcycle_stats.size(), overall_active_goals_reasoningcycle, overall_active_intentions_reasoningcycle);
        computeAvgPhiI(phiI_stats.size(), overall_active_goals_phiI, overall_active_intentions_phiI);
        computeAvgAchievedGoals(tot_achieved_goals, simulation_analyzed);
        computeAvgCompletedGoals(overall_needed_time_to_goal_completion, tot_completed_goals);
        computeAvgCompletedIntentions(overall_needed_time_to_intention_completion, tot_completed_intentions);
        computeAvgStillActive(overall_still_in_ready, overall_still_in_release, overall_still_in_intentions, overall_still_in_intentions_waiting_completion, simulation_analyzed);
        computeAvgUnexpected(overall_unexpected_executions, simulation_analyzed);
        computeAvgGeneratedErrors(overall_generated_errors, simulation_analyzed);
        computeAvgGeneratedWarnings(overall_generated_warnings, simulation_analyzed);

        save_simulation_analysis(final_output_filename, write_folder_path, user);
    }
}


void AnalyzeSimulations::computeAvgAchievedGoals(double tot_achieved_goals, double simulation_analyzed) {
    double avg_goals;
    json main;
    if(simulation_analyzed > 0) {
        avg_goals = tot_achieved_goals / simulation_analyzed;
        logger->print(Logger::EssentialInfo, " > AVG Achieved Goals: "+ std::to_string(avg_goals));
    }
    else {
        avg_goals = 0;
        logger->print(Logger::EssentialInfo, " > AVG Achieved Goals: 0");
    }

    main = { { "AVG_achieved_goals", avg_goals } };
    write_simulation_analysis(main);
}

void AnalyzeSimulations::computeAvgGeneratedErrors(double tot_generated_errors, double simulation_analyzed)
{
    double avg_errors;
    json json_errors;
    if(simulation_analyzed > 0) {
        avg_errors = tot_generated_errors / simulation_analyzed;
        logger->print(Logger::EssentialInfo, " > AVG Generated Errors: "+ std::to_string(avg_errors));
    }
    else {
        avg_errors = 0;
        logger->print(Logger::EssentialInfo, " > AVG Generated Errors: 0");
    }

    json_errors = { { "Generated_errors_avg", avg_errors }};
    write_simulation_analysis(json_errors);
}

void AnalyzeSimulations::computeAvgGeneratedWarnings(double tot_generated_warnings, double simulation_analyzed)
{
    double avg_warnings;
    json json_warnings;
    if(simulation_analyzed > 0) {
        avg_warnings = tot_generated_warnings / simulation_analyzed;
        logger->print(Logger::EveryInfo, " > AVG Generated Warnings: "+ std::to_string(avg_warnings));
    }
    else {
        avg_warnings = 0;
        logger->print(Logger::EveryInfo, " > AVG Generated Warnings: 0");
    }

    json_warnings = { { "Generated_warnings_avg", avg_warnings }};
    write_simulation_analysis(json_warnings);
}

void AnalyzeSimulations::computeAvgDeadlinemiss(std::vector<std::shared_ptr<AnalyzeSimulations_Deadlinemiss>>& gen_deadlinemiss,
        json gen_ddlmiss_sim_names, double total_lateness, double tot_completed_tasks, double tot_deadline_miss_tasks, double simulations)
{
    double avg_lateness = 0, avg_deadlinemiss = 0, avg_ddl_miss_tasks = 0;
    double records = gen_deadlinemiss.size();
    json json_lateness;
    json json_gen_deadlinemiss;
    if(records > 0) {
        avg_lateness = total_lateness / records;
    }
    if(simulations > 0) {
        avg_deadlinemiss = records / simulations;
    }
    if(tot_completed_tasks > 0) {
        avg_ddl_miss_tasks = (tot_deadline_miss_tasks / tot_completed_tasks) * 100.0;
    }

    logger->print(Logger::EssentialInfo, " > AVG Lateness: "+ std::to_string(avg_lateness)
            +", Tot Lateness: "+ std::to_string(total_lateness)
            +", AVG Deadlinemiss: "+ std::to_string(avg_deadlinemiss)
            +", Tot Deadlinemiss: "+ std::to_string(records));
    logger->print(Logger::EssentialInfo, " > Tot Deadline miss tasks: "+ std::to_string(tot_deadline_miss_tasks)
            +", Tot Completed Tasks: "+ std::to_string(tot_completed_tasks)
            +", AVG Deadline miss tasks (%): "+ std::to_string(avg_ddl_miss_tasks));

    // compute all the tasks genereting miss
    for(int i = 0; i < (int)gen_deadlinemiss.size(); i++) {
        json_gen_deadlinemiss.push_back({
            { "Id", gen_deadlinemiss[i]->getId() },
            { "Simulation", gen_deadlinemiss[i]->getSimName() },
            { "Lateness", gen_deadlinemiss[i]->getLateness() },
            { "LatenessLog", gen_deadlinemiss[i]->getLog() },
            { "N_exec", gen_deadlinemiss[i]->getNExec() },
            { "Original_plan", gen_deadlinemiss[i]->getOriginalPlanId() },
            { "Plan", gen_deadlinemiss[i]->getPlanId() },
            { "Time", gen_deadlinemiss[i]->getTime() }
        });
    }

    json_lateness = { { "Average_lateness", avg_lateness },
            { "Deadline_miss_tasks", json_gen_deadlinemiss },
            { "Deadline_miss_simulation_names", gen_ddlmiss_sim_names },
            { "Tot_lateness", total_lateness },
            { "Tot_miss_generated", records },
            { "Tot_completed_tasks", tot_completed_tasks } };
    write_simulation_analysis(json_lateness);
}

void AnalyzeSimulations::computeAvgReasoningcycle(double records, double tot_goals, double tot_intentions) {
    double avg_goals, avg_intentions;
    json json_rc;
    if(records > 0) {
        avg_goals = tot_goals / records;
        avg_intentions = tot_intentions / records;
        logger->print(Logger::EssentialInfo, " > Reasoning cycle > AVG Goals: "+ std::to_string(avg_goals)
                +", AVG Intentions: "+ std::to_string(avg_intentions));
    }
    else {
        avg_goals = 0; avg_intentions = 0;
        logger->print(Logger::EssentialInfo, " > Reasoning cycle > AVG Goals: 0, AVG Intentions: 0");
    }

    json_rc = { { "AVG_goals_reasoningcycle", avg_goals },
            { "AVG_intentions_reasoningcycle", avg_intentions } };
    write_simulation_analysis(json_rc);
}

void AnalyzeSimulations::computeAvgResponseTime(double tot_executed_tasks,
        double tot_response_time, double tot_simulations)
{
    double avg_resptime, avg_executed_tasks;
    json json_resptime;
    if(tot_executed_tasks > 0) {
        avg_resptime = tot_response_time / tot_executed_tasks;
        avg_executed_tasks = tot_executed_tasks / tot_simulations;
        logger->print(Logger::EssentialInfo, " > Response Time > Tasks: "+ std::to_string(tot_executed_tasks)
                +", AVG Response Time: "+ std::to_string(avg_resptime)
                +", AVG Tasks per simulation: "+ std::to_string(avg_executed_tasks));
    }
    else {
        avg_resptime = 0; avg_executed_tasks = 0;
        logger->print(Logger::EssentialInfo, " > Response Time > Tasks: 0, AVG Response Time: 0, AVG Tasks per simulation: 0");
    }

    json_resptime = { { "AVG_response_time", avg_resptime },
            { "AVG_executed_tasks", avg_executed_tasks },
            { "Tot_executed_tasks", tot_executed_tasks } };
    write_simulation_analysis(json_resptime);
}

void AnalyzeSimulations::computeAvgPhiI(double records, double tot_goals, double tot_intentions) {
    double avg_goals, avg_intentions;
    json json_phi;
    if(records > 0) {
        avg_goals = tot_goals / records;
        avg_intentions = tot_intentions / records;
        logger->print(Logger::EssentialInfo, " > PhiI > AVG Goals: "+ std::to_string(avg_goals)
                +", AVG Intentions: "+ std::to_string(avg_intentions));
    }
    else {
        avg_goals = 0; avg_intentions = 0;
        logger->print(Logger::EssentialInfo, " > PhiI > AVG Goals: 0, AVG Intentions: 0");
    }

    json_phi = { { "AVG_goals_phi", avg_goals },
            { "AVG_intentions_phi", avg_intentions } };
    write_simulation_analysis(json_phi);
}

void AnalyzeSimulations::computeAvgCompletedGoals(double overall_needed_time_to_goal_completion, double tot_completed_goals) {
    double avg = 0;
    json json_completed_goals;
    if(tot_completed_goals > 0) {
        avg = overall_needed_time_to_goal_completion / tot_completed_goals;
        logger->print(Logger::EssentialInfo, " > AVG Goals Completion Time: "+ std::to_string(avg));
    }
    else {
        logger->print(Logger::EssentialInfo, " > AVG Goals Completion Time: null (no activated goals).");
    }

    json_completed_goals = { { "AVG_goals_completiontime", avg } };
    write_simulation_analysis(json_completed_goals);
}

void AnalyzeSimulations::computeAvgCompletedIntentions(double overall_needed_time_to_intention_completion, double tot_completed_intentions) {
    double avg = 0;
    json json_completed_intentions;
    if(tot_completed_intentions > 0) {
        avg = overall_needed_time_to_intention_completion / tot_completed_intentions;
        logger->print(Logger::EssentialInfo, " > AVG Intentions Completion Time: "+ std::to_string(avg));
    }
    else {
        logger->print(Logger::EssentialInfo, " > AVG Intentions Completion Time: null (no activated intentions).");
    }

    json_completed_intentions = { { "AVG_intentions_completiontime", avg } };
    write_simulation_analysis(json_completed_intentions);
}

void AnalyzeSimulations::computeAvgStillActive(double still_ready, double still_release, double still_intentions, double waiting_completion_intentions, double simulations) {
    double avg_ready, avg_release, avg_intentions, avg_waiting_completion;
    json json_still_active;

    if(simulations > 0) {
        avg_ready = still_ready / simulations;
        avg_release = still_release / simulations;
        avg_intentions = still_intentions / simulations;
        avg_waiting_completion = waiting_completion_intentions / simulations;
        logger->print(Logger::EssentialInfo, " > AVG Still in Ready: "+ std::to_string(avg_ready)
                +", Still in Release: "+ std::to_string(avg_release)
                +", Still in Intentions: "+ std::to_string(avg_intentions)
                +", Waiting for completion: "+ std::to_string(avg_waiting_completion));
    }
    else {
        avg_ready = 0; avg_release = 0; avg_intentions = 0;
        logger->print(Logger::EssentialInfo, " > AVG Still in Ready: 0, Still in Release: 0, Still in Intentions: 0, Waiting for completion: 0");
    }

    json_still_active = { { "AVG_still_ready", avg_ready },
            { "AVG_still_release", avg_release },
            { "AVG_still_intentionset", avg_intentions },
            { "AVG_waiting_completion", avg_waiting_completion } };
    write_simulation_analysis(json_still_active);
}

void AnalyzeSimulations::computeAvgUnexpected(double unexpected_executions, double simulations) {
    double avg_unexpected;
    json json_unexpected;

    if(simulations > 0) {
        avg_unexpected = unexpected_executions / simulations;
        logger->print(Logger::EssentialInfo, " > AVG Unexpected Events: "+ std::to_string(avg_unexpected));
    }
    else {
        avg_unexpected = 0;
        logger->print(Logger::EssentialInfo, " > AVG Unexpected Events: 0");
    }

    json_unexpected = { { "AVG_unexpected_events", avg_unexpected } };
    write_simulation_analysis(json_unexpected);
}

void AnalyzeSimulations::compute_simulation_analysis_header(std::string simulation_name, std::string mainfolder_path,
        int simulation_analyzed, std::string scheduling_algorithm, int num_of_simulations,
        std::vector<std::string> crashed_simulations, std::vector<std::string> forced_stop_simulations,
        std::vector<std::string> simulation_time_reached, std::vector<std::string> simulation_time_reached_errors)
{
    json json_header, json_crashed_simulations, json_stopped_simulations, json_time_limit_error;
    logger->print(Logger::EssentialInfo, "\n Simulation: "+ mainfolder_path);
    logger->print(Logger::EssentialInfo, " Simulations analyzed:["+ std::to_string(simulation_analyzed) +" of "+ std::to_string(num_of_simulations) +"]");
    logger->print(Logger::EssentialInfo, " Scheduling algorithm: "+ scheduling_algorithm);

    if(crashed_simulations.size() > 0) {
        logger->print(Logger::Error, " *** Crashed simulations:["+ std::to_string(crashed_simulations.size()) +" of "+ std::to_string(num_of_simulations) +"]");
        for(int i = 0; i < (int)crashed_simulations.size(); i++) {
            json tmp = { { "Simulation_name", crashed_simulations[i] } };
            json_crashed_simulations.push_back(tmp);
        }
    }

    if(forced_stop_simulations.size() > 0) {
        logger->print(Logger::Error, " *** Forced stop simulations:["+ std::to_string(forced_stop_simulations.size()) +" of "+ std::to_string(num_of_simulations) +"]");
        for(int i = 0; i < (int)forced_stop_simulations.size(); i++) {
            json tmp = { { "Simulation_name", forced_stop_simulations[i] } };
            json_stopped_simulations.push_back(tmp);
        }
    }

    if(simulation_time_reached_errors.size() > 0) {
        logger->print(Logger::Error, " *** Time limit Errors:["+ std::to_string(simulation_time_reached_errors.size()) +" of "+ std::to_string(num_of_simulations) +"]");
        for(int i = 0; i < (int)simulation_time_reached_errors.size(); i++) {
            json tmp = { { "Simulation_name", simulation_time_reached_errors[i] } };
            json_time_limit_error.push_back(tmp);
        }
    }

    json_header = { { "Simulation_name", simulation_name },
            { "Simulation_path", mainfolder_path },
            { "Num_of_simulations", num_of_simulations },
            { "Simulation_analyzed", simulation_analyzed },
            { "Scheduling_algorithm", scheduling_algorithm },
            { "Crashed_simulations", json_crashed_simulations },
            { "Forced_stop_simulations", json_stopped_simulations },
            { "Num_of_crashed_simulations", crashed_simulations.size() },
            { "Num_of_forced_stop_simulations", forced_stop_simulations.size() },
            { "Num_of_time_limit_errors", simulation_time_reached_errors.size() },
            { "Num_of_time_limit_reached", simulation_time_reached.size() },
            { "Time_limit_errors", json_time_limit_error }
    };

    write_simulation_analysis(json_header);
}

std::string AnalyzeSimulations::boolToString(const bool val) {
    if(val) {
        return "true";
    }
    return "false";
}

// ----------------------------------------------------------------------------------------------------------- //
// ------------------------------------- AnalyzeSimulations_Deadlinemiss ------------------------------------- //
// ----------------------------------------------------------------------------------------------------------- //
AnalyzeSimulations_Deadlinemiss::AnalyzeSimulations_Deadlinemiss(std::string _name, double _time, int _id, double _lateness, std::string _log,
        int _n_exec, int _origianal_planid, int _planid)
{
    time = _time;
    id = _id;
    lateness = _lateness;
    sim_name = _name;
    log = _log;
    n_exec = _n_exec;
    planid = _planid;
    origianal_planid = _origianal_planid;
}

int AnalyzeSimulations_Deadlinemiss::getId() {
    return id;
}

int AnalyzeSimulations_Deadlinemiss::getNExec() {
    return n_exec;
}

int AnalyzeSimulations_Deadlinemiss::getPlanId() {
    return planid;
}

int AnalyzeSimulations_Deadlinemiss::getOriginalPlanId() {
    return origianal_planid;
}

double AnalyzeSimulations_Deadlinemiss::getTime() {
    return time;
}

double AnalyzeSimulations_Deadlinemiss::getLateness() {
    return lateness;
}

std::string AnalyzeSimulations_Deadlinemiss::getSimName() {
    return sim_name;
}

std::string AnalyzeSimulations_Deadlinemiss::getLog() {
    return log;
}

// ----------------------------------------------------------------------------------------------------------- //
// ---------------------------------- AnalyzeSimulations_ReasoningcycleStats --------------------------------- //
// ----------------------------------------------------------------------------------------------------------- //
AnalyzeSimulations_Stats::AnalyzeSimulations_Stats(int _active_goals, int _active_intentions, int _iteration, double _time) {
    active_goals = _active_goals;
    active_intentions = _active_intentions;
    iteration = _iteration;
    time = _time;
}

int AnalyzeSimulations_Stats::getActiveGoals() {
    return active_goals;
}

int AnalyzeSimulations_Stats::getActiveIntentions() {
    return active_intentions;
}

int AnalyzeSimulations_Stats::getIteration() {
    return iteration;
}

double AnalyzeSimulations_Stats::getTime() {
    return time;
}

// ----------------------------------------------------------------------------------------------------------- //
// ---------------------------------- AnalyzeSimulations_Goals_activation ------------------------------------ //
// ----------------------------------------------------------------------------------------------------------- //
AnalyzeSimulations_Goals_activation::AnalyzeSimulations_Goals_activation(std::string _belief_name,
        double _activation_time, double _completion_time, double _deadline,
        std::string _completion_reason, int _goal_id, int _plan_id, int _original_plan_id, std::string _annidation, int _pref_index)
{
    belief_name = _belief_name;
    activation_time = _activation_time;
    completion_time = _completion_time;
    deadline = _deadline;
    completion_reason = _completion_reason;
    goal_id = _goal_id;
    plan_id = _plan_id;
    original_plan_id = _original_plan_id;
    annidation_level = _annidation;
    pref_index = _pref_index;
}

// ----------------------------------------------------------------------------------------------------------- //
// ----------------------------- AnalyzeSimulations_Intentions_activation ------------------------------------ //
// ----------------------------------------------------------------------------------------------------------- //
AnalyzeSimulations_Intentions_activation::AnalyzeSimulations_Intentions_activation(double _activation_time,
        double _completion_time, std::string _completion_reason, int _goal_id, int _original_plan_id, int _intention_id)
{
    activation_time = _activation_time;
    completion_time = _completion_time;
    completion_reason = _completion_reason;
    goal_id = _goal_id;
    original_plan_id = _original_plan_id;
    intention_id = _intention_id;
}

