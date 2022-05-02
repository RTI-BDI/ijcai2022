/*
 * agent.cc
 *
 *  Created on: Apr 3, 2020
 *      Author: francesco
 */

#include "../headers/agent.h"

agent::agent() { }

agent::~agent() { }

void agent::initialize()
{
    // Initialize locks -------------------------------------------
    lk = std::unique_lock<std::mutex>(my_mutex);
    lock_thread = std::unique_lock<std::mutex>(thread_mutex);
    // -----------------------------------------------------------

    // Debug: used to compute how much time is needed to fully complete the simulation
    begin_sim_time = std::chrono::steady_clock::now();

    /* Default value. It will be updated by the client.
     * If the server-client mechanism is not used, 'run_until' is also not considered. */
    run_until = 0;

    // Set scheduling algorithm (EDF, RR, FCFS)
    int sched_type = (int) par("sched_type");
    int log_level = (int) par("logger_level");

    // sched_type is always needed during the reasoning cycle in order to select the correct phiI function
    selected_sched_type = (SchedType) sched_type;

    // Set the level of logged information (both in console and in omnetpp simulation window)
    logger = std::make_shared<Logger>(log_level);
    ag_metric = std::make_shared<Metric>(log_level);

    /* Start the background thread simulating a TCP server */
    if(read_from_file == false) {
        start_server();
    }
    // ****************************************************************** //

    ag_support = std::make_shared<AgentSupport>(log_level);

    // generate private_ag_settings
    ag_settings = std::make_shared<Ag_settings>();

    operator_entity = std::make_unique<Operator>(); // instantiate the class 'Operator'
    jsonfileHandler = std::make_unique<JsonfileHandler>(logger, ag_metric);

    // get directory parameters
    std::string folder = par("path");
    std::string user = getParentModule()->par("user");

    if(read_from_file == false) { // files are stored in "/{default_user/default_path"
        folder = "";
        user = "/default_simulation";
    }
    glob_path = folder;
    glob_simulation_folder = "../simulations/" + user + folder + "/inputs/";
    glob_user = user;

    // *** BDI *********************************
    std::size_t found = folder.find_last_of("/");
    logger->print(Logger::EssentialInfo, " Simulating: "+ (folder.substr(found + 1)) +"\n Path: "+ (glob_simulation_folder));
    // ************************************

    // initialize server handler (for EDF)
    if(sched_type == EDF)
    {
        //to handle msg server
        ag_Server_Handler = std::make_shared<ServerHandler>();
        ag_Sched = std::make_shared<Ag_Scheduler>((SchedType) sched_type, logger->get_level(), -1);
    }
    else {
        double RR_QUANTUM = par("quantum");
        ag_Sched = std::make_unique<Ag_Scheduler>((SchedType) sched_type, logger->get_level(), RR_QUANTUM);
    }

    // if I'm the DF
    if (strcmp("DF", getName()) == 0) {
        // DO NOTHING (needed only in multi-agent systems)
    }
    else { // if I'm an AGENT
        /* remove metrics.json e metrics_simpletext.json from the simulation folder in order to avoid
         * old data if the current simulation crashes */
        ag_support->initialize_delete_files(folder, glob_user, sched_type);

        if(read_from_file == true) {
            // schedule the self messages used to read the json files and initialize agent data
            initialize_knowledgbase();
        }

        /* schedule self-messages in order to read task.json input files.
         * + if the file is not empty, it start executing the tasks.
         * + set up the scheduler
         * + apply chosen scheduler type (EDF) and execute tasks */
        agentMSG* msg = set_ag_scheduler();
        msg->setSchedulingPriority(0); // prioritize!
        scheduleAt(sim_current_time(), msg);
        scheduled_msg += 1;
        ag_support->setScheduleInFuture("SCHEDULE - setup scheduler"); // METRICS purposes

        // once everything is setup, let the agent start working
        scheduleAt(sim_current_time(), set_reasoning_cycle());
        scheduled_msg += 1;
        ag_support->setScheduleInFuture("AG_REASONING_CYCLE - first reasoning cycle"); // METRICS purposes
    }
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
// *-*-*-*-*-*-*-*-*-*-*-*      CORE FUNCTION that manages the REASONING CYCLE       *-*-*-*-*-*-*-*-*-*-*-*-*-
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
void agent::ag_reasoning_cycle(agentMSG* agMsg)
{
    performing_reasoning_cycle = true; // the agent entered the reasoning process...

    logger->print(Logger::Default, "-----ag_reasoning_cycle-----");
    ag_support->setSimulationLastExecution("ag_reasoning_cycle");
    bool applyReasiningCycleAgain = false;
    bool approachMode = (bool) par("MCTIdentity");
    std::string warning_msg = "";
    goals_forcing_reasoning_cycle.clear();
    no_solution_found = false; // reset value

    if(sim_current_time() > lastComputed_reasoning_cycle)
    {   /* when true, it means time has elapsed and we are dealing with a new reasoning cycle.
     * we have to compute the list of new activated goals to send to the client
     * At time t we may have multiple reasoning cycles (e.g. due to multiple sensors).
     * Take a snapshot of the system as soon as the first cycle is triggered.
     * Then, for every other cycle, update the snapshot with new information. */
        lastComputed_reasoning_cycle = sim_current_time();
        reasoning_cycle_already_active_goals = ag_goal_set;
    }

    // DEBUG *************************************************************************
    if(logger->get_level() >= Logger::EssentialInfo)
    {
        ag_support->printIntentions(ag_intention_set, ag_goal_set);
        ag_Sched->ev_ag_tasks_vector_to_release();
        logger->print(Logger::Default, "-----------------------------");
        ag_Sched->ev_ag_tasks_vector_ready();
        logger->print(Logger::Default, "-----------------------------");
    }
    // *******************************************************************************

    // activateGoalBRF: convert Desires into Goals, if the preconditions of the desires are valid
    if(activateGoalBRF(ag_belief_set, ag_desire_set, ag_goal_set) > 0)
    {
        if(logger->get_level() >= Logger::EveryInfo) {
            ag_support->printGoalset(ag_goal_set);
        }

        if(selected_sched_type == EDF)
        { /* ADDED if statement (31/08/2021):
         * - FCFS goals must be kept sorted according to their activation time and not based on priority (otherwise the order may change and it would not be a FCFS anymore)
         * - RR changing the order of active goals may lead to a wrong alignment of goals and intentions.
         *   Due to the fact that with RR we do not perform the schedulability test, and all valid goals are converted to intention, we can leave the list of goal "unsorted"  */

            // sort goals based on the priority value:
            ag_support->orderGoalsetBasedOnPriority(ag_goal_set);
        }

        if(logger->get_level() >= Logger::EveryInfo) {
            logger->print(Logger::Default, "Ordered Goal based on priority:");
            ag_support->printGoalset(ag_goal_set);
        }

        logger->print(Logger::EssentialInfo, "*********************************");
        ag_support->printGoalset(ag_goal_set);
        logger->print(Logger::EssentialInfo, "*********************************");

        //------------------------------------------------------------------------------------------------//
        // PHASE5: Filter Plans based on Goals -----------------------------------------------------------//
        //------------------------------------------------------------------------------------------------//
        // Array of array containing, for each goal, the set of valid plans, ordered by 'preference'(pref cost value)
        std::vector<std::vector<std::shared_ptr<Plan>>> applicable_plans(ag_goal_set.size());
        unifyGoals(ag_goal_set, ag_plan_set, ag_belief_set, applicable_plans);

        //------------------------------------------------------------------------------------------------//
        // PHASE6: Filter Plans based on preconditions ---------------------------------------------------//
        //------------------------------------------------------------------------------------------------//
        unifyPreconditions(ag_goal_set, applicable_plans);

        // Sort applicable_plans based on their 'pref' value
        ag_support->orderPlansBasedOnPref(applicable_plans);
        if(logger->get_level() >= Logger::EveryInfo) {
            logger->print(Logger::Default, "\nApplicable plans:");
            ag_support->printApplicablePlans(applicable_plans, ag_goal_set);
        }

        //------------------------------------------------------------------------------------------------//
        // PHASE7: Filter plans based on deadlines -------------------------------------------------------//
        //------------------------------------------------------------------------------------------------//
        std::vector<std::vector<std::shared_ptr<MaximumCompletionTime>>> feasible_plans(applicable_plans.size());
        logger->print(Logger::EssentialInfo, "# of goals:["+ std::to_string(ag_goal_set.size()) +"]");
        if(approachMode) {
            logger->print(Logger::EssentialInfo, " Selected Mode for computing DDL is: 'OPTIMISTIC'");
        }
        else {
            logger->print(Logger::EssentialInfo, " Selected Mode for computing DDL is: 'PESSIMISTIC'");
        }

        bool loop_detected;
        bool skip_intentionset = false;
        int debug_plan_id = -1, index_intention = -1;
        double first_batch_delta_time;
        std::shared_ptr<MaximumCompletionTime> tmp_mct;
        std::vector<std::shared_ptr<Plan>> contained_plans; // used to detect loops among plans
        int log_level = logger->get_level(); // Debug
        applyReasiningCycleAgain = false;

        // Only used in FCFS -----------------
        check_intentions_fcfs = true;
        intentions_fcfs.clear();
        // -----------------------------------

        for(int g = 0; g < (int)ag_goal_set.size(); g++)
        { // loop over every goal...
            if(selected_sched_type == FCFS && g > 0)
            { /* The goalset is kept sorted according to the order in which desires
             * and internal goals have been activated (they are not sorted according to priority
             * (see 'if(selected_sched_type == EDF)' after 'if(activateGoalBRF(ag_belief_set, ag_desire_set, ag_goal_set) > 0)')).
             * "check_intentions_fcfs" is used to force function "computeGoalDDL" to only consider
             * the intentionset when the first action of the first intention is a goal (and for the internal goals that derives from that).
             * Every other active goal (from g = 1...N) will use the selectedPlans to compute their ddl */
                check_intentions_fcfs = false;

                for(int k = 0; k < (int)intentions_fcfs.size(); k++) {
                    if(intentions_fcfs[k]->get_goal_name() == ag_goal_set[g]->get_goal_name()) {
                        check_intentions_fcfs = true;
                        break;
                    }
                }
            }

            for(int p = 0; p < (int)applicable_plans[g].size(); p++)
            { // for each goal, compute MCT of every applicable_plan...
                loop_detected = false;
                contained_plans.clear();
                debug_plan_id = applicable_plans[g][p]->get_id();
                logger->print(Logger::EssentialInfo, "==============================================\n > Compute DDL of plan:["+ std::to_string(debug_plan_id)
                +"], goal name:["+ applicable_plans[g][p]->get_goal_name() +"]\n==============================================");

                // ---------------------------------------------------------------------------------------------------------
                // /* xx */logger->set_level(Logger::EveryInfoPlus);
                // /* xx */logger->set_level(Logger::PrintNothing);

                first_batch_delta_time = sim_current_time().dbl();
                if(ag_support->checkIfPlanWasAlreadyIntention(applicable_plans[g][p], index_intention, ag_intention_set)) {
                    first_batch_delta_time = ag_intention_set[index_intention]->get_batch_startTime();
                }
                logger->print(Logger::EveryInfo, "first_batch_delta_time: "+ std::to_string(first_batch_delta_time));
                tmp_mct = computeGoalDDL(applicable_plans[g][p], contained_plans, loop_detected, first_batch_delta_time, 0, false, skip_intentionset);

                logger->set_level(log_level); // Debug
                // ---------------------------------------------------------------------------------------------------------
                if(tmp_mct == nullptr) {
                    warning_msg = "[WARNING] Plan:["+ std::to_string(applicable_plans[g][p]->get_id());
                    warning_msg += "], goal:["+ applicable_plans[g][p]->get_goal_name();
                    warning_msg += "] is NOT valid, no plans for the internal goals!";
                    logger->print(Logger::Warning, warning_msg);
                    ag_metric->addGeneratedWarning(sim_current_time(), "ag_reasoning_cycle", warning_msg);
                }
                else {
                    logger->print(Logger::EssentialInfo, " -- Plan:["+ std::to_string(applicable_plans[g][p]->get_id())
                    +"], goal:["+ applicable_plans[g][p]->get_goal_name()
                    +"] has deadline: MCT: "+ std::to_string(tmp_mct->getMCTValue())
                    +" taskset.size: "+ std::to_string(tmp_mct->getTaskSet().size())
                    +" (plan.body.size: "+ std::to_string(applicable_plans[g][p]->get_body().size()) +")");

                    feasible_plans[g].push_back(tmp_mct);
                }
            }

            bool atLeastOnePlanValid = false;

            if(feasible_plans[g].size() > 0)
            {
                // once we have the list of feasible plans for goal 'g', we have to check if they respect the deadline of the linked goal
                logger->print(Logger::EssentialInfo, " Checking feasible plan's deadline: ");
                for(int i = 0; i < (int)feasible_plans[g].size(); i++)
                {
                    if(ag_goal_set[g]->get_DDL() == -1) {
                        logger->print(Logger::EssentialInfo, " --> Is plan:["+ std::to_string(feasible_plans[g][i]->getInternalGoalsSelectedPlans()[0].first->get_id())
                        +"] valid? Goal.DDL is -1, plan["+ std::to_string(g) +"]["+ std::to_string(i)
                        +"].DDL: "+ std::to_string(feasible_plans[g][i]->getMCTValue()) +" -> TRUE");
                        atLeastOnePlanValid = true;
                    }
                    else {
                        if(feasible_plans[g][i]->getMCTValue() == -1) {
                            logger->print(Logger::EssentialInfo, " --> Is plan:["+ std::to_string(feasible_plans[g][i]->getInternalGoalsSelectedPlans()[0].first->get_id())
                            +"] valid? Goal.DDL: "+ std::to_string(ag_goal_set[g]->get_DDL()) +", plan["+ std::to_string(g) +"]["+ std::to_string(i) +"].DDL: -1 -> FALSE!!!");

                            feasible_plans[g].erase(feasible_plans[g].begin() + i);
                            i -= 1;
                        }
                        else {
                            if((feasible_plans[g][i]->getMCTValue()) <= (ag_goal_set[g]->get_DDL() + ag_goal_set[g]->get_activation_time())) {
                                logger->print(Logger::EssentialInfo, " INTERVAL: --> Is plan:["+ std::to_string(feasible_plans[g][i]->getInternalGoalsSelectedPlans()[0].first->get_id())
                                +"], name:["+ feasible_plans[g][i]->getInternalGoalsSelectedPlans()[0].first->get_goal_name()
                                +"] valid: (("+ std::to_string(feasible_plans[g][i]->getMCTValue())
                                +") <= ("+ std::to_string(ag_goal_set[g]->get_DDL())
                                +" + "+ std::to_string(ag_goal_set[g]->get_activation_time())
                                +")) --> ("+ std::to_string(feasible_plans[g][i]->getMCTValue())
                                +" <= "+ std::to_string(ag_goal_set[g]->get_DDL() + ag_goal_set[g]->get_activation_time())
                                +"): TRUE");
                                atLeastOnePlanValid = true;
                            }
                            else { // DDL not repected, remove the plan...
                                logger->print(Logger::EssentialInfo, " INTERVAL: --> Is plan:["+ std::to_string(feasible_plans[g][i]->getInternalGoalsSelectedPlans()[0].first->get_id())
                                +"], name:["+ feasible_plans[g][i]->getInternalGoalsSelectedPlans()[0].first->get_goal_name()
                                +"] valid: (("+ std::to_string(feasible_plans[g][i]->getMCTValue())
                                +") <= ("+ std::to_string(ag_goal_set[g]->get_DDL())
                                +" + "+ std::to_string(ag_goal_set[g]->get_activation_time())
                                +")) --> ("+ std::to_string(feasible_plans[g][i]->getMCTValue())
                                +" <= "+ std::to_string(ag_goal_set[g]->get_DDL() + ag_goal_set[g]->get_activation_time())
                                +"): FALSE!!!");

                                feasible_plans[g].erase(feasible_plans[g].begin() + i);
                                i -= 1;
                            }
                        }
                    }
                }
            }

            if(atLeastOnePlanValid == false)
            { // there are no valid plans for the goal 'g'

                std::shared_ptr<Goal> goal = std::make_shared<Goal>(*ag_goal_set[g]);
                warning_msg = "[WARNING] There are no feasible plans for Goal:["+ std::to_string(ag_goal_set[g]->get_id());
                warning_msg += "], name:["+ ag_goal_set[g]->get_goal_name() +"] -> removed.";
                logger->print(Logger::Warning, warning_msg);
                ag_metric->addGeneratedWarning(sim_current_time(), "ag_reasoning_cycle", warning_msg);

                int tmp_index_intention = -1, planid = -1;
                if(ag_support->checkIfGoalIsLinkedToIntention(goal, ag_intention_set, tmp_index_intention)) {
                    logger->print(Logger::Default, "Goal:["+ std::to_string(goal->get_id())
                    +"] linked to intention:["+ std::to_string(ag_intention_set[tmp_index_intention]->get_id())
                    +"] debug:[index = "+ std::to_string(tmp_index_intention) +"]");

                    planid = ag_intention_set[tmp_index_intention]->get_original_planID();
                }

                feasible_plans.erase(feasible_plans.begin() + g);
                applicable_plans.erase(applicable_plans.begin() + g);

                // METRICS: -----------------------------------------
                if(tmp_index_intention >= 0 && tmp_index_intention < (int)ag_intention_set.size()) {
                    goals_forcing_reasoning_cycle.push_back(std::make_tuple(ag_goal_set[g], debug_plan_id, ag_intention_set[tmp_index_intention]->get_id()));
                }
                else {
                    goals_forcing_reasoning_cycle.push_back(std::make_tuple(ag_goal_set[g], debug_plan_id, -1));
                }
                // --------------------------------------------------

                ag_goal_set.erase(ag_goal_set.begin() + g);
                g -= 1;

                /* force reasoning cycle only if the goal was already active.
                 * Otherwise is just a new goal that has no valid plans in the whole library (based on current status) */
                if(tmp_index_intention != -1)
                {
                    cancel_next_reasoning_cycle_msg(msg_name_reasoning_cycle, sim_current_time());  // cancel previously activated reasoning cycles
                    cancel_next_redundant_msg(msg_name_select_next_intentions_task);    // cancel phiI

                    /* Whenever a goal does not respect its deadline, it must be re-initialized, forcing a new reasoning cycle.
                     * If there is a valid plan, or if the old one can be used to achieve the goal, a NEW intention gets instantiated. */
                    if((int)ag_intention_set.size() > tmp_index_intention && tmp_index_intention >= 0)
                    {
                        int removedIntentionPlanId = ag_intention_set[tmp_index_intention]->getTopInternalSelectedPlan()->get_id();

                        reasoning_cycle_stopped_goals.push_back(ag_intention_set[tmp_index_intention]->get_goal_name()); // data for the client

                        removeInternalgoalTasksFromSchedulerVectors(removedIntentionPlanId);

                        cancel_message_internal_goal_arrival(msg_name_internal_goal_arrival, ag_intention_set[tmp_index_intention]->get_planID(), ag_intention_set[tmp_index_intention]->get_original_planID());
                        cancel_message_intention_completion(msg_name_intention_completion,
                                ag_intention_set[tmp_index_intention]->get_goal_name(),
                                ag_intention_set[tmp_index_intention]->get_waiting_for_completion(),
                                ag_intention_set[tmp_index_intention]->get_goalID(),
                                ag_intention_set[tmp_index_intention]->get_planID(),
                                ag_intention_set[tmp_index_intention]->get_original_planID());

                        ag_metric->setActivatedIntentionCompletion(ag_intention_set[tmp_index_intention], sim_current_time().dbl(), "no feasible plans for goal - FORCE REASONING CYCLE");
                        ag_intention_set.erase(ag_intention_set.begin() + tmp_index_intention);

                        // Reset the body of subgoals that refers to this goal and update intention's startTime if needed...
                        ag_support->manageForceReasoningCycle(ag_intention_set, ag_plan_set, removedIntentionPlanId);
                    }

                    // For each active intention set the goal that is FORCING the new REASONING cycle to: isBeenActivated = false and forceReasoningCycle = true
                    ag_support->checkIfGoalWasActivatedInOtherIntentions(goal, ag_intention_set);

                    // if removed goal was linked to active intentions with first batch already in release/ready,
                    // and batch contains an activated internal goals -> update internal goals planID
                    checkIfIntentionHadActiveInternalGoals(planid, ag_intention_set);

                    scheduleAt(sim_current_time(), set_reasoning_cycle(true));
                    scheduled_msg += 1;
                    ag_support->setScheduleInFuture("AG_REASONING_CYCLE - force reasoning cycle"); // METRICS purposes

                    logger->print(Logger::EssentialInfo, "->-> FORCE A NEW REASONING CYCLE... ");
                    ag_metric->setActivatedGoalCompletion(goal, sim_current_time().dbl(), "FORCE REASONING CYCLE");
                    applyReasiningCycleAgain = true;
                }
            }
        }
        //------------------------------------------------------------------------------------------------//

        logger->print(Logger::EssentialInfo, "*********************************");
        ag_support->printGoalset(ag_goal_set);
        logger->print(Logger::EssentialInfo, "*********************************");

        if(applyReasiningCycleAgain == false)
        {
            // ------------------------------------------------------------------------------------------
            // Sort feasible plans based on chosen approach (PESS or OPT)
            ag_support->orderFeasiblePlansBasedOnDDL(feasible_plans, approachMode, ag_intention_set, ag_Sched);
            if(logger->get_level() >= Logger::EssentialInfo) {
                ag_support->printFeasiblePlans(feasible_plans);
            }
            // ------------------------------------------------------------------------------------------

            if(selected_sched_type == EDF)
            {
                //-------------------------------------------------------------------------------------------------------------//
                // PHASE8: Apply PhiF to feasible plans and get the best plan for each goal, s.t. can be scheduled together. --//
                //-------------------------------------------------------------------------------------------------------------//
                /* In phiF it may happen that the least "pr" goal is discarded due to a not feasible combination of plan has been found.
                 * If all goals are removed, a valid combination is not present.
                 * At this point, restore the original set of active goals and feasible plans
                 * and discard the plans of the goal with the highest "pr" value.
                 * Then, check if there is a valid combination of schedulable plans. */
                std::vector<std::vector<std::shared_ptr<MaximumCompletionTime>>> feasible_plans_copy = feasible_plans;
                std::vector<std::shared_ptr<Goal>> ag_goal_set_copy = ag_goal_set;
                std::vector<std::shared_ptr<Intention>> ag_intention_set_copy = ag_intention_set;
                std::vector<std::shared_ptr<MaximumCompletionTime>> ag_selected_plans_copy = ag_selected_plans;
                std::pair<std::vector<std::shared_ptr<MaximumCompletionTime>>, std::vector<int>> metrics;
                bool checkPlansCombination = true;

                while(checkPlansCombination)
                {
                    checkPlansCombination = false;
                    metrics = phiF(ag_goal_set_copy, feasible_plans_copy);
                    ag_selected_plans = metrics.first;
                    logger->print(Logger::EssentialInfo, "\n ag_selected_plans: "+ std::to_string(ag_selected_plans.size()));

                    if(ag_selected_plans.size() == 0)
                    {
                        logger->print(Logger::EssentialInfo, " --- Drop goal with highest priority:["+ ag_goal_set[0]->get_goal_name() +"]!");

                        // ag_goal_set and feasible_plans must always be aligned...
                        if(ag_goal_set.size() > 0)
                        { // drop goal with highest priority and check if a new feasible combination of plans exists...
                            ag_metric->setActivatedGoalCompletion(ag_goal_set[0], sim_current_time().dbl(), "Drop goal with highest priority!");
                            ag_goal_set.erase(ag_goal_set.begin());
                        }
                        if(feasible_plans.size() > 0) {
                            feasible_plans.erase(feasible_plans.begin());
                        }
                        if(ag_goal_set.size() > 0 && feasible_plans.size() > 0) {
                            logger->print(Logger::EssentialInfo, "Check new combination of plans...");
                            ag_goal_set_copy = ag_goal_set;
                            feasible_plans_copy = feasible_plans;
                            checkPlansCombination = true;
                        }

                        /* dropping the highest goal means that we must have removed every other active goal, plan and also intention
                         * thus, we have to restore the state of intention as it was before phiF */
                        if(ag_intention_set.size() == 0)
                        {
                            ag_intention_set = ag_intention_set_copy;
                        }
                        else {
                            warning_msg = "[WARNING] All goals have been removed from execution, BUT there are still active intentions:[tot="+ std::to_string(ag_intention_set.size()) +"] (must be 0)";
                            logger->print(Logger::Warning, warning_msg);
                            ag_metric->addGeneratedWarning(sim_current_time(), "ag_reasoning_cycle", warning_msg);
                        }
                    }
                    else
                    {
                        logger->print(Logger::EveryInfo, "checking intention size: "+ std::to_string(ag_intention_set.size()) +" < "+ std::to_string(ag_intention_set_copy.size()));
                        if(ag_intention_set.size() < ag_intention_set_copy.size())
                        {
                            /* check if there were waiting for completions intentions among the ones removed by phiF
                             * if there were, restore them (because they do not occupy U) */
                            ag_support->checkIfRemovedIntentionsWaitingCompletion(ag_goal_set, ag_goal_set_copy, ag_selected_plans, ag_selected_plans_copy, ag_intention_set, ag_intention_set_copy);
                        }

                        // update the original set with the new information (best combination of feasible plans)
                        ag_metric->setActivatedGoalsSelectedPlan(ag_goal_set_copy, ag_selected_plans, metrics.second, feasible_plans);

                        /* Restore "ag_intention_set" to the value it had before starting executing phiF.
                         * If some intention has been removed during phiF, restore it. It will be correctly managed and removed from "removeInvalidIntentions" */
                        ag_intention_set = ag_intention_set_copy;
                        ag_goal_set = ag_goal_set_copy;
                    }
                }
            }
            else if(selected_sched_type == FCFS || selected_sched_type == RR) {
                ag_selected_plans.clear();
                for(int p = 0; p < (int)feasible_plans.size(); p++) {
                    ag_selected_plans.push_back(feasible_plans[p][0]);
                }
                std::vector<int> plans_pref_index(ag_selected_plans.size(), 0);
                ag_metric->setActivatedGoalsSelectedPlan(ag_goal_set, ag_selected_plans, plans_pref_index, feasible_plans);
            }

            //-------------------------------------------------------------------------------------------------------------//
            // PHASE9: Remove from Intentions every plan that is not the best choice for any active goals, or that it's not linked to any goal
            //-------------------------------------------------------------------------------------------------------------//
            ag_support->updateSelectedPlans(ag_selected_plans);

            // update the subplans of the already active intentions...
            ag_support->update_internal_goal_selectedPlans(ag_selected_plans, ag_intention_set);

            removeInvalidIntentions(ag_goal_set, ag_selected_plans);

            if(ag_selected_plans.size() > 0)
            {
                //--------------------------------------------------------------------------------------------//
                // PHASE10: Given the set of ag_selected_plans, update the list of intentions ----------------//
                //--------------------------------------------------------------------------------------------//
                bool new_reasoning_cycle_needed = false;
                if(updateIntentionset(ag_selected_plans, ag_belief_set, ag_intention_set)) {
                    new_reasoning_cycle_needed = true;
                }

                updateIntentionsContent(ag_intention_set);

                if(logger->get_level() > Logger::EssentialInfo)
                { // PRINT data if needed....
                    logger->print(Logger::EssentialInfo, "Active Intentions:");
                    for(std::shared_ptr<Intention> intention : ag_intention_set) {
                        logger->print(Logger::EssentialInfo, " - Intention:["+ std::to_string(intention->get_id())
                        +"], plan:["+ std::to_string(intention->get_planID())
                        +"], original_plan:["+ std::to_string(intention->get_original_planID())
                        +"] has "+ std::to_string(intention->get_Actions().size()) +" action(s) left");
                    }
                }

                if(new_reasoning_cycle_needed) {
                    //-------------------------------------------------------------------------------------------------------//
                    // PHASE11.a: Immediately repeat a new reasoning cycle to deal with updated beliefset (due to effects) --//
                    //-------------------------------------------------------------------------------------------------------//
                    scheduleAt(sim_current_time(), set_reasoning_cycle(true));
                    scheduled_msg += 1;
                    ag_support->setScheduleInFuture("AG_REASONING_CYCLE - repeat reasoning cycle due to 'effects_begin'"); // METRICS purposes

                    logger->print(Logger::EssentialInfo, " AG_REASONING_CYCLE ->-> BELIEFSET CHANGED DUE TO EFFECTS -> SCHEDULE NEW REASONING CYCLE... <-<-");
                }
                else {
                    //------------------------------------------------------------------------------------------//
                    // PHASE11.b: Given the set of Intentions, select for each one, which action execute next --//
                    //------------------------------------------------------------------------------------------//
                    logger->print(Logger::EssentialInfo, "\nStart executing actions....");
                    cancel_next_redundant_msg(msg_name_select_next_intentions_task); // cancel phiI // ADDED: 17/12/2020: se attivo N sensori allo stesso tempo t + vengono attivati 2...N goal, questo impedisce di ripetere phiI 2..N volta. Verrà eseguito solo 1.
                    scheduleAt(sim_current_time(), set_select_next_intentions_task()); // schedule phiI
                    scheduled_msg += 1;
                    ag_support->setScheduleInFuture("AG_TASK_SELECTION_PHII - start reasoning cycle execution"); // METRICS purposes
                }
            }
            else {
                warning_msg = "[WARNING] There are no Plans that satisfy the active goal(s).";
                logger->print(Logger::Warning, warning_msg);
                ag_metric->addGeneratedWarning(sim_current_time(), "ag_reasoning_cycle", warning_msg);
            }
        }
        else {
            // DEBUG: -------------------------------------------------------------------
            std::string force_due_to = "";
            for(int k = 0; k < (int)goals_forcing_reasoning_cycle.size(); k++) {
                force_due_to += "\n  -- goal:[id="+ std::to_string(std::get<0>(goals_forcing_reasoning_cycle[k])->get_id());
                force_due_to += "], name:["+ std::get<0>(goals_forcing_reasoning_cycle[k])->get_goal_name();
                force_due_to += "], plan_id:["+ std::to_string(std::get<1>(goals_forcing_reasoning_cycle[k]));
                force_due_to += "], intention_id:["+ std::to_string(std::get<2>(goals_forcing_reasoning_cycle[k])) +"]";
            }
            // --------------------------------------------------------------------------

            warning_msg = "[WARNING] FORCE NEW REASONING CYCLE due to: "+ force_due_to;
            logger->print(Logger::Warning, warning_msg);
            ag_metric->addGeneratedWarning(sim_current_time(), "ag_reasoning_cycle", warning_msg);
        }
    }
    else {
        warning_msg = "[WARNING] Stop agent reasoning cycle, there are no goals to check!";
        logger->print(Logger::Warning, warning_msg);
        ag_metric->addGeneratedWarning(sim_current_time(), "ag_reasoning_cycle", warning_msg);

        cancel_next_redundant_msg(msg_name_next_task_arrival);   // cancel Scheduler message
        cancel_next_redundant_msg(msg_name_select_next_intentions_task); // cancel PhiI message
    }

    // DEBUG *************************************************
    ag_support->printGoalset(ag_goal_set);
    ag_support->printIntentions(ag_intention_set, ag_goal_set);

    ag_Sched->ev_ag_tasks_vector_to_release();
    logger->print(Logger::Default, "---------------------------");
    ag_Sched->ev_ag_tasks_vector_ready();
    logger->print(Logger::Default, "---------------------------");
    // ******************************************************

    // update statistics....
    ag_metric->addReasoningCycleStats(ag_goal_set.size(), ag_intention_set.size(), sim_current_time().dbl());

    // If the reasoning cycle did not active any goal, send COMMAND: NO_SOLUTION to the client... -------
    if(ag_goal_set.size() == 0 && checkIfAllMessagesAreInFuture(sim_current_time().dbl()))
    {   // v2: no_solution_found is used in function handleMessage(...)
        no_solution_found = true;
    }
    // --------------------------------------------------------------------------------------------------

    performing_reasoning_cycle = false; // the agent has completed the reasoning process...
    send_new_solution_msg = true;

    logger->print(Logger::EssentialInfo, "END of function 'ag_reasoning_cycle'");
}

void agent::ag_internal_goal_activation(agentMSG* msg)
{
    logger->print(Logger::EssentialInfo, " ----ag_internal_goal_activation---- ");

    std::vector<std::shared_ptr<Task>> empty_release_queue(0); // declared only because required by function "checkAndManageIntentionFalseConditions"
    std::vector<std::shared_ptr<Action>> intention_stack;
    std::shared_ptr<Action> top; // intention's stack top action
    std::shared_ptr<Goal> goal;
    std::shared_ptr<Task> task;
    std::shared_ptr<MaximumCompletionTime> MCT_main_p;
    std::vector<std::pair<std::shared_ptr<Plan>, int>> internal_goals_plans;
    std::vector<std::shared_ptr<Effect>> effects_at_begin, effects_at_end;
    std::shared_ptr<MaximumCompletionTime> MCTplan;
    std::shared_ptr<Plan> ig_plan;
    std::shared_ptr<Plan> plan_copy; // make a copy of the selected plan for this goal. Note: the action in the body are shared_ptr that are pass as reference in the new plan!
    std::vector<std::shared_ptr<Action>> body_actions;
    std::string warning_msg = "";

    json post_conditions;

    int index_intention = -1;
    int tmp_cnt = -1;
    int next_intention_original_plan_id;
    int tmp_counter = 0;
    int ig_plan_annidation = 0;
    bool checkNextAction = true;
    bool updated_beliefset = false;
    bool updated_intentionset = false;
    bool new_intention_created = false;
    bool get_next_internal_goal = false;
    bool internal_goal_plan_valid_conditions = false;
    bool invalid_sub_sub_goal = false;

    int log_level = logger->get_level(); // Debug
    // /* xx */logger->set_level(Logger::EveryInfo);

    logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
            +": fabs("+ sim_current_time().str()
            +" - ("+ std::to_string(msg->getAg_internal_goal_request_time())
    +" + "+ std::to_string(msg->getAg_internal_goal_arrival_time())
    +")) < "+ std::to_string(ag_Sched->EPSILON)
    +"? -> "+ std::to_string(sim_current_time().dbl() - (msg->getAg_internal_goal_request_time() + msg->getAg_internal_goal_arrival_time()))
    +" < "+ std::to_string(ag_Sched->EPSILON)
    +"? "+ ag_support->boolToString(fabs(sim_current_time().dbl() - (msg->getAg_internal_goal_request_time() + msg->getAg_internal_goal_arrival_time())) < ag_Sched->EPSILON));

    // check if the message arrived at the expected moment...
    if(fabs(sim_current_time().dbl() - (msg->getAg_internal_goal_request_time() + msg->getAg_internal_goal_arrival_time())) < ag_Sched->EPSILON)
    {
        logger->print(Logger::EveryInfo, " Get intention with original plan id:["+ std::to_string(msg->getAg_internal_goal_original_plan_id()) +"]");

        // check if the intention containing the internal goal is still active
        for(int i = 0; i < (int)ag_intention_set.size(); i++)
        {
            if(ag_intention_set[i]->get_original_planID() == msg->getAg_internal_goal_original_plan_id())
            {
                logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                        +", ag_internal_goal_activation: internal goal id:["+ std::to_string(msg->getAg_internal_goal_id())
                +"], goal name:["+ msg->getAg_internal_goal_name()
                +"], plan_id:["+ std::to_string(msg->getAg_internal_goal_plan_id())
                +"], original_plan_id:["+ std::to_string(msg->getAg_internal_goal_original_plan_id())
                +"] is linked to intention:["+ std::to_string(i)
                +"], intention_id:["+ std::to_string(ag_intention_set[i]->get_id()) +"]");

                index_intention = i;
                break;
            }
        }

        if(index_intention == -1)
        {
            logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                    +", ag_internal_goal_activation: internal goal id:["+ std::to_string(msg->getAg_internal_goal_id())
            +"], goal name:["+ msg->getAg_internal_goal_name()
            +"], plan_id:["+ std::to_string(msg->getAg_internal_goal_plan_id())
            +"], original_plan_id:["+ std::to_string(msg->getAg_internal_goal_original_plan_id())
            +"] is NOT linked to any intention! -> ignore it!");
        }
        else
        { // we found the intention that contains the specified internal goal.
            intention_stack = ag_intention_set[index_intention]->get_Actions();

            // check if the goal is in the first batch of the intention...
            for(int i = 0; i < (int)intention_stack.size() && checkNextAction; i++)
            {
                top = intention_stack[i];
                if(top->get_type() == "task")
                {
                    task = std::dynamic_pointer_cast<Task>(top);
                    if(task->getTaskisBeenActivated() == false)
                    { //
                        logger->print(Logger::EssentialInfo, " + Intention:[original_plan_id="+ std::to_string(ag_intention_set[index_intention]->get_original_planID())
                        +"], batch 1 contains a task:["+ std::to_string(task->getTaskId())
                        +"] that has not been activated! -> Stop!");
                        checkNextAction = false;
                    }
                    else if(task->getTaskExecutionType() == Task::EXEC_SEQUENTIAL && i != 0)
                    { // we just reached the second batch of the intention
                        logger->print(Logger::EssentialInfo, " + Intention:[original_plan_id="+ std::to_string(ag_intention_set[index_intention]->get_original_planID())
                        +"], batch 1 does not contain the specified goal:[id="+ std::to_string(msg->getAg_internal_goal_id())
                        +"], name:["+ msg->getAg_internal_goal_name() +"]");
                        checkNextAction = false;
                    }
                }
                else if(top->get_type() == "goal")
                {
                    goal = std::dynamic_pointer_cast<Goal>(top);
                    logger->print(Logger::EssentialInfo, "Internal goal:["+ goal->get_goal_name() +"], id: "+ std::to_string(goal->get_id())
                    +", plan: "+ std::to_string(goal->get_plan_id())
                    +", original_plan: "+ std::to_string(goal->get_original_plan_id())
                    +", execution_type: "+ goal->getGoalExecutionType_as_string());

                    if(goal->getGoalExecutionType() == Goal::EXEC_SEQUENTIAL && i != 0)
                    {  // we just reached the second batch of the intention
                        logger->print(Logger::EssentialInfo, " + Intention:[original_plan_id="+ std::to_string(ag_intention_set[index_intention]->get_original_planID())
                        +"], batch 1 does not contain the specified goal:[id="+ std::to_string(msg->getAg_internal_goal_id())
                        +"], name:["+ msg->getAg_internal_goal_name() +"]");
                        checkNextAction = false;
                    }
                    else {
                        logger->print(Logger::EveryInfo, "Action:["+ std::to_string(i) +" of "+ std::to_string(intention_stack.size())
                        +"] Goal id: ("+ std::to_string(goal->get_id()) +" == "+ std::to_string(msg->getAg_internal_goal_id())
                        +") && name: ("+ goal->get_goal_name() +" == "+ msg->getAg_internal_goal_name()
                        +")\n && arrival_time: ("+ std::to_string(goal->get_arrival_time()) +" == "+ std::to_string(msg->getAg_internal_goal_arrival_time())
                        +") && isBeenActivated: "+ ag_support->boolToString(goal->getGoalisBeenActivated()) +" == false)"
                        +" ==> "+ ag_support->boolToString(goal->get_id() == msg->getAg_internal_goal_id()
                                && goal->get_goal_name() == msg->getAg_internal_goal_name()
                                && goal->get_arrival_time() == msg->getAg_internal_goal_arrival_time()
                                && goal->getGoalisBeenActivated() == false)
                                +"\n-------------------------");

                        if(goal->get_id() == msg->getAg_internal_goal_id()
                                && goal->get_goal_name() == msg->getAg_internal_goal_name()
                                && fabs(goal->get_arrival_time() - msg->getAg_internal_goal_arrival_time()) < ag_Sched->EPSILON
                                && goal->getGoalisBeenActivated() == false)
                        { // we found the goal that triggered this function...

                            logger->print(Logger::EssentialInfo, "Action:["+ std::to_string(i) +" of "+ std::to_string(intention_stack.size())
                            +"] Goal id: ("+ std::to_string(goal->get_id()) +" == "+ std::to_string(msg->getAg_internal_goal_id())
                            +") && name: ("+ goal->get_goal_name() +" == "+ msg->getAg_internal_goal_name()
                            +")\n && arrival_time: ("+ std::to_string(goal->get_arrival_time()) +" == "+ std::to_string(msg->getAg_internal_goal_arrival_time())
                            +") && isBeenActivated: "+ ag_support->boolToString(goal->getGoalisBeenActivated()) +" == false)"
                            +" ==> "+ ag_support->boolToString(goal->get_id() == msg->getAg_internal_goal_id()
                                    && goal->get_goal_name() == msg->getAg_internal_goal_name()
                                    && goal->get_arrival_time() == msg->getAg_internal_goal_arrival_time()
                                    && goal->getGoalisBeenActivated() == false)
                                    +"\n-------------------------");

                            /* set the goal to activated. If the goal is already in the goalset, set it anyway
                             * this happens when we have the same ig within the same batch, same intention
                             * + the arrivaltime of the second ig make the goal activate before the first one completes
                             * e.g., move_right_c1 = 0 SEQ, move_right_c1 = 2 PAR and the task contained in move_right_c1 has compTime > 2 */
                            goal->setGoalisBeenActivated(true);

                            // !!! code taken from phI -------------------------------------------
                            // 1. find the plan that satisfy the goal and that has the highest PREF value
                            MCT_main_p = ag_intention_set[index_intention]->getOriginalMainPlan();
                            internal_goals_plans = MCT_main_p->getInternalGoalsSelectedPlans();
                            get_next_internal_goal = false;

                            for(int p = 1; p < (int)internal_goals_plans.size() && get_next_internal_goal == false; p++)
                            { // loop over all internal goals of the main plan
                                if(ag_support->checkIfPlanWasAlreadyIntention(internal_goals_plans[p].first, ag_intention_set) == false)
                                {
                                    if(goal->get_goal_name() == internal_goals_plans[p].first->get_goal_name())
                                    {
                                        if(internal_goals_plans[p].second == 1)
                                        { // plans with nesting level = 1 are the actual internal goals of the main plan...
                                            if(ag_support->checkIfGoalIsInGoalset(goal, ag_goal_set) == false)
                                            {
                                                goal->setActivation_time(sim_current_time().dbl());
                                                goal->setGoalisBeenActivated(true);
                                                effects_at_begin = internal_goals_plans[p].first->get_effects_at_begin();
                                                effects_at_end = internal_goals_plans[p].first->get_effects_at_end();
                                                post_conditions = internal_goals_plans[p].first->get_post_conditions();

                                                MCTplan = std::make_shared<MaximumCompletionTime>();
                                                tmp_counter = 0;
                                                ig_plan_annidation = 0;
                                                internal_goal_plan_valid_conditions = false;

                                                MCTplan->setMCTValue(goal->get_DDL());

                                                for(int x = p; x < (int)internal_goals_plans.size(); x++)
                                                { // stay in here as long as the next .second is greater than 1. [*]
                                                    ig_plan = internal_goals_plans[x].first;
                                                    ig_plan_annidation = internal_goals_plans[x].second;
                                                    plan_copy = ig_plan->makeACopyOfPlan(); // make a copy of the selected plan for this goal

                                                    if(x == p)
                                                    {
                                                        logger->print(Logger::EssentialInfo, "New internal goal: chosen plan:["+ std::to_string(plan_copy->get_id()) +"]");
                                                        ag_intention_set[index_intention]->push_internal_goal(goal, plan_copy->get_id(), plan_copy->get_body().size());
                                                        next_intention_original_plan_id = plan_copy->get_id();
                                                    }

                                                    if(ag_support->checkIfPreconditionsAreValid(plan_copy, ag_belief_set, "ag_internal_goal_activation"))
                                                    {
                                                        internal_goal_plan_valid_conditions = true;
                                                        body_actions = plan_copy->get_body();
                                                        logger->print(Logger::EveryInfo, "plan:["+ std::to_string(plan_copy->get_id()) +"]");

                                                        // update the tasks in of the selected plan 'ig_plan' and add them to 'MCTplan'...
                                                        for(int k = 0; k < (int)body_actions.size(); k++)
                                                        {
                                                            logger->print(Logger::EveryInfoPlus, " -- Action["+ std::to_string(k)
                                                            +"] is of type: "+ body_actions[k]->get_type());

                                                            // if it's a task, we have to update it's t_intention_id
                                                            if(body_actions[k]->get_type() == "task")
                                                            {
                                                                task = std::dynamic_pointer_cast<Task>(body_actions[k]);
                                                                task->setTaskPlanId(ag_intention_set[index_intention]->get_planID());
                                                                task->setTaskOriginalPlanId(plan_copy->get_id());

                                                                logger->print(Logger::EveryInfo, "task original plan id: "+ std::to_string(plan_copy->get_id())
                                                                +", set to: "+ std::to_string(task->getTaskOriginalPlanId()));

                                                                task->setTaskisBeenActivated(false);
                                                            }
                                                        }
                                                        MCTplan->add_internal_goal_selectedPlan(plan_copy, internal_goals_plans[x].second - 1);
                                                    }
                                                    else {
                                                        internal_goal_plan_valid_conditions = false;
                                                        break;
                                                    }
                                                    tmp_counter += 1;

                                                    // if the next plan has .second = 1, it means that it's an internal goal of the main plan
                                                    if(x + 1 < (int)internal_goals_plans.size() && internal_goals_plans[x + 1].second == 1) {
                                                        get_next_internal_goal = true;
                                                        break;
                                                    }
                                                }
                                                p += tmp_counter; // move forward the 'p' index as well, in order to be sure it's aligned and pointing to correct next tmp_plan
                                                // --------------------------------------------------

                                                if(internal_goal_plan_valid_conditions)
                                                {
                                                    // create a new intention for the internal goal, using the selected (and valid) plan 'ig_plan'...
                                                    int new_intention_id = 0;
                                                    if(ag_intention_set.size() > 0) {
                                                        new_intention_id = ag_intention_set[ag_intention_set.size() - 1]->get_id() + 1; // id of the last store intention + 1
                                                    }

                                                    // compute "goal" priority using the specified expression ...............................
                                                    ag_support->computeGoalPriority(goal, ag_belief_set);
                                                    // ......................................................................................

                                                    ag_goal_set.push_back(goal); // add internal goal to goal_set
                                                    ag_metric->addActivatedGoal(goal, sim_current_time().dbl(), ag_intention_set[index_intention]->get_planID(),
                                                            next_intention_original_plan_id, "internal goal: "+ std::to_string(ig_plan_annidation));

                                                    logger->print(Logger::EveryInfo, "print sizes: "+ std::to_string(ag_intention_set.size())
                                                    +", "+ std::to_string(ag_intention_set[ag_intention_set.size() - 1]->getInternalGoalsSelectedPlansSize()));

                                                    ag_selected_plans.push_back(MCTplan);

                                                    // update the 'firstActivation' field of the intention
                                                    if(ag_intention_set[index_intention]->get_firstActivation() == -1) {
                                                        ag_intention_set[index_intention]->set_firstActivation(sim_current_time().dbl());
                                                    }

                                                    logger->print(Logger::EssentialInfo, "A NEW INTENTION FROM GOAL:["+ std::to_string(goal->get_id()) +"], goal name:["+ goal->get_goal_name() +"]");
                                                    reasoning_cycle_new_activated_goals.push_back(goal->get_goal_name()); // list of goals activated during the current instant of time and that will be send to che client
                                                    ag_intention_set.push_back(
                                                            std::make_shared<Intention>(new_intention_id,
                                                                    goal,
                                                                    ag_intention_set[index_intention]->get_planID(),
                                                                    MCTplan, ag_belief_set,
                                                                    effects_at_begin, effects_at_end, post_conditions,
                                                                    sim_current_time().dbl())
                                                    );

                                                    // --------------------------------------------------------------------------
                                                    // check and apply (if present) EFFECTS_AT_BEGIN of this intention...
                                                    tmp_cnt = ag_support->updateBeliefsetDueToEffect(ag_belief_set,
                                                            ag_intention_set[ag_intention_set.size()-1],
                                                            ag_intention_set[ag_intention_set.size()-1]->get_effects_at_begin());

                                                    if(tmp_cnt > 0) {
                                                        updated_beliefset = true;
                                                    }

                                                    /* Clear list of "effects_at_begin" as soon as they have been applied.
                                                     * The reason is that if they update the beliefset,
                                                     * a reasoning cycle must be performed immediately.
                                                     * Thus, clearing the list guarantees that no reasoning cycle loops get activated */
                                                    ag_intention_set[ag_intention_set.size()-1]->clear_effects_at_begin();
                                                    // --------------------------------------------------------------------------

                                                    ag_metric->addActivatedIntentions(ag_intention_set[ag_intention_set.size() - 1], sim_current_time().dbl());
                                                    logger->print(Logger::EveryInfo, "original: "+ std::to_string(ag_intention_set[ag_intention_set.size() - 1]->get_original_planID())
                                                    + ", will be update to: "+ std::to_string(next_intention_original_plan_id));

                                                    ag_intention_set[ag_intention_set.size() - 1]->set_original_planID(next_intention_original_plan_id);
                                                    new_intention_created = true;

                                                    checkNextAction = false;
                                                }
                                                else { // internal goal has invalid preconditions
                                                    goal->setGoalisBeenActivated(false); // in order to manage the goal in phiI
                                                    updated_intentionset = true; // in order to perform a new reasoning cycle

                                                    get_next_internal_goal = true; // exit the for loop
                                                    checkNextAction = false;
                                                }
                                            }
                                            else {
                                                logger->print(Logger::EssentialInfo, "Internal goal:["+ std::to_string(goal->get_id()) +"] is already in Goalset...");
                                                goal->setActivation_time(sim_current_time().dbl());
                                                goal->setGoalisBeenActivated(true);
                                                checkNextAction = false;
                                            }
                                        }
                                    }
                                }
                            } // -- end for loop

                            if(invalid_sub_sub_goal)
                            {
                                if(logger->get_level() >= Logger::EveryInfo) {
                                    ag_support->printIntentions(ag_intention_set, ag_goal_set);
                                    logger->print(Logger::Default, "-----------------------------");
                                    ag_Sched->ev_ag_tasks_vector_to_release();
                                    logger->print(Logger::Default, "-----------------------------");
                                    ag_Sched->ev_ag_tasks_vector_ready();
                                    logger->print(Logger::Default, "-----------------------------");
                                }

                                warning_msg = "[WARNING] Internal goal invalid preconditions - goal:["+ goal->get_goal_name();
                                warning_msg += "], plan:["+ std::to_string(plan_copy->get_id());
                                warning_msg += "] for the internal goal:["+ plan_copy->get_goal_name();
                                warning_msg += "] has invalid preconditions! -> Remove intention:[id="+ std::to_string(ag_intention_set[index_intention]->get_id());
                                warning_msg += "] name:["+ ag_intention_set[index_intention]->get_goal_name() +"]";
                                logger->print(Logger::Warning, warning_msg);
                                ag_metric->addGeneratedWarning(sim_current_time(), "ag_reasoning_cycle", warning_msg);

                                updated_intentionset = true; // in order to

                                reasoning_cycle_stopped_goals.push_back(ag_intention_set[index_intention]->get_goal_name()); // data for the clientforce a new reasoning cycle

                                removeInternalgoalTasksFromSchedulerVectors(ag_intention_set[index_intention]->get_original_planID());

                                cancel_message_internal_goal_arrival(msg_name_internal_goal_arrival, ag_intention_set[index_intention]->get_planID(), ag_intention_set[index_intention]->get_original_planID());
                                cancel_message_intention_completion(msg_name_intention_completion,
                                        ag_intention_set[index_intention]->get_goal_name(),
                                        ag_intention_set[index_intention]->get_waiting_for_completion(),
                                        ag_intention_set[index_intention]->get_goalID(),
                                        ag_intention_set[index_intention]->get_planID(),
                                        ag_intention_set[index_intention]->get_original_planID());

                                // remove it from intention set:
                                ag_metric->setActivatedIntentionCompletion(ag_intention_set[index_intention], sim_current_time().dbl(), "ag_internal_goal_activation - goal removed due to internal goal");
                                ag_intention_set.erase(ag_intention_set.begin() + index_intention);

                                if(logger->get_level() >= Logger::EveryInfo) {
                                    ag_support->printIntentions(ag_intention_set, ag_goal_set);
                                    logger->print(Logger::Default, "-----------------------------");
                                    ag_Sched->ev_ag_tasks_vector_to_release();
                                    logger->print(Logger::Default, "-----------------------------");
                                    ag_Sched->ev_ag_tasks_vector_ready();
                                    logger->print(Logger::Default, "-----------------------------");
                                }

                                break; // -- exit the for loop
                            }
                        }
                    }
                }
            }
        }
    }

    logger->print(Logger::EveryInfo, " > updated_beliefset:["+ ag_support->boolToString(updated_beliefset)
            +"], updated_intentionset:["+ ag_support->boolToString(updated_intentionset) +"]");
    if(updated_beliefset)
    { // Immediately repeat a new reasoning cycle to deal with updated beliefset (due to effects)
        scheduleAt(sim_current_time(), set_reasoning_cycle(true));
        scheduled_msg += 1;
        ag_support->setScheduleInFuture("repeat reasoning cycle due to 'effects_begin'"); // METRICS purposes

        logger->print(Logger::EssentialInfo, "->-> BELIEFSET CHANGED DUE TO EFFECTS -> SCHEDULE NEW REASONING CYCLE... <-<-");
    }
    else if(updated_intentionset)
    { /* Immediately repeat a new reasoning cycle to deal with the updated intention-set.
     * Note: if there are multiple goals that activates at the same time, only schedule one reasoning cycle (after all goals have been managed) */
        scheduleAt(sim_current_time(), set_reasoning_cycle(true));
        scheduled_msg += 1;
        ag_support->setScheduleInFuture("repeat reasoning cycle due to removed intention"); // METRICS purposes

        logger->print(Logger::EssentialInfo, "->-> INTENTION REMOVED -> SCHEDULE NEW REASONING CYCLE... <-<-");
    }
    else if(new_intention_created)
    {
        scheduleAt(sim_current_time(), set_select_next_intentions_task());
        scheduled_msg += 1;
        ag_support->setScheduleInFuture("Apply phiI to manage new intention"); // METRICS purposes
    }
    else if(fabs(ag_intention_set.size() - 0) < ag_Sched->EPSILON)
    { /* for instance, there is only one main plan active and contains an internal goal with U = 1.
     * When the internal goal has to be activated, if its preconditions are false, the entire intention is removed.
     * It may happen that one or more other plans had been discarded due to the U of the main plan selected.
     * Removing it may allow others to be activated...
     */
        scheduleAt(sim_current_time(), set_reasoning_cycle(true));
        scheduled_msg += 1;
        ag_support->setScheduleInFuture("repeat reasoning cycle due to 'empty intention_set'"); // METRICS purposes

        logger->print(Logger::EssentialInfo, "The intention set is empty. Repeat the reasoning cycle to check if other intentions can be triggered ");
    }

    if(logger->get_level() >= Logger::EssentialInfo) {
        ag_support->printIntentions(ag_intention_set, ag_goal_set);
        logger->print(Logger::Default, "-----------------------------");
        ag_Sched->ev_ag_tasks_vector_to_release();
        logger->print(Logger::Default, "-----------------------------");
        ag_Sched->ev_ag_tasks_vector_ready();
        logger->print(Logger::Default, "-----------------------------");
    }

    logger->set_level(log_level); // Debug
    // END of function "ag_internal_goal_activation"
}
/********************************************************************************/

// *****************************************************************************
// *******                        BEHAVIOURS-GP                          *******
// *****************************************************************************
void agent::schedule_taskset(agentMSG* msg) {
    logger->print(Logger::EssentialInfo, "----schedule_taskset----");
    ag_support->setSimulationLastExecution("schedule_taskset");

    // get directory params
    std::string path = glob_simulation_folder; // structure: "../simulations/" + glob_user + "/" + glob_simulation_folder + "/inputs/";

    if (ag_Sched->get_tasks_vector_to_release().empty() == false)
    {
        // sort ag_tasks_vector_to_release based on task's arrivalTime
        ag_Sched->ag_sort_tasks_arrival();

        if(logger->get_level() >= Logger::EveryInfo) {
            logger->print(Logger::Default, "--------------------------------");
            ag_Sched->ev_ag_tasks_vector_to_release();
            logger->print(Logger::Default, "::::::::::::::::::::::::::::::::");
            ag_Sched->ev_ag_tasks_vector_ready();
            logger->print(Logger::Default, "--------------------------------");
        }

        std::shared_ptr<Task> p_task = ag_Sched->get_tasks_vector_to_release()[0];
        std::shared_ptr<Task> np_task; // cloned task

        logger->print(Logger::EveryInfo, "task["+ std::to_string(p_task->getTaskId())
        +"], plan_id["+ std::to_string(p_task->getTaskPlanId())
        +"], original_plan_id["+ std::to_string(p_task->getTaskOriginalPlanId())
        +"] Arrival Time: "+ std::to_string(p_task->getTaskArrivalTime())
        +" now: "+ sim_current_time().str()
        +" has status ["+ p_task->getTaskStatus_as_string() +"]");

        /* If the arrivalTime of the task is greater than time t (now),
         * we have to re-schedule the activation of the task to the rigt time.
         * Basically, the task has to 'wait' for being activated. */
        if (fabs(p_task->getTaskArrivalTime() - sim_current_time().dbl()) > ag_Sched->EPSILON
                && p_task->getTaskStatus() == Task::TSK_IDLE)
        {
            p_task->setTaskStatus(Task::TSK_ACTIVE);
            double tmp_task_arrival = p_task->getTaskArrivalTime();

            // write utilization report (non real-time)
            write_util_json_report(p_task, getIndex(), sim_current_time().dbl(), ag_Sched->get_current_util(ag_Server_Handler), "schedule-release");

            simtime_t at_time = convert_double_to_simtime_t(p_task->getTaskArrivalTime());
            scheduleAt(at_time, set_next_task_arrival());
            scheduled_msg += 1;
            ag_support->setScheduleInFuture("SCHEDULE - schedule task activation time", p_task->getTaskArrivalTime()); // METRICS purposes

            logger->print(Logger::Default, "t: "+ sim_current_time().str()
                    +", task:["+ std::to_string(p_task->getTaskId())
            +"], plan:["+ std::to_string(p_task->getTaskPlanId())
            +"], original_plan:["+ std::to_string(p_task->getTaskOriginalPlanId())
            +"] arrival is scheduled at t: "+ std::to_string(p_task->getTaskArrivalTime()));

            /* The following loop is used in order to manage the case in which
             * we have more than 1 task with the same ArrivalTime (in RELEASE queue).
             * When this happens, loop over all tasks after the currently scheduled one.
             * For all those ones with the same Arrival Time, schedule the future ARRIVAL.
             * Note: RELEASE is kept sorted based on Arrival Time.
             * Thus, we can stop the loop as soon as we find a task with a different Arrival Time. */
            for(int i = 1; i < (int)ag_Sched->get_tasks_vector_to_release().size(); i++) {
                p_task = ag_Sched->get_tasks_vector_to_release()[i];
                if(p_task->getTaskArrivalTime() == tmp_task_arrival && p_task->getTaskStatus() == Task::TSK_IDLE)
                {
                    p_task->setTaskStatus(Task::TSK_ACTIVE);

                    // write utilization report (non real-time)
                    write_util_json_report(p_task, getIndex(), sim_current_time().dbl(), ag_Sched->get_current_util(ag_Server_Handler), "schedule-release");

                    simtime_t at_time = convert_double_to_simtime_t(p_task->getTaskArrivalTime());
                    scheduleAt(at_time, set_next_task_arrival());
                    scheduled_msg += 1;
                    ag_support->setScheduleInFuture("SCHEDULE - schedule task activation time", p_task->getTaskArrivalTime()); // METRICS purposes

                    logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                            +", task:["+ std::to_string(p_task->getTaskId())
                    +"], plan:["+ std::to_string(p_task->getTaskPlanId())
                    +"], original_plan:["+ std::to_string(p_task->getTaskOriginalPlanId())
                    +"] arrival is scheduled at t: "+ std::to_string(p_task->getTaskArrivalTime()));
                }
                else {
                    break;
                }
            }
            // ---------------------------------
        }
        else if (fabs(p_task->getTaskArrivalTime() - sim_current_time().dbl()) < ag_Sched->EPSILON
                && (p_task->getTaskStatus() == Task::TSK_IDLE
                        || p_task->getTaskStatus() == Task::TSK_ACTIVE
                        || p_task->getTaskStatus() == Task::TSK_EXEC))
        {
            /* if there is a task in release_queue, with type IDLE, in position second or more (3,4,...),
             * with arrivalTime = now, and the first task had the same arrivalTime,
             * and at arrivalTime the intention of the first task gets removed during a reasoning cycle at t: arrivaltime,
             * the second task becomes the head of the queue. It has arrival = now but state still in IDLE.
             * Else statement activated when an incoming task has arrivalTime equal to now. Thus, it's the right time to execute it... */

            p_task->setTaskStatus(Task::TSK_READY);
            if(p_task->is_server()) {
                p_task->get_head()->setTaskStatus(Task::TSK_READY);
            }

            // write utilization report (non real-time)
            write_util_json_report(p_task, getIndex(), sim_current_time().dbl(), ag_Sched->get_current_util(ag_Server_Handler), "release");

            //if the task's number of execution is set, decrease it
            int n_exec = p_task->getTaskNExec();
            if(n_exec > 0) {
                p_task->setTaskNExec(n_exec - 1);
            }

            // if the task has not been activated, set the release time at t_now
            if (p_task->getTaskFirstActivation() == -1) {
                p_task->setTaskReleaseTime(sim_current_time().dbl()); // set the instant the task gets activated and moved to ready
            }

            logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                    +", ARRIVED tskId:["+ std::to_string(p_task->getTaskId())
            +"], plan:["+ std::to_string(p_task->getTaskPlanId())
            +"], original_plan:["+ std::to_string(p_task->getTaskOriginalPlanId())
            +"], NExec left: "+ std::to_string(n_exec)
            +", tskRt: "+ std::to_string(p_task->getTaskReleaseTime()));

            // Check if t is periodic
            if (p_task->getTaskPeriod() > 0)
            {
                double n_arrival_time = sim_current_time().dbl() + p_task->getTaskPeriod();
                np_task = p_task->cloneTask();
                np_task->setTaskStatus(Task::TSK_IDLE); // Added due to shared_ptr
                np_task->setTaskLastActivation(-1);
                np_task->setTaskFirstActivation(-1);
                np_task->setTaskCompTimeRes(p_task->getTaskCompTime());
                np_task->setTaskArrivalTime(n_arrival_time);

                n_exec = p_task->getTaskNExec();
                if (n_exec > 0 || n_exec == -1)
                {
                    logger->print(Logger::EssentialInfo, "(GP) Task:["+ std::to_string(p_task->getTaskId())
                    +"], plan:["+ std::to_string(p_task->getTaskPlanId())
                    +"], original_plan:["+ std::to_string(p_task->getTaskOriginalPlanId())
                    +"] is periodic, copied back into the -to-be-released-queue!");
                    ag_Sched->ag_add_task_in_vector_to_release(np_task);
                }
            }

            if (ag_Sched->get_tasks_vector_to_release().empty() == false)
            {
                ag_Sched->ag_sort_tasks_arrival();

                /* (04/05/20)
                 * instead of scheduling the arrival of just the next event,
                 * schedule one for each event with the next arrivalTime.
                 * (es. if t=0 and there are two events that arrives a t=10,
                 * schedule two messages in order to activate them)
                 * If we don't do this, periodic task may still have status=IDLE when it's their time to be executed */
                bool checkNextTask = true;
                std::shared_ptr<Task> task_to_release;

                for(int i = 1; i < (int)ag_Sched->get_tasks_vector_to_release().size() && checkNextTask; i++)
                {
                    checkNextTask = false;
                    task_to_release = ag_Sched->get_tasks_vector_to_release()[i];

                    if(fabs(task_to_release->getTaskArrivalTime() - sim_current_time().dbl()) < ag_Sched->EPSILON
                            && task_to_release->getTaskStatus() == Task::TSK_IDLE)
                    {
                        logger->print(Logger::EssentialInfo, " > Task_to_release:["+ std::to_string(task_to_release->getTaskId())
                        +"], plan:["+ std::to_string(task_to_release->getTaskPlanId())
                        +"], original_plan:["+ std::to_string(task_to_release->getTaskOriginalPlanId())
                        +"] status:["+ task_to_release->getTaskStatus_as_string()
                        +"] force update to:[ACTIVE] has arrivalTime: "+ std::to_string(task_to_release->getTaskArrivalTime()));

                        checkNextTask = true;
                        task_to_release->setTaskStatus(Task::TSK_ACTIVE);
                    }
                }

                simtime_t at_time = convert_double_to_simtime_t(ag_Sched->get_tasks_vector_to_release()[0]->getTaskArrivalTime());
                scheduleAt(at_time, set_next_task_arrival());
                scheduled_msg += 1;
                ag_support->setScheduleInFuture("SCHEDULE - more task in release vector", ag_Sched->get_tasks_vector_to_release()[0]->getTaskArrivalTime()); // METRICS purposes
                logger->print(Logger::EssentialInfo, "(GP) More Task(s) to release in the vector, scheduling next check for arrival");
            }

            bool single_ready_task = ag_Sched->ag_task_arrived();

            // task ddl_miss check at arrival time
            //             double t_ddl_miss = p_task->getTaskArrivalTime() + p_task->getTaskDeadline();
            //
            //            logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
            //                    +", DDL check for task:["+ std::to_string(p_task->getTaskId())
            //                    +"], plan:["+ std::to_string(p_task->getTaskPlanId())
            //                    +"], original_plan:["+ std::to_string(p_task->getTaskOriginalPlanId())
            //                    +"] scheduled at t: "+ std::to_string(t_ddl_miss));

            if (single_ready_task)
            {   // only one task in the ready queue so it gets the CPU - scheduling the task execution
                agentMSG* task_activation_msg = set_task_activation();
                double schedule_msg_time = ag_Sched->get_tasks_vector_ready()[0]->getTaskArrivalTime(); // first element is a PERIODIC task

                simtime_t at_time = convert_double_to_simtime_t(schedule_msg_time);
                scheduleAt(at_time, task_activation_msg);
                scheduled_msg += 1;
                ag_support->setScheduleInFuture("ACTIVATE_TASK - single task ready queue"); // METRICS purposes
            }
            else {
                // sort the -to-release-vector in order to correctly manage the arrival of tasks
                ag_Sched->ag_sort_tasks_arrival();
            }
        }
        else if ((sim_current_time().dbl() > ag_Sched->get_tasks_vector_to_release()[0]->getTaskArrivalTime())
                && (ag_Sched->get_tasks_vector_to_release()[0]->getTaskCompTimeRes() > 0))
        {
            std::string msg = "Task:["+ std::to_string(ag_Sched->get_tasks_vector_to_release()[0]->getTaskId());
            msg += "], planid:["+ std::to_string(ag_Sched->get_tasks_vector_to_release()[0]->getTaskPlanId());
            msg += "], original_planid:["+ std::to_string(ag_Sched->get_tasks_vector_to_release()[0]->getTaskOriginalPlanId());
            msg += "] to be released has 'arrival time' in the past (at t: "+ std::to_string(ag_Sched->get_tasks_vector_to_release()[0]->getTaskArrivalTime()) +")!";
            logger->print(Logger::Error, "[ERROR] " + msg);
            ag_metric->addGeneratedError(sim_current_time(), "schedule_taskset", msg);
        }
    }

    //    // DEBUG *************************************************************************
    //    ag_support->printIntentions(ag_intention_set, ag_goal_set);
    //    logger->print(Logger::Default, "-----------------------------");
    ag_Sched->ev_ag_tasks_vector_to_release();
    logger->print(Logger::Default, "-----------------------------");
    ag_Sched->ev_ag_tasks_vector_ready();
    logger->print(Logger::Default, "-----------------------------");
    //    // *******************************************************************************
}

void agent::check_task_completion(agentMSG* agMsg)
{
    logger->print(Logger::Default, "----check_task_completion----");
    ag_support->setSimulationLastExecution("check_task_completion");
    std::string report_value = "", warning_msg = "";
    double task_absolute_deadline;

    if (ag_Sched->get_tasks_vector_ready().empty() == false)
    {
        std::shared_ptr<Task> p_task = ag_Sched->get_tasks_vector_ready()[0];

        logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                +", checking COMPLETION for task[id="+ std::to_string(agMsg->getAg_task_id())
        +"], plan:["+ std::to_string(agMsg->getAg_task_plan_id())
        +"], original_plan:["+ std::to_string(agMsg->getAg_task_original_plan_id()) +"]");

        /* it might have been killed due to ddl_miss so we need the following check
         * or it can be preempted so it didn't finish and we need to check in a second time
         * --------------------------------------------------------------------------------
         * [*] 12/02/21: commented due to the fact that:
         * - t:0 T0,7 original plan 9 is activated. CompTime = 1 and first in ready queue.
         * - t:1 P7 is accomplished (due to sensor)
         * - t:1 check_task_completion_rt is triggered to check if T0,7-9 has been completed.
         * - T0,7-9 has been completed, but it's not called T0,7-9 anymore
         * - when P7 gets satisfied, T0,7-9 becomes T0,9. T
         * - Therefore, we just need to check task_id and original_plan_id. */
        if ((agMsg->getAg_task_id() == p_task->getTaskId())
                && (p_task->getTaskOriginalPlanId() == agMsg->getAg_task_original_plan_id())
                && (p_task->getTaskDemander() == agMsg->getAg_task_demander())
                && (p_task->getTaskReleaseTime() == agMsg->getAg_task_release_time())
                && (p_task->getTaskLastActivation() != -1)
                && (p_task->getTaskLastActivation() < sim_current_time().dbl())
                && p_task->getTaskStatus() == Task::TSK_EXEC)
        {
            double t_CRes = p_task->getTaskCompTimeRes();
            double last_computation = sim_current_time().dbl() - p_task->getTaskLastActivation();
            double comp_time_res = t_CRes - last_computation;

            if (comp_time_res < 0 && p_task->getTaskStatus() != Task::TSK_EXEC)
            {
                warning_msg = "[WARNING] COMPLETION CHECK > schedule task activation > Task:["+ std::to_string(p_task->getTaskId());
                warning_msg += "], plan:["+ std::to_string(p_task->getTaskPlanId());
                warning_msg += "], original_plan:["+ std::to_string(p_task->getTaskOriginalPlanId());
                warning_msg += "], t_CRes: "+ std::to_string(t_CRes);
                warning_msg += ", last_computation: "+ std::to_string(comp_time_res);
                warning_msg += ", comp_time_res: "+ std::to_string(last_computation);
                logger->print(Logger::Warning, warning_msg);
                ag_metric->addGeneratedWarning(sim_current_time(), "check_task_completion_rt", warning_msg);

                scheduleAt(sim_current_time(), set_task_activation());
                scheduled_msg += 1;
                ag_support->setScheduleInFuture("ACTIVATE_TASK - task has more compTimeRes"); // METRICS purposes
            }
            else if (fabs(comp_time_res) < ag_Sched->EPSILON)
            {
                p_task->setTaskStatus(Task::TSK_COMPLETED);
                lastCompletedTaskTime = sim_current_time().dbl(); // update the instant when the last task has been completed.

                logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                        +", COMPLETED tskId:["+ std::to_string(p_task->getTaskId())
                +"], plan:["+ std::to_string(p_task->getTaskPlanId())
                +"], original_id:["+ std::to_string(p_task->getTaskOriginalPlanId())
                +"], N_exec left:["+ std::to_string(p_task->getTaskNExec())
                +" of "+ std::to_string(p_task->getTaskNExecInitial())
                +"], tskRt: "+ std::to_string(p_task->getTaskReleaseTime())
                +", status:["+ p_task->getTaskStatus_as_string() +"]");

                report_value = "t: "+ sim_current_time().str();
                report_value += " COMPLETED tskId:["+ std::to_string(p_task->getTaskId());
                report_value += "], plan:["+ std::to_string(p_task->getTaskPlanId());
                report_value += "], original_id:["+ std::to_string(p_task->getTaskOriginalPlanId());
                report_value += "], deadline:["+ std::to_string(p_task->getTaskDeadline()) +"]";

                // decrease the n_exec parameter of p_task in the intention that contains it
                ag_support->updatePeriodicTasksInsideIntention(p_task, ag_intention_set);

                // check if the completed tasks was unexpected -------------------------------------------------------
                ag_metric->checkIfUnexpectedTaskExecution(p_task->makeCopyOfTask(), ag_intention_set);
                // ---------------------------------------------------------------------------------------------------

                //remove head on the ordered vector
                ag_Sched->ag_remove_head_in_ready_tasks_vector();

                // get the index of the intention containing this task...
                int index_intention = -1;
                for(int i = 0; i < (int)ag_intention_set.size(); i++) {
                    logger->print(Logger::EveryInfoPlus, std::to_string(ag_intention_set[i]->get_planID())
                    +" == "+ std::to_string(p_task->getTaskPlanId())
                    +" && "+ std::to_string(ag_intention_set[i]->get_original_planID())
                    +" == "+ std::to_string(p_task->getTaskOriginalPlanId()));

                    if(ag_intention_set[i]->get_planID() == p_task->getTaskPlanId()
                            && ag_intention_set[i]->get_original_planID() == p_task->getTaskOriginalPlanId())
                    {
                        index_intention = i;
                        break;
                    }
                }
                if(index_intention < 0 && index_intention >= (int)ag_intention_set.size()) {
                    std::string msg = "[ERROR] COMPLETED TASK:["+ std::to_string(p_task->getTaskId());
                    msg += "], plan_id:["+ std::to_string(p_task->getTaskPlanId());
                    msg += "], original_plan_id:["+ std::to_string(p_task->getTaskOriginalPlanId());
                    msg += "], IS NOT LINKED TO ANY ACTIVE INTENTION";

                    throwRuntimeException(true, glob_path, glob_user, msg, "agent", "check_task_completion", sim_current_time().dbl());
                }
                else {
                    logger->print(Logger::EveryInfo, "start: "+ std::to_string(ag_intention_set[index_intention]->get_startTime())
                    +", firstAct: "+ std::to_string(ag_intention_set[index_intention]->get_firstActivation())
                    +", task_firstAct: "+ std::to_string(agMsg->getAg_task_firstActivation())
                    +", task_lastAct: "+ std::to_string(agMsg->getAg_task_lastActivation())
                    +", task_releaseT: "+ std::to_string(agMsg->getAg_task_release_time()));

                    double lateness = -1;
                    double resp_time = sim_current_time().dbl() - p_task->getTaskReleaseTime();
                    std::string latenessLog = "";

                    logger->print(Logger::EssentialInfo, " > RESP_TIME:["+ std::to_string(sim_current_time().dbl()  - p_task->getTaskReleaseTime())
                    +"] = "+ sim_current_time().str()
                    +" - "+ std::to_string(p_task->getTaskReleaseTime()));

                    if(ag_Sched->get_sched_type() == RR)
                    {
                        logger->print(Logger::EssentialInfo, "RR Task:["+ std::to_string(p_task->getTaskId())
                        +"], plan:["+ std::to_string(p_task->getTaskPlanId())
                        +"], original_plan:["+ std::to_string(p_task->getTaskOriginalPlanId())
                        +"], lastActivation:["+ std::to_string(p_task->getTaskLastActivation())
                        +"], quantum:["+ std::to_string(ag_Sched->get_quantum())
                        +"], now:["+ sim_current_time().str()
                        +"], used quantum:["+ std::to_string(sim_current_time().dbl() - p_task->getTaskLastActivation()) +"]");

                        if(p_task->getTaskReleaseTime() == p_task->getTaskArrivalTime())
                        {
                            double ddl = p_task->getTaskReleaseTime() + p_task->getTaskDeadline();
                            lateness = sim_current_time().dbl() - ddl;
                            task_absolute_deadline = ddl;

                            latenessLog = "RR t: "+ sim_current_time().str() +" case A > Lateness: "+ sim_current_time().str();
                            latenessLog += " - ("+ std::to_string(p_task->getTaskReleaseTime());
                            latenessLog += " + "+ std::to_string(p_task->getTaskDeadline());
                            latenessLog += ") -> lateness:[["+ std::to_string(lateness) +"]]";
                            logger->print(Logger::EssentialInfo, latenessLog);
                        }
                        else {
                            lateness = sim_current_time().dbl() - (agMsg->getAg_task_firstActivation() + p_task->getTaskDeadline());
                            task_absolute_deadline = agMsg->getAg_task_firstActivation() + p_task->getTaskDeadline();

                            latenessLog = "RR t: "+ sim_current_time().str() +" case B > Lateness: "+ sim_current_time().str();
                            latenessLog += " - ("+ std::to_string(agMsg->getAg_task_firstActivation());
                            latenessLog += " + "+ std::to_string(p_task->getTaskDeadline());
                            latenessLog += ") -> lateness:[["+ std::to_string(lateness) +"]]";
                            logger->print(Logger::EssentialInfo, latenessLog);
                        }

                        if (lateness > 0) {
                            report_value += ", lateness:["+ std::to_string(lateness) +"], ddl_miss:[YES]";

                            // Write lateness.json report file --------------------------------------------------
                            write_lateness_json_report(p_task, getIndex(), lateness, sim_current_time().dbl());

                            ag_settings->add_ddl_miss();
                            ag_tasks_lateness.push_back(std::make_shared<Lateness>(sim_current_time().dbl(), p_task->getTaskId(), p_task->getTaskPlanId(), p_task->getTaskOriginalPlanId(), p_task->getTaskNExec(), lateness, latenessLog));

                            ddl_json_report(ag_Sched, p_task, t_CRes);
                            // ---------------------------------------------------------------------------------

                            logger->print(Logger::EveryInfo, "RR t: "+ sim_current_time().str() +" case C > Lateness:["+ std::to_string(lateness)
                            +"] -> Task:[id="+ std::to_string(p_task->getTaskId())
                            +"], plan:[id="+ std::to_string(p_task->getTaskPlanId())
                            +"], original_plan:[id="+ std::to_string(p_task->getTaskOriginalPlanId()) +"]");
                        }
                        else {
                            report_value += ", lateness:["+ std::to_string(lateness) +"], ddl_miss:[NO]";
                        }
                        logger->print(Logger::Debug, "----DEBUG: inhere----");
                    }
                    else if(ag_Sched->get_sched_type() == FCFS)
                    {
                        double taskComputetionTime = 0;
                        std::string latenessLog = "";
                        lateness = 0;

                        logger->print(Logger::EveryInfo, "start: "+ std::to_string(ag_intention_set[index_intention]->get_startTime())
                        +" != firstAct: "+ std::to_string(ag_intention_set[index_intention]->get_firstActivation())
                        +" -> "+ ag_support->boolToString(ag_intention_set[index_intention]->get_startTime() != ag_intention_set[index_intention]->get_firstActivation()));

                        if(ag_intention_set[index_intention]->get_firstActivation() != ag_intention_set[index_intention]->get_startTime()) {
                            if(agMsg->getAg_task_release_time() == agMsg->getAg_intention_firstActivation())
                            {
                                taskComputetionTime = sim_current_time().dbl();
                                lateness = taskComputetionTime - (agMsg->getAg_intention_startTime() + p_task->getTaskDeadline());
                                task_absolute_deadline = agMsg->getAg_intention_startTime() + p_task->getTaskDeadline();

                                latenessLog = "t: "+ sim_current_time().str() +" case A > Lateness: "+ std::to_string(taskComputetionTime);
                                latenessLog += " - ("+ std::to_string(agMsg->getAg_intention_startTime());
                                latenessLog += " + "+ std::to_string(p_task->getTaskDeadline());
                                latenessLog += ") --> lateness:[["+ std::to_string(lateness) +"]]";
                                logger->print(Logger::EssentialInfo, latenessLog);
                            }
                            else {
                                taskComputetionTime = p_task->getTaskReleaseTime() + p_task->getTaskCompTime();
                                lateness = taskComputetionTime - (p_task->getTaskReleaseTime() + p_task->getTaskDeadline());
                                task_absolute_deadline = p_task->getTaskReleaseTime() + p_task->getTaskDeadline();

                                latenessLog = "t: "+ sim_current_time().str() +" case B > Lateness: "+ std::to_string(taskComputetionTime);
                                latenessLog += " - ("+ std::to_string(p_task->getTaskReleaseTime());
                                latenessLog += " + "+ std::to_string(p_task->getTaskDeadline());
                                latenessLog += ") --> lateness:[["+ std::to_string(lateness) +"]]";
                                logger->print(Logger::EssentialInfo, latenessLog);

                                report_value += ", lateness:["+ std::to_string(lateness) +"]";
                            }
                        }
                        else {
                            taskComputetionTime = p_task->getTaskReleaseTime() + p_task->getTaskCompTime();
                            lateness = taskComputetionTime - (p_task->getTaskReleaseTime() + p_task->getTaskDeadline());
                            task_absolute_deadline = p_task->getTaskReleaseTime() + p_task->getTaskDeadline();

                            latenessLog = "t: "+ sim_current_time().str() +" case C > Lateness: "+ std::to_string(taskComputetionTime);
                            latenessLog += " - ("+ std::to_string(p_task->getTaskReleaseTime());
                            latenessLog += " + "+ std::to_string(p_task->getTaskDeadline());
                            latenessLog += ") --> lateness:[["+ std::to_string(lateness) +"]]";
                            logger->print(Logger::EssentialInfo, latenessLog);
                        }

                        if(lateness > 0) {
                            report_value += ", lateness:["+ std::to_string(lateness) +"], ddl_miss:[YES]";

                            logger->print(Logger::EssentialInfo, "DDL MISS > Lateness:["+ std::to_string(lateness)
                            +"] -> Task:[id="+ std::to_string(agMsg->getAg_task_id())
                            +"], plan:[id="+ std::to_string(agMsg->getAg_task_plan_id())
                            +"], original_plan:[id="+ std::to_string(agMsg->getAg_task_original_plan_id()) +"]");

                            // Write lateness.json report file
                            write_lateness_json_report(p_task, getIndex(), lateness, sim_current_time().dbl());

                            ag_settings->add_ddl_miss(); // will be used as data for the reports
                            ag_tasks_lateness.push_back(std::make_shared<Lateness>(sim_current_time().dbl(), agMsg->getAg_task_id(), agMsg->getAg_task_plan_id(), agMsg->getAg_task_original_plan_id(), p_task->getTaskNExec(), lateness, latenessLog));

                            ddl_json_report(ag_Sched, p_task, t_CRes);
                            // ---------------------------------------------------------------------------------

                            logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                                    +", task:["+ std::to_string(agMsg->getAg_task_id())
                            +"], plan_id:["+ std::to_string(agMsg->getAg_task_plan_id())
                            +"], original_plan_id:["+ std::to_string(agMsg->getAg_task_original_plan_id())
                            +"] MISSED the DDL");
                        }
                        else {
                            report_value += ", lateness:["+ std::to_string(lateness) +"], ddl_miss:[NO]";
                        }
                        logger->print(Logger::Debug, "----DEBUG: inhere----");
                    }

                    // WRITING TO FILE -------------------------------------------------------------------------
                    // write simulation events report
                    write_simulation_timeline_json_report(report_value);

                    // write ddl_check
                    write_ddl_checks(agMsg->getAg_task_plan_id(), agMsg->getAg_task_original_plan_id(), agMsg->getAg_task_id(), getIndex());

                    // write response time report
                    write_response_json_report(p_task, getIndex(), sim_current_time().dbl(), resp_time);
                    ag_metric->addTaskResponseTime(p_task, sim_current_time().dbl(), resp_time);

                    // write response time per task report
                    write_json_resp_per_task(p_task, getIndex(), sim_current_time().dbl(), resp_time);

                    // write utilization report
                    write_util_json_report(p_task, getIndex(), sim_current_time().dbl(), ag_Sched->get_current_util(ag_Server_Handler), "task completed");

                    /* BDI: this means that the execution of p_task has been performed by the same agent that needed it ...
                     * .. if the task has no more executions left, notify the agent */
                    if(p_task->getTaskNExec() == 0)
                    {   // directly call function that removes the accomplished task
                        ag_intention_task_completed(set_intention_task_completed(p_task, task_absolute_deadline));
                    }

                    // schedule next activation
                    if (ag_Sched->get_tasks_vector_ready().empty() == false) {
                        p_task = ag_Sched->get_tasks_vector_ready()[0];
                        scheduleAt(sim_current_time(), set_task_activation());
                        scheduled_msg += 1;
                        ag_support->setScheduleInFuture("ACTIVATE_TASK - schedule next action"); // METRICS purposes
                    }
                }
            }
            else if (p_task->getTaskCompTimeRes() > 0)
            {
                if (ag_Sched->get_sched_type() == RR)
                {
                    logger->print(Logger::EssentialInfo, "RR --- Task:["+ std::to_string(p_task->getTaskId())
                    +"], plan:["+ std::to_string(p_task->getTaskPlanId())
                    +"], original_plan:["+ std::to_string(p_task->getTaskOriginalPlanId())
                    +"], lastActivation:["+ std::to_string(p_task->getTaskLastActivation())
                    +"], cRes:["+ std::to_string(p_task->getTaskCompTimeRes())
                    +"], quantum:["+ std::to_string(ag_Sched->get_quantum())
                    +"], now:["+ sim_current_time().str()
                    +"], used quantum:["+ std::to_string(sim_current_time().dbl() - p_task->getTaskLastActivation()) +"] ---");

                    // removed the EXPIRED task from the ready vector
                    ag_Sched->ag_remove_head_in_ready_tasks_vector();
                    double next_arrival_time = sim_current_time().dbl();
                    if (ag_Sched->get_tasks_vector_ready().empty() == false) {
                        next_arrival_time = sim_current_time().dbl();
                        logger->print(Logger::EssentialInfo, "RR: Next Activation: "+ std::to_string(next_arrival_time));
                    }
                    // Update the task's STATUS from EXEC to READY -------------------
                    if(p_task->is_server()) {
                        if(p_task->queue_length() > 0) {
                            p_task->get_ith_task()->setTaskStatus(Task::TSK_READY);
                            p_task->setTaskStatus(Task::TSK_READY);
                        }
                    }
                    else {
                        p_task->setTaskStatus(Task::TSK_READY);
                    }
                    // --------------------------------------------------------------

                    p_task->setTaskCompTimeRes(comp_time_res);
                    p_task->setTaskArrivalTime(next_arrival_time);
                    p_task->setTaskLastActivation(sim_current_time().dbl());
                    // push back the EXPIRED task in the ready vector
                    ag_Sched->ag_add_task_in_ready_tasks_vector(p_task);

                    logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                            +", EXPIRED Task:["+ std::to_string(p_task->getTaskId())
                    +"], plan:["+ std::to_string(p_task->getTaskPlanId())
                    +"], original_plan:["+ std::to_string(p_task->getTaskOriginalPlanId())
                    +"], compTimeRes:["+ std::to_string(p_task->getTaskCompTimeRes())
                    +"], initial compTime:["+ std::to_string(p_task->getTaskCompTime())
                    +"], tskRt:["+ std::to_string(p_task->getTaskReleaseTime())+"]");

                    report_value = "t: "+ sim_current_time().str();
                    report_value += ", EXPIRED tskId:["+ std::to_string(p_task->getTaskId());
                    report_value += "], plan:["+ std::to_string(p_task->getTaskPlanId());
                    report_value += "], original_plan:["+ std::to_string(p_task->getTaskOriginalPlanId());
                    report_value += "], tot_quantum:["+ std::to_string(ag_Sched->get_quantum());
                    report_value += "], task_res_compTime:["+ std::to_string(p_task->getTaskCompTimeRes()) +"]";

                    logger->print(Logger::Debug, "----DEBUG: inhere----");

                    // check if the completed tasks was unexpected
                    ag_metric->checkIfUnexpectedTaskExecution(p_task->makeCopyOfTask(), ag_intention_set);
                    // -------------------------------------------------------

                    // write simulation events report
                    write_simulation_timeline_json_report(report_value);

                    scheduleAt(sim_current_time(), set_task_activation());
                    scheduled_msg += 1;
                    ag_support->setScheduleInFuture("ACTIVATE_TASK - task expired"); // METRICS purposes
                }
            }
        }
    }
}

// *****************************************************************************
// *******                        BEHAVIOURS-RT                          *******
// *****************************************************************************
void agent::manage_ag_run_until_t(agentMSG* msg)
{
    logger->print(Logger::EveryInfoPlus, " -- Manage last message run until t: "+ sim_current_time().str());
}

void agent::schedule_taskset_rt(agentMSG* msg)
{
    logger->print(Logger::EssentialInfo, "----schedule_taskset_rt----");
    ag_support->setSimulationLastExecution("schedule_taskset_rt");

    // get directory params
    std::string path = glob_simulation_folder; // structure: "../simulations/" + glob_user + "/" + glob_simulation_folder + "/inputs/";

    if (ag_Sched->get_tasks_vector_to_release().empty() == false)
    {
        // sort ag_tasks_vector_to_release based on task's arrivalTime
        ag_Sched->ag_sort_tasks_arrival();

        if(logger->get_level() >= Logger::EveryInfo) {
            logger->print(Logger::Default, "--------------------------------");
            ag_Sched->ev_ag_tasks_vector_to_release();
            logger->print(Logger::Default, "::::::::::::::::::::::::::::::::");
            ag_Sched->ev_ag_tasks_vector_ready();
            logger->print(Logger::Default, "--------------------------------");
        }

        std::shared_ptr<Task> p_task = ag_Sched->get_tasks_vector_to_release()[0];
        std::shared_ptr<Task> p_server = nullptr;
        std::shared_ptr<Task> np_task = nullptr;// cloned task

        logger->print(Logger::EveryInfo, "task["+ std::to_string(p_task->getTaskId())
        +"], plan_id["+ std::to_string(p_task->getTaskPlanId())
        +"], original_plan_id["+ std::to_string(p_task->getTaskOriginalPlanId())
        +"] Arrival Time: "+ std::to_string(p_task->getTaskArrivalTime())
        +" now: "+ sim_current_time().str()
        +" has status ["+ p_task->getTaskStatus_as_string() +"] IS SERVER? "+ std::to_string(p_task->getTaskServer()));

        /* If the arrivalTime of the task is greater than time t (now),
         * we have to re-schedule the activation of the task to the rigt time.
         * Basically, the task has to 'wait' for being activated. */
        if (fabs(p_task->getTaskArrivalTime() - sim_current_time().dbl()) > ag_Sched->EPSILON
                && p_task->getTaskStatus() == Task::TSK_IDLE)
        {
            // update the status.
            // This is needed in order to prevent scheduling the arrival of a task multiple times.
            // Without this, it may happen that the next execution of a task is scheduled multiple time,
            // because, for instance, a new intention get activated
            p_task->setTaskStatus(Task::TSK_ACTIVE);
            double tmp_task_arrival =  p_task->getTaskArrivalTime();

            logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                    +" task:["+ std::to_string(p_task->getTaskId())
            +"], plan:["+ std::to_string(p_task->getTaskPlanId())
            +"], original_plan:["+ std::to_string(p_task->getTaskOriginalPlanId())
            +"], status updated to ["+ p_task->getTaskStatus_as_string() +"]");

            // write utilization report
            write_util_json_report(p_task, getIndex(), sim_current_time().dbl(), ag_Sched->get_current_util(ag_Server_Handler, compute_MsgUtil_sever()), "schedule-release");

            simtime_t at_time = convert_double_to_simtime_t(p_task->getTaskArrivalTime());
            scheduleAt(at_time, set_next_task_arrival());
            scheduled_msg += 1;
            ag_support->setScheduleInFuture("SCHEDULE - schedule task activation time", p_task->getTaskArrivalTime()); // METRICS purposes

            logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                    +", task ["+ std::to_string(p_task->getTaskId())
            +"], plan_id ["+ std::to_string(p_task->getTaskPlanId())
            +"], original_plan:["+ std::to_string(p_task->getTaskOriginalPlanId())
            +"], arrival is scheduled at t: "+ std::to_string(p_task->getTaskArrivalTime()));

            /* The following loop is used in order to manage the case in which
             * we have more than 1 task with the same ArrivalTime (in RELEASE queue).
             * When this happens, loop over all tasks after the currently scheduled one.
             * For all those ones with the same Arrival Time, schedule the future ARRIVAL.
             * Note: RELEASE is kept sorted based on Arrival Time.
             * Thus, we can stop the loop as soon as we find a task with a different Arrival Time. */
            for(int i = 1; i < (int)ag_Sched->get_tasks_vector_to_release().size(); i++)
            {
                p_task = ag_Sched->get_tasks_vector_to_release()[i];
                if(p_task->getTaskArrivalTime() == tmp_task_arrival && p_task->getTaskStatus() == Task::TSK_IDLE)
                {
                    p_task->setTaskStatus(Task::TSK_ACTIVE);

                    // write utilization report (non real-time)
                    write_util_json_report(p_task, getIndex(), sim_current_time().dbl(), ag_Sched->get_current_util(ag_Server_Handler), "schedule-release");

                    simtime_t at_time = convert_double_to_simtime_t(p_task->getTaskArrivalTime());
                    scheduleAt(at_time, set_next_task_arrival());
                    scheduled_msg += 1;
                    ag_support->setScheduleInFuture("SCHEDULE - schedule task activation time", p_task->getTaskArrivalTime()); // METRICS purposes

                    logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                            +", task:["+ std::to_string(p_task->getTaskId())
                    +"], plan:["+ std::to_string(p_task->getTaskPlanId())
                    +"], original_plan:["+ std::to_string(p_task->getTaskOriginalPlanId())
                    +"] arrival is scheduled at t: "+ std::to_string(p_task->getTaskArrivalTime()));
                }
                else {
                    break;
                }
            }
            // ---------------------------------
        }
        else if (fabs(p_task->getTaskArrivalTime() - sim_current_time().dbl()) < ag_Sched->EPSILON
                && (p_task->getTaskStatus() == Task::TSK_IDLE
                        || p_task->getTaskStatus() == Task::TSK_ACTIVE
                        || p_task->getTaskStatus() == Task::TSK_EXEC))
        {   /* if there is a task in release_queue, with type IDLE, in position second or more (3,4,...),
         * with arrivalTime = now, and the first task had the same arrivalTime,
         * and at arrivalTime the intention of the first task gets removed during a reasoning cycle at t: arrivaltime,
         * the second task becomes the head of the queue. It has arrival = now but state still in IDLE.
         * Else statement activated when an incoming task has arrivalTime equal to now. Thus, it's the right time to execute it... */

            logger->print(Logger::EveryInfo, "Task:["+ std::to_string(p_task->getTaskId())
            +"], status:["+ p_task->getTaskStatus_as_string()
            +"] arrivalTime: "+ std::to_string(p_task->getTaskArrivalTime())
            +", residual_comp_time:"+ std::to_string(p_task->getTaskCompTimeRes())
            +" -> result: "+ std::to_string(fabs(p_task->getTaskArrivalTime() - sim_current_time().dbl()) < ag_Sched->EPSILON && (p_task->getTaskStatus() == Task::TSK_ACTIVE || p_task->getTaskStatus() == Task::TSK_EXEC)));

            p_task->setTaskStatus(Task::TSK_READY); // update task status
            if(p_task->is_server()) {
                p_task->get_head()->setTaskStatus(Task::TSK_READY); // update seerver first element in queue
            }

            // write utilization report
            write_util_json_report(p_task, getIndex(), sim_current_time().dbl(), ag_Sched->get_current_util(ag_Server_Handler, compute_MsgUtil_sever()), "release");

            logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                    +" task:["+ std::to_string(p_task->getTaskId())
            +"], plan:["+ std::to_string(p_task->getTaskPlanId())
            +"], original plan:["+ std::to_string(p_task->getTaskOriginalPlanId())
            +"], status updated to ["+ p_task->getTaskStatus_as_string() +"]");

            // if the task's number of execution is set (and it's not periodic -1), decrease it
            int n_exec = p_task->getTaskNExec();
            if(n_exec > 0) {
                p_task->setTaskNExec(n_exec - 1);
            }

            // if the task has not been activated, set the release time at t_now
            if (p_task->getTaskFirstActivation() == -1)
            {
                p_task->setTaskReleaseTime(sim_current_time().dbl());
                int s_id = p_task->getTaskServer(); // CBS: handle service
                if (s_id != -1)
                {
                    p_server = ag_Server_Handler->get_server(s_id);

                    if(p_server == nullptr) {
                        // p_server is null if task has 'n_exec'= -1 and an invalid server_id (one not present in the ag_server)
                        std::string msg = "[ERROR] Task:["+ std::to_string(p_task->getTaskId()) +"] is associated with a not available server:["+ std::to_string(s_id) + "]! (function: ‘schedule_taskset_rt’)";
                        throwRuntimeException(true, glob_path, glob_user, msg, "agent", "schedule_taskset_rt", sim_current_time().dbl());
                    }
                    else if (p_server->is_empty())
                    {
                        p_server->setTaskId(p_task->getTaskId());
                        p_server->setTaskPlanId(p_task->getTaskPlanId());// BDI
                        p_server->setTaskOriginalPlanId(p_task->getTaskOriginalPlanId()); // BDI
                        p_server->setTaskisBeenActivated(p_task->getTaskisBeenActivated()); // BDI
                        p_server->setTaskDemander(p_task->getTaskDemander());
                        p_server->setTaskExecuter(p_task->getTaskExecuter());
                        p_server->setTaskReleaseTime(sim_current_time().dbl());
                        p_server->setTaskArrivalTime(p_task->getTaskArrivalTime());
                        p_server->setTaskDeadLine(p_server->get_curr_ddl());
                        p_server->setTaskStatus(p_task->getTaskStatus()); // BDI
                        p_server->setTaskExecutionType(p_task->getTaskExecutionType()); // BDI
                        p_server->set_startTime(sim_current_time().dbl()); // BDI

                        p_server->push_back(p_task);
                        p_server->reset(sim_current_time().dbl(), p_task->getTaskCompTime(), true);
                        p_server->setTaskDeadLine(p_server->get_curr_ddl());

                        logger->print(Logger::EssentialInfo, "Initialize(Reset) server:["+ std::to_string(p_server->getTaskServer())
                        +"], new curr_ddl: "+ std::to_string(p_server->get_curr_ddl()));

                        // replace the task with the server
                        ag_Sched->ag_replace_task_to_release(p_server);
                    }
                    else {
                        int task_index = ag_support->checkIfTaskAlreadyInServerQueue(p_server, p_task);

                        logger->print(Logger::Default, "task:["+ std::to_string(p_task->getTaskId())
                        +"], plan:["+ std::to_string(p_task->getTaskPlanId())
                        +"], original_plan:["+ std::to_string(p_task->getTaskPlanId())
                        +"], task index in queue: "+ std::to_string(task_index));

                        if(task_index == -1) { // the task is not in the queue yet
                            p_server->push_back(p_task);
                        }
                    }
                }
            }

            logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                    +", ARRIVED tskId:["+ std::to_string(p_task->getTaskId())
            +"], plan:["+ std::to_string(p_task->getTaskPlanId())
            +"], original plan:["+ std::to_string(p_task->getTaskOriginalPlanId())
            +"], NExec left: "+ std::to_string(n_exec)
            +", tskRt: "+ std::to_string(p_task->getTaskReleaseTime()));

            // check if it is periodic
            if (p_task->getTaskPeriod() > 0) {
                double n_arrival_time = sim_current_time().dbl() + p_task->getTaskPeriod();
                np_task = p_task->cloneTask();
                np_task->setTaskStatus(Task::TSK_IDLE); // Added due to shared_ptr
                np_task->setTaskLastActivation(-1);
                np_task->setTaskFirstActivation(-1);
                np_task->setTaskCompTimeRes(p_task->getTaskCompTime());
                np_task->setTaskArrivalTime(n_arrival_time);

                n_exec = p_task->getTaskNExec();
                if (n_exec > 0 || n_exec == -1)
                {
                    logger->print(Logger::EssentialInfo, "(RT) Task:["+ std::to_string(p_task->getTaskId())
                    +"], plan:["+ std::to_string(p_task->getTaskPlanId())
                    +"], original plan:["+ std::to_string(p_task->getTaskOriginalPlanId())
                    +"] is periodic, copied back into the -to-be-released-queue!\n - ArrivalTime: "+ std::to_string(np_task->getTaskArrivalTime())
                    +", DDL: "+ std::to_string(np_task->getTaskDeadline())
                    +", N_exec left:["+ std::to_string(np_task->getTaskNExec())
                    +" of "+ std::to_string(np_task->getTaskNExecInitial()) +"]");

                    ag_Sched->ag_add_task_in_vector_to_release(np_task);
                }
            }
            if (ag_Sched->get_tasks_vector_to_release().empty() == false)
            {
                ag_Sched->ag_sort_tasks_arrival();

                /* (04/05/20)
                 * instead of scheduling the arrival of just the next event,
                 * schedule one for each event with the next arrivalTime.
                 * (es. if t=0 and there are two events that arrives a t=10,
                 * schedule two messages in order to activate them)
                 * If we don't do this, periodic task may still have status=IDLE when it's their time to be executed */
                bool checkNextTask = true;
                std::shared_ptr<Task> task_to_release;

                // skip task 0 because it's already being managed...
                for(int i = 1; i < (int)ag_Sched->get_tasks_vector_to_release().size() && checkNextTask; i++)
                {
                    checkNextTask = false;
                    task_to_release = ag_Sched->get_tasks_vector_to_release()[i];

                    if(fabs(task_to_release->getTaskArrivalTime() - sim_current_time().dbl()) < ag_Sched->EPSILON
                            && task_to_release->getTaskStatus() == Task::TSK_IDLE)
                    {
                        checkNextTask = true;

                        logger->print(Logger::Default, " > Task_to_release:["+ std::to_string(task_to_release->getTaskId())
                        +"], plan:["+ std::to_string(task_to_release->getTaskPlanId())
                        +"] status:["+ task_to_release->getTaskStatus_as_string()
                        +"] force update to:[ACTIVE] has arrivalTime: "+ std::to_string(task_to_release->getTaskArrivalTime()));

                        task_to_release->setTaskStatus(Task::TSK_ACTIVE);
                    }
                }

                simtime_t at_time = convert_double_to_simtime_t(ag_Sched->get_tasks_vector_to_release()[0]->getTaskArrivalTime());
                scheduleAt(at_time, set_next_task_arrival());
                scheduled_msg += 1;
                ag_support->setScheduleInFuture("SCHEDULE - more task in release vector", ag_Sched->get_tasks_vector_to_release()[0]->getTaskArrivalTime()); // METRICS purposes
                logger->print(Logger::EssentialInfo, "(RT) More Task(s) to release in the vector, scheduling next check for arrival");
            }

            // task ddl_miss check at arrival time (skip for servers!)
            double t_ddl;
            if (p_task->getTaskServer() == -1) {
                t_ddl = p_task->getTaskArrivalTime() + p_task->getTaskDeadline();
                p_task->setTaskDeadLine(t_ddl);
            }

            // controlla c_res_head_ready negativo!
            double c_res_head_ready = 0;
            if (ag_Sched->get_tasks_vector_ready().empty() == false) {
                if (ag_Sched->get_tasks_vector_ready()[0]->is_server()) {
                    c_res_head_ready =
                            ag_Sched->get_tasks_vector_ready()[0]->get_head()->getTaskCompTimeRes()
                            - (sim_current_time().dbl() - ag_Sched->get_tasks_vector_ready()[0]->get_head()->getTaskLastActivation());
                } else {
                    c_res_head_ready =
                            ag_Sched->get_tasks_vector_ready()[0]->getTaskCompTimeRes()
                            - (sim_current_time().dbl() - ag_Sched->get_tasks_vector_ready()[0]->getTaskLastActivation());
                }
            }

            bool single_ready_task = ag_Sched->ag_task_arrived(p_server); // CBS

            if (single_ready_task)
            { // only one task in the ready queue so it gets the CPU
                if(p_server == nullptr || p_server->queue_length() == 1)
                {
                    agentMSG* task_activation_msg = set_task_activation();
                    double schedule_msg_time;

                    // --------------------------------------------------------
                    // Added in order to avoid cases where sim_current_time().dbl() returns a value that is slightly in the past (i.e. 9677.2560000000849 but is actually t: 9677.2560000000867)
                    if(p_server == nullptr) {
                        schedule_msg_time = ag_Sched->get_tasks_vector_ready()[0]->getTaskArrivalTime(); // first element is a PERIODIC task
                    }
                    else {
                        schedule_msg_time = p_server->get_ith_task(0)->getTaskArrivalTime(); // first element is a APERIODIC task -> get server's first task
                    }
                    // --------------------------------------------------------

                    simtime_t at_time = convert_double_to_simtime_t(schedule_msg_time);
                    scheduleAt(at_time, task_activation_msg);
                    scheduled_msg += 1;
                    ag_support->setScheduleInFuture("ACTIVATE_TASK - single task ready queue"); // METRICS purposes
                }
            }
            else if (ag_Sched->get_tasks_vector_ready().empty() == false)
            {
                // EDF preemption...more than one task in the ready queue: evaluate possible preemption
                bool preemption_needed = false;
                std::shared_ptr<Task> sorted_first_task;
                std::shared_ptr<Task> initial_first_task = ag_Sched->get_tasks_vector_ready()[0];

                preemption_needed = is_preemption_needed(ag_Sched, c_res_head_ready, sorted_first_task, initial_first_task);

                if(preemption_needed)
                { // Preemption
                    std::shared_ptr<Task> preempted_task = ag_Sched->get_tasks_vector_ready()[1];

                    // update the execution state of both preempted and preempting tasks ---------
                    if(sorted_first_task->is_server()) {
                        sorted_first_task->setTaskStatus(Task::TSK_EXEC);
                        sorted_first_task->get_ith_task()->setTaskStatus(Task::TSK_EXEC);
                    }
                    else {
                        sorted_first_task->setTaskStatus(Task::TSK_EXEC); // task is not the first one to be executed, restore the previous state
                    }
                    if(preempted_task->getTaskStatus() == Task::TSK_EXEC)
                    { /* If the preempted task is NOT TSK_EXEC, it means that it was not already in execution.
                     * Thus, we priorityze the 'sorted_first_task' but preemption is not needed. */
                        if(preempted_task->is_server()) {
                            preempted_task->setTaskStatus(Task::TSK_READY);
                            preempted_task->get_ith_task()->setTaskStatus(Task::TSK_READY);
                        }
                        else {
                            preempted_task->setTaskStatus(Task::TSK_READY); // task is not the first one to be executed, restore the previous state
                        }
                        if(preempted_task->is_server()) {
                            logger->print(Logger::EveryInfo, "After PREEMPTION: Task:"+ std::to_string(preempted_task->getTaskId())
                            +", curr_budget: "+ std::to_string(preempted_task->get_curr_budget())
                            +", curr_ddl: "+ std::to_string(preempted_task->get_curr_ddl()));
                        }
                        scheduleAt(sim_current_time(), set_task_preemption(p_task, preempted_task));
                        scheduled_msg += 1;
                        ag_support->setScheduleInFuture("PREEMPT - multiple tasks in ready queue"); // METRICS purposes
                    }
                    else {
                        scheduleAt(sim_current_time(), set_task_activation());
                        scheduled_msg += 1;
                        ag_support->setScheduleInFuture("ACTIVATE_TASK - single task ready queue"); // METRICS purposes
                    }
                    // ---------------------------------------------------------------------------
                }
                else { // otherwise no preemption, the scheduler keeps going
                    logger->print(Logger::EssentialInfo, "Do we need Preemption? NO, first task:["+ std::to_string(sorted_first_task->getTaskId())
                    +"], original_planid:["+ std::to_string(sorted_first_task->getTaskOriginalPlanId())
                    +"], is server:["+ ag_support->boolToString(sorted_first_task->is_server()) +"] did not changed");
                }
            }
            else if(p_server != nullptr)
            {
                if(ag_Sched->ag_vector_ready_contains_server(p_server) == false)
                {
                    logger->print(Logger::EveryInfo, "P_SERVER NOT IN READY");

                    ag_Sched->ag_add_task_in_ready_tasks_vector(ag_Sched->get_tasks_vector_to_release()[0]);
                    ag_Sched->ag_remove_head_in_release_tasks_vector();

                    bool needs_preemption = false;
                    std::shared_ptr<Task> sorted_first_task;
                    std::shared_ptr<Task> initial_first_task = ag_Sched->get_tasks_vector_ready()[0];

                    needs_preemption = is_preemption_needed(ag_Sched, c_res_head_ready, sorted_first_task, initial_first_task);

                    if(needs_preemption)
                    { // Apply Preemption
                        std::shared_ptr<Task> preempted_task = ag_Sched->get_tasks_vector_ready()[1];

                        // update the execution state of both preempted and preempting tasks ---------
                        if(sorted_first_task->is_server()) {
                            sorted_first_task->setTaskStatus(Task::TSK_EXEC);
                            sorted_first_task->get_ith_task()->setTaskStatus(Task::TSK_EXEC);
                        }
                        else {
                            sorted_first_task->setTaskStatus(Task::TSK_EXEC); // task is not the first one to be executed, restore the previous state
                        }
                        if(preempted_task->getTaskStatus() == Task::TSK_EXEC)
                        { /* If the preempted task is NOT TSK_EXEC, it means that it was not already in execution.
                         * Thus, we priorityze the 'sorted_first_task' but preemption is not needed. */
                            if(preempted_task->is_server()) {
                                preempted_task->setTaskStatus(Task::TSK_READY);
                                preempted_task->get_ith_task()->setTaskStatus(Task::TSK_READY);
                            }
                            else {
                                preempted_task->setTaskStatus(Task::TSK_READY); // task is not the first one to be executed, restore the previous state
                            }
                            if(preempted_task->is_server()) {
                                logger->print(Logger::EveryInfo, "After PREEMPTION: Task:"+ std::to_string(preempted_task->getTaskId())
                                +", curr_budget: "+ std::to_string(preempted_task->get_curr_budget())
                                +", curr_ddl: "+ std::to_string(preempted_task->get_curr_ddl()));
                            }

                            // ---------------------------------------------------------------------------
                            scheduleAt(sim_current_time(), set_task_preemption(p_task, preempted_task));
                            scheduled_msg += 1;
                            ag_support->setScheduleInFuture("PREEMPT - preemption needed"); // METRICS purposes
                        }
                        else {
                            scheduleAt(sim_current_time(), set_task_activation());
                            scheduled_msg += 1;
                            ag_support->setScheduleInFuture("ACTIVATE_TASK - single task ready queue"); // METRICS purposes
                        }
                    }
                    else { // otherwise no preemption, the scheduler keeps going
                        logger->print(Logger::EssentialInfo, "Do we need Preemption? NO, first task:["+ std::to_string(sorted_first_task->getTaskId())
                        +"], original_planid:["+ std::to_string(sorted_first_task->getTaskOriginalPlanId())
                        +"], is server:["+ ag_support->boolToString(sorted_first_task->is_server()) +"] did not changed");
                    }
                }
            }

            logger->print(Logger::EssentialInfo, "Tasks running: "+ std::to_string(ag_Sched->get_tasks_vector_ready().size()));
        }
        else if (sim_current_time().dbl() > ag_Sched->get_tasks_vector_to_release()[0]->getTaskArrivalTime()
                && ag_Sched->get_tasks_vector_to_release()[0]->getTaskCompTimeRes() > 0)
        {
            std::string msg = "Task:["+ std::to_string(ag_Sched->get_tasks_vector_to_release()[0]->getTaskId());
            msg += "], planid:["+ std::to_string(ag_Sched->get_tasks_vector_to_release()[0]->getTaskPlanId());
            msg += "], original_planid:["+ std::to_string(ag_Sched->get_tasks_vector_to_release()[0]->getTaskOriginalPlanId());
            msg += "] to be released has 'arrival time' in the past (at t: "+ std::to_string(ag_Sched->get_tasks_vector_to_release()[0]->getTaskArrivalTime()) +")!";
            logger->print(Logger::Error, "[ERROR] "+ msg);
            ag_metric->addGeneratedError(sim_current_time(), "schedule_taskset_rt", msg);
        }
    }

    // DEBUG *************************************************************************
    logger->print(Logger::EssentialInfo, " ---------- End of function 'schedule_taskset_rt' ----------");
    ag_Sched->ev_ag_tasks_vector_to_release();
    logger->print(Logger::EssentialInfo, "-----------------------------");
    ag_Sched->ev_ag_tasks_vector_ready();
    logger->print(Logger::EssentialInfo, "-----------------------------");
    // *******************************************************************************
}

void agent::check_task_completion_rt(agentMSG* agMsg)
{
    logger->print(Logger::Default, "----check_task_completion_rt----");
    ag_support->setSimulationLastExecution("check_task_completion_rt");
    std::string report_value = "";
    double task_absolute_deadline;
    double server_deadline;
    int index_intention = -1;
    std::string warning_msg = "";

    logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
            +", checking COMPLETION for msg_task_id["+ std::to_string(agMsg->getAg_task_id())
    +"], msg_plan_id["+ std::to_string(agMsg->getAg_task_plan_id())
    +"], msg_original_plan_id["+ std::to_string(agMsg->getAg_task_original_plan_id())
    +"] tskDm "+ std::to_string(agMsg->getAg_task_demander()));

    //--CHECK TASK FINITO--
    if (ag_Sched->get_tasks_vector_ready().empty() == false)
    {
        std::shared_ptr<Task> p_task = ag_Sched->get_tasks_vector_ready()[0];
        std::shared_ptr<Task> p_server = nullptr;

        /* it might have been killed due to ddl_miss so we need the following check
         * or it can be preempted so it didn't finish and we need to check in a second time
         * --------------------------------------------------------------------------------
         * [*] 12/02/21: commented due to the fact that:
         * - t:0 T0,7 original plan 9 is activated. CompTime = 1 and first in ready queue.
         * - t:1 P7 is accomplished (due to sensor)
         * - t:1 check_task_completion_rt is triggered to check if T0,7-9 has been completed.
         * - T0,7-9 has been completed, but it's not called T0,7-9 anymore
         * - when P7 gets satisfied, T0,7-9 becomes T0,9. T
         * - Therefore, we just need to check task_id and original_plan_id. */
        if ((agMsg->getAg_task_id() == p_task->getTaskId())
                && (p_task->getTaskOriginalPlanId() == agMsg->getAg_task_original_plan_id())
                && (p_task->getTaskDemander() == agMsg->getAg_task_demander())
                && (p_task->getTaskReleaseTime() == agMsg->getAg_task_release_time())
                && (p_task->getTaskLastActivation() != -1)
                && (p_task->getTaskLastActivation() < sim_current_time().dbl())
                && p_task->getTaskStatus() == Task::TSK_EXEC)
        {
            logger->print(Logger::EveryInfo, "agMsg_taskID: "+ std::to_string(agMsg->getAg_task_id())
            +" == ptask_id: "+ std::to_string(p_task->getTaskId())
            +"\n\t && agMsg_task_original_planID: "+ std::to_string(agMsg->getAg_task_original_plan_id()) +" == ptask_original_plan: "+ std::to_string(p_task->getTaskOriginalPlanId())
            +"\n\t && agMsg_task_demander: "+ std::to_string(agMsg->getAg_task_demander()) +" == ptask_id_demander: "+ std::to_string(p_task->getTaskDemander())
            +"\n\t && agMsg_task_release: "+ std::to_string(agMsg->getAg_task_release_time()) +" == ptask_release: "+ std::to_string(p_task->getTaskReleaseTime())
            +"\n\t && -1 != ptask_lastActivation: "+ std::to_string(p_task->getTaskLastActivation())
            +"\n\t && now: "+ sim_current_time().str() +" > "+ std::to_string(p_task->getTaskLastActivation())
            +"\n\t && STATUS EXEC == "+ p_task->getTaskStatus_as_string()
            +"\n RESULT: "+ std::to_string((agMsg->getAg_task_id() == p_task->getTaskId())
                    && (p_task->getTaskOriginalPlanId() == agMsg->getAg_task_original_plan_id())
                    && (p_task->getTaskDemander() == agMsg->getAg_task_demander())
                    && (p_task->getTaskReleaseTime() == agMsg->getAg_task_release_time())
                    && (p_task->getTaskLastActivation() != -1)
                    && (p_task->getTaskLastActivation() < sim_current_time().dbl())
                    && p_task->getTaskStatus() == Task::TSK_EXEC));

            if(fabs(lastCompletedTaskTime - sim_current_time().dbl()) > ag_Sched->EPSILON)
            {
                double server_ddl = -1;
                ag_settings->add_ddl_check();

                if(p_task->is_server()) {
                    p_server = p_task;
                    server_ddl = p_server->get_curr_ddl();
                    p_task = p_server->get_head();
                }

                double t_CRes = p_task->getTaskCompTimeRes();
                double last_computation = sim_current_time().dbl() - p_task->getTaskLastActivation();
                double comp_time_res = t_CRes - last_computation;

                if (comp_time_res < 0 && p_task->getTaskStatus() != Task::TSK_EXEC)
                {
                    warning_msg = "[WARNING] COMPLETION CHECK > schedule task activation > Task:["+ std::to_string(p_task->getTaskId());
                    warning_msg += "], plan:["+ std::to_string(p_task->getTaskPlanId());
                    warning_msg += "], original_plan:["+ std::to_string(p_task->getTaskOriginalPlanId());
                    warning_msg += "], t_CRes: "+ std::to_string(t_CRes);
                    warning_msg += ", last_computation: "+ std::to_string(comp_time_res);
                    warning_msg += ", comp_time_res: "+ std::to_string(last_computation);
                    logger->print(Logger::Warning, warning_msg);
                    ag_metric->addGeneratedWarning(sim_current_time(), "check_task_completion_rt", warning_msg);

                    scheduleAt(sim_current_time(), set_task_activation());
                    scheduled_msg += 1;
                    ag_support->setScheduleInFuture("ACTIVATE_TASK - task has more compTimeRes"); // METRICS purposes
                }
                else if (fabs(comp_time_res) < ag_Sched->EPSILON)
                {
                    p_task->setTaskStatus(Task::TSK_COMPLETED);
                    lastCompletedTaskTime = sim_current_time().dbl(); // update the instant when the last task has been completed.

                    if(p_server != nullptr)
                    {
                        server_deadline = p_server->get_curr_ddl();
                        logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                                +", SERVER:["+ std::to_string(p_server->getTaskServer())
                        +"] COMPLETED tskId:["+ std::to_string(p_task->getTaskId())
                        +"], plan:["+ std::to_string(p_task->getTaskPlanId())
                        +"], original_id:["+ std::to_string(p_task->getTaskOriginalPlanId())
                        +"], tskRt: "+ std::to_string(p_task->getTaskReleaseTime())
                        +", budget: "+ std::to_string(p_server->get_curr_budget())
                        +", deadline: "+ std::to_string(p_server->get_curr_ddl())
                        +", status:["+ p_server->getTaskStatus_as_string() +"]");

                        report_value = "t: "+ sim_current_time().str();
                        report_value += ", SERVER:["+ std::to_string(p_server->getTaskServer());
                        report_value += "] COMPLETED tskId:["+ std::to_string(p_task->getTaskId());
                        report_value += "], plan:["+ std::to_string(p_task->getTaskPlanId());
                        report_value += "], original_id:["+ std::to_string(p_task->getTaskOriginalPlanId());
                        report_value += "], deadline:["+ std::to_string(p_server->get_curr_ddl()) +"]";
                    }
                    else {
                        logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                                +", COMPLETED tskId:["+ std::to_string(p_task->getTaskId())
                        +"], plan:["+ std::to_string(p_task->getTaskPlanId())
                        +"], original_id:["+ std::to_string(p_task->getTaskOriginalPlanId())
                        +"], N_exec left:["+ std::to_string(p_task->getTaskNExec())
                        +" of "+ std::to_string(p_task->getTaskNExecInitial())
                        +"], tskRt: "+ std::to_string(p_task->getTaskReleaseTime())
                        +", status:["+ p_task->getTaskStatus_as_string() +"]");

                        report_value = "t: "+ sim_current_time().str();
                        report_value += " COMPLETED tskId:["+ std::to_string(p_task->getTaskId());
                        report_value += "], plan:["+ std::to_string(p_task->getTaskPlanId());
                        report_value += "], original_id:["+ std::to_string(p_task->getTaskOriginalPlanId());
                        report_value += "], deadline:["+ std::to_string(p_task->getTaskDeadline()) +"]";

                        // decrease the n_exec parameter of p_task in the intention that contains it
                        ag_support->updatePeriodicTasksInsideIntention(p_task, ag_intention_set);
                    }

                    // check if the completed tasks was unexpected -------------------------------------------------------
                    ag_metric->checkIfUnexpectedTaskExecution(p_task->makeCopyOfTask(), ag_intention_set);
                    // ---------------------------------------------------------------------------------------------------

                    //remove head on the ordered vector
                    ag_Sched->ag_remove_head_in_ready_tasks_vector();

                    if (p_server != nullptr)
                    { // SERVERS : task completed
                        logger->print(Logger::EveryInfo, "Updating budget to "+ 
                                std::to_string(p_server->get_curr_budget() - last_computation));

                        p_server->update_budget(last_computation);
                        logger->print(Logger::EveryInfo, "POP HEAD: task:["+ std::to_string(p_server->get_head()->getTaskId())
                        +"], plan:["+ std::to_string(p_server->get_head()->getTaskPlanId()) +"]");

                        p_server->pop_head();
                        if(p_server->get_server_type() == CBS) {
                            p_server->reset(sim_current_time().dbl());

                            if(ag_support->checkIfDoublesAreEqual(p_server->get_curr_ddl(), server_deadline, ag_Sched->EPSILON) == false) {
                                logger->print(Logger::EssentialInfo, " -- RESET SERVER:["+ std::to_string(p_server->getTaskServer())
                                +"], from:["+ std::to_string(server_deadline) +"] TO CURR_DDL:["+ std::to_string(p_server->get_curr_ddl()) +"] --");
                            }
                        }

                        int n_exec = p_task->getTaskNExec();
                        if (n_exec != -1) { //note: added for repeated services
                            n_exec -= 1;
                            if (n_exec != 0 && n_exec != -1 && p_task->get_period() != 0) {
                                logger->print(Logger::EssentialInfo, "Service executions left: "+ std::to_string(n_exec));
                                p_task->setTaskNExec(n_exec);
                                p_server->push_back(p_task);
                            }
                        }
                        if (p_server->is_empty() == false)
                        {
                            p_server->setTaskId(p_server->get_head()->getTaskId());
                            p_server->setTaskPlanId(p_server->get_head()->getTaskPlanId()); // BDI
                            p_server->setTaskOriginalPlanId(p_server->get_head()->getTaskOriginalPlanId()); // BDI
                            p_server->setTaskisBeenActivated(p_server->get_head()->getTaskisBeenActivated()); // BDI
                            p_server->setTaskDemander(p_server->get_head()->getTaskDemander());
                            p_server->setTaskExecuter(p_server->get_head()->getTaskExecuter());
                            p_server->setTaskReleaseTime(p_server->get_head()->getTaskReleaseTime());
                            p_server->setTaskDeadLine(p_server->get_curr_ddl());
                            p_server->setTaskArrivalTime(sim_current_time().dbl());

                            /* before adding the server back in the RELEASE queue, let's update its Status
                             * and the one of the head task in its queue.
                             * Setting both to "ACTIVE" allows schedule_taskset_rt(..) to update them back to READY
                             * as soon as the server is moved back in the ready queue */
                            p_server->setTaskStatus(Task::TSK_ACTIVE); // added: 04/03/21
                            p_server->get_head()->setTaskStatus(Task::TSK_ACTIVE); // added: 04/03/21

                            ag_Sched->ag_add_task_in_vector_to_release(p_server);

                            // WRITE utilization report
                            write_util_json_report(p_task, getIndex(), sim_current_time().dbl(), ag_Sched->get_current_util(ag_Server_Handler, compute_MsgUtil_sever()), "server-activation");

                            scheduleAt(sim_current_time(), set_next_task_arrival());
                            scheduled_msg += 1;
                            ag_support->setScheduleInFuture("SCHEDULE - task completed (i-th exec)"); // METRICS purposes
                        }
                    }

                    double lateness = sim_current_time().dbl() - p_task->getTaskDeadline();
                    task_absolute_deadline = p_task->getTaskDeadline();

                    if(p_server == nullptr) {
                        logger->print(Logger::Default, " > Lateness:["+ std::to_string(lateness)
                        +"] = "+ sim_current_time().str() +" - "+ std::to_string(p_task->getTaskDeadline())
                        +" -> was_server: FALSE");
                    }
                    else {
                        lateness = sim_current_time().dbl() - server_ddl;
                        task_absolute_deadline = server_ddl;

                        logger->print(Logger::Default, " > Lateness:["+ std::to_string(lateness)
                        +"] = "+ sim_current_time().str() + " - " + std::to_string(server_ddl)
                        +" -> was_server: TRUE -> now: "+ sim_current_time().str()
                        +",\n budget: "+ std::to_string(p_server->get_curr_budget())
                        +", curr_ddl: "+ std::to_string(p_server->get_curr_ddl())
                        +", first_activation: "+ std::to_string(p_server->getTaskFirstActivation())
                        +", absDDL: "+ std::to_string(p_task->getTaskAbsDDL())
                        +", release_time: "+ std::to_string(p_task->getTaskReleaseTime())
                        +", ddl: "+ std::to_string(p_task->getTaskDeadline())
                        +", curr_ddl: "+ std::to_string(p_server->get_curr_ddl())
                        +", ddl used: "+ std::to_string(server_ddl)
                        +", period: "+ std::to_string(p_server->get_period())
                        +" -> now - (FirstAct + ddl) > 0 ? -> "+ std::to_string(lateness > 0));
                    }

                    report_value += ", lateness:["+ std::to_string(lateness) +"]";
                    if (lateness > 0) {
                        report_value += ", ddl_miss:[YES]";

                        // Write lateness.json report file
                        int agent_id = getIndex();
                        write_lateness_json_report(p_task, agent_id, lateness, sim_current_time().dbl());

                        ag_settings->add_ddl_miss();
                        ag_tasks_lateness.push_back(std::make_shared<Lateness>(sim_current_time().dbl(), p_task->getTaskId(), p_task->getTaskPlanId(), p_task->getTaskOriginalPlanId(), p_task->getTaskNExec(), lateness, report_value));

                        logger->print(Logger::Default, "EDF > Lateness:["+ std::to_string(lateness)
                        +"] -> Task:["+ std::to_string(p_task->getTaskId())
                        +"], plan_id["+ std::to_string(p_task->getTaskPlanId())
                        +"], original_plan_id["+ std::to_string(p_task->getTaskOriginalPlanId())
                        +"] now: "+ sim_current_time().str()
                        +", tskArrivalTime:["+ std::to_string(p_task->getTaskArrivalTime())
                        +"], tskReleaseTime:["+ std::to_string(p_task->getTaskReleaseTime())
                        +"], tskCompTime:["+ std::to_string(p_task->getTaskCompTime())
                        +"] and tskDDL:["+ std::to_string(p_task->getTaskDeadline()) +"]");
                    }
                    else {
                        report_value += ", ddl_miss:[NO]";
                    }

                    // WRITING TO FILE -------------------------------------------------------------------------
                    double resp_time = sim_current_time().dbl() - p_task->getTaskReleaseTime();

                    logger->print(Logger::EssentialInfo, " > RESP_TIME:["+ std::to_string(sim_current_time().dbl() - p_task->getTaskReleaseTime())
                    +"] = "+ sim_current_time().str()
                    +" - "+ std::to_string(p_task->getTaskReleaseTime()));

                    // write simulation events report
                    write_simulation_timeline_json_report(report_value);

                    // write ddl_check
                    write_ddl_checks(agMsg->getAg_task_plan_id(), agMsg->getAg_task_original_plan_id(), agMsg->getAg_task_id(), getIndex());

                    // write response time report
                    write_response_json_report(p_task, getIndex(), sim_current_time().dbl(), resp_time);
                    ag_metric->addTaskResponseTime(p_task, sim_current_time().dbl(), resp_time);

                    // write response time per task report
                    write_json_resp_per_task(p_task, getIndex(), sim_current_time().dbl(), resp_time);

                    // write utilization report
                    write_util_json_report(p_task, getIndex(), sim_current_time().dbl(), ag_Sched->get_current_util(ag_Server_Handler), "task completed");

                    /* Update intention 'current_scheduled_completion_time' */
                    if(ag_support->checkIfTaskIsLinkedToIntention(p_task, ag_intention_set, index_intention)) {
                        // Update and set the MAX scheduled comp time
                        if(task_absolute_deadline > ag_intention_set[index_intention]->get_current_completion_time()) {
                            logger->print(Logger::EssentialInfo, " - Task:["+ std::to_string(p_task->getTaskId())
                            +"], Plan:["+ std::to_string(p_task->getTaskPlanId())
                            +"], original plan:["+ std::to_string(p_task->getTaskOriginalPlanId())
                            +"] has deadline:["+ std::to_string(task_absolute_deadline)
                            +"] greater than ABSOLUTE ddl:["+ std::to_string(ag_intention_set[index_intention]->get_current_completion_time()) +"] -> updated absolute");
                            ag_intention_set[index_intention]->set_current_scheduled_completion_time(task_absolute_deadline);
                        }
                        else {
                            // the last completed task has a lower deadline then previously performed ones...
                            logger->print(Logger::EssentialInfo, " - Task:["+ std::to_string(p_task->getTaskId())
                            +"], Plan:["+ std::to_string(p_task->getTaskPlanId())
                            +"], original plan:["+ std::to_string(p_task->getTaskOriginalPlanId())
                            +"] has deadline:["+ std::to_string(task_absolute_deadline)
                            +"] lower than ABSOLUTE ddl:["+ std::to_string(ag_intention_set[index_intention]->get_current_completion_time()) +"] -> keep absolute");
                            task_absolute_deadline = ag_intention_set[index_intention]->get_current_completion_time();
                        }
                    }

                    /* BDI: this means that the execution of p_task has been performed by the same agent that needed it ...
                     * .. if the task has no more executions left, notify the agent */
                    if(p_task->getTaskNExec() == 0)
                    {   // directly call function that removes the accomplished task
                        ag_intention_task_completed(set_intention_task_completed(p_task, task_absolute_deadline));
                    }

                    // schedule next activation
                    if (ag_Sched->get_tasks_vector_ready().empty() == false) {
                        p_task = ag_Sched->get_tasks_vector_ready()[0];
                        scheduleAt(sim_current_time(), set_task_activation());
                        scheduled_msg += 1;
                        ag_support->setScheduleInFuture("ACTIVATE_TASK - schedule next action"); // METRICS purposes
                    }

                    logger->print(Logger::Debug, "----DEBUG: inhere----");
                }
                else if (p_server != nullptr)
                { // SERVERS: aperiodic task - not completed
                    logger->print(Logger::Default, "APERIODIC: SERVER IS NOT NULL: fabs("+ std::to_string(p_server->get_curr_budget())
                    +" - "+ std::to_string(last_computation)
                    +") < "+ std::to_string(ag_Sched->EPSILON)
                    +" => "+ ag_support->boolToString(fabs(p_server->get_curr_budget() - last_computation) < ag_Sched->EPSILON));

                    double next_release = sim_current_time().dbl();
                    server_deadline = p_server->get_curr_ddl();

                    // if the budget has expired
                    if (fabs(p_server->get_curr_budget() - last_computation) < ag_Sched->EPSILON)
                    {
                        ag_Sched->ag_remove_head_in_ready_tasks_vector();
                        p_server->update_budget(last_computation);
                        if (p_server->get_server_type() == CBS) {
                            p_server->reset(sim_current_time().dbl());

                            if(ag_support->checkIfDoublesAreEqual(p_server->get_curr_ddl(), server_deadline, ag_Sched->EPSILON) == false)
                            {
                                logger->print(Logger::EssentialInfo, " -- RESET SERVER:["+ std::to_string(p_server->getTaskServer())
                                +"], from:["+ std::to_string(server_deadline) +"] TO CURR_DDL:["+ std::to_string(p_server->get_curr_ddl()) +"] --");
                            }
                        }
                        p_server->setTaskDeadLine(p_server->get_curr_ddl());
                        p_task->setTaskCompTimeRes(comp_time_res);
                        p_task->setTaskLastActivation(sim_current_time().dbl());
                        p_server->setTaskLastActivation(sim_current_time().dbl());

                        // Server needs to go back to release queue!
                        p_server->setTaskArrivalTime(next_release);
                        ag_Sched->ag_add_task_in_vector_to_release(p_server);

                        // write utilization report
                        write_util_json_report(p_task, getIndex(), sim_current_time().dbl(), ag_Sched->get_current_util(ag_Server_Handler, compute_MsgUtil_sever()), "server-release");

                        scheduleAt(sim_current_time(), set_next_task_arrival());
                        scheduled_msg += 1;
                        ag_support->setScheduleInFuture("SCHEDULE - task expired"); // METRICS purposes

                        logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                                +", SERVER:["+ std::to_string(p_server->getTaskServer())
                        +"] EXPIRED tskId:["+ std::to_string(p_task->getTaskId())
                        +"] plan:["+ std::to_string(p_task->getTaskPlanId())
                        +"] original_plan:["+ std::to_string(p_task->getTaskOriginalPlanId())
                        +"] tskDm: ag["+ std::to_string(p_task->getTaskDemander())
                        +"] tskEx: ag["+ std::to_string(p_task->getTaskExecuter())
                        +"] tskRt: "+ std::to_string(p_task->getTaskReleaseTime()));

                        report_value = "t: "+ sim_current_time().str();
                        report_value += ", SERVER:["+ std::to_string(p_server->getTaskServer());
                        report_value += "] EXPIRED tskId:["+ std::to_string(p_task->getTaskId());
                        report_value += "] plan:["+ std::to_string(p_task->getTaskPlanId());
                        report_value += "] original_plan:["+ std::to_string(p_task->getTaskOriginalPlanId());
                        +"], curr_budget:["+ std::to_string(p_server->get_curr_budget());
                        report_value += "], budget:["+ std::to_string(p_server->get_budget());
                        report_value += "], task_res_compTime:["+ std::to_string(p_task->getTaskCompTimeRes()) +"]";

                        // check if the completed tasks was unexpected -------------------------------------------------------
                        ag_metric->checkIfUnexpectedTaskExecution(p_task->makeCopyOfTask(), ag_intention_set);
                        // ---------------------------------------------------------------------------------------------------

                        // write simulation events report
                        write_simulation_timeline_json_report(report_value);

                        scheduleAt(sim_current_time(), set_task_activation());
                        scheduled_msg += 1;
                        ag_support->setScheduleInFuture("ACTIVATE_TASK - task expired"); // METRICS purposes
                        logger->print(Logger::Debug, "----DEBUG: inhere----");
                    }
                }
            }
            else {
                std::string msg = " t: "+ sim_current_time().str();
                msg += ", tskId:["+ std::to_string(p_task->getTaskId());
                msg += "], plan:["+ std::to_string(p_task->getTaskPlanId());
                msg += "], original_id:["+ std::to_string(p_task->getTaskOriginalPlanId());
                msg += "] was supposed to complete now, but it's been PREEMPTED by a task that completes before this instant.";
                logger->print(Logger::Error, "[ERROR] "+ msg);
                ag_metric->addGeneratedError(sim_current_time(), "check_task_completion_rt", msg);
            }
        }
    }
    else {
        logger->print(Logger::EssentialInfo, " - ready vector is empty");
    }

    // DEBUG *************************************************************************
    //    ag_support->printIntentions(ag_intention_set, ag_goal_set);
    logger->print(Logger::Default, "-----------------------------");
    ag_Sched->ev_ag_tasks_vector_to_release();
    logger->print(Logger::Default, "-----------------------------");
    ag_Sched->ev_ag_tasks_vector_ready();
    logger->print(Logger::Default, "-----------------------------");
    // *******************************************************************************
}

// *****************************************************************************
// *******                      BEHAVIOURS-RT+GP                         *******
// *****************************************************************************
void agent::activate_task(agentMSG* agMsg) {
    logger->print(Logger::Default, "----activate_task----");
    ag_support->setSimulationLastExecution("activate_task");

    //    // DEBUG *************************************************************************
    //    //    ag_support->printIntentions(ag_intention_set, ag_goal_set);
    //    logger->print(Logger::Default, "-----------------------------");
    //    ag_Sched->ev_ag_tasks_vector_to_release();
    //    logger->print(Logger::Default, "-----------------------------");
    //    ag_Sched->ev_ag_tasks_vector_ready();
    //    logger->print(Logger::Default, "-----------------------------");
    //    // *******************************************************************************


    // --ACTIVATE TASK--
    if (ag_Sched->get_tasks_vector_ready().empty() == false)
    {
        std::shared_ptr<Task> new_task;
        std::shared_ptr<Task> p_server = nullptr;
        new_task = ag_Sched->get_tasks_vector_ready()[0];
        new_task->setTaskStatus(Task::TSK_EXEC); // update the status of the task

        // server
        if (new_task->is_server())
        {
            p_server = new_task;
            new_task = p_server->get_head();
            new_task->setTaskStatus(Task::TSK_EXEC); // update the first task in server's queue

            logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                    +", SERVER:["+ std::to_string(p_server->getTaskServer())
            +"] ACTIVATED tskId:["+ std::to_string(new_task->getTaskId())
            +"], plan:["+ std::to_string(new_task->getTaskPlanId())
            +"], original_plan:["+ std::to_string(new_task->getTaskOriginalPlanId())
            +"] tskRt: "+ std::to_string(new_task->getTaskReleaseTime())
            +" currDdl: "+ std::to_string(p_server->get_curr_ddl())
            +" budget server:["+ std::to_string(p_server->get_curr_budget())
            +"], C_Res: "+ std::to_string(new_task->getTaskCompTimeRes())
            +" status:["+ new_task->getTaskStatus_as_string() +"]"
            +" head task status:["+ p_server->getTaskStatus_as_string() +"]");
        }
        else {
            logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                    +", ACTIVATED tskId:["+ std::to_string(new_task->getTaskId())
            +"], plan:["+ std::to_string(new_task->getTaskPlanId())
            +"], original_plan:["+ std::to_string(new_task->getTaskOriginalPlanId())
            +"] tskRt:["+ std::to_string(new_task->getTaskReleaseTime())
            +"], C_Res: "+ std::to_string(new_task->getTaskCompTimeRes())
            +" status:["+ new_task->getTaskStatus_as_string() +"]");
        }

        /* t_comp is assigned according to the scheduling algorithm */
        double t_comp;
        if (ag_Sched->get_sched_type() == RR) {
            const double QUANTUM = par("quantum");
            t_comp = std::min(QUANTUM + sim_current_time().dbl(), new_task->getTaskCompTimeRes() + sim_current_time().dbl());
        }
        else if(p_server != nullptr) { // CBS
            t_comp = std::min(p_server->get_curr_budget() + sim_current_time().dbl(), new_task->getTaskCompTimeRes() + sim_current_time().dbl());
        }
        else {
            t_comp = sim_current_time().dbl() + new_task->getTaskCompTimeRes();
        }

        // scheduling task completion check -----------------------------------------
        cancel_next_redundant_msg(msg_name_check_task_complete, new_task->getTaskId(), new_task->getTaskPlanId(), new_task->getTaskOriginalPlanId());

        simtime_t at_time = convert_double_to_simtime_t(t_comp);
        scheduleAt(at_time, set_check_task_complete(new_task, t_comp));
        scheduled_msg += 1;
        ag_support->setScheduleInFuture("CHECK_TASK_TERMINATED - check completion of activated task", t_comp); // METRICS purposes
        // ---------------------------------------------------------------------------

        logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                +", scheduled check for completion task:["+ std::to_string(new_task->getTaskId())
        +"] plan:["+ std::to_string(new_task->getTaskPlanId()) +"] original_plan:["+ std::to_string(new_task->getTaskOriginalPlanId())
        +"] at t: "+ std::to_string(t_comp));

        // if task's first activation
        if (new_task->getTaskFirstActivation() == -1) {
            new_task->setTaskFirstActivation(sim_current_time().dbl());
            new_task->setTaskCompTimeRes(new_task->getTaskCompTime());
            if(p_server != nullptr) {
                p_server->setTaskFirstActivation(sim_current_time().dbl());
            }
        }
        if (p_server != nullptr) {
            p_server->setTaskLastActivation(sim_current_time().dbl());
            new_task->setTaskLastActivation(sim_current_time().dbl());
            ag_Sched->update_active_task_in_sorted_tasks_vector(p_server);
        } else {
            new_task->setTaskLastActivation(sim_current_time().dbl());
            ag_Sched->update_active_task_in_sorted_tasks_vector(new_task);
        }
    }
}

void agent::preempt(agentMSG* agMsg) { // --- PREEMPTION ---
    logger->print(Logger::Default, "---preempt---");
    ag_support->setSimulationLastExecution("preempt");

    // DEBUG *************************************************************************
    // ag_support->printIntentions(ag_intention_set, ag_goal_set);
    // logger->print(Logger::Default, "-----------------------------");
    // ag_Sched->ev_ag_tasks_vector_to_release();
    // logger->print(Logger::Default, "-----------------------------");
    // ag_Sched->ev_ag_tasks_vector_ready();
    // logger->print(Logger::Default, "-----------------------------");
    // ******************************************************

    std::string report_value = "";
    double server_deadline;
    std::shared_ptr<Task> running_task = ag_Sched->get_tasks_vector_ready()[0];

    if (agMsg->getAg_task_id() == running_task->getTaskId()
            && agMsg->getAg_task_demander() == running_task->getTaskDemander()
            && agMsg->getAg_task_release_time() == running_task->getTaskReleaseTime())
    {
        std::shared_ptr<Task> preempted_task;
        std::shared_ptr<Task> p_server = nullptr;
        preempted_task = ag_Sched->get_tasks_vector_ready()[1];

        if(preempted_task != nullptr) {
            if(preempted_task->is_server()) {
                p_server = preempted_task;
                preempted_task = p_server->get_head();
                server_deadline = preempted_task->get_curr_ddl();
            }

            report_value = "t: "+ sim_current_time().str();
            report_value += " PREEMPTION task:["+ std::to_string(running_task->getTaskId());
            report_value += "], plan:["+ std::to_string(running_task->getTaskPlanId());
            report_value += "], original_plan:["+ std::to_string(running_task->getTaskOriginalPlanId());
            report_value += "] status:["+ running_task->getTaskStatus_as_string();
            report_value += "] compTimeRes:["+ std::to_string(running_task->getTaskCompTimeRes());
            report_value += "] tskDDL:["+ std::to_string(running_task->getTaskDeadline());
            report_value += "] SERVER: "+ std::to_string(running_task->getTaskServer());
            report_value += "\n PREEMPTS tskId:["+ std::to_string(preempted_task->getTaskId());
            report_value += "], plan:["+ std::to_string(preempted_task->getTaskPlanId());
            report_value += "], original_plan:["+ std::to_string(preempted_task->getTaskOriginalPlanId());
            report_value += "] status:["+ preempted_task->getTaskStatus_as_string();
            report_value += "] compTimeRes(original):["+ std::to_string(preempted_task->getTaskCompTimeRes());
            report_value += "] tskDDL:["+ std::to_string(preempted_task->getTaskDeadline());
            report_value += "] SERVER: "+ std::to_string(preempted_task->getTaskServer());

            logger->print(Logger::EssentialInfo, report_value);
            // write simulation events report
            write_simulation_timeline_json_report(report_value);

            // Update budget/residual_comp_time of the preempted task --------------------------------
            double t_CRes = preempted_task->getTaskCompTimeRes();
            double last_computation = 0;
            double n_t_CRes = t_CRes;
            bool updatePreemptedTask = false;
            if(ag_Sched->get_sched_type() == EDF)
            {
                if(preempted_task->getTaskStatus() == Task::TSK_READY)
                {   // TSK_READY: Because if we have preemption, it means that schedule_taskset_rt has been called
                    // and function 'is_preemption_needed' has returned true. Thus, preempted_task has been changed to READY
                    // in order to prioritize the execution of the new task
                    // --------------------------------------------------------------------------------
                    updatePreemptedTask = true;
                }
            }
            else if(ag_Sched->get_sched_type() == RR && preempted_task->getTaskStatus() == Task::TSK_EXEC) {
                updatePreemptedTask = true;
            }

            if(updatePreemptedTask)
            {
                if (preempted_task->getTaskFirstActivation() != -1) {
                    last_computation = sim_current_time().dbl() - preempted_task->getTaskLastActivation();
                }
                if (last_computation > 0)
                {
                    if(last_computation <= t_CRes)
                    {
                        preempted_task->setTaskCompTimeRes(t_CRes - last_computation);
                        n_t_CRes = t_CRes - last_computation;

                        if(p_server != nullptr)
                        { // server
                            p_server->update_budget(last_computation);

                            if (fabs(p_server->get_curr_budget()) < ag_Sched->EPSILON) {
                                logger->print(Logger::EssentialInfo, "Server.budget = 0 -> reset it!");
                                p_server->reset(sim_current_time().dbl());

                                if(ag_support->checkIfDoublesAreEqual(p_server->get_curr_ddl(), server_deadline, ag_Sched->EPSILON) == false) {
                                    logger->print(Logger::EssentialInfo, " -- RESET SERVER:["+ std::to_string(p_server->getTaskServer())
                                    +"], from:["+ std::to_string(server_deadline) +"] TO CURR_DDL:["+ std::to_string(p_server->get_curr_ddl()) +"] --");
                                }
                            }
                        }
                    }
                }
            }
            // ---------------------------------------------------------------------------------------

            if (p_server != nullptr) {
                report_value = "t: "+ sim_current_time().str();
                report_value += ", SERVER:["+ std::to_string(p_server->getTaskServer());
                report_value += "] PREEMPTED tskId:["+ std::to_string(preempted_task->getTaskId());
                report_value += "], plan:["+ std::to_string(preempted_task->getTaskPlanId());
                report_value += "], original_plan:["+ std::to_string(preempted_task->getTaskOriginalPlanId());
                report_value += "], updated status:["+ p_server->getTaskStatus_as_string();
                report_value += "], releaseTime:["+ std::to_string(preempted_task->getTaskReleaseTime());
                report_value += "], compTimeRes(updated):["+ std::to_string(n_t_CRes);
                report_value += "], ddl: "+ std::to_string(preempted_task->getTaskDeadline());
                report_value += ", budget: "+ std::to_string(p_server->get_curr_budget());
                report_value += ", lastAct: "+ std::to_string(preempted_task->getTaskLastActivation());

                logger->print(Logger::EssentialInfo, report_value);
                if(p_server->get_head()->getTaskCompTimeRes() < ag_Sched->EPSILON) {
                    checkIfPreemptedTaskIsCompleted(p_server, ag_Sched);
                }
                // ---------------------
            }
            else {
                report_value = "t: "+ sim_current_time().str();
                report_value += ", PREEMPTED tskId:["+ std::to_string(preempted_task->getTaskId());
                report_value += "], plan:["+ std::to_string(preempted_task->getTaskPlanId());
                report_value += "], original_plan:["+ std::to_string(preempted_task->getTaskOriginalPlanId());
                report_value += "], updated status:["+ preempted_task->getTaskStatus_as_string();
                report_value += "], releaseTime:["+ std::to_string(preempted_task->getTaskReleaseTime());
                report_value += "], compTimeRes(updated):["+ std::to_string(n_t_CRes);
                report_value += "], ddl: "+ std::to_string(preempted_task->getTaskDeadline());
                report_value += ", lastAct: "+ std::to_string(preempted_task->getTaskLastActivation());

                logger->print(Logger::EssentialInfo, report_value);
                if(preempted_task->getTaskCompTimeRes() < ag_Sched->EPSILON) {
                    checkIfPreemptedTaskIsCompleted(preempted_task, ag_Sched);
                }
                // ---------------------
            }

            // write simulation events report
            write_simulation_timeline_json_report(report_value);

            // schedule preempting task exec
            scheduleAt(sim_current_time(), set_task_activation());
            scheduled_msg += 1;
            ag_support->setScheduleInFuture("ACTIVATE_TASK - schedule preempted task"); // METRICS purposes
        }
        else {
            // preemption was not needed, active next task...
            scheduleAt(sim_current_time(), set_task_activation());
            scheduled_msg += 1;
            ag_support->setScheduleInFuture("ACTIVATE_TASK - preemption not needed, activate next task"); // METRICS purposes
        }

    }
    else { // EV << "MAGIA" << endl;
        /* Preemption was scheduled to preempt: x and start executing y.
         * However, another task/server has been activated and place in READY as head element.
         * Therefore, the two tasks that scheduled preemption are doesn't need it anymore.
         * Note: The execution should not be affected by this scenario, the agent should correctly manage this scenario. */
        std::string msg = "Something unknown went wrong during preemption!";
        logger->print(Logger::Error, "[ERROR] "+ msg);
        ag_metric->addGeneratedError(sim_current_time(), "preempt", msg);
    }

    // DEBUG *************************************************************************
    //    //        ag_support->printIntentions(ag_intention_set, ag_goal_set);
    //    logger->print(Logger::Default, "-----------------------------");
    ag_Sched->ev_ag_tasks_vector_to_release();
    logger->print(Logger::Default, "-----------------------------");
    ag_Sched->ev_ag_tasks_vector_ready();
    logger->print(Logger::Default, "-----------------------------");
    // ******************************************************
}

bool agent::is_preemption_needed(std::shared_ptr<Ag_Scheduler> ag_Sched, const double c_res_head_ready,
        std::shared_ptr<Task>& sorted_first_task, std::shared_ptr<Task>& initial_first_task)
{
    bool preemption_needed = false;
    logger->print(Logger::EveryInfo, "Check if correctly sorted");
    logger->print(Logger::EveryInfo, "CRes head ready: "+ std::to_string(c_res_head_ready));

    // PRINT ready vector to console...
    if(logger->get_level() > Logger::EveryInfo) {
        ag_Sched->ev_ag_tasks_vector_ready();
    }

    logger->print(Logger::EveryInfo, "PRE-sorting: Head task:["+ std::to_string(initial_first_task->getTaskId())
    +"], plan_id:["+ std::to_string(initial_first_task->getTaskPlanId())
    +"], original_planid:["+ std::to_string(initial_first_task->getTaskOriginalPlanId())
    +"], SERVER: "+ std::to_string(initial_first_task->getTaskServer())
    +", task ddl: "+ std::to_string(initial_first_task->getTaskDeadline()));

    // sort ready vector according to algorithm
    ag_Sched->ag_sort_tasks_ddl(c_res_head_ready);

    sorted_first_task = ag_Sched->get_tasks_vector_ready()[0];
    logger->print(Logger::EveryInfoPlus, "sorted_first_task:["+ std::to_string(sorted_first_task->getTaskId())
    +"], plan_id:["+ std::to_string(sorted_first_task->getTaskPlanId())
    +"], original_planid:["+ std::to_string(sorted_first_task->getTaskOriginalPlanId())
    +"], SERVER: "+ std::to_string(sorted_first_task->getTaskServer())
    +", ddl: "+ std::to_string(sorted_first_task->getTaskDeadline()));

    logger->print(Logger::EveryInfo, "Ordinato");
    ag_Sched->ev_ag_tasks_vector_ready();

    /* CHECK IF PREEMPTION IS ACTUALLY NEEDED.....
     * Note: tasks belonging to the same server NEVER generates preemption, they will simply by push_back in the queue.
     * Even if belonging to different plans. The reason is simply, same server > same [budget, period] > same ddl > no preemption */
    if(initial_first_task->getTaskOriginalPlanId() != sorted_first_task->getTaskOriginalPlanId()) {
        logger->print(Logger::EveryInfo, "PREEMPTION_case: different PLAN ID");
        preemption_needed = true;
    }
    else if(initial_first_task->getTaskId() != sorted_first_task->getTaskId()) { // same Plan, different task
        logger->print(Logger::EveryInfo, "PREEMPTION_case: different TASK ID");
        preemption_needed = true;
    }
    else if(initial_first_task->getTaskReleaseTime() != sorted_first_task->getTaskReleaseTime())
    {   // same plan, same task id, different release time (?)
        // non dovrebbe mai entrare qui
        logger->print(Logger::EveryInfo, "PREEMPTION_case: different RELEASE TIME");
        preemption_needed = true;
    }
    else if(initial_first_task->getTaskDemander() != sorted_first_task->getTaskDemander())
    {   // same everything, different demander
        // with just one agent it never enters this case...
        logger->print(Logger::EveryInfo, "PREEMPTION_case: different DEMANDER");
        preemption_needed = true;
    }

    logger->print(Logger::EveryInfo, "Head task id:["+ std::to_string(initial_first_task->getTaskId())
    +"], plan_id:["+ std::to_string(initial_first_task->getTaskPlanId())
    +"], original_plan_id:["+ std::to_string(initial_first_task->getTaskOriginalPlanId())
    +"], STATUS:["+ initial_first_task->getTaskStatus_as_string()
    +"], SERVER: "+ std::to_string(initial_first_task->getTaskServer())
    +", task ddl: "+ std::to_string(initial_first_task->getTaskDeadline()));
    logger->print(Logger::EveryInfo, "First task id:["+ std::to_string(sorted_first_task->getTaskId()) // EveryInfo
    +"], plan_id:["+ std::to_string(sorted_first_task->getTaskPlanId())
    +"], original_plan_id:["+ std::to_string(sorted_first_task->getTaskOriginalPlanId())
    +"], STATUS:["+ sorted_first_task->getTaskStatus_as_string()
    +"], SERVER: "+ std::to_string(sorted_first_task->getTaskServer())
    +" task ddl: "+ std::to_string(sorted_first_task->getTaskDeadline()));

    return preemption_needed;
}

void agent::checkIfPreemptedTaskIsCompleted(std::shared_ptr<Task> p_task, std::shared_ptr<Ag_Scheduler> ag_Sched) {
    std::string report_value = "";
    double server_ddl = -1;
    double task_absolute_deadline;
    std::shared_ptr<Task> p_server = nullptr;

    if(p_task->is_server()) {
        p_server = p_task;
        p_task = p_server->get_head();
        server_ddl = p_server->get_curr_ddl();
    }

    if (fabs(p_task->getTaskCompTimeRes()) < ag_Sched->EPSILON) {
        p_task->setTaskStatus(Task::TSK_COMPLETED);
        lastCompletedTaskTime = sim_current_time().dbl(); // update the instant when the last task has been completed.

        if(p_server != nullptr) {
            logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                    +", SERVER:["+ std::to_string(p_server->getTaskServer())
            +"] COMPLETED tskId:["+ std::to_string(p_task->getTaskId())
            +"], plan:["+ std::to_string(p_task->getTaskPlanId())
            +"], original_id:["+ std::to_string(p_task->getTaskOriginalPlanId())
            +"], tskRt: "+ std::to_string(p_task->getTaskReleaseTime())
            +", budget: "+ std::to_string(p_server->get_curr_budget())
            +", deadline: "+ std::to_string(p_server->get_curr_ddl())
            +", status:["+ p_server->getTaskStatus_as_string() +"]");

            report_value = "t: "+ sim_current_time().str();
            report_value += ", SERVER:["+ std::to_string(p_server->getTaskServer());
            report_value += "] COMPLETED tskId:["+ std::to_string(p_task->getTaskId());
            report_value += "], plan:["+ std::to_string(p_task->getTaskPlanId());
            report_value += "], original_id:["+ std::to_string(p_task->getTaskOriginalPlanId());
            report_value += "], deadline:["+ std::to_string(p_server->get_curr_ddl()) +"]";
        }
        else {
            logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                    +", COMPLETED tskId:["+ std::to_string(p_task->getTaskId())
            +"], plan:["+ std::to_string(p_task->getTaskPlanId())
            +"], original_id:["+ std::to_string(p_task->getTaskOriginalPlanId())
            +"], N_exec left:["+ std::to_string(p_task->getTaskNExec())
            +" of "+ std::to_string(p_task->getTaskNExecInitial())
            +"], tskRt: "+ std::to_string(p_task->getTaskReleaseTime())
            +", status:["+ p_task->getTaskStatus_as_string() +"]");

            report_value = "t: "+ sim_current_time().str();
            report_value += " COMPLETED tskId:["+ std::to_string(p_task->getTaskId());
            report_value += "], plan:["+ std::to_string(p_task->getTaskPlanId());
            report_value += "], original_id:["+ std::to_string(p_task->getTaskOriginalPlanId());
            report_value += "], deadline:["+ std::to_string(p_task->getTaskDeadline()) +"]";

            // decrease the n_exec parameter of p_task in the intention that contains it
            ag_support->updatePeriodicTasksInsideIntention(p_task, ag_intention_set);
        }

        // check if the completed tasks was unexpected
        ag_metric->checkIfUnexpectedTaskExecution(p_task->makeCopyOfTask(), ag_intention_set);

        // remove task from READY queue
        ag_Sched->ag_remove_complited_preempted_task_from_ready(p_task);

        double lateness = sim_current_time().dbl() - p_task->getTaskDeadline();
        task_absolute_deadline = p_task->getTaskDeadline();

        if(p_server == nullptr) {
            logger->print(Logger::Default, " > Lateness:["+ std::to_string(lateness)
            +"] = "+ sim_current_time().str() +" - "+ std::to_string(p_task->getTaskDeadline())
            +" -> was_server: FALSE");
        }
        else {
            lateness = sim_current_time().dbl() - server_ddl;
            task_absolute_deadline = server_ddl;

            logger->print(Logger::Default, " > Lateness:["+ std::to_string(lateness)
            +"] = "+ sim_current_time().str() + " - " + std::to_string(server_ddl)
            +" -> was_server: TRUE -> now: "+ sim_current_time().str()
            +",\n budget: "+ std::to_string(p_server->get_curr_budget())
            +", curr_ddl: "+ std::to_string(p_server->get_curr_ddl())
            +", first_activation: "+ std::to_string(p_server->getTaskFirstActivation())
            +", absDDL: "+ std::to_string(p_task->getTaskAbsDDL())
            +", release_time: "+ std::to_string(p_task->getTaskReleaseTime())
            +", ddl: "+ std::to_string(p_task->getTaskDeadline())
            +", curr_ddl: "+ std::to_string(p_server->get_curr_ddl())
            +", ddl used: "+ std::to_string(server_ddl)
            +", period: "+ std::to_string(p_server->get_period())
            +" -> now - (FirstAct + ddl) > 0 ? -> "+ std::to_string(lateness > 0));
        }

        report_value += ", lateness:["+ std::to_string(lateness) +"]";
        if (lateness > 0) {
            report_value += ", ddl_miss:[YES]";

            // Write lateness.json report file
            int agent_id = getIndex();
            write_lateness_json_report(p_task, agent_id, lateness, sim_current_time().dbl());

            ag_settings->add_ddl_miss();
            ag_tasks_lateness.push_back(std::make_shared<Lateness>(sim_current_time().dbl(), p_task->getTaskId(), p_task->getTaskPlanId(), p_task->getTaskOriginalPlanId(), p_task->getTaskNExec(), lateness, report_value));

            logger->print(Logger::Default, "EDF > Lateness:["+ std::to_string(lateness)
            +"] -> Task:["+ std::to_string(p_task->getTaskId())
            +"], plan_id["+ std::to_string(p_task->getTaskPlanId())
            +"], original_plan_id["+ std::to_string(p_task->getTaskOriginalPlanId())
            +"] now: "+ sim_current_time().str()
            +", tskArrivalTime:["+ std::to_string(p_task->getTaskArrivalTime())
            +"], tskReleaseTime:["+ std::to_string(p_task->getTaskReleaseTime())
            +"], tskCompTime:["+ std::to_string(p_task->getTaskCompTime())
            +"] and tskDDL:["+ std::to_string(p_task->getTaskDeadline()) +"]");
        }
        else {
            report_value += ", ddl_miss:[NO]";
        }

        if (p_server != nullptr)
        { // SERVERS : task completed
            ag_Sched->ag_remove_preempted_task_from_ready(p_server);
            if(p_server->queue_length() > 0)
            {
                p_server->setTaskStatus(Task::TSK_EXEC);
                p_server->get_ith_task()->setTaskStatus(Task::TSK_EXEC);

                int n_exec = p_task->getTaskNExec();
                //note: added for repeated services
                if (n_exec != -1) {
                    n_exec -= 1;
                    if (n_exec != 0 && n_exec != -1 && p_task->get_period() != 0) {
                        logger->print(Logger::EssentialInfo, "Service executions left: "+ std::to_string(n_exec));
                        p_task->setTaskNExec(n_exec);
                        p_server->push_back(p_task);
                    }
                }
                if (p_server->is_empty() == false) {
                    p_server->setTaskId(p_server->get_head()->getTaskId());
                    p_server->setTaskPlanId(p_server->get_head()->getTaskPlanId());                     // BDI
                    p_server->setTaskOriginalPlanId(p_server->get_head()->getTaskOriginalPlanId());     // BDI
                    p_server->setTaskisBeenActivated(p_server->get_head()->getTaskisBeenActivated());   // BDI
                    p_server->setTaskDemander(p_server->get_head()->getTaskDemander());
                    p_server->setTaskExecuter(p_server->get_head()->getTaskExecuter());
                    p_server->setTaskReleaseTime(p_server->get_head()->getTaskReleaseTime());
                    p_server->setTaskDeadLine(p_server->get_curr_ddl());
                    p_server->setTaskArrivalTime(sim_current_time().dbl());

                    ag_Sched->ag_add_task_in_vector_to_release(p_server);

                    // WRITE utilization report
                    write_util_json_report(p_task, getIndex(), sim_current_time().dbl(), ag_Sched->get_current_util(ag_Server_Handler, compute_MsgUtil_sever()), "server-activation");

                    scheduleAt(sim_current_time(), set_next_task_arrival());
                    scheduled_msg += 1;
                    ag_support->setScheduleInFuture("SCHEDULE - task completed (i-th exec)"); // METRICS purposes
                }
            }
            // otherwise: function "ag_Sched->ag_remove_complted_preempted_task_from_ready(p_task);"  has already removed the head task in queue
        }

        // WRITING TO FILE -------------------------------------------------------------------------
        double resp_time = sim_current_time().dbl() - p_task->getTaskReleaseTime();

        logger->print(Logger::EssentialInfo, " > RESP_TIME:["+ std::to_string(sim_current_time().dbl() - p_task->getTaskReleaseTime())
        +"] = "+ sim_current_time().str()
        +" - "+ std::to_string(p_task->getTaskReleaseTime()));

        // write simulation events report
        write_simulation_timeline_json_report(report_value);

        // write ddl_check
        write_ddl_checks(p_task->getTaskPlanId(), p_task->getTaskOriginalPlanId(), p_task->getTaskId(), getIndex());

        // write response time report
        write_response_json_report(p_task, getIndex(), sim_current_time().dbl(), resp_time);
        ag_metric->addTaskResponseTime(p_task, sim_current_time().dbl(), resp_time);

        // write response time per task report
        write_json_resp_per_task(p_task, getIndex(), sim_current_time().dbl(), resp_time);

        // write utilization report
        write_util_json_report(p_task, getIndex(), sim_current_time().dbl(), ag_Sched->get_current_util(ag_Server_Handler), "task completed");

        /* BDI: this means that the execution of p_task has been performed by the same agent that needed it ...
         * .. if the task is APERIODIC, and it has no more executions left, notify the agent */
        if(p_task->getTaskNExec() == 0)
        {   // directly call function that removes the accomplished task
            ag_intention_task_completed(set_intention_task_completed(p_task, task_absolute_deadline));
        }
    }
}

void agent::ag_intention_task_completed(std::shared_ptr<agentMSG> agMsg) {
    logger->print(Logger::Default, "----ag_intention_task_completed----");
    ag_support->setSimulationLastExecution("ag_intention_task_completed");

    logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
            +" Aperiodic task:["+ std::to_string(agMsg->getAg_task_id())
    +"], plan:[id="+ std::to_string(agMsg->getAg_task_plan_id())
    +"], original_plan:[id="+ std::to_string(agMsg->getAg_task_original_plan_id())
    +"] has been completed!!!");

    simtime_t at_time; // use to schedule completion message
    std::shared_ptr<Task> task_head;
    std::shared_ptr<Goal> goal_head;
    int intention_index = -1;
    bool checkNextInternalGoal = true;
    std::string goal_name;
    std::string report_value = "";
    std::variant<int, double, bool, std::string> belief_value;

    // This statement is needed in order to manage those cases where taskset.json already contains tasks before the start of execution...
    if(agMsg->getAg_task_plan_id() != -1) {
        /***************************************************************
         * BDI: added in manage the case in which n_exec of an aperiodic task becomes = 0.
         * When this happens:
         * 0. if the internal_goal stack is not empty, decrease the value of the top element
         * 1. remove the task from the 'top' of the stack
         * 2. if the next action is
         *      - task --> add to vector-to-be-released in order to execute it
         *      - internal goal --> - find the best plan(with higher PREF) that satisfy it,
         *                          - then remove the goal from stack
         *                          - and replace it with plan.body list of actions,
         *                          - until the top action is a task.
         * 3. if there are not actions left in the stack's intention, the plan associated to the intention has been completed.
         *      - remove the intention from the set
         *      - remove the goal from the goalset
         *      - remove the plan from selected plans
         *      - update belief
         *      - check if the update has triggered desires to goal conversion
         *****************************************************************/

        logger->print(Logger::EveryInfo, " - Decrease the counter for the internal goal...");
        for(int i = 0; i < (int)ag_intention_set.size(); i++)
        {
            logger->print(Logger::EveryInfoPlus, std::to_string(ag_intention_set[i]->get_planID()) +" == "+ std::to_string(agMsg->getAg_task_plan_id())
            +" && "+ std::to_string(ag_intention_set[i]->get_original_planID()) +" == "+ std::to_string(agMsg->getAg_task_original_plan_id()));

            if(ag_intention_set[i]->get_planID() == agMsg->getAg_task_plan_id()
                    && ag_intention_set[i]->get_original_planID() == agMsg->getAg_task_original_plan_id())
            {
                // Note: this condition should always be true at this point of the execution! Nevertheless, used to avoid 'Index Out Of Range Exception'
                if(ag_intention_set[i]->get_Actions().size() > 0)
                {   /* Note: at the end of this 'if' statement the outer FOR-LOOP must be interrupted
                 * and the execution moves to the 'if(intention_index >= 0)'.
                 * This is done by calling "break;" after instruction "intention_index = i;" */

                    std::vector<std::stack<std::tuple<std::shared_ptr<Goal>, int, int>>> tmp = ag_intention_set[i]->get_internal_goals();
                    for(int k = 0; k < (int)tmp.size(); k++) {
                        if(tmp[k].size() > 0 && std::get<1>(tmp[k].top()) == agMsg->getAg_task_original_plan_id())
                        {
                            std::stack<std::tuple<std::shared_ptr<Goal>, int, int>> tmp_internal_goals;
                            checkNextInternalGoal = true;

                            while(checkNextInternalGoal)
                            {
                                checkNextInternalGoal = false;
                                tmp_internal_goals = tmp[k];
                                logger->print(Logger::EveryInfoPlus, " - number of internal goal: " +std::to_string(tmp_internal_goals.size()));

                                if(tmp_internal_goals.size() > 0)
                                { // then there is at least one internal goal...
                                    ag_intention_set[i]->update_ith_top_internal_goal(k);

                                    logger->print(Logger::EveryInfoPlus, " - number of action for top internal goal: "
                                            +std::to_string(std::get<2>(ag_intention_set[i]->get_top_internal_goal(k))));

                                    if(std::get<2>(ag_intention_set[i]->get_top_internal_goal(k)) == 0)
                                    {   // goal achieved ...
                                        checkNextInternalGoal = true;

                                        // remove the goal from the stack of internal goal for the specific intention...
                                        ag_intention_set[i]->pop_internal_goals(k);
                                        ag_intention_set[i]->pop_internal_goal_plan(k);
                                    }
                                }
                            }
                            break;
                        }
                    }

                    /* Note: When a PERIODIC task is removed from the corresponding intetion,
                     * the interval of time between the current instant (now) when this step is performed
                     * and when the deadline (ddl) of the task is scheduled gets "erased".
                     * This means that the following task in READY can be immediately be executed. */
                    task_head = ag_intention_set[i]->pop_Action(agMsg->getAg_task_id(), agMsg->getAg_task_plan_id(), agMsg->getAg_task_original_plan_id());

                    logger->print(Logger::EssentialInfo, "Intention:["+ std::to_string(ag_intention_set[i]->get_id())
                    +"], goal:["+ ag_intention_set[i]->get_goal_name()
                    +"], plan:["+ std::to_string(ag_intention_set[i]->get_planID())
                    +"], original plan:["+ std::to_string(ag_intention_set[i]->get_original_planID())
                    +"], Actions:["+ std::to_string(ag_intention_set[i]->get_Actions().size())
                    +"], lastExecution:["+ std::to_string(ag_intention_set[i]->get_lastExecution()) +"]");

                    if(ag_intention_set[i]->get_mainPlanActions().size() > 0) {
                        if(ag_intention_set[i]->get_mainPlanActions()[0]->get_type() == "goal") {
                            goal_head = std::dynamic_pointer_cast<Goal>(ag_intention_set[i]->get_mainPlanActions()[0]);

                            logger->print(Logger::EssentialInfo, "HEAD is GOAL:[alreadyActive="+ ag_support->boolToString(goal_head->getGoalisBeenActivated()) +"]");
                            if(goal_head->getGoalisBeenActivated() == false) {
                                // This means that we executed the last action in the first batch,
                                // and now the second batch has became the first one.
                                ag_intention_set[i]->set_startEqualLastExecution(sim_current_time().dbl());
                            }
                        }
                        else if(ag_intention_set[i]->get_mainPlanActions()[0]->get_type() == "task") {
                            task_head = std::dynamic_pointer_cast<Task>(ag_intention_set[i]->get_mainPlanActions()[0]);

                            logger->print(Logger::EssentialInfo, "HEAD is TASK:[alreadyActive="+ ag_support->boolToString(task_head->getTaskisBeenActivated()) +"]");
                            if(task_head->getTaskisBeenActivated() == false) {
                                // This means that we executed the last action in the first batch,
                                // and now the second batch has became the first one.
                                ag_intention_set[i]->set_startEqualLastExecution(sim_current_time().dbl());
                            }
                        }
                    }
                    else { // if the intention is now empty (this function was called by the last task's execution), update lastExecution parameter
                        ag_intention_set[i]->set_startEqualLastExecution(sim_current_time().dbl());
                    }
                    logger->print(Logger::EssentialInfo, "-----------------------------------------");
                    intention_index = i;
                    break;
                }
            }
        }
        // If there are no more actions to be performed for the selected intention...
        if(intention_index >= 0)
        {
            if(ag_intention_set[intention_index]->get_Actions().size() == 0)
            {
                // ------------------------------------------------------------------------------------------------------------------------------
                /* Whenever the condition "if(ag_intention_set[intention_index]->get_Actions().size() == 0)" is met,
                 * schedule the 'intention completion' process at t: agMsg->getAg_task_absolute_ddl()
                 * (t = absolute deadline of the last accomplished task in the intention body)
                 * PROBLEM: leaving the intention active and with no actions in body is a problem if phiI or a new reasoning cycle is performed before t: agMsg->getAg_task_absolute_ddl()
                 * SOL: in class intention has been implemented the parameter 'waiting_for_completion'.
                 *      - It get set as soon as the last action is removed from the body (waiting_for_completion = true, false everywhere else)
                 *      - In ag_reasoning_cycle and more importantly, in phiI,
                 *          intentions with 'waiting_for_completion = true' are left active
                 *          as long as the intention's cont_conditions
                 *          and the linked goal's cont_conditions are valid */
                logger->print(Logger::EssentialInfo, " ++++ COMPLETED TASK: t: "+ sim_current_time().str()
                        +", task id:["+ std::to_string(task_head->getTaskId())
                +"], plan:["+ std::to_string(task_head->getTaskPlanId())
                +"], original_plan:["+ std::to_string(task_head->getTaskOriginalPlanId())
                +"], intention_index:["+ std::to_string(intention_index)
                +"], intention goal name:["+ ag_intention_set[intention_index]->get_goal_name()
                +"], task deadline:["+ std::to_string(task_head->getTaskDeadline())
                +"], task absolute deadline:["+ std::to_string(task_head->getTaskAbsDDL())
                +"], ABSOLUTE DEADLINE:["+ std::to_string(agMsg->getAg_task_absolute_ddl())
                +"] -> schedule Intention completion at t:["+ std::to_string(agMsg->getAg_task_absolute_ddl()) +"] ++++");

                ag_intention_set[intention_index]->set_scheduled_completion_time(agMsg->getAg_task_absolute_ddl());
                ag_intention_set[intention_index]->set_waiting_for_completion(true);

                // check if task_absolute_ddl >= now...
                // Note: doing agMsg->getAg_task_absolute_ddl() == sim_current_time().dbl() returns false for values: 18.00100 == 18.00100
                if((ag_support->checkIfDoublesAreEqual(agMsg->getAg_task_absolute_ddl(), sim_current_time().dbl(), ag_Sched->DELTA_comparison))
                        || agMsg->getAg_task_absolute_ddl() > sim_current_time().dbl())
                {
                    logger->print(Logger::EveryInfo, "Task.ddl >= now ? ("+ std::to_string(agMsg->getAg_task_absolute_ddl()) +" >= "+ sim_current_time().str()
                            +") = "+ ag_support->boolToString(agMsg->getAg_task_absolute_ddl() - ag_Sched->DELTA_comparison <= sim_current_time().dbl()
                                    && agMsg->getAg_task_absolute_ddl() + ag_Sched->DELTA_comparison >= sim_current_time().dbl()));

                    at_time = convert_double_to_simtime_t(agMsg->getAg_task_absolute_ddl());
                }
                else {
                    std::string warning_msg = " ++++ [WARNING][!] Task absolute deadline is in the past! t:["+ sim_current_time().str();
                    warning_msg += "], absolute deadline:["+ std::to_string(agMsg->getAg_task_absolute_ddl());
                    warning_msg += "] -> schedule intention completion to now:["+ sim_current_time().str() +"] ++++";
                    logger->print(Logger::Warning, warning_msg);

                    ag_metric->addGeneratedWarning(sim_current_time(), "ag_intention_task_completed", warning_msg);

                    /* Execution may reach this point after a deadline miss has occurred
                     * In EDF this might happen if a server is reset and executed before a task waiting in ready.
                     * When this happen, the task may generate a ddl miss if compTime and deadline are too close.
                     * e.g.,
                     * t: 126 task 0 completed, deadline = 128, n_exec left 1 of 10
                     * t: 127 activated task 1, deadline server 0 = 137
                     * t: 128 task 0 pushed to ready, deadline = 138, last execution 0 of 10
                     * t: 130 task 1 completed, server 0 deadline = 137
                     * t: 132 task 1 expired, server 0 reset deadline = 167
                     * t: 132 activated task 0, compTime = 8, deadline = 138
                     * t: 140 task 0 completed, LATENESS = 2 --> deadline miss generated by EDF */
                    at_time = sim_current_time();
                }

                agentMSG* msg_completion = set_intention_completion(
                        ag_intention_set[intention_index]->get_goalID(),
                        agMsg->getAg_task_plan_id(),
                        agMsg->getAg_task_original_plan_id(),
                        intention_index,
                        ag_intention_set[intention_index]->get_goal_name(),
                        true);
                scheduleAt(at_time, msg_completion);
                scheduled_msg += 1;
                ag_support->setScheduleInFuture(msg_name_intention_completion); // METRICS purposes
                // ---------------------------------------------------------------------------------------------------------------------------------------
            }
        }

        logger->print(Logger::EssentialInfo, " - Schedule PhiI....");
        scheduleAt(sim_current_time(), set_select_next_intentions_task());
        scheduled_msg += 1;
        ag_support->setScheduleInFuture("AG_TASK_SELECTION_PHII - intention task completed"); // METRICS purposes
    }

    //    // DEBUG ------------------------------------------------------
    //    ag_support->printIntentions(ag_intention_set, ag_goal_set);
    //    logger->print(Logger::Default, "-----------------------------");
    //    ag_Sched->ev_ag_tasks_vector_ready();
    //    logger->print(Logger::Default, "-----------------------------");
    //    ag_Sched->ev_ag_tasks_vector_to_release();
    //    logger->print(Logger::Default, "-----------------------------");
    //    // -------------------------------------------------------------
}

// *******************************************************************************
// Agent: file readings
// *******************************************************************************
std::pair<bool, json> agent::set_ag_beliefset(const bool initilization_phase, const bool read_from_file, std::string file_content)
{
    std::string path = glob_simulation_folder; // global variable 'simulation_folder' is set in "initialize()"
    json ret_val;

    logger->print(Logger::EssentialInfo, "---SETTING_AG_BELIEFSET---");

    //load data from file
    try {
        ret_val = jsonfileHandler->read_beliefset_from_json(path + "beliefset.json", ag_belief_set, getIndex(), file_content, read_from_file);
        return std::make_pair(true, ret_val);
    }
    catch (std::exception const& ex) {
        logger->print(Logger::Error, ex.what());
        ag_metric->addGeneratedError(sim_current_time(), "set_ag_beliefset", ex.what());

        ret_val = {
                { "status", "error" },
                { "description", ex.what() }
        };

        if(initilization_phase) {
            ag_metric->finishCrash(true, glob_path, glob_user, ex.what(), "JsonfileHandler", "read_beliefset_from_json", sim_current_time().dbl());
        }
        return std::make_pair(false, ret_val);
    }
}

std::pair<bool, json> agent::set_ag_desireset(const bool initilization_phase, const bool read_from_file, std::string file_content)
{
    std::string path = glob_simulation_folder; // global variable 'simulation_folder' is set in "initialize()"
    json ret_val;

    logger->print(Logger::EssentialInfo, "---SETTING_AG_DESIRESET---");

    // load data from file
    try {
        ret_val = jsonfileHandler->read_desireset_from_json(path + "desireset.json", ag_belief_set, ag_skill_set, ag_desire_set, getIndex(), file_content, read_from_file);
        return std::make_pair(true, ret_val);
    }
    catch (std::exception const& ex) {
        logger->print(Logger::Error, ex.what());
        ag_metric->addGeneratedError(sim_current_time(), "set_ag_desireset", ex.what());

        if(initilization_phase) { // terminate simulation if the file generated an error during the initialization of the agent
            ag_metric->finishCrash(true, glob_path, glob_user, ex.what(), "JsonfileHandler", "read_desireset_from_json", sim_current_time().dbl());
        }

        ret_val = {
                { "status", "error" },
                { "description", ex.what() }
        };

        return std::make_pair(false, ret_val);
    }
}

std::pair<bool, json> agent::set_ag_planset(const bool initilization_phase, const bool read_from_file, std::string file_content)
{
    std::string path = glob_simulation_folder; // global variable 'simulation_folder' is set in "initialize()"
    json ret_val;
    std::string scheduler_name = ag_support->convertSchedType_to_string(selected_sched_type);

    logger->print(Logger::EssentialInfo, "---SETTING_AG_PLANSET---");

    try {
        ret_val = jsonfileHandler->read_planset_from_json(path + "planset.json", ag_belief_set, ag_skill_set, ag_plan_set,
                ag_servers, getIndex(), scheduler_name, file_content, read_from_file);

        return std::make_pair(true, ret_val);
    }
    catch (std::exception const& ex) {
        logger->print(Logger::Error, ex.what());
        ag_metric->addGeneratedError(sim_current_time(), "set_ag_planset", ex.what());

        if(initilization_phase) {
            ag_metric->finishCrash(true, glob_path, glob_user, ex.what(), "JsonfileHandler", "read_planset_from_json", sim_current_time().dbl());
        }

        ret_val = {
                { "status", "error" },
                { "description", ex.what() }
        };
        return std::make_pair(false, ret_val);
    }
}

std::pair<bool, json> agent::set_ag_sensors(const bool initilization_phase, const bool read_from_file, std::string file_content) {
    std::string path = glob_simulation_folder; // global variable 'simulation_folder' is set in "initialize()"
    json ret_val;

    logger->print(Logger::EssentialInfo, "---SETTING_AG_SENSORS---");

    // load data from file
    try {
        ret_val = jsonfileHandler->read_sensors_from_json(path + "sensors.json", ag_sensors_time, getIndex(), file_content, read_from_file);

        try { // schedule only messages for those sensor reading that are time dependent (time != -1)
            set_ag_sensors_time(ag_sensors_time);
            return std::make_pair(true, ret_val);
        }
        catch (std::exception const& ex) {
            logger->print(Logger::Error, "[ERROR] Can not schedule message for scheduling sensors_time readings");
            ag_metric->addGeneratedError(sim_current_time(), "set_ag_sensors", ex.what());
            ret_val = {
                    { "status", "error" },
                    { "description", ex.what() }
            };
            return std::make_pair(false, ret_val);
        }
    }
    catch (std::exception const& ex) {
        logger->print(Logger::Error, ex.what());
        ag_metric->addGeneratedError(sim_current_time(), "set_ag_sensors", ex.what());

        if(initilization_phase) {
            ag_metric->finishCrash(true, glob_path, glob_user, ex.what(), "JsonfileHandler", "set_ag_sensors_time", sim_current_time().dbl());
        }

        ret_val = {
                { "status", "error" },
                { "description", ex.what() }
        };
        return std::make_pair(false, ret_val);
    }
}

std::pair<bool, json> agent::set_ag_servers(const bool initilization_phase, const bool read_from_file, std::string file_content) {
    std::string path = glob_simulation_folder; // global variable 'simulation_folder' is set in "initialize()"
    json ret_val;
    std::string scheduler_name = ag_support->convertSchedType_to_string(par("sched_type"));

    logger->print(Logger::EssentialInfo, "---SETTING_AG_SERVERS---");

    // load data from file
    try {
        ret_val = jsonfileHandler->read_servers_from_json(path + "servers.json", ag_Server_Handler, ag_servers, scheduler_name, getIndex(), file_content, read_from_file);
        return std::make_pair(true, ret_val);
    }
    catch (std::exception const& ex) {
        logger->print(Logger::Error, ex.what());
        ag_metric->addGeneratedError(sim_current_time(), "set_ag_servers", ex.what());

        if(initilization_phase) {
            ag_metric->finishCrash(true, glob_path, glob_user, ex.what(), "JsonfileHandler", "read_servers_from_json", sim_current_time().dbl());
        }

        ret_val = {
                { "status", "error" },
                { "description", ex.what() }
        };
        return std::make_pair(false, ret_val);
    }
}

std::pair<bool, json> agent::set_ag_skillset(const bool initilization_phase, const bool read_from_file, std::string file_content) {
    std::string path = glob_simulation_folder; // global variable 'simulation_folder' is set in "initialize()"
    json ret_val;

    logger->print(Logger::EssentialInfo, "---SETTING_AG_SKILLS---");

    // load data from file
    try {
        ret_val = jsonfileHandler->read_skillset_from_json(path + "skillset.json", ag_belief_set, ag_skill_set, getIndex(), file_content, read_from_file);
        return std::make_pair(true, ret_val);
    }
    catch (std::exception const& ex) {
        logger->print(Logger::Error, ex.what());
        ag_metric->addGeneratedError(sim_current_time(), "set_ag_skills", ex.what());

        if(initilization_phase) {
            ag_metric->finishCrash(true, glob_path, glob_user, ex.what(), "JsonfileHandler", "read_skillset_from_json", sim_current_time().dbl());
        }

        ret_val = {
                { "status", "error" },
                { "description", ex.what() }
        };
        return std::make_pair(false, ret_val);
    }
}

std::pair<bool, json> agent::set_ag_update_beliefset(const bool initilization_phase, const bool read_from_file, std::string file_content) {
    std::string path = glob_simulation_folder; // global variable 'simulation_folder' is set in "initialize()"
    json ret_val;

    logger->print(Logger::EssentialInfo, "---SETTING_AG_UPDATE_BELIEFSET---");

    // load tasks from file
    try {
        ret_val = jsonfileHandler->update_beliefset_from_json(path + "update-beliefset.json", ag_belief_set, getIndex(), file_content, read_from_file);

        if(logger->get_level() >= Logger::EveryInfo) {
            logger->print(Logger::Default, "\nBeliefset after Updating values:");
            ag_support->printBeliefset(ag_belief_set);
            ag_support->printDesireset(ag_desire_set);
        }
        return std::make_pair(true, ret_val);
    }
    catch (std::exception const& ex) {
        logger->print(Logger::Error, ex.what());
        ag_metric->addGeneratedError(sim_current_time(), "set_ag_update_beliefset", ex.what());

        if(initilization_phase) {
            ag_metric->finishCrash(true, glob_path, glob_user, ex.what(), "JsonfileHandler", "update_beliefset_from_json", sim_current_time().dbl());
        }

        ret_val = {
                { "status", "error" },
                { "description", ex.what() }
        };
        return std::make_pair(false, ret_val);
    }
}
/********************************************************************************/

// *****************************************************************************
// PREPARE AND WRITE JSON FUNCTIONS:
// *****************************************************************************
void agent::ddl_json_report(std::shared_ptr<Ag_Scheduler> ag_Sched, std::shared_ptr<Task> task, double c_res) {
    //calculate the number of tasks when a ddl_miss occurs (changed to ready)
    int num_tasks = ag_Sched->get_tasks_vector_ready().size();
    //calculate the utilization factor when a ddl_miss occurs
    double ag_utilization = ag_Sched->ag_sched_test(ag_Sched->get_tasks_vector_to_release());

    //add msg_server util if needed
    if((bool) par("msg_server_mode")) {
        double msg_util = compute_MsgUtil_sever(); // count both read and write msg servers
        ag_utilization += msg_util;
    }
    write_ddl_json_report(task, sim_current_time(), c_res, ag_utilization, num_tasks);
    // ---------------------------------------------------------------------------------
}
/********************************************************************************/

// *****************************************************************************
// *******                      SETTER/GETTER                            *******
// *****************************************************************************
agentMSG* agent::set_run_until_t() {

    logger->print(Logger::EveryInfo, "\t set_run_until_t()");

    int src = getIndex();  // our module index
    int dest = getIndex();

    logger->print(Logger::EssentialInfo, "A scheduled:["+ std::to_string(scheduled_msg)
    +"], cancelled:["+ std::to_string(executed_msg)
    +"], pending:["+ (std::to_string(pending_scheduled_messages()) +"]"));

    // Create message object and and set the contents
    agentMSG* msg = new agentMSG(msg_name_run_until_t);
    msg->setSchedulingPriority(0); // prioritize

    msg->setInformative(RUN_UNTIL_T);
    msg->setSource(src);
    msg->setDestination(dest);

    return msg;
}

//Event trigger: comunica all'agente di impostare lo scheduler
agentMSG* agent::set_ag_scheduler() {
    int src = getIndex();  // our module index
    int dest = getIndex();

    // Create message object and and set the contents
    agentMSG* msg = new agentMSG(msg_name_ag_scheduler);
    msg->setSchedulingPriority(1);

    msg->setInformative(SCHEDULE);
    msg->setSource(src);
    msg->setDestination(dest);
    return msg;
}

agentMSG* agent::set_reasoning_cycle(bool apply_reasoning_cycle_again) {
    int src = getIndex();  // our module index
    int dest = getIndex();

    // Create message object and and set the contents
    agentMSG* msg = new agentMSG(msg_name_reasoning_cycle);
    msg->setSchedulingPriority(0); // prioritize

    msg->setInformative(AG_REASONING_CYCLE);
    msg->setSource(src);
    msg->setDestination(dest);
    // added to manage the case APPLY REASONING CYCLE occurs
    msg->setAg_apply_reasoning_cycle_again(apply_reasoning_cycle_again);
    return msg;
}

agentMSG* agent::set_select_next_intentions_task() {
    int src = getIndex();  // our module index
    int dest = getIndex();

    // Create message object and and set the contents
    agentMSG* msg = new agentMSG(msg_name_select_next_intentions_task);
    msg->setSchedulingPriority(1);

    msg->setInformative(AG_TASK_SELECTION_PHII);
    msg->setSource(src);
    msg->setDestination(dest);

    return msg;
}

agentMSG* agent::set_intention_completion(const int intention_goal_id, const int intention_plan_id,
        const int intention_original_plan_id, const int index_intention, const std::string intention_goal_name, const bool scheduleNewReasoningCycle)
{
    int src = getIndex();  // our module index
    int dest = getIndex();

    // Create message object and and set the contents
    agentMSG* msg = new agentMSG(msg_name_intention_completion);
    msg->setSchedulingPriority(0); // prioritize

    msg->setInformative(INTENTION_COMPLETED);
    msg->setSource(src);
    msg->setDestination(dest);

    msg->setAg_intention_goal_id(intention_goal_id);
    msg->setAg_intention_plan_id(intention_plan_id);
    msg->setAg_intention_original_plan_id(intention_original_plan_id);
    msg->setAg_intention_goal_name(intention_goal_name);
    msg->setAg_scheduleNewReasoningCycle(scheduleNewReasoningCycle);

    return msg;
}

// *****************************************************************************
// *******                       SCHEDULER MSGS                          *******
// *****************************************************************************
agentMSG* agent::set_next_task_arrival() {
    int src = getIndex();  // our module index
    int dest = getIndex();

    // Create message object and and set the contents
    agentMSG* msg = new agentMSG(msg_name_next_task_arrival);
    msg->setSchedulingPriority(1);

    msg->setInformative(SCHEDULE);
    msg->setSource(src);
    msg->setDestination(dest);
    return msg;
}

agentMSG* agent::set_task_activation() {
    //task get cpu ...
    int src = getIndex();  // our module index
    int dest = getIndex();

    // Create message object and set the contents
    agentMSG* msg = new agentMSG(msg_name_task_activation);
    msg->setSchedulingPriority(1);

    msg->setInformative(ACTIVATE_TASK);
    msg->setSource(src);
    msg->setDestination(dest);
    return msg;
}

agentMSG* agent::set_check_task_complete(std::shared_ptr<Task> p_task, double t_comp) {
    int src = getIndex();  // our module index
    int dest = getIndex();

    agentMSG* msg = new agentMSG(msg_name_check_task_complete);
    msg->setSchedulingPriority(0); // prioritize!

    // Informative discriminate the meaning of the message
    msg->setInformative(CHECK_TASK_TERMINATED);
    msg->setAg_task_id(p_task->getTaskId());
    msg->setAg_task_plan_id(p_task->getTaskPlanId());
    msg->setAg_task_original_plan_id(p_task->getTaskOriginalPlanId());
    msg->setSource(src);
    msg->setDestination(dest);
    // EDF parameters
    msg->setAg_task_release_time(p_task->getTaskReleaseTime());
    msg->setAg_task_demander(p_task->getTaskDemander());
    // added for BDI
    msg->setAg_task_firstActivation(p_task->getTaskFirstActivation());
    msg->setAg_task_lastActivation(p_task->getTaskLastActivation());

    int intention_index = ag_support->getIntentionIndexGivenPlanId(p_task->getTaskPlanId(), ag_intention_set);
    if(intention_index != -1) {
        msg->setAg_intention_startTime(ag_intention_set[intention_index]->get_startTime());
        msg->setAg_intention_firstActivation(ag_intention_set[intention_index]->get_firstActivation());
    }
    else { // ERROR: should never enter this else, if the p_task exists, it must be assigned to an intention
        msg->setAg_intention_startTime(-1);
        msg->setAg_intention_firstActivation(-1);
    }

    return msg;
}

agentMSG* agent::set_task_preemption(std::shared_ptr<Task> p_task, std::shared_ptr<Task> preempted_task) {
    int src = getIndex();  // our module index
    int dest = getIndex();
    char msgname[45] = { 0 };

    sprintf(msgname, "Task Preemption: %d", preempted_task->getTaskId());
    agentMSG* msg = new agentMSG(msgname);
    msg->setSchedulingPriority(0); // prioritize!

    msg->setInformative(PREEMPT);
    msg->setAg_task_id(p_task->getTaskId());
    msg->setSource(src);
    msg->setDestination(dest);
    // EDF
    msg->setAg_task_release_time(p_task->getTaskReleaseTime());
    msg->setAg_task_demander(p_task->getTaskDemander());
    msg->setAg_task_ddl(p_task->getTaskDeadline());
    msg->setAg_task_server(p_task->getTaskServer());
    // preemption
    msg->setAg_task_plan_id(p_task->getTaskPlanId());
    msg->setAg_preempted_task_id(preempted_task->getTaskId());
    msg->setAg_preempted_task_plan_id(preempted_task->getTaskPlanId());
    msg->setAg_preempted_task_server_id(preempted_task->getTaskServer());
    msg->setAg_task_server(p_task->getTaskServer());

    return msg;
}

std::shared_ptr<agentMSG> agent::set_intention_task_completed(std::shared_ptr<Task> p_task, double absolute_ddl) {
    // BDI: called every time the task of an intention gets completed
    int src = getIndex();  // our module index
    int dest = getIndex();

    std::shared_ptr<agentMSG> msg = std::make_shared<agentMSG>(msg_name_intention_task_completed);
    msg->setSchedulingPriority(0); // prioritize!

    // Informative discriminate the meaning of the message
    msg->setInformative(INTENTION_TASK_COMPLETED);
    msg->setAg_task_id(p_task->getTaskId());
    msg->setAg_task_plan_id(p_task->getTaskPlanId());
    msg->setAg_task_original_plan_id(p_task->getTaskOriginalPlanId());
    msg->setAg_task_absolute_ddl(absolute_ddl);

    msg->setSource(src);
    msg->setDestination(dest);
    return msg;
}

agentMSG* agent::set_internal_goal_arrival(std::shared_ptr<Goal> goal, int intention_original_plan_id)
{
    int src = getIndex();  // our module index
    int dest = getIndex();

    agentMSG* msg = new agentMSG(msg_name_internal_goal_arrival);
    msg->setSchedulingPriority(1);

    // Informative discriminate the meaning of the message
    msg->setInformative(INTERNAL_GOAL_ARRIVAL);
    msg->setAg_internal_goal_id(goal->get_id());
    msg->setAg_internal_goal_plan_id(goal->get_plan_id());
    msg->setAg_internal_goal_original_plan_id(intention_original_plan_id);
    msg->setAg_internal_goal_name(goal->get_goal_name());
    msg->setAg_internal_goal_request_time(sim_current_time().dbl());
    msg->setAg_internal_goal_arrival_time(goal->get_arrival_time());

    msg->setSource(src);
    msg->setDestination(dest);
    return msg;
}

// BDI: sensors that are activated at given time 't', despite what the agent is doing at that moment
void agent::set_ag_sensors_time(std::vector<std::shared_ptr<Sensor>>& ag_sensors_time) {
    // schedule resume post-preemption
    int src = getIndex();  // our module index
    int dest = getIndex();
    agentMSG* msg;

    logger->print(Logger::EssentialInfo, "[!] Scheduling self-messages for sensors...");

    ag_support->sortSensorVectorBubbleSort(ag_sensors_time);

    for(std::shared_ptr<Sensor> sensor : ag_sensors_time)
    {
        if(sensor->get_scheduled() == false) // if true, then the sensor has already been scheduled
        {
            if(sensor->get_time() >= infinite_ddl) {
                logger->print(Logger::EssentialInfo, " - Sensor:["+ std::to_string(sensor->get_id())
                +"], belief:["+ sensor->get_belief_name()
                +"], value:["+ ag_support->variantToString(sensor->get_value())
                +"] has time t: "+ std::to_string(sensor->get_time())
                +"! It must be less than \"infinity\":["+ std::to_string(infinite_ddl) +"] -> sensor NOT scheduled");
            }
            else {
                if(sensor->get_time() >= sim_current_time().dbl())
                {
                    logger->print(Logger::EssentialInfo, " - Sensor:["+ std::to_string(sensor->get_id())
                    +"], belief:["+ sensor->get_belief_name()
                    +"] at time t: "+ std::to_string(sensor->get_time())
                    +" will be updated to value:["+ ag_support->variantToString(sensor->get_value()) +"]");

                    msg = new agentMSG(msg_name_ag_sensors_time);
                    msg->setSchedulingPriority(0); // prioritize!

                    // Informative discriminate the meaning of the message
                    msg->setInformative(UPDATE_BELIEF_TIME);
                    msg->setAg_sensor_id(sensor->get_id());
                    msg->setAg_sensor_belief_name(sensor->get_belief_name());
                    msg->setSource(src);
                    msg->setDestination(dest);

                    simtime_t at_time = convert_double_to_simtime_t(sensor->get_time());
                    scheduleAt(at_time, msg);
                    scheduled_msg += 1;
                    ag_support->setScheduleInFuture("UPDATE_BELIEF_TIME - schedule sensor activation", sim_current_time().dbl() + sensor->get_time()); // METRICS purposes

                    sensor->set_scheduled();
                }
                else {
                    logger->print(Logger::Warning, " - Sensor:["+ std::to_string(sensor->get_id())
                    +"], belief:["+ sensor->get_belief_name()
                    +"], value:["+ ag_support->variantToString(sensor->get_value())
                    +"] has activation time in the past! Now:["+ sim_current_time().str()
                    +"], Sensor t:["+ std::to_string(sensor->get_time())
                    +"]! -> sensor NOT scheduled");
                }
            }
        }
    }
}

// *****************************************************************************
// *******                         UTILITIES                             *******
// *****************************************************************************
void agent::cancel_all_messages() {
    cObject* event;
    const char* class_name = "agentMSG";
    int tot_msg = this->getSimulation()->getFES()->getLength();

    for(int i = 0; i < tot_msg; i++)
    {
        event = this->getSimulation()->getFES()->get(i);
        if (event != nullptr)
        {
            // check if(event->getClassName() == class_name AND event->getFullName() != full_name) then enter the statement
            if(strcmp(event->getClassName(), class_name) == 0)
            {
                agentMSG* msg = check_and_cast<agentMSG*>(event);
                cancelAndDelete(msg);
                i -= 1;
                tot_msg -= 1;
                executed_msg += 1;
            }
        }
    }
}

void agent::cancel_next_redundant_msg(const char* name) {
    int k = 0;
    bool found = false;
    cObject* event;
    const char* class_name = "agentMSG";
    const char* full_name = "endsimulation";

    while (found == false && k < this->getSimulation()->getFES()->getLength())
    {
        event = this->getSimulation()->getFES()->get(k);
        if (event != nullptr && (strcmp(event->getName(), name) == 0))
        {
            // check if(event->getClassName() == class_name AND event->getFullName() != full_name) then enter the statement
            if(strcmp(event->getClassName(), class_name) == 0 && strcmp(event->getFullName(), full_name) != 0)
            {
                agentMSG* msg = check_and_cast<agentMSG*>(event);
                if (msg->getSenderModule()->getFullName() == getFullName()) {
                    found = true;
                    cancelAndDelete(msg);
                }
            }
        }
        k++;
    }
}

void agent::cancel_next_redundant_msg(const char* name, const int task_id, const int plan_id, const int original_plan_id) {
    int k = 0;
    bool found = false;
    cObject* event;
    const char* class_name = "agentMSG";
    const char* full_name = "endsimulation";

    logger->print(Logger::EssentialInfo, "cancel_next_redundant_msg: TASK:["+ std::to_string(task_id)
    +"], plan:["+ std::to_string(plan_id)
    +"], original_plan:["+ std::to_string(original_plan_id) +"]");

    while (found == false && k < this->getSimulation()->getFES()->getLength())
    {
        event = this->getSimulation()->getFES()->get(k);
        if (event != nullptr && (strcmp(event->getName(), name) == 0))
        {
            // check if(event->getClassName() == class_name AND event->getFullName() != full_name) then enter the statement
            if(strcmp(event->getClassName(), class_name) == 0 && strcmp(event->getFullName(), full_name) != 0)
            {
                agentMSG* msg = check_and_cast<agentMSG*>(event);
                if (msg->getSenderModule()->getFullName() == getFullName())
                {
                    if(msg->getAg_task_id() == task_id
                            && msg->getAg_task_plan_id() == plan_id
                            && msg->getAg_task_original_plan_id() == original_plan_id)
                    {
                        found = true;
                        cancelAndDelete(msg);
                    }
                }
            }
        }
        k++;
    }
}

void agent::cancel_next_redundant_msg(const char* name, const int server_id) {
    int k = 0;
    bool found = false;
    cObject* event;
    const char* class_name = "agentMSG";
    const char* full_name = "endsimulation";

    logger->print(Logger::EssentialInfo, "cancel_next_redundant_msg: SERVER:["+ std::to_string(server_id) +"]");

    while (found == false && k < this->getSimulation()->getFES()->getLength())
    {
        event = this->getSimulation()->getFES()->get(k);
        if (event != nullptr && (strcmp(event->getName(), name) == 0))
        {
            // check if(event->getClassName() == class_name AND event->getFullName() != full_name) then enter the statement
            if(strcmp(event->getClassName(), class_name) == 0 && strcmp(event->getFullName(), full_name) != 0)
            {
                agentMSG* msg = check_and_cast<agentMSG*>(event);
                if (msg->getSenderModule()->getFullName() == getFullName())
                {
                    if(msg->getAg_server_id() == server_id)
                    {
                        found = true;
                        cancelAndDelete(msg);
                    }
                }
            }
        }
        k++;
    }
}

void agent::cancel_next_reasoning_cycle_msg(const char* name, const simtime_t time) {
    int k = 0;
    bool found = false;
    cObject* event;
    const char* class_name = "agentMSG";
    const char* full_name = "endsimulation";

    logger->print(Logger::EssentialInfo, "cancel_next_reasoning_cycle_msg: time:["+ time.str() +"]");

    while (found == false && k < this->getSimulation()->getFES()->getLength())
    {
        event = this->getSimulation()->getFES()->get(k);
        if (event != nullptr && (strcmp(event->getName(), name) == 0))
        {
            // check if(event->getClassName() == class_name AND event->getFullName() != full_name) then enter the statement
            if(strcmp(event->getClassName(), class_name) == 0 && strcmp(event->getFullName(), full_name) != 0)
            {
                agentMSG* msg = check_and_cast<agentMSG*>(event);
                if (msg->getSenderModule()->getFullName() == getFullName() && msg->getArrivalTime() == time)
                {
                    found = true;
                    cancelAndDelete(msg);
                }
            }
        }
        k++;
    }
}

void agent::cancel_message_internal_goal_arrival(const char* name, const int plan_id, const int original_plan_id)
{
    cObject* event;
    const char* class_name = "agentMSG";
    const char* full_name = "endsimulation";
    Logger::Level logger_level = Logger::EveryInfo;

    logger->print(logger_level, " ----cancel_message_internal_goal_arrival: plan:["+ std::to_string(plan_id)
    +"], original plan:["+ std::to_string(original_plan_id) +"]---- ");

    for(int i = 0; i < this->getSimulation()->getFES()->getLength(); i++)
    {
        event = this->getSimulation()->getFES()->get(i);
        if (event != nullptr && (strcmp(event->getName(), name) == 0))
        {
            // check if(event->getClassName() == class_name AND event->getFullName() != full_name) then enter the statement
            if(strcmp(event->getClassName(), class_name) == 0 && strcmp(event->getFullName(), full_name) != 0)
            {
                agentMSG* msg = check_and_cast<agentMSG*>(event);
                if (msg->getSenderModule()->getFullName() == getFullName())
                {
                    logger->print(logger_level, "plan_id: "+ std::to_string(msg->getAg_internal_goal_plan_id()) +" == "+ std::to_string(plan_id)
                    +" && original plan id: "+ std::to_string(msg->getAg_internal_goal_original_plan_id()) +" == "+ std::to_string(original_plan_id));

                    if(msg->getAg_internal_goal_plan_id() == plan_id && msg->getAg_internal_goal_original_plan_id() == original_plan_id)
                    {
                        logger->print(logger_level, "Cancel: plan id: "+ std::to_string(plan_id)
                        +", original plan id: "+ std::to_string(original_plan_id)
                        +", index: "+ std::to_string(i));
                        cancelAndDelete(msg);
                        i -= 1; // update index
                        executed_msg += 1;
                    }
                }
            }
        }
    }
}

void agent::cancel_message_intention_completion(const char* name, const std::string intention_goal_name, const bool intention_waiting_for_completion,
        const int intention_goal_id, const int intention_plan_id, const int intention_original_plan_id)
{
    logger->print(Logger::EveryInfoPlus, " ----cancel_message_intention_completion---- ");
    cObject* event;

    if(intention_waiting_for_completion)
    {
        for (int i = 0; i < this->getSimulation()->getFES()->getLength(); i++) {
            event = this->getSimulation()->getFES()->get(i);
            if (event != nullptr)
            {
                if(strcmp(event->getName(), msg_name_intention_completion) == 0)
                {
                    agentMSG* msg = check_and_cast<agentMSG*>(event);

                    if (msg->getSenderModule()->getFullName() == getFullName())
                    {
                        if(msg->getAg_intention_goal_id() == intention_goal_id
                                && msg->getAg_intention_goal_name() == intention_goal_name // in order to manage the case when the intention is linked to an internal goal and the main one is removed
                                && msg->getAg_intention_original_plan_id() == intention_original_plan_id)
                        {
                            cancelAndDelete(msg);
                            i -= 1; // update index
                            executed_msg += 1;
                        }
                    }
                }
            }
        }
    }
}

void agent::cancel_message_removed_task(const int id, bool isPlanId) {
    // if 'isPlanId' is TRUE, remove those messages that refers to 'plan' with specified 'id'
    // otherwise, compare 'id' with plan's 'original_planID'
    logger->print(Logger::EveryInfoPlus, " ----cancel_message_removed_task---- ");
    int cnt_removedTasks = 0;
    cObject* event;

    for (int i = 0; i < this->getSimulation()->getFES()->getLength(); i++) {
        event = this->getSimulation()->getFES()->get(i);
        if (event != nullptr) {

            if((strcmp(event->getName(), msg_name_check_task_complete) == 0)
                    || (strcmp(event->getName(), "Task completed") == 0))
            {
                agentMSG* msg = check_and_cast<agentMSG*>(event);

                if (msg->getSenderModule()->getFullName() == getFullName()) {
                    if(isPlanId == true && msg->getAg_task_plan_id() == id) {
                        cancelAndDelete(msg);
                        cnt_removedTasks += 1;
                        executed_msg += 1;
                    }
                    else if(isPlanId == false && msg->getAg_task_original_plan_id() == id) {
                        cancelAndDelete(msg);
                        cnt_removedTasks += 1;
                        executed_msg += 1;
                    }
                }
            }
        }
    }

    logger->print(Logger::EveryInfo, "cancel_message_removed_task: use plan? "+ ag_support->boolToString(isPlanId)
            +", id:["+ std::to_string(id) +"], removed tasks:["+ std::to_string(cnt_removedTasks) +"]");
    logger->print(Logger::EveryInfoPlus, " -- DEBUG --- ");
}

int agent::pending_scheduled_messages(bool count_end_of_sim) {
    if(count_end_of_sim) {
        return this->getSimulation()->getFES()->getLength();
    }
    else {
        return this->getSimulation()->getFES()->getLength() - 1;
    }
}

void agent::print_simulation_msgs()
{   // Note: used for debug purposes
    // during the loop, when the message "simulation.scheduled-events.endsimulation" gets analyzed,
    // an error gets triggered.

    cObject* event;
    agentMSG* msg = nullptr;
    const char* class_name = "agentMSG";
    const char* full_name = "endsimulation";
    std::string error_msg = "";

    logger->print(Logger::EveryInfo, "---print_simulation_msgs---");
    logger->print(Logger::EveryInfo, "TOT MSG: "+ std::to_string(this->getSimulation()->getFES()->getLength() - 1)); // -1 do not count "simulation end" message

    for (int k = 0; k < this->getSimulation()->getFES()->getLength(); k++)
    {
        try
        {
            event = this->getSimulation()->getFES()->get(k);

            if(strcmp(event->getClassName(), class_name) == 0 && strcmp(event->getFullName(), full_name) != 0)
            {
                // DEBUG: -----------------------------------------------------
                // logger->print(Logger::Debug, event->getClassName());
                // logger->print(Logger::Debug, event->getFullName());
                // logger->print(Logger::Debug, event->getFullPath());
                // logger->print(Logger::Debug, event->getName());
                // logger->print(Logger::Debug, event->getThisPtr()->str());
                // ------------------------------------------------------------

                msg = check_and_cast<agentMSG*>(event);
                if (msg->getSenderModule()->getFullName() == getFullName()) {
                    logger->print(Logger::Default, std::to_string(k + 1)
                    +", Info: "+ std::to_string(msg->getInformative())
                    +", Order: "+ std::to_string(msg->getInsertOrder())
                    +", Time: "+ std::to_string(msg->getArrivalTime().dbl())
                    +", Priority: "+ std::to_string(msg->getSchedulingPriority())
                    +", Type:["+ msg->getFullName()
                    +"], goal name:["+ msg->getAg_intention_goal_name()
                    +"], plan_id:["+ std::to_string(msg->getAg_intention_plan_id())
                    +"], original plan_id:["+ std::to_string(msg->getAg_intention_original_plan_id()) +"]");
                }
                logger->print(Logger::EveryInfoPlus, " --- DEBUG --- ");
            }
        }
        catch(cRuntimeError const& ex) {
            error_msg = ex.what();
            logger->print(Logger::Error, "[cRuntimeError] "+ error_msg);
            ag_metric->addGeneratedError(sim_current_time(), "print_simulation_msgs", error_msg);
        }
        catch(std::runtime_error const& ex) {
            error_msg = ex.what();
            logger->print(Logger::Error, "[cRuntimeError] "+ error_msg);
            ag_metric->addGeneratedError(sim_current_time(), "print_simulation_msgs", error_msg);
        }
    }

    logger->print(Logger::EssentialInfo, "--------------------------");
}

// *****************************************************************************
// *******                   BDI FUNCTIONS IMPLEMENTATION                *******
// *****************************************************************************
int agent::activateGoalBRF(const std::vector<std::shared_ptr<Belief>>& beliefset,
        const std::vector<std::shared_ptr<Desire>>& desireset, std::vector<std::shared_ptr<Goal>>& goalset)
{
    logger->print(Logger::EssentialInfo, "\nComputing 'Belief Revision Function' on desireset... ");

    bool isPreconditionValid, isCon_conditionValid;
    bool keepIntention = false;
    std::string conditions_check_return_msg = "";
    std::string warning_msg = "";
    std::shared_ptr<Goal> goal;
    std::vector<std::shared_ptr<Goal>> achieved_goals;
    std::vector<std::shared_ptr<Intention>> achieved_intentions;
    std::vector<std::shared_ptr<Task>> empty_release_queue(0); // declared only because required by function "checkAndManageIntentionFalseConditions"

    // DEBUG *************************************************
    //    ag_Sched->ev_ag_tasks_vector_ready();
    //    logger->print(Logger::Default, "---------------------------");
    //    ag_Sched->ev_ag_tasks_vector_to_release();
    //    logger->print(Logger::Default, "---------------------------");
    //    ag_support->printIntentions(ag_intention_set, goalset);
    //    logger->print(Logger::Default, "---------------------------");
    //    ag_support->printGoalset(goalset);
    //    logger->print(Logger::Default, "---------------------------");
    // ******************************************************

    int log_level = logger->get_level(); // Debug
    // /* xx */logger->set_level(Logger::EveryInfoPlus);

    // Check if already active goals are still NOT satisfied.
    // Needed to manage the case when the reasoning cycle is activated due to a sensor reading.
    for(int i = 0; i < (int)goalset.size(); i++)
    {   // for each already active goal, check the validity of cont_conditions...
        logger->print(Logger::EveryInfo, " - Check Goal:[id=" + std::to_string(goalset[i]->get_id()) + "]");
        keepIntention = false;

        isCon_conditionValid = ag_support->checkPlanOrDesireConditions(beliefset, goalset[i]->get_cont_conditions(), conditions_check_return_msg, "goal", goalset[i]->get_id());

        logger->print(Logger::EveryInfo, "isCon_conditionValid:["+ ag_support->boolToString(isCon_conditionValid)
                +"] AND message:["+ conditions_check_return_msg +"]");

        if(isCon_conditionValid == false)
        { // Goal[i] is not valid anymore: remove goal, plan, but do not apply effects_at_end...
            logger->print(Logger::EssentialInfo, " -- Goal [id="+ std::to_string(goalset[i]->get_id())
            +"], name:["+ goalset[i]->get_goal_name()
            +"] has invalid context conditions! (debug: "+ conditions_check_return_msg
            +")\n Cont_conditions: "+ ag_support->dump_expression(goalset[i]->get_cont_conditions(), beliefset));

            logger->print(Logger::EveryInfo, " -- If goal was linked to intention, remove intention as well...");
            logger->print(Logger::EveryInfoPlus, " --- DEBUG --- ");

            // if the intention[y] is only linked to Goal[i], then remove it
            // if it is linked to other goals other than [i], keep intention active
            for(int y = 0; y < (int)ag_intention_set.size(); y++)
            {
                if(i >= 0 && i < (int)goalset.size())
                {
                    logger->print(Logger::EveryInfo, " -- "+ std::to_string(ag_intention_set[y]->get_goalID())
                    +" == "+ std::to_string(goalset[i]->get_id()));

                    if(ag_intention_set[y]->get_goalID() == goalset[i]->get_id()
                            && ag_intention_set[y]->get_goal_name() == goalset[i]->get_goal_name())
                    {
                        // check if there are other goals linked to this intention...
                        for(int k = 0; k < (int)goalset.size(); k++)
                        {
                            if(i != k && ag_intention_set[y]->get_goalID() == goalset[k]->get_id())
                            { // the intention is linked to at least to goals -> keep intention active
                                keepIntention = true;
                                break;
                            }
                        }

                        logger->print(Logger::EveryInfo, " - keepIntention: "+ ag_support->boolToString(keepIntention));
                        if(keepIntention == false)
                        {
                            std::shared_ptr<MaximumCompletionTime> plan = ag_intention_set[y]->getMainPlan();
                            std::vector<std::pair<std::shared_ptr<Plan>, int>> internal_goals = plan->getInternalGoalsSelectedPlans();

                            // remove the intention linked to this goal + the one containing it as internal goal
                            // + remove the goals linked to the removed intentions
                            checkAndManageIntentionFalseConditions(internal_goals[0].first, ag_selected_plans, goalset, ag_intention_set, beliefset, empty_release_queue);

                            // restart the loops from the beginning
                            i = -1; // start main loop from the beginning (because we do not know what "checkAndManageIntentionFalseConditions" did)
                            break;  // exit loop "for y"
                        }
                    }
                }
            }
        } // -- end of if(invalid cont_conditions)
    }
    // ----------------------------------------------------------------------------------------------

    if(logger->get_level() >= Logger::EssentialInfo) {
        logger->print(Logger::Default, "*********************************");
        logger->print(Logger::Default, " - Number of still active goals: "+ std::to_string(goalset.size()));
    }

    // check which plans are still valid (cont_conditions are valid)
    checkIfContconditionsAreValid(goalset, ag_selected_plans, beliefset, ag_intention_set);

    logger->print(Logger::EveryInfo, " CHECK DESIRESET (tot = "+ std::to_string(desireset.size()) +")");
    logger->print(Logger::EveryInfo, " - Number of still active goals: "+ std::to_string(goalset.size()));
    logger->print(Logger::EveryInfoPlus, " --- DEBUG --- ");

    // check all the desires ....
    for(std::shared_ptr<Desire> desire : desireset)
    {   // for each desire that is not already a goal, check preconditions...
        logger->print(Logger::EveryInfo, " - Check Desire:[id="+ std::to_string(desire->get_id()) +"], name:["+ desire->get_goal_name() +"]");

        if(ag_support->checkIfDesireIsGoal(desire, goalset)) {
            logger->print(Logger::EveryInfo, " - Desire:[id="+ std::to_string(desire->get_id()) +"], name:["+ desire->get_goal_name() +"] is already a goal.");
        }
        else if(ag_support->checkIfDesireIsLinkedToValidSkill(desire, ag_skill_set) == false) {
            warning_msg = "[WARNING] INVALID DESIRE:["+ desire->get_goal_name() +"] NOT linked to any skill! -> Can not be activated!";
            logger->print(Logger::Warning, warning_msg);
            ag_metric->addGeneratedWarning(sim_current_time(), "activateGoalBRF", warning_msg);
        }
        else {
            isPreconditionValid = true;
            conditions_check_return_msg = "";

            isPreconditionValid = ag_support->checkPlanOrDesireConditions(beliefset, desire->get_preconditions(), conditions_check_return_msg, "desire", desire->get_id());

            logger->print(Logger::EveryInfo, "isPreconditionValid:["+ ag_support->boolToString(isPreconditionValid)
                    +"] AND message:["+ conditions_check_return_msg +"]");

            if(isPreconditionValid == false)
            {   // The Desire can't be activated....
                logger->print(Logger::EveryInfo, " -- Desire:[id="+ std::to_string(desire->get_id()) + "], name:["+ desire->get_goal_name() +"] not valid! "+ conditions_check_return_msg
                        +", Preconditions: "+ ag_support->dump_expression(desire->get_preconditions(), beliefset));

                /* The selected desire can't be transformed into goal due to it's invalid preconditions.
                 * We need to check if it was a goal before BRF got called...
                 * Note: this "if" should always be false. If a desire was already a goal,
                 * it has been managed by the previous loop "for(int i = 0; i < (int)goalset.size(); i++)" */
                if(checkIfDesireWasAGoalAndRemoveIt(desire)) {
                    logger->print(Logger::Error, "[CAREFUL!] Goal:["+ std::to_string(desire->get_id())
                    +"], name:["+ desire->get_goal_name() +"] has invalid preconditions -> removed... ");
                    logger->print(Logger::EveryInfo, " --- And it was a goal. Goal it's been removed (removed its plan and its intention if set)");
                }
            }
            else { // The desire preconditions are valid. It may either be a new one or can already be a goal...
                // dynamically update the desire's priority value
                ag_support->computeDesirePriority(desire, beliefset);

                // Convert the desire to goal (the goal will have the same priority of the desire!)
                goal = std::make_shared<Goal>(desire->get_id(),
                        -1, -1, // plan and original plan id for this goal are not set yet
                        desire->get_goal_name(),
                        desire->get_preconditions(),
                        desire->get_cont_conditions(),
                        desire->get_priority(),
                        sim_current_time().dbl(),
                        0, // arrival time of goals derived from desires is always 0
                        Goal::ACTIVE, desire->get_DDL());

                if(ag_support->checkDesireToAvoidDuplicatedGoals(goalset, goal) == false)
                {
                    int goal_index = goal->is_desire_in_goalset(goalset);
                    // if the desire was not a goal in the previous reasoning cycle...
                    if(goal_index == -1)
                    { // then, it's a NEW goal. Add it to the goal set and move on with the reasoning cycle...
                        goalset.push_back(goal);
                        ag_metric->addActivatedGoal(goal, sim_current_time().dbl());
                        logger->print(Logger::EveryInfo, " - ACTIVATED GOAL: "+ goal->get_goal_name()); // DEBUG:

                        logger->print(Logger::EveryInfo, " -- Desire [id="+ std::to_string(desire->get_id()) +"], name:["+ desire->get_goal_name() +"] has been added to goalset");
                        logger->print(Logger::EveryInfoPlus, " -- Number of goals activated: "+ std::to_string(goalset.size()));
                    }
                    else { /* otherwise, if the desire was already a goal we have to: */
                        // 1. update goal priority value
                        goalset[goal_index]->set_priority(desire->get_priority());

                        // 2. check if its selected_plan is still valid
                        logger->print(Logger::EssentialInfo, "The desire '"+ goal->get_goal_name() +"' was already a goal, we have to check if its selected_plan is still valid...");

                        if(checkIfGoalsPlanIsStillValid(desire, goal) == false) {
                            logger->print(Logger::EveryInfo, "Goal's plan is not valid anymore.");
                            ag_metric->setActivatedGoalCompletion(goal, sim_current_time().dbl(), "Goal's plan is not valid anymore.");
                        }
                        else {
                            logger->print(Logger::EveryInfo, "Goal's plan STILL valid!");
                        }
                    }
                }
            }
            logger->print(Logger::EveryInfo, "-------------");
        }
    }
    logger->print(Logger::EveryInfoPlus, " - Number of goals active by BRF: "+ std::to_string(goalset.size()));

    // update the content of the already active intentions
    updateIntentionsContent(ag_intention_set);

    // remove intention that are not valid anymore (a different plan must be used for the same goal; a goal is not active anymore...)
    removeInvalidIntentions(goalset, ag_selected_plans);

    logger->set_level(log_level); // Debug
    return goalset.size();
}

// If Goal.Belief equal Plan.Belief the plan is added to a filtered_list of 'applicable_plans' that can lead to achieve the Goal
void agent::unifyGoals(const std::vector<std::shared_ptr<Goal>>& goalset,
        std::vector<std::shared_ptr<Plan>>& planset, const std::vector<std::shared_ptr<Belief>>& beliefset,
        std::vector<std::vector<std::shared_ptr<Plan>>>& applicable_plans)
{
    logger->print(Logger::EssentialInfo, "\nComputing function 'unifyGoals'...");
    int goal_index = 0;
    std::vector<std::shared_ptr<Action>> body;

    for(std::shared_ptr<Goal> goal : goalset)
    {
        logger->print(Logger::EveryInfoPlus, " - Check goal... " + goal->get_goal_name());

        // check all the available plans ...
        for(int p = 0; p < (int)planset.size(); p++)
        {
            logger->print(Logger::EveryInfoPlus, " -- Check plans: "+ goal->get_goal_name() + " == " + planset[p]->get_goal_name());

            int intention_index = -1;
            if(ag_support->checkIfPlanWasAlreadyIntention(planset[p], intention_index, ag_intention_set)) {
                body = ag_intention_set[intention_index]->get_mainPlanActions();
            }
            else { // the plan is not already an intention:
                body = planset[p]->make_a_copy_of_body();
            }

            // If Goal.belief equal Plan.belief, then the plan may satisfy the goal
            if(goal->get_goal_name() == planset[p]->get_goal_name())
            { /* then, this plan may be valid for achieving the specified goal.
             * add the plan to the list of plans for the specific goal */

                /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
                double plan_pref = planset[p]->get_pref();
                plan_pref = ag_support->computePlanPref(planset[p], beliefset);
                /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

                applicable_plans[goal_index].push_back(std::make_shared<Plan>(
                        planset[p]->get_id(),
                        planset[p]->get_goal_name(),
                        plan_pref,
                        planset[p]->get_preconditions(),
                        planset[p]->get_cont_conditions(),
                        planset[p]->get_post_conditions(),
                        planset[p]->get_effects_at_begin(),
                        planset[p]->get_effects_at_end(),
                        body
                ));
                logger->print(Logger::EveryInfoPlus, " - Plan [id="+ std::to_string(planset[p]->get_id())+ "] is valid for this goal ...");
            }
        }
        logger->print(Logger::EveryInfoPlus, "-------------------");
        goal_index++;
    }
    logger->print(Logger::EveryInfo, "*********************************");
    logger->print(Logger::EveryInfo, " -> num of valid Plan(s): " + std::to_string(ag_support->countGoalsPlans(applicable_plans)));
    logger->print(Logger::EveryInfo, "*********************************");
}

/* When this function is called, each active goal has a set of 'applicable_plans' that can be used to achieve it.
 * For each goal, remove all the plans that have not valid preconditions... */
void agent::unifyPreconditions(std::vector<std::shared_ptr<Goal>>& goalset, std::vector<std::vector<std::shared_ptr<Plan>>>& applicable_plans)
{
    logger->print(Logger::EssentialInfo, "\nComputing function 'unifyPreconditions'...");

    std::vector<std::shared_ptr<Task>> empty_release_queue(0); // declared only because required by function "checkAndManageIntentionFalseConditions"
    bool allPreconditionsAreValid = true;
    int index_intention = -1;
    std::string conditions_check_return_msg = "";
    json precs; // plan's preconditions or context-conditions
    std::shared_ptr<Plan> plan;
    std::shared_ptr<Goal> goal;

    // for each goal ...
    for(int g = 0; g < (int)applicable_plans.size(); g++)
    {
        logger->print(Logger::EveryInfoPlus, "goal:["+ goalset[g]->get_goal_name() +"]");

        // loop over each plan assigned to the goal ...
        for(int p = 0; p < (int)applicable_plans[g].size(); p++)
        {
            plan = applicable_plans[g][p];
            /******************************************************************
             * Check if the plan was already assigned to a goal in a previous BRF.
             * If it was, check its 'context_cond' instead of 'prec'
             ******************************************************************/
            if(ag_support->checkIfPlanWasAlreadyIntention(applicable_plans[g][p], index_intention, ag_intention_set) == false)
            { // if no, just check the plan's preconditions...
                precs = plan->get_preconditions();
                logger->print(Logger::EveryInfoPlus, " - Plan: "+ plan->get_goal_name() +" has "+ std::to_string(precs.size()) +" precondition(s)");

                allPreconditionsAreValid = ag_support->checkPlanOrDesireConditions(ag_belief_set, precs, conditions_check_return_msg, "plan", plan->get_id());

                logger->print(Logger::EveryInfo, "allPreconditionsAreValid:["+ ag_support->boolToString(allPreconditionsAreValid)
                        +"] AND message:["+ conditions_check_return_msg +"]");
                logger->print(Logger::EveryInfoPlus, " --- DEBUG --- ");

                if(allPreconditionsAreValid == false)
                {
                    logger->print(Logger::EssentialInfo, " (unifyPreconditions) -> Removed Plan:[id=" + std::to_string(plan->get_id())
                    + "], name:["+ plan->get_goal_name() +"], yet applicable plans:["+ std::to_string(applicable_plans[g].size() - 1)
                    +"] because preconditions DO NOT hold! Preconditions: "+ ag_support->dump_expression(precs, ag_belief_set));

                    // remove this plan from the list of filtered ones, it's not valid:
                    allPreconditionsAreValid = false;
                    applicable_plans[g].erase(applicable_plans[g].begin() + p);
                    p -= 1; // after removing a plan from the list, all the others are moved toward 0 of 1 position
                }
            }
            else { // if yes, we must check the context-conditions of the plan
                precs = plan->get_cont_conditions();
                logger->print(Logger::EveryInfoPlus, " - Plan: "+ plan->get_goal_name() +" has "+ std::to_string(precs.size()) +" cont-condition(s)");

                allPreconditionsAreValid = ag_support->checkPlanOrDesireConditions(ag_belief_set, precs,
                        conditions_check_return_msg, "plan", plan->get_id(),
                        ag_intention_set[index_intention]->get_expression_constants(), ag_intention_set[index_intention]->get_expression_variables());

                logger->print(Logger::EveryInfo, "allPreconditionsAreValid:["+ ag_support->boolToString(allPreconditionsAreValid)
                        +"] AND message:["+ conditions_check_return_msg +"]");

                if(allPreconditionsAreValid == false)
                {
                    goal = std::make_shared<Goal>(*goalset[g]); // store the current goal...

                    logger->print(Logger::EveryInfo, " -> Removed Plan [id=" + std::to_string(plan->get_id())
                    + "], name:["+ plan->get_goal_name() +"], yet applicable plans:["+ std::to_string(applicable_plans[g].size() - 1)
                    +"] because context-conditions do not hold! Details: "+ conditions_check_return_msg);
                    logger->print(Logger::EssentialInfo, " Cont_conditions: "+ ag_support->dump_expression(precs, ag_belief_set,
                            ag_intention_set[index_intention]->get_expression_constants(), ag_intention_set[index_intention]->get_expression_variables()));

                    // remove this plan from the list of filtered ones, it's not valid:
                    allPreconditionsAreValid = false;
                    applicable_plans[g].erase(applicable_plans[g].begin() + p);

                    // remove the intention and the linked ones...
                    checkAndManageIntentionFalseConditions(plan, ag_selected_plans, goalset, ag_intention_set, ag_belief_set, empty_release_queue);

                    /* --------------------------------------------------------------------------------------
                     * if goalset[g] still has applicable plans after 'p'-th has been removed,
                     * then we need to add back goal 'g' in the goalset (g was removed due to funtion "checkAndManageIntentionFalseConditions") */
                    if((int)applicable_plans[g].size() > 0) {
                        goalset.insert(goalset.begin() + g, goal);
                    }
                    // --------------------------------------------------------------------------------------

                    p -= 1; // after removing a plan from the list, all the others are moved toward 0 of 1 position
                }
            }
        }

        if(applicable_plans[g].size() == 0) {
            logger->print(Logger::EssentialInfo, " >>> Goal:["+ std::to_string(goalset[g]->get_id())
            +"], belief: '"+ goalset[g]->get_goal_name() +"' has no applicable plans -> removed! <<< ");

            ag_metric->setActivatedGoalCompletion(goalset[g], sim_current_time().dbl(), "Goal's plan has invalid preconditions");
            goalset.erase(goalset.begin() + g);
            applicable_plans.erase(applicable_plans.begin() + g);
            g -= 1;
        }
        logger->print(Logger::EveryInfoPlus, "-------------------");
    }
    logger->print(Logger::EveryInfo, "*********************************");
    logger->print(Logger::EveryInfo, " -> num of valid Plan(s): " + std::to_string(ag_support->countGoalsPlans(applicable_plans)));
    logger->print(Logger::EveryInfo, "*********************************");
}

bool agent::updateIntentionset(const std::vector<std::shared_ptr<MaximumCompletionTime>>& selectedPlans,
        std::vector<std::shared_ptr<Belief>>& beliefset,
        std::vector<std::shared_ptr<Intention>>& intentionset)
{   // Given the set of selectedPlans, update the list of intentions.
    logger->print(Logger::EssentialInfo, "\nUpdating Intentionset[tot="+ std::to_string(intentionset.size()) +"]... ");

    std::shared_ptr<Action> current_action;
    std::shared_ptr<Intention> current_intention;
    bool isPlanAlreadyAnIntention = false;
    bool updated_beliefset = false; // if at least one effects_at_begin changes the beliefset, updated_beliefset = true and a new reasoning cycle must be scheduled immediately
    int tmp_cnt; // number of beliefs update due to effects_at_begin
    int insert_index;

    for(int i = 0; i < (int)selectedPlans.size(); i++)
    {
        std::shared_ptr<MaximumCompletionTime> MCTplan = selectedPlans[i];
        std::shared_ptr<Plan> plan = MCTplan->getInternalGoalsSelectedPlans()[0].first;
        isPlanAlreadyAnIntention = false; // In order to avoid adding multiple times the same plan to intention_set.
        insert_index = i;

        if(plan != nullptr) {
            for(std::shared_ptr<Intention> intention : intentionset)
            {
                if(intention->get_goal_name() == plan->get_goal_name())
                {
                    isPlanAlreadyAnIntention = true;
                    current_intention = intention;
                    break;
                }
            }

            if(isPlanAlreadyAnIntention == false)
            {
                logger->print(Logger::EssentialInfo, " - Selected Plan:["+ std::to_string(plan->get_id())
                +"] for Goal:["+ std::to_string(ag_goal_set[i]->get_id())
                +"], name:["+ ag_goal_set[i]->get_goal_name() +"], contains "+ std::to_string(plan->get_body().size()) +" actions.");

                logger->print(Logger::EveryInfoPlus, "i >= (int)intentionset.size(): "+ std::to_string(i) +" >= "+ std::to_string((int)intentionset.size()));
                if(i >= (int)intentionset.size()) {
                    insert_index = intentionset.size(); // basically pushing the new intention to the end of the list
                }

                reasoning_cycle_new_activated_goals.push_back(ag_goal_set[i]->get_goal_name()); // list of goals activated during the current instant of time and that will be send to che client
                intentionset.insert(intentionset.begin() + insert_index,
                        std::make_shared<Intention>(phiIFCFS_index, ag_goal_set[i],
                                plan->get_id(), MCTplan, ag_belief_set,
                                plan->get_effects_at_begin(), plan->get_effects_at_end(),
                                plan->get_post_conditions(),
                                sim_current_time().dbl()
                        )
                );

                // --------------------------------------------------------------------------
                // check and apply (if present) EFFECTS_AT_BEGIN of this intention...
                tmp_cnt = ag_support->updateBeliefsetDueToEffect(ag_belief_set,
                        intentionset[intentionset.size()-1],
                        intentionset[intentionset.size()-1]->get_effects_at_begin());

                logger->print(Logger::EssentialInfo, " Updated beliefs: "+ std::to_string(tmp_cnt));

                if(tmp_cnt > 0) {
                    updated_beliefset = true;
                }
                /* Clear list of "effects_at_begin" as soon as they have been applied.
                 * The reason is that if they update the beliefset,
                 * a reasoning cycle must be performed immediately.
                 * Thus, clearing the list guarantees that no reasoning cycle loops get activated */
                intentionset[intentionset.size()-1]->clear_effects_at_begin();
                // --------------------------------------------------------------------------

                ag_metric->addActivatedIntentions(intentionset[intentionset.size()-1], sim_current_time().dbl());
                phiIFCFS_index += 1;
            }
            else { // ****** DEBUG **************************************************************************
                logger->print(Logger::EveryInfo, " - Plan's body [id="+ std::to_string(current_intention->get_planID())
                +" original_plan:["+ std::to_string(current_intention->get_original_planID())
                +"] already an intention. Actions left: "+ std::to_string(current_intention->get_Actions().size()));
            } // ***************************************************************************************
        }
        else {
            logger->print(Logger::EveryInfo, " Plan is null!");
        }
    }

    return updated_beliefset;
}

void agent::updateIntentionsContent(std::vector<std::shared_ptr<Intention>>& intentionset) {
    logger->print(Logger::EveryInfo, " \n updateIntentionsContent... ");
    std::shared_ptr<Task> task;
    std::shared_ptr<Goal> goal;
    std::vector<std::shared_ptr<Action>> actions;
    int plan_id, original_plan_id;
    int intention_index = -1;

    for(int i = 0; i < (int)intentionset.size(); i++) {
        actions = intentionset[i]->get_Actions();

        original_plan_id = intentionset[i]->get_original_planID();
        plan_id = intentionset[i]->get_planID();

        for(int y = 0; y < (int)actions.size(); y++)
        {
            if(actions[y]->get_type() == "task")
            {
                task = std::dynamic_pointer_cast<Task>(actions[y]);
                if((plan_id == original_plan_id && task->getTaskPlanId() != plan_id))
                {   /* before this iteration of the reasoning cycle, this intention was linked to a different plan...
                 * Such plan is not active anymore (intentionset[i]->get_planID() == intentionset[i]->get_original_planID()).
                 * Thus, update isBeenActivated() of the task.
                 * Note: N_exec is not restored, we keep the current one...
                 * PROBLEM: tmp_t will be managed as soon as phiI is called...
                 * Es. tmp_t: compTime 1, ddl 10, N_exec 2. tmp_t completed at t: 1. If this happens at t:8, the second repetition will be anticipated from 10 to 8. */

                    // 1. check if the task was already in RELEASE or READY QUEUE:
                    std::shared_ptr<Task> foundTaskReady = ag_Sched->get_ready_task(task->getTaskId(), task->getTaskPlanId(), task->getTaskOriginalPlanId(), task->getTaskDemander(), task->getTaskReleaseTime());
                    std::shared_ptr<Task> foundTaskRelease = ag_Sched->get_release_task(task->getTaskId(), task->getTaskPlanId(), task->getTaskOriginalPlanId(), task->getTaskDemander(), task->getTaskReleaseTime());

                    if(foundTaskReady != nullptr || foundTaskRelease != nullptr) {
                        // 2. otherwise, update their planid
                        ag_Sched->update_tasks_planid_ready(task->getTaskId(), task->getTaskPlanId(), task->getTaskOriginalPlanId(), plan_id);
                        ag_Sched->update_tasks_planid_release(task->getTaskId(), task->getTaskPlanId(), task->getTaskOriginalPlanId(), plan_id);
                    }
                    else { // 2. if no in ready nor release, update isBeenActivated
                        task->setTaskisBeenActivated(false);
                    }

                }
                task->setTaskPlanId(plan_id);
                task->setTaskOriginalPlanId(original_plan_id);
            }
            else if(actions[y]->get_type() == "goal") {
                goal = std::dynamic_pointer_cast<Goal>(actions[y]);

                intention_index = -1;
                ag_support->checkIfGoalIsLinkedToIntention(goal, intentionset, intention_index);

                if((plan_id == original_plan_id && goal->get_plan_id() != plan_id) && (intention_index == -1)) {
                    goal->setGoalisBeenActivated(false);
                }
                if(intention_index >= 0 && intention_index == i) {
                    goal->setGoalisBeenActivated(true);
                }
                goal->set_plan_id(plan_id); // FORCE update plan id
            }
        }
    }
}

void agent::validate_knowledgebase(const std::pair<bool, json> status) {
    // if the setup of the agent generates an error, terminate the entire simulation
    if(status.first == false) {
        if(status.second.find("status") != status.second.end()) {
            if(status.second["status"] == "error") {
                exit(EXIT_FAILURE);
            }
        }
    }
}

/******** PHIF + support functions *********************************************************************/
std::pair<std::vector<std::shared_ptr<MaximumCompletionTime>>, std::vector<int>> agent::phiF(std::vector<std::shared_ptr<Goal>>& goalset,
        std::vector<std::vector<std::shared_ptr<MaximumCompletionTime>>>& feasiblePlans)
{
    logger->print(Logger::EssentialInfo, "\nComputing phiF function... ");

    int log_level = logger->get_level(); // Debug
    //    /* xx */logger->set_level(Logger::PrintNothing);
    //    /* xx */ag_support->setLogger_level(Logger::PrintNothing);

    bool isFeasible = true;
    int num_of_goals = goalset.size(); // called 'k' in the paper RT_BDI_Journal
    int index_intention = -1;

    /* Index of the currently selected plan for each goal
     * (feasiblePlans are ordered by pref. Select the first one for each goal. It's the most preferred one.) */
    std::vector<int> sPIndexes(num_of_goals, 0);
    std::vector<std::shared_ptr<MaximumCompletionTime>> selectedPlans(num_of_goals); // Array of currently chosen plans, initially empty

    for(int i = 0; i < num_of_goals; i++)
    {
        isFeasible = false;

        if((int)feasiblePlans[i].size() > sPIndexes[i])
        {
            logger->print(Logger::EssentialInfo, "\n Check feasibility for Plan Original ID:["+ std::to_string(feasiblePlans[i][sPIndexes[i]]->getInternalGoalsSelectedPlans()[0].first->get_id())
            +"], name:["+ feasiblePlans[i][sPIndexes[i]]->getInternalGoalsSelectedPlans()[0].first->get_goal_name() +"]");

            isFeasible = IsSchedulable_v2(feasiblePlans[i][sPIndexes[i]], selectedPlans);
            logger->print(Logger::EveryInfo, "isFeasible: "+ ag_support->boolToString(isFeasible));

            if(isFeasible) {
                selectedPlans[i] = feasiblePlans[i][sPIndexes[i]];
            }
        }

        if(isFeasible == false)
        {   // v2: remove intention of dropped goals --------------
            sPIndexes[i] += 1; // Try next plan for the goal...
            if(sPIndexes[i] >= (int)feasiblePlans[i].size()) { // No more plans for goal i...
                sPIndexes[i] = 0;
                if(i > 0) { // Try again by lowering the preference of preceding goal..
                    i -= 1;
                    sPIndexes[i] = sPIndexes[i] + 1;
                    selectedPlans[i] = nullptr; // ADDED w.r.t. to paper
                }
                else if(i == 0)
                { // Drop least priority goal
                    logger->print(Logger::EssentialInfo, " Drop least prioritize goal:["+ std::to_string(goalset[goalset.size() - 1]->get_id())
                    +"], name:["+ goalset[goalset.size() - 1]->get_goal_name() +"]");

                    // if the removed plan was an intention, remove the intention as well.
                    // Otherwise, even if a goal gets dropped, its intention may remain active an occupy part of the U
                    if(ag_support->checkIfGoalIsLinkedToIntention(goalset[goalset.size() - 1], ag_intention_set, index_intention))
                    {
                        logger->print(Logger::EssentialInfo, " Removed intention:[id="+ std::to_string(ag_intention_set[index_intention]->get_id())
                        +"], name:["+ ag_intention_set[index_intention]->get_goal_name() +"]");

                        ag_intention_set.erase(ag_intention_set.begin() + index_intention);
                    }

                    feasiblePlans.erase(feasiblePlans.end());
                    sPIndexes.pop_back();
                    selectedPlans.erase(selectedPlans.end());
                    num_of_goals -= 1;
                    ag_metric->setActivatedGoalCompletion(goalset[goalset.size()-1], sim_current_time().dbl(), "Drop least prioritize goal. Scheduling not feasible");
                    goalset.erase(goalset.end()); // ADDED 10/02/21: without this line, dropped goals are actually never removed from ag_goal_set, not even in following functions
                }
            }
            // v2 -------------------------------------------------

            i -= 1; // perform another iteration and check again...
        }
    }

    logger->set_level(log_level); // Debug
    ag_support->setLogger_level(log_level); // Debug
    return std::make_pair(selectedPlans, sPIndexes);
}

bool agent::IsSchedulable_v2(std::shared_ptr<MaximumCompletionTime>& MCT_plan,
        const std::vector<std::shared_ptr<MaximumCompletionTime>>& selectedPlans)
{
    Logger::Level logger_level = Logger::EssentialInfo;
    int log_level = logger->get_level(); // debug purposes
    logger->print(logger_level, "-----------------------\n-----IsSchedulable-----\n-----------------------");

    // in order to guarantee that we have a valid plan from which to extract a list of actions...
    if(MCT_plan == nullptr) {
        return false;
    }

    /* list of servers already summed to U:
     * the concept is that for all consecutive parallel tasks, I do:
     * - U += task.U if the task is PERIODIC
     * - U += task.server.U if the server is not been added already */
    std::vector<int> servers_needed;

    std::shared_ptr<Plan> plan = MCT_plan->getInternalGoalsSelectedPlans()[0].first;
    std::shared_ptr<Task> task;
    std::shared_ptr<Goal> goal;
    std::vector<std::shared_ptr<Action>> body = plan->get_body();
    std::shared_ptr<MaximumCompletionTime> ddl_intention;
    std::vector<std::shared_ptr<Plan>> contained_plans; // used to detect loops among plans
    std::vector<schedulable_interval> intervals_U;
    std::string report_value = "";
    std::string warning_msg = "";
    int batch = 0; // 0 default value, 1 first batch of PAR tasks, 2 second batch and so on...
    int index_internal_goal = 1, last_used_index_interna_goal = 1; // refer to i-th correct goal in the main plan
    int start_index_internal_goal = 1; // value of index_internal_goal at the beginning of the for loop
    int intention_goal_index = -1;
    int plan_id = plan->get_id();
    int intention_plan_index = -1;
    double t_start = 0, t_end = 0;
    double t_arrivalTime_delta = sim_current_time().dbl();
    double t_end_batch = 0; // t_start-t_end_batch of the PAR tasks in the current batch
    double tmp_t_end = 0;
    double interval_t_start = 0;
    double server_original_ddl;
    double U = 0;   // utilization factor task
    double Uis = 0; // utilization factor of the intention-set (IS)
    double tmp_first_batch_delta_time;
    double first_batch_delta_time = sim_current_time().dbl();
    double intention_batch_startTime = sim_current_time().dbl();
    bool isGoalLinkedToIntention = false; // used to avoid summing "intention.start" with "goal.arrivalTime" ("goal.arrivalTime" has already been considered to compute "intention.start")
    bool isTaskInAlreadyActiveServer = false; // used when we find an APERIODIC task connected to an active server
    bool checkNextTask = true; // false as soon as the next task is not PARALLEL
    bool get_start_from_intention = false;
    bool server_already_counted = false; // used to avoid counting the same server multiple times in the same batch
    bool loop_detected = false;
    bool skip_intentionset = false;
    bool internal_goal_waiting_to_completion = false;

    /* Every time we find an internal goal, we add it here.
     * We compute the start,end interval of the batch that contains it, and then we computeU of the internal goal.
     * If the internal goal it's SEQ and the following action is SEQ, start and end will be:
     * - start = t_arrivalTime_delta
     * - end = t_arrivalTime_delta + ddl_goal (because the tasks of the main plan will need to wait for the internal goal to complete, before proceeding
     * After the internal goal, start = end; end will be computed based on the following tasks */
    std::vector<std::shared_ptr<Goal>> internal_goal_in_batch;

    /* We must avoid consider the same plan more than once within the same batch.
     * Note: this vector must be reset before every batch of the main plan. */
    std::vector<std::shared_ptr<Plan>> plans_in_batch;

    logger->print(logger_level, "Plan:["+ std::to_string(plan_id) +"], name:["+ plan->get_goal_name() +"], Body.size: "+ std::to_string(body.size()));
    // write simulation events report
    write_debug_isschedulable_json_report("time: "+ sim_current_time().str() +", Plan:["+ std::to_string(plan_id) +"]");

    if(ag_support->checkIfPlanWasAlreadyIntention(plan, intention_plan_index, ag_intention_set))
    {
        intention_batch_startTime = ag_intention_set[intention_plan_index]->get_batch_startTime();
        first_batch_delta_time = intention_batch_startTime;
        tmp_first_batch_delta_time = intention_batch_startTime;

        body = ag_intention_set[intention_plan_index]->get_Actions();

        logger->print(logger_level, "Debug > Intention:["+ std::to_string(ag_intention_set[intention_plan_index]->get_id())
        +"] for plan:["+ std::to_string(ag_intention_set[intention_plan_index]->get_planID())
        +"] original_plan:["+ std::to_string(ag_intention_set[intention_plan_index]->get_original_planID())
        +"], batch_startTime:["+ std::to_string(intention_batch_startTime)
        +"], first_batch_delta_time:["+ std::to_string(first_batch_delta_time) +"]");
        logger->print(Logger::EveryInfoPlus, " --- DEBUG --- ");
    }
    else {
        logger->print(logger_level, "Debug > Plan:["+ std::to_string(plan->get_id())
        +"] original_plan:["+ std::to_string(plan->get_id())
        +"], batch_startTime:["+ std::to_string(intention_batch_startTime)
        +"], first_batch_delta_time:["+ std::to_string(first_batch_delta_time) +"]");
    }
    logger->print(Logger::EveryInfoPlus, " --- DEBUG --- ");

    for(int i = 0; i < (int)body.size(); i++)
    {
        servers_needed.clear();
        plans_in_batch.clear();
        intervals_U.clear();
        checkNextTask = true;
        server_already_counted = false;
        isGoalLinkedToIntention = false;
        internal_goal_waiting_to_completion = false;
        skip_intentionset = true;
        U = 0;
        start_index_internal_goal = index_internal_goal;
        batch += 1;

        logger->print(Logger::EveryInfo, "index_internal_goal: "+ std::to_string(index_internal_goal));

        report_value = "[Main plan] New Batch: "+ std::to_string(batch);
        ag_support->checkAndAddPlanToPlansInBatch(plans_in_batch, plan);

        // write simulation events report
        write_debug_isschedulable_json_report(report_value);
        logger->print(logger_level, "\n"+ report_value +"\n------------------------");
        logger->print(logger_level, "OVERLAP: start of new batch!");
        logger->print(logger_level, "t_start: "+ std::to_string(t_start) +", t_end: "+ std::to_string(t_end)
        +", t_arrivalTime_delta: "+ std::to_string(t_arrivalTime_delta)
        +", t_end_batch: "+ std::to_string(t_end_batch)
        +", tmp_t_end: "+ std::to_string(tmp_t_end)
        +", intention_batch_startTime: "+ std::to_string(intention_batch_startTime)
        +", first_batch_delta_time: "+ std::to_string(first_batch_delta_time));

        if(body[i]->get_type() == "goal")
        { // here, the goal must be SEQ
            goal = std::dynamic_pointer_cast<Goal>(body[i]);

            // ******************************************************************************************
            // check the correct goal among the ones that have annidation level = 1
            if(index_internal_goal < MCT_plan->getInternalGoalsSelectedPlansSize())
            { // get the current index of goal
                for(int k = index_internal_goal; k < MCT_plan->getInternalGoalsSelectedPlansSize(); k++)
                {
                    std::shared_ptr<Plan> tmp_plan = MCT_plan->getInternalGoalsSelectedPlans()[k].first;
                    int annidation_level = MCT_plan->getInternalGoalsSelectedPlans()[k].second;

                    if(tmp_plan->get_id() == -1 && annidation_level == 1) { // we are analyzing an already satisfied plan
                        last_used_index_interna_goal = index_internal_goal;
                        index_internal_goal = -1;
                        break;
                    }
                    else {
                        if(tmp_plan->get_goal_name() == goal->get_goal_name()
                                && annidation_level == 1)
                        {
                            break;
                        }
                        else {
                            index_internal_goal += 1;
                        }
                    }
                }
            }
            // ******************************************************************************************

            if(index_internal_goal < 0) {
                warning_msg = "[WARNING] IsSchedulable.a: Internal Goal:["+ std::to_string(goal->get_id()) +"] already satisfied -> skip it...";
                logger->print(Logger::Warning, warning_msg);
                ag_metric->addGeneratedWarning(sim_current_time(), "IsSchedulable_v2", warning_msg);

                index_internal_goal = last_used_index_interna_goal + 1;
            }
            else
            {
                // the first element in the batch is a Goal:
                if(batch == 1)
                {
                    skip_intentionset = false;
                    if(ag_support->checkIfGoalIsLinkedToIntention(goal, ag_intention_set, intention_goal_index))
                    {
                        ddl_intention = computeGoalDDL(MCT_plan->getInternalGoalsSelectedPlans()[index_internal_goal].first,
                                contained_plans, loop_detected, ag_intention_set[intention_goal_index]->get_batch_startTime(),
                                0, true, skip_intentionset);

                        if(ddl_intention == nullptr) {
                            logger->print(Logger::EveryInfo, "Skip intention...");
                            isGoalLinkedToIntention = false;
                            skip_intentionset = true;
                        }
                        else {
                            logger->print(logger_level, " SEQ ddl_intention ->->->->-> "+ std::to_string(ddl_intention->getMCTValue()));

                            /* the goal's plan is linked to an active intention,
                             * and the goal arrival time is lower than the deadline of the intention.
                             * This means that when the goal will be activated, it will be "executed" and "completed" accordingly to how the active intention behaves.
                             * Note: this check is useful when we have multiple plans with the same internal goal,
                             * because we are going to simulate all those goals with just one intention */
                            if(goal->get_arrival_time() >= ddl_intention->getMCTValue())
                            {
                                logger->print(logger_level, "The internal goal:[arrival="+ std::to_string(goal->get_arrival_time())
                                +"] does not overlap intention:["+ std::to_string(ag_intention_set[intention_goal_index]->get_id())
                                +"], intention_ddl:["+ std::to_string(ddl_intention->getMCTValue()) +"]");

                                isGoalLinkedToIntention = false;
                                skip_intentionset = true;
                            }
                            else { // goal and intention overlap
                                t_start = ag_intention_set[intention_goal_index]->get_batch_startTime();

                                isGoalLinkedToIntention = true;
                            }
                        }
                    }
                    // the goal is not linked to any active intention...
                    else if(goal->get_absolute_arrival_time() > 0)
                    {
                        t_start = intention_batch_startTime;
                        isGoalLinkedToIntention = false;
                    }
                    else {
                        t_start = intention_batch_startTime;
                    }
                }

                logger->print(logger_level, "Batch t_start: "+ std::to_string(t_start));

                // Compute "tmp_first_batch_delta_time" (used to 'computeGoalDDL' of the goal)
                if(batch == 1 // only goals in the first batch can be linked to intentions [*]
                        && isGoalLinkedToIntention // in order to ensure that [*] is true and that the rest of the statement is ignored
                        && ag_support->checkIfPlanWasAlreadyIntention(MCT_plan->getInternalGoalsSelectedPlans()[index_internal_goal].first, intention_plan_index, ag_intention_set))
                {
                    tmp_first_batch_delta_time = ag_intention_set[intention_plan_index]->get_batch_startTime();

                    if(ag_intention_set[intention_plan_index]->get_waiting_for_completion())
                    { // the intention linked to the goal has no more actions and it's waiting for completion

                        logger->print(Logger::EveryInfo, "check limits: "+ std::to_string(ag_intention_set[intention_plan_index]->get_scheduled_completion_time())
                        +" < ("+ std::to_string(intention_batch_startTime) +" + "+ std::to_string(goal->get_arrival_time())
                        +") = "+ std::to_string(ag_intention_set[intention_plan_index]->get_scheduled_completion_time())
                        +" < "+ std::to_string(intention_batch_startTime + goal->get_arrival_time()));
                        if(ag_intention_set[intention_plan_index]->get_scheduled_completion_time() < intention_batch_startTime + goal->get_arrival_time())
                        {   // the arrival time of the goal is further in time w.r.t. the scheduled end of the intention

                            logger->print(logger_level, " - Internal goal:["+ ag_intention_set[intention_plan_index]->get_goal_name()
                                    +"] is linked to intention:["+ std::to_string(ag_intention_set[intention_plan_index]->get_id())
                            +"], plan:["+ std::to_string(ag_intention_set[intention_plan_index]->get_planID())
                            +"], original plan:["+ std::to_string(ag_intention_set[intention_plan_index]->get_original_planID())
                            +"], completion time at: "+ std::to_string(ag_intention_set[intention_plan_index]->get_scheduled_completion_time()));

                            tmp_first_batch_delta_time = intention_batch_startTime + goal->get_arrival_time();
                            logger->print(logger_level, " - Use goal data! Intention completed before the arrival of the goal -- tmp_first_batch_delta_time: "+ std::to_string(tmp_first_batch_delta_time));
                        }
                        else { // The intention and the goal overlap
                            logger->print(logger_level, " - Use Intention data! tmp_first_batch_delta_time: "+ std::to_string(tmp_first_batch_delta_time));
                            internal_goal_waiting_to_completion = true;
                        }
                    }
                }
                else { // either not in batch 1, or isGoalLinkedToIntention = false, or goal not linked to intention
                    tmp_first_batch_delta_time = intention_batch_startTime + goal->get_arrival_time();
                }

                if(internal_goal_waiting_to_completion == false)
                {   // compute the goal ddl using the actions of the intention (skip_intentionset = false) or plan (skip_intentionset = true) linked to the goal.
                    logger->print(Logger::EveryInfo, "GOAL: computeGoalDDL("+ std::to_string(tmp_first_batch_delta_time) +")");
                    t_end = computeGoalDDL(MCT_plan->getInternalGoalsSelectedPlans()[index_internal_goal].first,
                            contained_plans, loop_detected, tmp_first_batch_delta_time, 0, false, skip_intentionset)->getMCTValue();
                }
                else {  // the goal is linked to an intention waiting for completion
                    t_end = ddl_intention->getMCTValue();
                }
                t_end_batch = t_end;

                // ----------------------------------------------
                if(isGoalLinkedToIntention == false) {
                    logger->print(logger_level, "Internal goal SEQ: t_start: "+ std::to_string(t_start)
                    +" + goal.arrivalTime: "+ std::to_string(goal->get_arrival_time())
                    +" = "+ std::to_string(t_start + goal->get_arrival_time()));
                    interval_t_start = t_start + goal->get_arrival_time();
                }
                else {
                    logger->print(Logger::EveryInfo, "Internal goal SEQ: t_start: "+ std::to_string(t_start));
                    interval_t_start = t_start;
                }
                logger->print(Logger::EveryInfo, " interval_t_start: "+ std::to_string(interval_t_start));
                // ----------------------------------------------

                logger->print(logger_level, "Batch t_start:["+ std::to_string(t_start) +"], t_end:["+ std::to_string(t_end) +"]");
                // internal_goal_in_batch are analyzed later, as soon as the actions in the current batch have been managed
                internal_goal_in_batch.push_back(goal);

                // Add the new interval
                manage_new_schedulable_interval(intervals_U, define_struct(interval_t_start, t_end));

                logger->print(logger_level, " 1.batch Main Plan:["+ std::to_string(plan_id)
                +"], name:["+ MCT_plan->getInternalGoalsSelectedPlans()[index_internal_goal].first->get_goal_name()
                +"], computed for Plan:["+ std::to_string(MCT_plan->getInternalGoalsSelectedPlans()[index_internal_goal].first->get_id())
                +"] -> computeGoalDDL: "+ std::to_string(t_end));
                index_internal_goal += 1;
            }
        }
        else if(body[i]->get_type() == "task")
        {
            task = std::dynamic_pointer_cast<Task>(body[i]);

            logger->print(logger_level, " Task:["+ std::to_string(task->getTaskId())
            +"], plan:["+ std::to_string(task->getTaskPlanId())
            +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId())
            +"], bach_startTime:["+ std::to_string(intention_batch_startTime)
            +"], first_batch_delta_time:["+ std::to_string(first_batch_delta_time)
            +"] has ArrivalTime: '"+ std::to_string(task->getTaskArrivalTime()) +"'");

            get_start_from_intention = false;
            if(batch == 1)
            {
                for(int k = 0; k < (int)ag_intention_set.size(); k++)
                {
                    logger->print(Logger::EveryInfo,"Is "+ std::to_string(ag_intention_set[k]->get_planID()) +" == "+ std::to_string(task->getTaskPlanId())
                    +" && "+ std::to_string(ag_intention_set[k]->get_original_planID()) +" == "+ std::to_string(task->getTaskOriginalPlanId()));

                    if(ag_intention_set[k]->get_planID() == task->getTaskPlanId()
                            && ag_intention_set[k]->get_original_planID() == task->getTaskOriginalPlanId())
                    {
                        t_arrivalTime_delta = ag_intention_set[k]->get_batch_startTime();
                        t_start = t_arrivalTime_delta;

                        logger->print(Logger::EveryInfo,"[YES] t_arrivalTime_delta: "+ std::to_string(t_arrivalTime_delta)
                        +", t_start: "+ std::to_string(t_start));
                        get_start_from_intention = true;
                        break;
                    }
                }
            }

            if(get_start_from_intention == false)
            {
                t_start = t_arrivalTime_delta + task->getTaskArrivalTime();
                logger->print(Logger::EveryInfo, " ££ t_start: "+ std::to_string(t_arrivalTime_delta)
                +" + "+ std::to_string(task->getTaskArrivalTime())
                +" = "+ std::to_string(t_start)+ " ££ ");
            }

            ag_support->checkIfServerAlreadyCounted(ag_Server_Handler, task, server_already_counted,
                    servers_needed, Logger::EssentialInfo);

            t_end = ag_support->getTaskDeadline_based_on_nexec(task, isTaskInAlreadyActiveServer,
                    server_original_ddl, ag_servers, ag_Sched->get_tasks_vector_ready(),
                    ag_Sched->get_tasks_vector_to_release());

            logger->print(logger_level, " Task:["+ std::to_string(task->getTaskId())
            +"], plan:["+ std::to_string(task->getTaskPlanId())
            +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId())
            +"] -> computeGoalDDL: "+ std::to_string(t_end));

            // Add the intention start time only to elements in the first batch...
            if(task->getTaskisPeriodic())
            { // PERIODIC task -> save the ddl of the task
                double periodic_task_ddl = t_end;
                if(task->getTaskNExecInitial() == -1) {
                    periodic_task_ddl = infinite_ddl;
                }

                logger->print(Logger::EveryInfo, "PERIODIC > Task:["+ std::to_string(task->getTaskId())
                +"], plan:["+ std::to_string(task->getTaskPlanId())
                +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId())
                +"], DDL: ("+ std::to_string(periodic_task_ddl)
                +" + "+ std::to_string(intention_batch_startTime)
                +") --> "+ std::to_string(intention_batch_startTime + periodic_task_ddl));
                t_end = periodic_task_ddl + intention_batch_startTime;
            }
            else
            { // APERIODIC task
                if(isTaskInAlreadyActiveServer)
                { // the selected server is already active
                    if(t_start > t_end)
                    {   /* This means that the server absolute ddl is lower then the current analyzed time
                     * Thus, use the default server period as deadline */
                        logger->print(Logger::EveryInfo, "(Used server A) > Server:["+ std::to_string(task->getTaskServer())
                        +"], Task:["+ std::to_string(task->getTaskId())
                        +"], plan:["+ std::to_string(task->getTaskPlanId())
                        +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId())
                        +"], DDL: "+ std::to_string(server_original_ddl));

                        t_end = server_original_ddl;
                    }
                    else { // This means that the server overlaps this batch
                        /* ---------------------   taskDDL
                         *              ****        batch   --> ddl = interval (batch to DDL)
                         *                    ****  batch   --> ddl = interval (batch to DDL) */
                        logger->print(Logger::EveryInfo, "(Used server B) > Server:["+ std::to_string(task->getTaskServer())
                        +"], Task:["+ std::to_string(task->getTaskId())
                        +"], plan:["+ std::to_string(task->getTaskPlanId())
                        +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId())
                        +"], DDL:("+ std::to_string(t_end)
                        +") --> "+ std::to_string(t_end));
                    }
                }
                else { // not already instantiated server
                    logger->print(Logger::EveryInfo, "(NOT Used server) > Server:["+ std::to_string(task->getTaskServer())
                    +"], Task:["+ std::to_string(task->getTaskId())
                    +"], plan:["+ std::to_string(task->getTaskPlanId())
                    +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId())
                    +"], DDL: ("+ std::to_string(t_end)
                    +" + "+ std::to_string(intention_batch_startTime)
                    +") --> "+ std::to_string(intention_batch_startTime + t_end));
                    t_end = intention_batch_startTime + t_end;
                }
            }
            logger->print(logger_level, " t_end_batch: "+ std::to_string(t_end));
            // -------------------------------------------------------------------
            t_end_batch = t_end;

            // V2: every task linked to the same server influence the overall U --------------------------------
            U = computeTaskU(task);
            logger->print(logger_level, " >>> (SEQ) UPDATED U = "+ std::to_string(U));

            manage_new_schedulable_interval(intervals_U, define_struct(t_start, t_end, U));
            // -------------------------------------------------------------------------------------------------

            /* V1: in order to update the U only once for every task linked to the server, wrap v2 in:
             * if(server_already_counted == false) { v2 } else { manage_new_schedulable_interval(intervals_U, define_struct(t_start, t_end)); } */
        }

        logger->print(Logger::EveryInfo, "a- t_start:["+ std::to_string(t_start) +"], t_end:["+ std::to_string(t_end) +"]");
        logger->print(Logger::EveryInfoPlus, "----DEBUG----");

        // stay in the loop as long as we have PAR actions...
        while(checkNextTask && i + 1 < (int)body.size())
        {
            i += 1; // action position w.r.t. to the body
            isGoalLinkedToIntention = false;
            internal_goal_waiting_to_completion = false;

            if(body[i]->get_type() == "goal")
            {
                goal = std::dynamic_pointer_cast<Goal>(body[i]);

                if(goal->getGoalExecutionType() == Goal::EXEC_SEQUENTIAL) {
                    checkNextTask = false;
                    i -= 1;
                    logger->print(Logger::EveryInfo, " GOAL SEQ - end of batch");
                    break; // end of batch
                }
                else if(goal->getGoalExecutionType() == Goal::EXEC_PARALLEL)
                {
                    checkNextTask = true;
                    skip_intentionset = true;

                    // check the correct goal among the ones that have annidation level = 1
                    if(index_internal_goal < MCT_plan->getInternalGoalsSelectedPlansSize())
                    { // get the current index of goal
                        for(int k = index_internal_goal; k < MCT_plan->getInternalGoalsSelectedPlansSize(); k++)
                        {
                            std::shared_ptr<Plan> tmp_plan = MCT_plan->getInternalGoalsSelectedPlans()[k].first;
                            int annidation_level = MCT_plan->getInternalGoalsSelectedPlans()[k].second;

                            if(tmp_plan->get_id() == -1 && annidation_level == 1) { // we are analyzing an already satisfied plan
                                last_used_index_interna_goal = index_internal_goal;
                                index_internal_goal = -1;
                                break;
                            }
                            else
                            {
                                if(tmp_plan->get_goal_name() == goal->get_goal_name() && annidation_level == 1) {
                                    break;
                                }
                                else {
                                    index_internal_goal += 1;
                                }
                            }
                        }
                    }
                    // ---------------------------------------------------

                    if(index_internal_goal < 0) {
                        warning_msg = "[WARNING] IsSchedulable.b: Internal Goal:["+ std::to_string(goal->get_id());
                        warning_msg += "] already satisfied -> skip it...";
                        logger->print(Logger::Warning, warning_msg);
                        ag_metric->addGeneratedWarning(sim_current_time(), "IsSchedulable_v2", warning_msg);

                        index_internal_goal = last_used_index_interna_goal + 1;
                    }
                    else if(index_internal_goal >= MCT_plan->getInternalGoalsSelectedPlansSize()) {
                        logger->print(Logger::Error, "index_internal_goal >= MCT_plan->getInternalGoalsSelectedPlansSize() -> "+ std::to_string(index_internal_goal)
                        +" >= "+ std::to_string(MCT_plan->getInternalGoalsSelectedPlansSize()));
                    }
                    else
                    {
                        if(batch == 1)
                        {
                            skip_intentionset = false;

                            if(ag_support->checkIfGoalIsLinkedToIntention(goal, ag_intention_set, intention_goal_index))
                            {
                                ddl_intention = computeGoalDDL(MCT_plan->getInternalGoalsSelectedPlans()[index_internal_goal].first,
                                        contained_plans, loop_detected, ag_intention_set[intention_goal_index]->get_batch_startTime(),
                                        0, true, skip_intentionset);

                                if(ddl_intention == nullptr) {
                                    logger->print(Logger::EveryInfo, "Skip intention...");
                                }
                                else {
                                    logger->print(logger_level, " PAR ddl_intention ->->->->-> "+ std::to_string(ddl_intention->getMCTValue()));

                                    if(goal->get_arrival_time() >= ddl_intention->getMCTValue()) {
                                        logger->print(logger_level, "overlapping internal goal:[arrival="+ std::to_string(goal->get_arrival_time())
                                        +"], intention_ddl:["+ std::to_string(ddl_intention->getMCTValue()) +"]");

                                        isGoalLinkedToIntention = false;
                                        skip_intentionset = true;
                                    }
                                    else {
                                        isGoalLinkedToIntention = true;
                                        logger->print(logger_level, "while SET: intention_startTime = "+ std::to_string(ag_intention_set[intention_goal_index]->get_startTime()) +", t_start: "+ std::to_string(t_start));
                                    }
                                }
                            }
                            else {
                                logger->print(Logger::EveryInfo, "while SET: t_start: "+ std::to_string(t_start));
                            }
                        }
                        else {
                            logger->print(Logger::EveryInfo, "while SET batch["+ std::to_string(batch)
                            +"]: batch_startTime = "+ std::to_string(intention_batch_startTime)
                            +", t_start: "+ std::to_string(t_start));
                        }

                        logger->print(logger_level, "while Batch t_start: "+ std::to_string(t_start));

                        std::vector<std::shared_ptr<Plan>> contained_plans; // used to detect loops among plans
                        if(batch == 1 && isGoalLinkedToIntention
                                && ag_support->checkIfPlanWasAlreadyIntention(MCT_plan->getInternalGoalsSelectedPlans()[index_internal_goal].first,
                                        intention_plan_index, ag_intention_set))
                        {
                            tmp_first_batch_delta_time = ag_intention_set[intention_plan_index]->get_batch_startTime();

                            if(ag_intention_set[intention_plan_index]->get_waiting_for_completion())
                            {
                                logger->print(logger_level, "check limits: "+ std::to_string(ag_intention_set[intention_plan_index]->get_scheduled_completion_time())
                                +" < ("+ std::to_string(intention_batch_startTime) +" + "+ std::to_string(goal->get_arrival_time())
                                +") = "+ std::to_string(ag_intention_set[intention_plan_index]->get_scheduled_completion_time())
                                +" < "+ std::to_string(intention_batch_startTime + goal->get_arrival_time()));

                                if(ag_intention_set[intention_plan_index]->get_scheduled_completion_time() < intention_batch_startTime + goal->get_arrival_time())
                                {
                                    logger->print(logger_level, " - Internal goal:["+ ag_intention_set[intention_plan_index]->get_goal_name()
                                            +"] is linked to intention:["+ std::to_string(ag_intention_set[intention_plan_index]->get_id())
                                    +"], plan:["+ std::to_string(ag_intention_set[intention_plan_index]->get_planID())
                                    +"], original plan:["+ std::to_string(ag_intention_set[intention_plan_index]->get_original_planID())
                                    +"], completion time at: "+ std::to_string(ag_intention_set[intention_plan_index]->get_scheduled_completion_time()));

                                    tmp_first_batch_delta_time = intention_batch_startTime + goal->get_arrival_time();
                                    logger->print(logger_level, " - Use goal data! Intention completed before the arrival of the goal -- tmp_first_batch_delta_time: "+ std::to_string(tmp_first_batch_delta_time));
                                }
                                else {
                                    logger->print(logger_level, " - Use Intention data! tmp_first_batch_delta_time: "+ std::to_string(tmp_first_batch_delta_time));
                                    internal_goal_waiting_to_completion = true;
                                }
                            }
                        }
                        else {
                            tmp_first_batch_delta_time = intention_batch_startTime + goal->get_arrival_time();
                        }

                        if(internal_goal_waiting_to_completion == false)
                        {
                            logger->print(logger_level, "GOAL:["+ goal->get_goal_name() +"], computeGoalDDL("+ std::to_string(tmp_first_batch_delta_time) +")");
                            tmp_t_end = computeGoalDDL(MCT_plan->getInternalGoalsSelectedPlans()[index_internal_goal].first,
                                    contained_plans, loop_detected, tmp_first_batch_delta_time, 0, false, skip_intentionset)->getMCTValue();
                        }
                        else {
                            tmp_t_end = ddl_intention->getMCTValue(); // the goal is linked to an intention that will complete after
                        }
                        logger->print(logger_level, " t_end = std::max("+ std::to_string(t_end) +", "+ std::to_string(tmp_t_end) +")");
                        t_end = std::max(t_end, tmp_t_end);
                        t_end_batch = std::max(t_end_batch, t_end);

                        // -----------------------------------------------------------------
                        if(isGoalLinkedToIntention == false)
                        {
                            logger->print(logger_level, "Internal goal PAR: t_start: "+ std::to_string(t_start)
                            +", first_batch_delta_time: "+ std::to_string(first_batch_delta_time)
                            +" + goal.arrivalTime: "+ std::to_string(goal->get_arrival_time())
                            +" = "+ std::to_string(first_batch_delta_time + goal->get_arrival_time()));

                            interval_t_start = first_batch_delta_time + goal->get_arrival_time();
                        }
                        else {
                            logger->print(logger_level, "Internal goal PAR: t_start: "+ std::to_string(t_start));
                            interval_t_start = t_start;
                        }
                        logger->print(logger_level, " interval_t_start: "+ std::to_string(interval_t_start));
                        // -----------------------------------------------------------------

                        logger->print(logger_level, "Batch t_start:["+ std::to_string(t_start) +"], t_end:["+ std::to_string(t_end) +"]");

                        internal_goal_in_batch.push_back(goal);
                        manage_new_schedulable_interval(intervals_U, define_struct(interval_t_start, tmp_t_end));

                        logger->print(logger_level, " 2. Main Plan:["+ std::to_string(plan_id)
                        +"], name:["+ MCT_plan->getInternalGoalsSelectedPlans()[index_internal_goal].first->get_goal_name()
                        +"], computed for Plan:["+ std::to_string(MCT_plan->getInternalGoalsSelectedPlans()[index_internal_goal].first->get_id())
                        +"] -> computeGoalDDL: "+ std::to_string(tmp_t_end));
                        index_internal_goal += 1;
                    }
                }
            }
            else if(body[i]->get_type() == "task")
            {
                task = std::dynamic_pointer_cast<Task>(body[i]);

                if(task->getTaskExecutionType() == Task::EXEC_SEQUENTIAL) {
                    checkNextTask = false;
                    i -= 1;
                    logger->print(Logger::EveryInfo, " TASK SEQ - end of batch");
                    break; // end of batch
                }
                else if(task->getTaskExecutionType() == Task::EXEC_PARALLEL)
                {
                    logger->print(logger_level, " (PAR) Task:["+ std::to_string(task->getTaskId())
                    +"], plan:["+ std::to_string(task->getTaskPlanId())
                    +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId())
                    +"], bach_startTime:["+ std::to_string(intention_batch_startTime)
                    +"], first_batch_delta_time:["+ std::to_string(first_batch_delta_time)
                    +"], t_arrivalTime_delta:["+ std::to_string(t_arrivalTime_delta)
                    +"] has ArrivalTime: '"+ std::to_string(task->getTaskArrivalTime()) +"'");

                    logger->print(Logger::EveryInfo, " t_start = min("+ std::to_string(t_start)
                    +", ("+ std::to_string(task->getTaskArrivalTime()) +"+ "+ std::to_string(t_arrivalTime_delta) +")");
                    t_start = std::min(t_start, task->getTaskArrivalTime() + t_arrivalTime_delta);

                    ag_support->checkIfServerAlreadyCounted(ag_Server_Handler, task, server_already_counted, servers_needed, Logger::EveryInfo);

                    tmp_t_end = ag_support->getTaskDeadline_based_on_nexec(task, isTaskInAlreadyActiveServer, server_original_ddl, ag_servers, ag_Sched->get_tasks_vector_ready(), ag_Sched->get_tasks_vector_to_release());
                    logger->print(logger_level, " --> task deadline (tmp_t_end) = "+ std::to_string(tmp_t_end));

                    // Add the intention start time only to elements in the first batch...
                    if(task->getTaskisPeriodic())
                    { // PERIODIC task -> save task ddl
                        double periodic_task_ddl = tmp_t_end;
                        if(task->getTaskNExecInitial() == -1) {
                            periodic_task_ddl = infinite_ddl;
                        }

                        logger->print(Logger::EveryInfo, "PERIODIC > Task:["+ std::to_string(task->getTaskId())
                        +"], plan:["+ std::to_string(task->getTaskPlanId())
                        +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId())
                        +"], DDL: ("+ std::to_string(periodic_task_ddl)
                        +" + "+ std::to_string(intention_batch_startTime)
                        +") --> "+ std::to_string(intention_batch_startTime + periodic_task_ddl));
                        tmp_t_end = periodic_task_ddl + intention_batch_startTime;
                    }
                    else
                    { // APERIODIC task
                        if(isTaskInAlreadyActiveServer)
                        { // the selected server is already active
                            if(t_start > tmp_t_end)
                            {   // This means that the server absolute ddl is lower then the current analyzed time
                                // Thus, use the default server period as deadline
                                logger->print(Logger::EveryInfo, "(Used server A) > Server:["+ std::to_string(task->getTaskServer())
                                +"], Task:["+ std::to_string(task->getTaskId())
                                +"], plan:["+ std::to_string(task->getTaskPlanId())
                                +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId())
                                +"], DDL: "+ std::to_string(server_original_ddl));

                                tmp_t_end = server_original_ddl;
                            }
                            else {// This means that the server overlaps this batch
                                /* ---------------------   taskDDL
                                 *              ****        batch   --> ddl = interval (batch to DDL)
                                 *                    ****  batch   --> ddl = interval (batch to DDL) */
                                logger->print(Logger::EveryInfo, "(Used server B) > Server:["+ std::to_string(task->getTaskServer())
                                +"], Task:["+ std::to_string(task->getTaskId())
                                +"], plan:["+ std::to_string(task->getTaskPlanId())
                                +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId())
                                +"], DDL:("+ std::to_string(tmp_t_end)
                                +") --> "+ std::to_string(tmp_t_end));
                            }
                        }
                        else { // not already instantiated server
                            logger->print(Logger::EveryInfo, "(NOT Used server) > Server:["+ std::to_string(task->getTaskServer())
                            +"], Task:["+ std::to_string(task->getTaskId())
                            +"], plan:["+ std::to_string(task->getTaskPlanId())
                            +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId())
                            +"], DDL: ("+ std::to_string(tmp_t_end)
                            +" + "+ std::to_string(intention_batch_startTime)
                            +") --> "+ std::to_string(intention_batch_startTime + tmp_t_end));
                            tmp_t_end = intention_batch_startTime + tmp_t_end;
                        }
                    }
                    logger->print(logger_level, " --> task absolute deadline PAR (t_end_batch): "+ std::to_string(tmp_t_end));
                    // ------------------------------------------------------------
                    logger->print(logger_level, " t_end = std::max("+ std::to_string(t_end) +", "+ std::to_string(tmp_t_end) +")");
                    t_end = std::max(t_end, tmp_t_end);
                    t_end_batch = std::max(t_end_batch, t_end);

                    logger->print(Logger::EveryInfo, "b- t_start:["+ std::to_string(t_start) +"], t_end:["+ std::to_string(t_end) +"]");


                    // V2: every task linked to the same server influence the overall U --------------------------------
                    // [IMPORTANT] To simulate worst case, multiple overlapping tasks linked to the same server are considered as indipendent when U is computed
                    double tmp_task_U = computeTaskU(task);

                    logger->print(logger_level, " >>> (PAR) UPDATED U = "+ std::to_string(U)
                    +" + "+ std::to_string(tmp_task_U)
                    + " = "+ std::to_string(U + tmp_task_U));
                    U = U + tmp_task_U;

                    manage_new_schedulable_interval(intervals_U, define_struct(t_start, tmp_t_end, tmp_task_U));
                    // -------------------------------------------------------------------------------------------------

                    /* V1: in order to update the U only once for every task linked to the server, wrap v2 in:
                     * if(server_already_counted == false) { v2 } else { manage_new_schedulable_interval(intervals_U, define_struct(t_start, tmp_t_end)); } */
                }
            }
        } // -- end while actions are PAR (belong to the current analyzed batch)
        // *********************************************************************************************
        // ---------------------------------------------------------------------------------------------
        // *********************************************************************************************
        logger->print(logger_level, "=============================\n INTERVAL: U: "+ std::to_string(U)
        +" (only tasks), t_start: "+ std::to_string(t_start)
        +", t_end: "+ std::to_string(t_end)
        +", t_arrivalTime_delta: "+ std::to_string(t_arrivalTime_delta)
        +", t_end_batch: "+ std::to_string(t_end_batch)
        +", batch_startTime: "+ std::to_string(intention_batch_startTime)
        +", first_batch_delta_time: "+ std::to_string(first_batch_delta_time)
        +", index_internal_goal: "+ std::to_string(index_internal_goal)
        +" restored to: "+ std::to_string(start_index_internal_goal)
        +"\n=============================");
        index_internal_goal = start_index_internal_goal;

        if(logger_level >= Logger::EssentialInfo) {
            print_schedulable_interval(intervals_U);
        }

        logger->print(logger_level, " COMPUTE U OF INTERNAL GOALS (PAR)[tot="+ std::to_string(internal_goal_in_batch.size())
        +"] present in this batch:["+ std::to_string(batch) +"]...");
        logger->print(logger_level, "--------------------------------------------------------------------");

        // --------------------------------------------------------------------------------
        // COMPUTE U OF INTERNAL GOALS (PAR) present in this batch.
        // Note: if the batch starts with a Goal, it must be SEQ. This is the only SEQ goal that could be present in a batch!
        std::vector<std::pair<int, bool>> alreadyAnalizedIntentions; // pair: <intention ID, has been activated Yes/No>
        std::vector<std::pair<std::shared_ptr<Plan>, int>> goalsSelectedPlans = MCT_plan->getInternalGoalsSelectedPlans();
        double end_prev_batch = sim_current_time().dbl();
        bool check_internal_goal = true;
        int index_intention = -1;
        ag_support->initializeAlreadyActiveIntention(alreadyAnalizedIntentions, ag_intention_set);

        for(int ig = 0; ig < (int)internal_goal_in_batch.size(); ig++)
        {
            goal = internal_goal_in_batch[ig];
            check_internal_goal = true;
            logger->print(logger_level, "internal goal index: "+ std::to_string(ig)
            +", goal id: "+ std::to_string(goal->get_id())
            +", goal name:("+ goal->get_goal_name()
            +"), index_internal_goal: "+ std::to_string(index_internal_goal));

            if((int)goalsSelectedPlans.size() > index_internal_goal)
            {
                if(index_internal_goal < (int)goalsSelectedPlans.size())
                { // get the current index of goal
                    for(int k = index_internal_goal; k < (int)goalsSelectedPlans.size(); k++) {
                        std::shared_ptr<Plan> tmp_plan = goalsSelectedPlans[k].first;
                        int annidation_level = goalsSelectedPlans[k].second;

                        if(tmp_plan->get_goal_name() == goal->get_goal_name()
                                && annidation_level == 1)
                        {
                            end_prev_batch = t_start;
                            break;
                        }
                        else {
                            index_internal_goal += 1;
                        }
                    }
                }
                // ---------------------------------------------------

                if(MCT_plan->getInternalGoalsSelectedPlansSize() > index_internal_goal)
                {
                    std::shared_ptr<Plan> tmp_plan = MCT_plan->getInternalGoalsSelectedPlans()[index_internal_goal].first;
                    if(batch == 1 && ag_support->checkIfPlanWasAlreadyIntention(tmp_plan, index_intention, ag_intention_set))
                    {
                        // compute intention deadline:
                        logger->set_level(Logger::PrintNothing); // debug purposes
                        ddl_intention = computeGoalDDL(tmp_plan, contained_plans, loop_detected,
                                ag_intention_set[index_intention]->get_batch_startTime(), 0, false, skip_intentionset);
                        logger->set_level(log_level); // debug purposes

                        logger->print(logger_level, "goal.arrivalTime:["+ std::to_string(goal->get_arrival_time())
                        +"] > ddl_intention:["+ std::to_string(ddl_intention->getMCTValue()) +"]? --> "+ ag_support->boolToString((goal->get_arrival_time() > ddl_intention->getMCTValue())));

                        if(goal->get_arrival_time() > ddl_intention->getMCTValue()) // '>' in because if intention ends exactly when the goal starts, the goal has to be fully execute
                        {
                            skip_intentionset = true;
                            check_internal_goal = true;
                        }
                        else {
                            skip_intentionset = false;
                            check_internal_goal = false;
                            logger->print(logger_level, "Internal goal:["+ std::to_string(goal->get_id())
                            +"], plan:["+ std::to_string(tmp_plan->get_id())
                            +"] already an intention:[index="+ std::to_string(index_intention) +"]!");

                            if(ag_intention_set[index_intention]->get_waiting_for_completion())
                            {
                                logger->print(logger_level, "check limits: "+ std::to_string(ag_intention_set[index_intention]->get_scheduled_completion_time())
                                +" < ("+ std::to_string(intention_batch_startTime) +" + "+ std::to_string(goal->get_arrival_time())
                                +") = "+ std::to_string(ag_intention_set[index_intention]->get_scheduled_completion_time())
                                +" < "+ std::to_string(intention_batch_startTime + goal->get_arrival_time()));

                                if(ag_intention_set[index_intention]->get_scheduled_completion_time() < intention_batch_startTime + goal->get_arrival_time())
                                {
                                    logger->print(logger_level, " - Internal goal:["+ ag_intention_set[index_intention]->get_goal_name()
                                            +"] is linked to intention:["+ std::to_string(ag_intention_set[index_intention]->get_id())
                                    +"], plan:["+ std::to_string(ag_intention_set[index_intention]->get_planID())
                                    +"], original plan:["+ std::to_string(ag_intention_set[index_intention]->get_original_planID())
                                    +"], completion time at: "+ std::to_string(ag_intention_set[index_intention]->get_scheduled_completion_time()));

                                    check_internal_goal = true;
                                    logger->print(logger_level, " - Use goal data!");
                                }
                                else {
                                    logger->print(logger_level, " - Use Intention data!");
                                }
                            }
                        }
                    }

                    if(check_internal_goal)
                    {
                        logger->print(logger_level, "calling computeIntentionU. t_start:["+ std::to_string(t_start)
                        +"], t_end:["+ std::to_string(t_end_batch)
                        +"], t_arrivalTime_delta:["+ std::to_string(t_arrivalTime_delta)
                        +"], end_prev_batch:["+ std::to_string(end_prev_batch)
                        +"], first_batch_delta_time:["+ std::to_string(first_batch_delta_time)
                        +"], batch_startTime:["+ std::to_string(intention_batch_startTime)
                        +"]\n, internal goal arrivalTime:["+ std::to_string(goal->get_arrival_time())
                        +"], internal goal batch_start:["+ std::to_string(intention_batch_startTime + goal->get_arrival_time())
                        +"], index_internal_goal:["+ std::to_string(index_internal_goal) +"]");

                        // Case: internal goals batch
                        U = computeIntentionU_v3(MCT_plan, intervals_U, index_internal_goal,
                                servers_needed, t_start, t_end_batch, end_prev_batch, intention_batch_startTime + goal->get_arrival_time(),
                                alreadyAnalizedIntentions, plan_id, batch, plans_in_batch,
                                skip_intentionset);
                        index_internal_goal += 1;

                        logger->print(logger_level, "Plan:["+ std::to_string(MCT_plan->getInternalGoalsSelectedPlans()[0].first->get_id())
                        +"], name:["+ MCT_plan->getInternalGoalsSelectedPlans()[0].first->get_goal_name()
                        +"] updated U: "+ std::to_string(U));
                        logger->print(Logger::EveryInfoPlus, " --- DEBUG --- ");
                    }
                }
                else {
                    logger->print(Logger::EveryInfo, "MCT_plan->getInternalGoalsSelectedPlansSize() > index_internal_goal -> "
                            +std::to_string(MCT_plan->getInternalGoalsSelectedPlansSize())
                    +" > "+ std::to_string(index_internal_goal));
                }
            }
            else {
                logger->print(Logger::EveryInfo, "MCT_plan->getInternalGoalsSelectedPlansSize() > index_internal_goal -> "
                        + std::to_string(MCT_plan->getInternalGoalsSelectedPlansSize())
                +" > "+ std::to_string(index_internal_goal));
            }
        }
        internal_goal_in_batch.clear();

        logger->print(logger_level, "--------------------------------------------------------------------");
        logger->print(logger_level, " Batch "+ std::to_string(batch) +", Plan:["+ plan->get_goal_name() +"] , U: "+ std::to_string(U));
        logger->print(Logger::EveryInfo, " >>> Plan:["+ std::to_string(plan_id) +"] current U:["+ std::to_string(U) +"] <<< ");
        Uis = 0; // Utilization factor of the intention-set

        if(t_start < t_end)
        { // avoid computing the U if the current batch has interval null (start equal to end)
            logger->print(logger_level, "\n CHECK IF BATCH IS SCHEDULABLE WITH ALREADY selectedPlans:[tot="+ std::to_string(selectedPlans.size()) +"]...");
            logger->print(logger_level, "--------------------------------------------------------------------");

            // DEBUG: ----------------------------------
            if(logger->get_level() >= Logger::EssentialInfo) {
                print_schedulable_interval(intervals_U);
            }
            // -----------------------------------------

            // --------------------------------------------------------------------------------
            // CHECK IF BATCH IS COMPATIBLE with the already selected plans chosen in this reasoning cycle...
            for(int p = 0; p < (int)selectedPlans.size(); p++)
            {
                logger->print(Logger::EveryInfo, "selectedPlans["+ std::to_string(p) +"] != nullptr: " + ag_support->boolToString(selectedPlans[p] != nullptr));
                if(selectedPlans[p] != nullptr)
                {
                    if(ag_support->checkIfPlanWasAlreadyIntention(selectedPlans[p]->getInternalGoalsSelectedPlans()[0].first, index_intention, ag_intention_set) == false)
                    {
                        std::vector<std::shared_ptr<Plan>> contained_plans; // used to detect loops among plans
                        bool loop_detected = false;
                        double sp_first_batch_delta_time = sim_current_time().dbl(); // selected plans that are not intentions (the if statements guarantees that) must have first batch starting from NOW
                        double goal_ddl;

                        logger->print(logger_level, "GOAL:["+ selectedPlans[p]->getInternalGoalsSelectedPlans()[0].first->get_goal_name()
                                +"], computeGoalDDL("+ std::to_string(sp_first_batch_delta_time) +")");

                        goal_ddl = computeGoalDDL(selectedPlans[p]->getInternalGoalsSelectedPlans()[0].first, contained_plans,
                                loop_detected, sp_first_batch_delta_time, 0, false, false)->getMCTValue();

                        if(goal_ddl <= t_start) {
                            logger->print(logger_level, " selectedPlan:["+ std::to_string(selectedPlans[p]->getInternalGoalsSelectedPlans()[0].first->get_id())
                            +"] fully execute before mainPlan:["+ std::to_string(plan->get_id())
                            +"], selectedPlan:("+ std::to_string(+ goal_ddl)
                            +"), mainPlan:("+ std::to_string(t_start)
                            +", "+ std::to_string(t_end) +")");
                            logger->print(Logger::EveryInfoPlus, " --- DEBUG --- ");
                        }
                        else {
                            if(ag_support->checkAndAddPlanToPlansInBatch(plans_in_batch, selectedPlans[p]->getInternalGoalsSelectedPlans()[0].first) == false)
                            {
                                logger->print(logger_level, " Plan:["+ std::to_string(selectedPlans[p]->getInternalGoalsSelectedPlans()[0].first->get_id())
                                +"] has already present in batch:["+ std::to_string(batch) +"]");
                            }
                            else {
                                int tmp_index_intention = 0;
                                logger->print(logger_level, "Uis = "+ std::to_string(Uis));
                                logger->print(logger_level, "calling computeIntentionU. t_start:["+ std::to_string(t_start)
                                +"], t_end:["+ std::to_string(t_end_batch)
                                +"], t_arrivalTime_delta:["+ std::to_string(t_arrivalTime_delta)
                                +"], NOW:["+ sim_current_time().str() +"]");

                                // Case: selectedPlans
                                Uis = computeIntentionU_v3(selectedPlans[p], intervals_U,
                                        tmp_index_intention, servers_needed, t_start, t_end_batch,
                                        sim_current_time().dbl(), sp_first_batch_delta_time,
                                        alreadyAnalizedIntentions, plan_id, batch, plans_in_batch);

                                logger->print(logger_level, "Plan:["+ std::to_string(selectedPlans[p]->getInternalGoalsSelectedPlans()[0].first->get_id())
                                +"], name:["+ selectedPlans[p]->getInternalGoalsSelectedPlans()[0].first->get_goal_name()
                                +"] --> Updated Uis = "+ std::to_string(Uis));
                            }
                        }
                    }
                }
            } // --- end for

            // --------------------------------------------------------------------------------
            // CHECK IF BATCH IS COMPATIBLE with already active intentions
            // compare this feasible plan with the intentions already/still active
            logger->print(logger_level, "\n CHECK IF BATCH IS SCHEDULABLE WITH ACTIVE intentions:[tot="+ std::to_string(ag_intention_set.size()) +"]...");
            logger->print(logger_level, "--------------------------------------------------------------------");

            // DEBUG: ----------------------------------
            if(logger->get_level() >= Logger::EssentialInfo) {
                print_schedulable_interval(intervals_U);
            }
            // -----------------------------------------

            // -------------------------------------------------
            // check and manage intentions and selectedPlans that are referring to the beliefs with the same name,
            // and different chosen plans. Implemented due to simulation: _test-multiple-plans, time t: 57
            logger->print(Logger::EveryInfo, " Main Plan:["+ std::to_string(plan->get_id()) +"], name: "+ plan->get_goal_name());
            for(int i = 0; i < (int)ag_intention_set.size(); i++)
            {
                if(ag_intention_set[i]->get_goal_name() == plan->get_goal_name()
                        && plan->get_id() != ag_intention_set[i]->get_original_planID())
                {   // there is an intention that has the same name of the currently analyzed plan
                    logger->print(Logger::EveryInfo, " Intention:["+ std::to_string(i)
                    +"].planID = "+ std::to_string(ag_intention_set[i]->get_original_planID())
                    +", same belief of mainPlan.id:["+ std::to_string(plan->get_id())
                    +"], name = "+ plan->get_goal_name());

                    alreadyAnalizedIntentions[i].second = true;
                }
            }

            for(int i = 0; i < (int)ag_intention_set.size(); i++)
            {
                if(ag_intention_set[i]->get_original_planID() != plan_id
                        && alreadyAnalizedIntentions[i].second == false)
                {
                    int tmp_index_intention = 0;
                    logger->print(logger_level, "Intention:["+ std::to_string(ag_intention_set[i]->get_id())
                    +"], plan:["+ std::to_string(ag_intention_set[i]->get_planID())
                    +"], original_plan:["+ std::to_string(ag_intention_set[i]->get_original_planID())
                    +"], firstAct:["+ std::to_string(ag_intention_set[i]->get_firstActivation())
                    +"], startTime:["+ std::to_string(ag_intention_set[i]->get_startTime())
                    +"], batch_startTime:["+ std::to_string(ag_intention_set[i]->get_batch_startTime())
                    +"], lastExecution:["+ std::to_string(ag_intention_set[i]->get_lastExecution()) +"]");

                    // Case: intentions
                    Uis = computeIntentionU_v3(ag_intention_set[i]->getMainPlan(), intervals_U,
                            tmp_index_intention, servers_needed, t_start, t_end_batch,
                            ag_intention_set[i]->get_batch_startTime(),
                            ag_intention_set[i]->get_batch_startTime(), // <-- because the first batch of an intention will have the same value of "end_prev_batch"
                            alreadyAnalizedIntentions, plan_id, batch, plans_in_batch);

                    logger->print(logger_level, "Plan:["+ std::to_string(ag_intention_set[i]->getTopInternalSelectedPlan()->get_id())
                    +"], name:["+ ag_intention_set[i]->getTopInternalSelectedPlan()->get_goal_name()
                    +"] --> Updated Uis = "+ std::to_string(Uis));

                    if(i == (int)ag_intention_set.size() - 1) {
                        logger->print(logger_level, " -.-.-.-.- No more active intentions, Plan:["+ std::to_string(plan_id)
                        +"], end batch:["+ std::to_string(batch) +"] -.-.-.-.- ");
                    }
                    else {
                        logger->print(logger_level, " -.-.-.-.- check next active intention -.-.-.-.- ");
                    }
                }
            }
            /******************************************************************************/

            // DEBUG: ---------------------------
            if(logger->get_level() >= Logger::EssentialInfo) {
                print_schedulable_interval(intervals_U);
            }
            // ----------------------------------

            // extract from the list of intervals the max value of U...
            logger->print(Logger::EveryInfo, "DEBUG t_start: "+ std::to_string(t_start)
            +", t_end: "+ std::to_string(t_end)
            +", t_arrivalTime_delta: "+ std::to_string(t_arrivalTime_delta)
            +", first_batch_delta_time: "+ std::to_string(first_batch_delta_time)
            +"], batch_startTime:["+ std::to_string(intention_batch_startTime) +"]");
            U = compute_intervals_maxU(intervals_U, t_start, t_end);
            logger->print(logger_level, " -- Interval's MAX_U:["+ std::to_string(U) +"] -- ");

            // --------------------------------------------------------------------------------------------
            // update parameters and move to the next batch (if there are more actions to analyze and U is not already greater than 1)
            t_start = t_end;
            t_arrivalTime_delta = t_start;
            first_batch_delta_time = t_start;
            intention_batch_startTime = t_start;
            skip_intentionset = true;
            logger->print(logger_level, " t_start: "+ std::to_string(t_start)
            +", t_end: "+ std::to_string(t_end)
            +", t_arrivalTime_delta: "+ std::to_string(t_arrivalTime_delta)
            +", first_batch_delta_time: "+ std::to_string(first_batch_delta_time)
            +", batch_startTime: "+ std::to_string(intention_batch_startTime));

            if(U > 1)
            {
                report_value = "[WARNING] - U > 1? TRUE: plan:["+ std::to_string(plan->get_id());
                report_value += "], name:["+ plan->get_goal_name() +"] -> "+ std::to_string(U);
                logger->print(Logger::Warning, report_value);
                ag_metric->addGeneratedWarning(sim_current_time(), "ag_reasoning_cycle", report_value);

                // write simulation events report
                write_debug_isschedulable_json_report(report_value);

                return false;
            }
            else
            {
                report_value = " - U > 1? FALSE: plan:["+ std::to_string(plan->get_id());
                report_value += "], name:["+ plan->get_goal_name() +"] -> "+ std::to_string(U);
                logger->print(logger_level, report_value);

                // write simulation events report
                write_debug_isschedulable_json_report(report_value);
            }
            // --------------------------------------------------------------------------------------------
        }
    } // --- end for loop (i < body.size())

    if((int)body.size() == 0) {
        U = 0;
        report_value = " - U > 1? FALSE: plan:["+ std::to_string(plan_id) +"] -> "+ std::to_string(U);
        logger->print(logger_level, report_value);

        // write simulation events report
        write_debug_isschedulable_json_report(report_value);
    }

    return true;
}

agent::schedulable_interval agent::define_struct(double _start, double _end, double _U) {
    schedulable_interval interval;

    interval.start = _start;
    interval.end = _end;
    interval.U = _U;

    return interval;
}

double agent::manage_new_schedulable_interval(std::vector<agent::schedulable_interval>& intervals, agent::schedulable_interval new_interval)
{ /* given the new interval, insert in the correct spot and update the rest of the vector accordingly.
 * The idea is that we want to have an array of INDEPENDENT intervals that NEVER overlaps and that are in INCREASING order.
 * e.g., given [(5, 10, 0.1), (10, 20, 0.1)] and a new interval (7, 12, 0.2) the result is: [(5, 7), (7, 10), (10, 12), (12, 20)]
 * The U of a split interval is update to corrent_U + new U: [(0.1), (0.1 + 0.2 = 0.3), (0.1 + 0.2 = 0.3), (0.1)]
 * ---------------------------------
 * The max value of U stored in "intervals" is then returned to the caller */

    Logger::Level logger_level = Logger::EveryInfo;
    double max_U = 0;
    bool only_computes_U = false;
    std::string added_intervals = "";
    agent::schedulable_interval original_interval = new_interval;

    if(logger->get_level() >= logger_level) {
        print_schedulable_interval(intervals);
    }

    logger->print(logger_level, "New interval: start:["+ std::to_string(new_interval.start)
    +"], end:["+ std::to_string(new_interval.end)
    +"], U:["+ std::to_string(new_interval.U) +"]");

    if(new_interval.start > new_interval.end) {
        logger->print(Logger::EssentialInfo, " >> Skip interval, start is greater than end: ("+ std::to_string(new_interval.start)
        +", "+ std::to_string(new_interval.end) +")");
        only_computes_U = true; // DO NOT add this interval to the set
    }

    if(new_interval.start - ag_Sched->EPSILON <= new_interval.end && new_interval.start + ag_Sched->EPSILON >= new_interval.end && only_computes_U == false)
    {   // start == end, ignore interval
        only_computes_U = true; // DO NOT add this interval to the set
        logger->print(Logger::EssentialInfo, "New interval is empty start:["+ std::to_string(new_interval.start)
        +"] == end:["+ std::to_string(new_interval.end) +"]");
    }

    if((int)intervals.size() == 0 && only_computes_U == false)
    {
        intervals.push_back(new_interval);
        added_intervals += "["+ std::to_string(new_interval.start) +", "+ std::to_string(new_interval.end) +"], "; // debug

        logger->print(Logger::EveryInfo, "max_U = std::max("+ std::to_string(max_U) +", "+ std::to_string(intervals[0].U) +")");
        max_U = std::max(max_U, intervals[0].U);
    }
    else {
        for(int i = 0; i < (int)intervals.size(); i++)
        {
            logger->print(logger_level, "max_U = std::max("+ std::to_string(max_U) +", "+ std::to_string(intervals[i].U)
            +") = "+ std::to_string(std::max(max_U, intervals[i].U)));
            max_U = std::max(max_U, intervals[i].U);

            if(new_interval.start < intervals[i].start && new_interval.end <= intervals[i].start && only_computes_U == false)
            {   // i:       ---
                // new: ---

                intervals.insert(intervals.begin() + i, define_struct(new_interval.start, new_interval.end, new_interval.U));
                added_intervals += "["+ std::to_string(new_interval.start) +", "+ std::to_string(new_interval.end) +"], "; // debug

                max_U = std::max(max_U, new_interval.U);

                logger->print(logger_level, "max_U = std::max("+ std::to_string(max_U) +", "+ std::to_string(intervals[i].U) +")");
                max_U = std::max(max_U, intervals[i].U);

                // stop checking...
                only_computes_U = true;
            }
            else if(new_interval.start < intervals[i].start && new_interval.end > intervals[i].start && new_interval.end < intervals[i].end
                    && only_computes_U == false)
            {   // i:     ----
                // new: ----
                double i_end = intervals[i].end;
                double i_U = intervals[i].U;

                // Add new interval before 'i'
                intervals.insert(intervals.begin() + i, define_struct(new_interval.start, intervals[i].start, new_interval.U));
                added_intervals += "["+ std::to_string(new_interval.start) +", "+ std::to_string(intervals[i].start) +"], "; // debug

                // Update original interval U
                intervals[i + 1].U = intervals[i + 1].U + new_interval.U;

                // update original interval [i.start, new.end]
                intervals[i + 1].end = new_interval.end;

                // update new interval
                new_interval.start = new_interval.end;
                new_interval.end = i_end;

                if(i + 1 >= (int)intervals.size())
                {
                    // push back the new interval
                    intervals.push_back(define_struct(new_interval.start, new_interval.end, i_U));
                    added_intervals += "["+ std::to_string(new_interval.start) +", "+ std::to_string(new_interval.end) +"], "; // debug

                    logger->print(logger_level, "max_U = std::max("+ std::to_string(max_U) +", "+ std::to_string(intervals[i + 1].U) +")");
                    max_U = std::max(max_U, intervals[i + 1].U);

                    // stop checking...
                    only_computes_U = true;
                }
                else {
                    // check the next interval (do nothing here)...
                }
            }
            else if(new_interval.start < intervals[i].start && new_interval.end > intervals[i].start && new_interval.end > intervals[i].end
                    && only_computes_U == false)
            {   // i:     ----
                // new: ---------
                // Add new interval before 'i'
                intervals.insert(intervals.begin() + i, define_struct(new_interval.start, intervals[i].start, new_interval.U));
                added_intervals += "["+ std::to_string(new_interval.start) +", "+ std::to_string(intervals[i].start) +"], "; // debug

                // Update original interval U
                intervals[i + 1].U = intervals[i + 1].U + new_interval.U;

                // update "new_interval" start value
                new_interval.start = intervals[i + 1].end;

                if(i + 1 >= (int)intervals.size())
                {
                    // push back the new interval
                    intervals.push_back(define_struct(new_interval.start, new_interval.end, new_interval.U));
                    added_intervals += "["+ std::to_string(new_interval.start) +", "+ std::to_string(new_interval.end) +"], "; // debug

                    logger->print(logger_level, "max_U = std::max("+ std::to_string(max_U) +", "+ std::to_string(intervals[i + 1].U) +")");
                    max_U = std::max(max_U, intervals[i + 1].U);

                    // stop checking...
                    only_computes_U = true;
                }
                else {
                    // check the next interval (do nothing here)...
                }
            }
            else if(new_interval.start < intervals[i].start && new_interval.end > intervals[i].start
                    && fabs(new_interval.end - intervals[i].end) < ag_Sched->EPSILON
                    && only_computes_U == false)
            {   // i:     ----
                // new: ------

                // Add new interval before 'i'
                intervals.insert(intervals.begin() + i, define_struct(new_interval.start, intervals[i].start, new_interval.U));
                added_intervals += "["+ std::to_string(new_interval.start) +", "+ std::to_string(intervals[i].start) +"], "; // debug

                // Update original interval U
                intervals[i + 1].U = intervals[i + 1].U + new_interval.U;

                logger->print(logger_level, "max_U = std::max("+ std::to_string(max_U) +", "+ std::to_string(intervals[i + 1].U) +")");
                max_U = std::max(max_U, intervals[i + 1].U);

                // stop checking...
                only_computes_U = true;
            }
            else if(fabs(new_interval.start - intervals[i].start) < ag_Sched->EPSILON
                    && new_interval.end < intervals[i].end && only_computes_U == false)
            {   // i:   -------
                // new: ----

                double i_end = intervals[i].end;
                double i_U = intervals[i].U;

                // Update the U of [i.start, new.end]
                intervals[i].U = intervals[i].U + new_interval.U;

                // Updated original end value
                intervals[i].end = new_interval.end;

                // insert interval [new.end, i.end]
                intervals.insert(intervals.begin() + i + 1, define_struct(intervals[i].end, i_end, i_U));
                added_intervals += "["+ std::to_string(intervals[i].end) +", "+ std::to_string(i_end) +"], "; // debug

                logger->print(logger_level, "max_U = std::max("+ std::to_string(max_U) +", "+ std::to_string(intervals[i].U) +")");
                max_U = std::max(max_U, intervals[i].U);

                // stop checking...
                only_computes_U = true;
            }
            else if(fabs(new_interval.start - intervals[i].start) < ag_Sched->EPSILON
                    && fabs(new_interval.end - intervals[i].end) < ag_Sched->EPSILON
                    && only_computes_U == false)
            {   // i:   -------
                // new: -------
                // Update interval U
                intervals[i].U = intervals[i].U + new_interval.U;

                logger->print(logger_level, "max_U = std::max("+ std::to_string(max_U) +", "+ std::to_string(intervals[i].U) +")");
                max_U = std::max(max_U, intervals[i].U);

                // stop checking...
                only_computes_U = true;
            }
            else if(fabs(new_interval.start - intervals[i].start) < ag_Sched->EPSILON
                    && new_interval.end > intervals[i].end && only_computes_U == false)
            {   // i:   ----
                // new: -------

                // Update interval U
                intervals[i].U = intervals[i].U + new_interval.U;

                // update "new interval" start value
                new_interval.start = intervals[i].end;

                if(i + 1 >= (int)intervals.size())
                { // push back the new interval
                    intervals.push_back(define_struct(new_interval.start, new_interval.end, new_interval.U));
                    added_intervals += "["+ std::to_string(new_interval.start) +", "+ std::to_string(new_interval.end) +"], "; // debug

                    logger->print(logger_level, "max_U = std::max("+ std::to_string(max_U) +", "+ std::to_string(intervals[i].U) +")");
                    max_U = std::max(max_U, intervals[i].U);

                    // stop checking...
                    only_computes_U = true;
                }
                else {
                    // check the next interval (do nothing here)...
                }
            }
            else if(new_interval.start > intervals[i].start && new_interval.end < intervals[i].end && only_computes_U == false)
            {   // i:   ---------
                // new:   ----

                double i_end = intervals[i].end;
                double i_U = intervals[i].U;

                // Updated interval[i].end
                intervals[i].end = new_interval.start;

                // Add new interval [new.start, new.end] with Updated U
                intervals.insert(intervals.begin() + i + 1, define_struct(new_interval.start, new_interval.end, intervals[i].U + new_interval.U));
                added_intervals += "["+ std::to_string(new_interval.start) +", "+ std::to_string(new_interval.end) +"], "; // debug

                // Add new interval [new.end, i.end]
                intervals.insert(intervals.begin() + i + 2, define_struct(new_interval.end, i_end, i_U));
                added_intervals += "["+ std::to_string(new_interval.end) +", "+ std::to_string(i_end) +"], "; // debug

                logger->print(logger_level, "max_U = std::max("+ std::to_string(max_U) +", "+ std::to_string(intervals[i + 1].U) +")");
                max_U = std::max(max_U, intervals[i + 1].U);

                // stop checking...
                only_computes_U = true;
            }
            else if(new_interval.start > intervals[i].start && fabs(new_interval.end - intervals[i].end) < ag_Sched->EPSILON && only_computes_U == false)
            {   // i:   ------
                // new:   ----

                // Updated interval[i].end
                intervals[i].end = new_interval.start;

                // Add new interval [new.start, new.end] with Updated U
                intervals.insert(intervals.begin() + i + 1, define_struct(new_interval.start, new_interval.end, intervals[i].U + new_interval.U));
                added_intervals += "["+ std::to_string(new_interval.start) +", "+ std::to_string(new_interval.end) +"], "; // debug

                logger->print(logger_level, "max_U = std::max("+ std::to_string(max_U) +", "+ std::to_string(intervals[i + 1].U) +")");
                max_U = std::max(max_U, intervals[i + 1].U);

                // stop checking...
                only_computes_U = true;
            }
            else if(new_interval.start > intervals[i].start && new_interval.start < intervals[i].end
                    && new_interval.end > intervals[i].end && only_computes_U == false)
            {   // i:   ------
                // new:   -------

                double i_end = intervals[i].end;
                double i_U = intervals[i].U;

                // Updated interval[i].end
                intervals[i].end = new_interval.start;

                // Add new interval [new.start, new.end] with Updated U
                intervals.insert(intervals.begin() + i + 1, define_struct(new_interval.start, i_end, i_U + new_interval.U));
                added_intervals += "["+ std::to_string(new_interval.start) +", "+ std::to_string(i_end) +"], "; // debug

                // Update new interval start
                new_interval.start = i_end;

                if(i + 1 >= (int)intervals.size())
                { // push back the new interval
                    intervals.push_back(define_struct(new_interval.start, new_interval.end, new_interval.U));
                    added_intervals += "["+ std::to_string(new_interval.start) +", "+ std::to_string(new_interval.end) +"], "; // debug

                    logger->print(logger_level, "max_U = std::max("+ std::to_string(max_U) +", "+ std::to_string(intervals[i + 1].U) +")");
                    max_U = std::max(max_U, intervals[i + 1].U);

                    // stop checking...
                    only_computes_U = true;
                }
                else {
                    // check the next interval (do nothing here)...
                }
            }
            else if(new_interval.start >= intervals[i].end && new_interval.end > new_interval.start && only_computes_U == false)
            {   // i:   ---
                // new:    ---

                if(i + 1 >= (int)intervals.size())
                { // push back the new interval
                    intervals.push_back(define_struct(new_interval.start, new_interval.end, new_interval.U));
                    added_intervals += "["+ std::to_string(new_interval.start) +", "+ std::to_string(new_interval.end) +"], "; // debug

                    logger->print(logger_level, "max_U = std::max("+ std::to_string(max_U) +", "+ std::to_string(new_interval.U) +")");
                    max_U = std::max(max_U, new_interval.U);

                    // stop checking...
                    only_computes_U = true;
                }
                else {
                    // check the next interval (do nothing here)...
                }
            }
        }
    }

    if(logger->get_level() >= Logger::EssentialInfo) {
        logger->print(Logger::EssentialInfo, "END OF FUNCTION. Interval:("+ std::to_string(original_interval.start)
        +", "+ std::to_string(original_interval.end) +", "+ std::to_string(original_interval.U)
        +") -> RESULTS: ");

        print_schedulable_interval(intervals);
        logger->print(Logger::EssentialInfo, " Created intervals: "+ added_intervals);
        logger->print(Logger::EssentialInfo, "MAX U: "+ std::to_string(max_U));
    }

    return max_U;
}

double agent::compute_intervals_maxU(const std::vector<schedulable_interval>& intervals, const double t_start, const double t_end)
{
    double max_U = 0;
    logger->print(Logger::EveryInfo, " Interval:["+ std::to_string(t_start)
    +", "+ std::to_string(t_end) +"]");

    for(int i = 0; i < (int)intervals.size(); i++)
    {
        logger->print(Logger::EveryInfo, " - Interval:("+ std::to_string(intervals[i].start)
        +", "+ std::to_string(intervals[i].end)
        +", U: "+ std::to_string(intervals[i].U) +")");

        if(intervals[i].start < t_end && intervals[i].end > t_start) {
            max_U = std::max(max_U, intervals[i].U);
        }
    }

    return max_U;
}

void agent::print_schedulable_interval(const std::vector<schedulable_interval>& intervals)
{
    logger->print(Logger::Default, " +++ print_schedulable_interval:[tot = "+ std::to_string(intervals.size()) +"] +++ ");

    for(int i = 0; i < (int)intervals.size(); i++) {
        logger->print(Logger::Default, " - Interval:("+ std::to_string(intervals[i].start)
        +", "+ std::to_string(intervals[i].end)
        +", U: "+ std::to_string(intervals[i].U) +")");
    }
}

double agent::computeIntentionU_v3(const std::shared_ptr<MaximumCompletionTime> MCT_plan,
        std::vector<schedulable_interval>& intervals_U,
        int& index_internal_goal, std::vector<int>& servers_needed, double start_batch,
        double end_batch, double end_prev_batch, double first_batch_delta_time,
        std::vector<std::pair<int, bool>>& alreadyAnalizedIntentions,
        const int analyzed_plan_id, const int fatherPlanBatch, std::vector<std::shared_ptr<Plan>>& plans_in_batch,
        bool skip_intentionset, double goal_arrivalTime)
{
    Logger::Level logger_level = Logger::EssentialInfo;
    logger->print(logger_level, "----computeIntentionU_v3----");

    //////////////////////////////////////////////////////
    std::string msg = "";

    if(MCT_plan == nullptr) {
        msg = "computeIntentionU: MCT_plan is null!";
        logger->print(Logger::Error, "[ERROR] "+ msg);
        ag_metric->addGeneratedError(sim_current_time(), "computeIntentionU", msg);
        return 0;
    }

    if(index_internal_goal >= MCT_plan->getInternalGoalsSelectedPlansSize()) {
        logger->print(logger_level, std::to_string(index_internal_goal)
        + " < " + std::to_string(MCT_plan->getInternalGoalsSelectedPlansSize()) + "?");

        msg = "computeIntentionU.a: index out of range! "+ std::to_string(index_internal_goal);
        msg += " must be lower than "+ std::to_string(MCT_plan->getInternalGoalsSelectedPlansSize());
        logger->print(Logger::Error, "[ERROR] "+ msg);
        ag_metric->addGeneratedError(sim_current_time(), "computeIntentionU", msg);
        return 0;
    }
    else if(MCT_plan->getInternalGoalsSelectedPlans()[index_internal_goal].first->get_id() == -1) {
        msg = "[WARNING] computeIntentionU.a: Plan has id "+ std::to_string(-1);
        msg += ", it means that we are considering an already satisfied goal! -> terminate function...";
        logger->print(Logger::Warning, msg);
        ag_metric->addGeneratedWarning(sim_current_time(), "computeIntentionU_v3", msg);
        return 0;
    }

    std::shared_ptr<Plan> main_plan = MCT_plan->getInternalGoalsSelectedPlans()[index_internal_goal].first;
    std::vector<std::shared_ptr<Action>> body = main_plan->get_body();
    int main_plan_nesting_level = MCT_plan->getInternalGoalsSelectedPlans()[index_internal_goal].second;
    std::shared_ptr<MaximumCompletionTime> mctp;
    //////////////////////////////////////////////////////

    std::shared_ptr<Task> task, next_task;
    std::shared_ptr<Goal> goal, next_goal;
    std::string debug_goal_name = main_plan->get_goal_name();
    std::string report_value = "";
    double U = 0, Up = 0;   // utilization factor task
    double t_start = end_prev_batch, t_end = 0, t_arrivalTime_delta = end_prev_batch; // in order to understand how are managed the batch in the internal goal
    double tmp_t_end = 0, t_action_end = 0;
    double tmp_task_U;
    double server_original_ddl;
    double tmp_first_batch_delta_time;
    double intention_batch_startTime = 0;

    int batch = 1;
    int debug_plan_id = main_plan->get_id();
    int index_intention = -1;
    int tmpIndex = -1;
    bool validAction = true;
    bool loop_detected = false;
    bool isTaskInAlreadyActiveServer = false; // used when we find an APERIODIC task connected to an active server
    bool server_already_counted = false; // used to avoid counting the same server multiple times in the same batch
    bool skip_intention_internal_goals = false; // used when function is called recursively (Note: recursion does not impact the value in the current function)
    std::vector<std::pair<std::shared_ptr<Goal>, int>> internal_goal_in_batch;

    std::vector<std::shared_ptr<Plan>> contained_plans; // used to detect loops among plans tmp_t_end = computeGoalDDL(plan)->getMCTValue();

    logger->print(logger_level, "Plan:["+ std::to_string(debug_plan_id)
    +"], name:["+ main_plan->get_goal_name()
    +"], nesting level: "+ std::to_string(main_plan_nesting_level)
    +", Body.size: "+ std::to_string(body.size()));
    logger->print(logger_level, "start_batch: "+ std::to_string(start_batch)
    +", end_batch: "+ std::to_string(end_batch) +", end_prev_batch: "+ std::to_string(end_prev_batch)
    +", is plan:["+ std::to_string(debug_plan_id) +"], name:["+ main_plan->get_goal_name()
    +"] an intention: "+ ag_support->boolToString(ag_support->checkIfPlanWasAlreadyIntention(main_plan, ag_intention_set))
    +", mainPlanBatch: "+ std::to_string(fatherPlanBatch));

    logger->print(logger_level, "Internal goal.arrivalTime:["+ std::to_string(goal_arrivalTime) +"]");

    if(ag_support->checkIfPlanWasAlreadyIntention(main_plan, index_intention, ag_intention_set) && skip_intentionset == false)
    {
        // then we are analyzing the current main plan with an intention in the intention set
        // ADDED because the internal goals of plans are not up-to-date ---------------------
        body = ag_intention_set[index_intention]->get_Actions();
        // ----------------------------------------------------------------------------------

        logger->print(logger_level, "Intention index:["+ std::to_string(index_intention)
        +"] - Batch: "+ std::to_string(fatherPlanBatch) +", keep t_start = "+ std::to_string(ag_intention_set[index_intention]->get_lastExecution())
        +", t_arrivalTime_delta = "+ std::to_string(ag_intention_set[index_intention]->get_lastExecution()));
        t_start = ag_intention_set[index_intention]->get_batch_startTime();
        t_arrivalTime_delta = ag_intention_set[index_intention]->get_batch_startTime();
        intention_batch_startTime = ag_intention_set[index_intention]->get_batch_startTime();

        /***********************************************/
        // if the currently analyzed intention is waiting for completion, return immediately
        if(ag_intention_set[index_intention]->get_waiting_for_completion()) {
            logger->print(logger_level, "Update U before return: std::max("+ std::to_string(U) +", "+ std::to_string(Up) +") = "+ std::to_string(std::max(U, Up)));
            U = std::max(U, Up);
            if(index_internal_goal < MCT_plan->getInternalGoalsSelectedPlansSize()) {
                logger->print(logger_level, "Plan:["+ std::to_string(debug_plan_id)
                +"], name:["+ debug_goal_name
                +"] linked to intention:[index="+ std::to_string(index_intention)
                +"] waiting for completion at:["+ std::to_string(ag_intention_set[index_intention]->get_scheduled_completion_time())
                +"] -> RETURN U: "+ std::to_string(U));
            }
            else {
                std::string warning_msg = "[WARNING] index '"+ std::to_string(index_internal_goal) +"' out of range '"+ std::to_string(MCT_plan->getInternalGoalsSelectedPlansSize()) +"'!";
                logger->print(Logger::Warning, warning_msg);
                ag_metric->addGeneratedWarning(sim_current_time(), "ag_reasoning_cycle", warning_msg);
            }
            logger->print(logger_level, "------------END of computeIntentionU (empty intention)------------");

            return U;
        }
        /***********************************************/
    }
    else {
        t_start = first_batch_delta_time + goal_arrivalTime;
        t_arrivalTime_delta = first_batch_delta_time + goal_arrivalTime;
    }

    logger->print(logger_level, "Batch: "+ std::to_string(fatherPlanBatch) +", updated t_start: "+ std::to_string(t_start) +", t_arrivalTime_delta: "+ std::to_string(t_arrivalTime_delta));
    logger->print(Logger::EveryInfoPlus, " --- DEBUG --- ");

    for(int i = 0; i < (int)body.size(); i++)
    {
        if(body[i]->get_type() == "goal")
        {
            goal = std::dynamic_pointer_cast<Goal>(body[i]);

            logger->print(Logger::EveryInfo, "MCT_plan->getInternalGoalsSelectedPlansSize(): "+ std::to_string(MCT_plan->getInternalGoalsSelectedPlansSize()));

            // check the correct goal among the ones that have  annidation level = 1
            if(index_internal_goal < MCT_plan->getInternalGoalsSelectedPlansSize())
            { // get the current index of goal
                index_internal_goal += 1; // in order to start analyzing from the goal after the last analyzed one
                for(int k = index_internal_goal; k < MCT_plan->getInternalGoalsSelectedPlansSize(); k++)
                {
                    std::shared_ptr<Plan> plan = MCT_plan->getInternalGoalsSelectedPlans()[k].first;
                    logger->print(Logger::EveryInfo, std::to_string(plan->get_id())
                    +", "+ plan->get_goal_name() +" == "+ goal->get_goal_name());

                    if(plan->get_id() == -1) { // we are analyzing an already satisfied plan
                        index_internal_goal = -1;
                        break;
                    }
                    else if(plan->get_goal_name() == goal->get_goal_name()) {
                        break;
                    }
                    else {
                        index_internal_goal += 1;
                    }
                }
            }
            else {
                index_internal_goal += 1;
            }
            // ---------------------------------------------------

            if(index_internal_goal >= MCT_plan->getInternalGoalsSelectedPlansSize()) {
                std::string msg = "computeIntentionU.b: index out of range! "+ std::to_string(index_internal_goal);
                msg += " must be lower than "+ std::to_string(MCT_plan->getInternalGoalsSelectedPlansSize());
                logger->print(Logger::EveryInfo, " >> "+ msg);
                return 0;
            }
            else if(index_internal_goal == -1) {
                logger->print(Logger::EveryInfo, " >> computeIntentionU.b: index out of range! "+ std::to_string(index_internal_goal)
                +" means that we are considering an internal goal already satisfied! -> analyze following action...");
            }
            else
            {
                std::shared_ptr<Plan> plan = MCT_plan->getInternalGoalsSelectedPlans()[index_internal_goal].first;
                logger->print(logger_level, "Internal goal:["+ std::to_string(goal->get_id())
                +"], name:["+ goal->get_goal_name()
                +"] is linked to plan:["+ std::to_string(plan->get_id()) +"]");

                validAction = true;
                tmpIndex = -1;
                if(ag_support->checkIfPlanWasAlreadyIntention(plan, tmpIndex, ag_intention_set) && batch == 1)
                {
                    if(alreadyAnalizedIntentions[tmpIndex].second)
                    {
                        /* we don't want to sum the same intentions multiple times
                         * if the plan linked to the internal goal has already be anaylzed during the computation of U,
                         * let's move on to the next action */
                        validAction = false;
                    }
                    else {
                        plan->set_body(ag_intention_set[tmpIndex]->get_Actions()); // update the plan's body with the current actions from the corresponding intention
                        alreadyAnalizedIntentions[tmpIndex].second = true;
                    }
                }

                if(validAction)
                {
                    logger->print(Logger::EveryInfo, "Intention plan_id:["+ std::to_string(analyzed_plan_id)
                    +"] equal ["+ std::to_string(plan->get_id()) +"] ? "+ ag_support->boolToString(analyzed_plan_id == plan->get_id()));
                    if(analyzed_plan_id == plan->get_id())
                    {
                        contained_plans.clear(); // used to detect loops among plans
                        loop_detected = false;
                        if(ag_support->checkIfPlanWasAlreadyIntention(plan, index_intention, ag_intention_set) && batch == 1) {
                            tmp_first_batch_delta_time = ag_intention_set[index_intention]->get_batch_startTime();
                            skip_intentionset = false;
                        }
                        else {
                            tmp_first_batch_delta_time = first_batch_delta_time + goal->get_arrival_time();
                            skip_intentionset = true;
                        }
                        logger->print(logger_level, " tmp_first_batch_delta_time: "+ std::to_string(tmp_first_batch_delta_time));
                        mctp = computeGoalDDL(plan, contained_plans, loop_detected, tmp_first_batch_delta_time, 0, false, skip_intentionset);

                        if(mctp != nullptr)
                        {
                            tmp_t_end = mctp->getMCTValue();
                            // -----------------------------------------------------------------
                            logger->print(logger_level, " 7. Main Plan:["+ std::to_string(debug_plan_id)
                            +"], name:["+ main_plan->get_goal_name()
                            +"], computed for Plan:["+ std::to_string(main_plan->get_id())
                            +"] -> computeGoalDDL: "+ std::to_string(tmp_t_end));

                            if(tmp_t_end == -1) {
                                tmp_t_end = t_arrivalTime_delta + infinite_ddl;
                            }

                            if(goal->getGoalExecutionType() == Goal::EXEC_PARALLEL) {
                                t_end = std::max(t_end, tmp_t_end);
                            }
                            else if(goal->getGoalExecutionType() == Goal::EXEC_SEQUENTIAL)
                            {
                                t_end = tmp_t_end;
                            }
                            logger->print(logger_level, "After IntGoal: t_start: "+ std::to_string(t_start) +", t_end: "+ std::to_string(t_end));

                            // Version 2 -----------------------------------------------------------------
                            logger->print(logger_level, "Internal goal PAR: t_start: "+ std::to_string(t_start)
                            +" + goal.arrivalTime: "+ std::to_string(goal->get_arrival_time())
                            +" = "+ std::to_string(t_start + goal->get_arrival_time())
                            +" vs: "+ std::to_string(tmp_first_batch_delta_time));
                            logger->print(logger_level, " define_struct:("+ std::to_string(tmp_first_batch_delta_time)
                            +", "+ std::to_string(t_end) +")");

                            manage_new_schedulable_interval(intervals_U, define_struct(tmp_first_batch_delta_time, t_end));
                            // ---------------------------------------------------------------------------

                            logger->print(logger_level, "Current t_start: "+ std::to_string(t_start)
                            +", t_arrivalTime_delta: "+ std::to_string(t_arrivalTime_delta)
                            +", t_end: "+ std::to_string(t_end));
                            if(tmpIndex >= 0 && tmpIndex < (int)ag_intention_set.size()) {
                                logger->print(logger_level, " >> Skip Intention original id:["+ std::to_string(ag_intention_set[tmpIndex]->get_original_planID())
                                +"], name:["+ ag_intention_set[tmpIndex]->get_goal_name() +"]");
                            }
                        }
                    }
                    else
                    {
                        if(goal->getGoalExecutionType() == Goal::EXEC_PARALLEL)
                        {
                            logger->print(logger_level, " 5.1 Main Plan:["+ std::to_string(debug_plan_id)
                            +"], name:["+ main_plan->get_goal_name()
                            +"], computed for Plan:["+ std::to_string(main_plan->get_id())
                            +"] -> computeGoalDDL: "+ std::to_string(tmp_t_end));

                            logger->print(logger_level, "Before: tmp_first_batch_delta_time:["+ std::to_string(tmp_first_batch_delta_time)
                            +"], intention_batch_startTime:["+ std::to_string(intention_batch_startTime)
                            +"], first_batch_delta_time:["+ std::to_string(first_batch_delta_time)
                            +"], goal_arrivalTime:["+ std::to_string(goal_arrivalTime) +"]");

                            contained_plans.clear(); // used to detect loops among plans
                            loop_detected = false;
                            if(ag_support->checkIfPlanWasAlreadyIntention(plan, index_intention, ag_intention_set) && batch == 1) {
                                tmp_first_batch_delta_time = ag_intention_set[index_intention]->get_batch_startTime();
                                skip_intentionset = false;
                            }
                            else {
                                tmp_first_batch_delta_time = first_batch_delta_time + goal->get_arrival_time();
                                skip_intentionset = true;
                            }
                            logger->print(logger_level, " tmp_first_batch_delta_time:["+ std::to_string(tmp_first_batch_delta_time)
                            +"], intention_batch_startTime:["+ std::to_string(intention_batch_startTime)
                            +"], first_batch_delta_time:["+ std::to_string(first_batch_delta_time)
                            +"], goal_arrivalTime:["+ std::to_string(goal_arrivalTime) +"]");

                            mctp = computeGoalDDL(plan, contained_plans, loop_detected, tmp_first_batch_delta_time, 0, false, skip_intentionset);

                            if(mctp != nullptr)
                            {
                                tmp_t_end = mctp->getMCTValue();

                                logger->print(logger_level, " 5. Main Plan:["+ std::to_string(debug_plan_id)
                                +"], name:["+ main_plan->get_goal_name()
                                +"], computed for Plan:["+ std::to_string(main_plan->get_id())
                                +"] -> computeGoalDDL: "+ std::to_string(tmp_t_end));

                                if(tmp_t_end == -1) {
                                    tmp_t_end = t_arrivalTime_delta + infinite_ddl;
                                }

                                t_end = std::max(t_end, tmp_t_end);
                                logger->print(logger_level, "After IntGoal: t_start: "+ std::to_string(t_start) +", t_end: "+ std::to_string(t_end));

                                logger->print(logger_level, " check if the (PAR) goal overlaps with the batch: "+ std::to_string(t_start)+ " < "+ std::to_string(end_batch)
                                +" && "+ std::to_string(t_end) +" > "+ std::to_string(start_batch)
                                +" --> Plan:["+ std::to_string(plan->get_id())
                                +"] = "+ ag_support->boolToString(t_start < end_batch && t_end > start_batch));

                                if(t_start > t_end)
                                {
                                    logger->print(logger_level, " :: (PAR) Skip internal goal:["+ goal->get_goal_name()
                                            +"] because t_start: "+ std::to_string(t_start) +" > t_end: "+ std::to_string(t_end) +" ::");
                                    ag_support->checkAndRemovePlanToPlansInBatch(plans_in_batch, plan);
                                    alreadyAnalizedIntentions[tmpIndex].second = false;
                                    // + DO NOT update t_end because we are in a PAR block
                                }
                                else {
                                    if(t_start < end_batch && t_end > start_batch)
                                    {
                                        if(ag_support->checkAndAddPlanToPlansInBatch(plans_in_batch, plan) == false)
                                        {
                                            logger->print(logger_level, " Plan:["+ std::to_string(MCT_plan->getInternalGoalsSelectedPlans()[0].first->get_id())
                                            +"] has already present in batch:["+ std::to_string(batch) +"]");
                                        }
                                        else
                                        {
                                            if(batch == 1) {
                                                skip_intention_internal_goals = false; // check if the goal is an intention
                                            }
                                            else {
                                                skip_intention_internal_goals = true; // check the selected plan for the goal
                                            }

                                            logger->print(logger_level, "Internal goal : t_start: "+ std::to_string(t_start)
                                            +" + goal.arrivalTime: "+ std::to_string(goal->get_arrival_time())
                                            +" = "+ std::to_string(t_start + goal->get_arrival_time())
                                            +" vs: "+ std::to_string(tmp_first_batch_delta_time));
                                            logger->print(logger_level, " define_struct:("+ std::to_string(tmp_first_batch_delta_time)
                                            +", "+ std::to_string(t_end) +")");

                                            manage_new_schedulable_interval(intervals_U, define_struct(tmp_first_batch_delta_time, t_end));

                                            logger->print(logger_level, "B) Internal goal arrival time: ("+ std::to_string(goal_arrivalTime)
                                            +" + "+ std::to_string(goal->get_arrival_time())
                                            +") = "+ std::to_string(goal_arrivalTime + goal->get_arrival_time()));

                                            logger->print(logger_level, "B) tmp_first_batch_delta_time: "+ std::to_string(tmp_first_batch_delta_time)
                                            +", first_batch_delta_time: "+ std::to_string(first_batch_delta_time)
                                            +", main_plan_nesting_level: "+ std::to_string(main_plan_nesting_level)
                                            +", (0 = tmp_first_batch_delta_time, > 0 = first_batch_delta_time)");

                                            // Case: B
                                            Up = computeIntentionU_v3(MCT_plan, intervals_U,
                                                    index_internal_goal, servers_needed,
                                                    start_batch, end_batch, t_start, tmp_first_batch_delta_time,
                                                    alreadyAnalizedIntentions, analyzed_plan_id, batch, plans_in_batch, skip_intention_internal_goals); // v3
                                            logger->print(Logger::EveryInfoPlus, " --- DEBUG --- ");
                                        }
                                    }
                                    else {
                                        logger->print(logger_level, " (PAR) Plan:["+ std::to_string(plan->get_id())
                                        +"], t_start: "+ std::to_string(t_start)
                                        +", t_end: "+ std::to_string(t_end)
                                        +", does not overlaps batch: "+ std::to_string(batch)
                                        +", start_batch: "+ std::to_string(start_batch)
                                        +", end_batch: "+ std::to_string(end_batch));
                                    }
                                }
                            }
                        }
                        else if(goal->getGoalExecutionType() == Goal::EXEC_SEQUENTIAL)
                        {
                            logger->print(logger_level, " 6.1. Main Plan:["+ std::to_string(debug_plan_id)
                            +"], name:["+ main_plan->get_goal_name()
                            +"], computed for Plan:["+ std::to_string(main_plan->get_id())
                            +"] -> computeGoalDDL: "+ std::to_string(tmp_t_end));

                            contained_plans.clear(); // used to detect loops among plans
                            loop_detected = false;
                            if(ag_support->checkIfPlanWasAlreadyIntention(plan, index_intention, ag_intention_set) && batch == 1) {
                                tmp_first_batch_delta_time = ag_intention_set[index_intention]->get_batch_startTime();
                                skip_intentionset = false;
                            }
                            else {
                                tmp_first_batch_delta_time = first_batch_delta_time + goal->get_arrival_time();
                                skip_intentionset = true;
                            }
                            logger->print(logger_level, " tmp_first_batch_delta_time: "+ std::to_string(tmp_first_batch_delta_time));
                            mctp = computeGoalDDL(plan, contained_plans, loop_detected, tmp_first_batch_delta_time, 0, false, skip_intentionset);

                            if(mctp != nullptr)
                            {
                                tmp_t_end = mctp->getMCTValue();

                                logger->print(logger_level, " 6. Main Plan:["+ std::to_string(debug_plan_id)
                                +"], name:["+ main_plan->get_goal_name()
                                +"], computed for Plan:["+ std::to_string(main_plan->get_id())
                                +"] -> computeGoalDDL: "+ std::to_string(tmp_t_end));

                                if(tmp_t_end == -1) {
                                    t_end = t_start + infinite_ddl;
                                }
                                else {
                                    t_end = tmp_t_end;
                                }
                                logger->print(logger_level, "After IntGoal["+ std::to_string(goal->get_id())
                                +"]: t_start: "+ std::to_string(t_start) +", t_end: "+ std::to_string(t_end));

                                logger->print(logger_level, " check if the (SEQ) goal overlaps with the batch: "+ std::to_string(t_start)+ " < "+ std::to_string(end_batch)
                                +" && "+ std::to_string(t_end) +" > "+ std::to_string(start_batch)
                                +" --> Plan:["+ std::to_string(plan->get_id())
                                +"] = "+ ag_support->boolToString(t_start < end_batch && t_end > start_batch));

                                if(t_start > t_end)
                                {
                                    logger->print(logger_level, " :: (SEQ) Skip internal goal:["+ goal->get_goal_name()
                                            +"] because t_start: "+ std::to_string(t_start) +" > t_end: "+ std::to_string(t_end) +" ::");
                                    t_end = t_start; // I can do this because it's SEQ
                                    ag_support->checkAndRemovePlanToPlansInBatch(plans_in_batch, plan);
                                    alreadyAnalizedIntentions[tmpIndex].second = false;
                                }
                                else {
                                    if(t_start < end_batch && t_end > start_batch)
                                    {
                                        if(ag_support->checkAndAddPlanToPlansInBatch(plans_in_batch, plan) == false)
                                        {
                                            logger->print(logger_level, " Plan:["+ std::to_string(MCT_plan->getInternalGoalsSelectedPlans()[0].first->get_id())
                                            +"] has already present in batch:["+ std::to_string(batch) +"]");
                                        }
                                        else
                                        {
                                            if(batch == 1) {
                                                skip_intention_internal_goals = false; // check if the goal is an intention
                                            }
                                            else {
                                                skip_intention_internal_goals = true; // check the selected plan for the goal
                                            }

                                            // Version 2 ---------------
                                            logger->print(logger_level, "Internal goal: t_start: "+ std::to_string(t_start)
                                            +" + goal.arrivalTime: "+ std::to_string(goal->get_arrival_time())
                                            +" = "+ std::to_string(t_start + goal->get_arrival_time())
                                            +" use -->: "+ std::to_string(tmp_first_batch_delta_time) +", "+ std::to_string(t_end));
                                            logger->print(logger_level, " define_struct:("+ std::to_string(tmp_first_batch_delta_time)
                                            +", "+ std::to_string(t_end) +")");

                                            manage_new_schedulable_interval(intervals_U, define_struct(tmp_first_batch_delta_time, t_end));
                                            // -------------------------

                                            logger->print(logger_level, "A) Internal goal arrival time: ("+ std::to_string(goal_arrivalTime)
                                            +" + "+ std::to_string(goal->get_arrival_time())
                                            +") = "+ std::to_string(goal_arrivalTime + goal->get_arrival_time()));

                                            logger->print(logger_level, "AA) start_batch: "+ std::to_string(start_batch)
                                            +", end_batch: "+ std::to_string(end_batch)
                                            +", t_start: "+ std::to_string(t_start)
                                            +", first_batch_delta_time: "+ std::to_string(first_batch_delta_time)
                                            +" + goal.arrivalTime: "+ std::to_string(goal->get_arrival_time())
                                            +" = "+ std::to_string(first_batch_delta_time + goal->get_arrival_time()));

                                            // case A:
                                            Up = computeIntentionU_v3(MCT_plan, intervals_U,
                                                    index_internal_goal, servers_needed,
                                                    start_batch, end_batch, t_start, first_batch_delta_time + goal->get_arrival_time(),
                                                    alreadyAnalizedIntentions, analyzed_plan_id, batch, plans_in_batch, skip_intention_internal_goals);
                                            logger->print(Logger::EveryInfoPlus, " --- DEBUG --- ");
                                        }
                                    }
                                    else {
                                        logger->print(logger_level, " (SEQ) Plan:["+ std::to_string(plan->get_id())
                                        +"], t_start: "+ std::to_string(t_start)
                                        +", t_end: "+ std::to_string(t_end)
                                        +", does not overlaps batch: "+ std::to_string(batch)
                                        +", start_batch: "+ std::to_string(start_batch)
                                        +", end_batch: "+ std::to_string(end_batch)
                                        +" --> SKIP internal goal:["+ plan->get_goal_name() +"]");
                                    }
                                }
                            }
                        }

                        if(index_internal_goal < MCT_plan->getInternalGoalsSelectedPlansSize()) {
                            logger->print(logger_level, "Plan:["+ std::to_string(debug_plan_id)
                            +"], name:["+ main_plan->get_goal_name() +"], Up = "+ std::to_string(Up));
                        }
                    }
                }
                else // validAction = false -> intention already analyzed...
                { // plan has already been evaluated. Still, we need to update t_end based on the plan DDL
                    contained_plans.clear(); // used to detect loops among plans
                    loop_detected = false;

                    plan->set_body(ag_intention_set[tmpIndex]->get_Actions()); // update the plan's body with the current actions from the corresponding intention
                    alreadyAnalizedIntentions[tmpIndex].second = true;

                    if(ag_support->checkIfPlanWasAlreadyIntention(plan, index_intention, ag_intention_set) && batch == 1) {
                        tmp_first_batch_delta_time = ag_intention_set[index_intention]->get_batch_startTime();
                        skip_intentionset = false;
                    }
                    else {
                        tmp_first_batch_delta_time = first_batch_delta_time + goal->get_arrival_time();
                        skip_intentionset = true;
                    }
                    logger->print(logger_level, " tmp_first_batch_delta_time: "+ std::to_string(tmp_first_batch_delta_time));
                    mctp = computeGoalDDL(plan, contained_plans, loop_detected, tmp_first_batch_delta_time, 0, false, skip_intentionset);

                    if(mctp != nullptr)
                    {
                        tmp_t_end = mctp->getMCTValue();

                        logger->print(logger_level, " 7. Main Plan:["+ std::to_string(debug_plan_id)
                        +"], name:["+ main_plan->get_goal_name()
                        +"], computed for Plan:["+ std::to_string(main_plan->get_id())
                        +"] -> computeGoalDDL: "+ std::to_string(tmp_t_end));

                        t_end = std::max(t_end, tmp_t_end);
                        logger->print(logger_level, "After IntGoal: t_start: "+ std::to_string(t_start) +", t_end: "+ std::to_string(t_end));

                        logger->print(logger_level, "Internal goal: t_start: "+ std::to_string(t_start)
                        +" + goal.arrivalTime: "+ std::to_string(goal->get_arrival_time())
                        +" = "+ std::to_string(t_start + goal->get_arrival_time())
                        +" vs: "+ std::to_string(tmp_first_batch_delta_time));
                        logger->print(logger_level, " define_struct:("+ std::to_string(tmp_first_batch_delta_time)
                        +", "+ std::to_string(t_end) +")");

                        manage_new_schedulable_interval(intervals_U, define_struct(tmp_first_batch_delta_time, t_end));
                    }
                }
            }
        }
        else if(body[i]->get_type() == "task")
        {
            task = std::dynamic_pointer_cast<Task>(body[i]);

            // update t_start and t_end of the internal batches based on their EXEC type
            if(task->getTaskExecutionType() == Task::EXEC_PARALLEL)
            {
                // same PAR block: update the t_end value
                tmp_t_end = ag_support->getTaskDeadline_based_on_nexec(task, isTaskInAlreadyActiveServer, server_original_ddl, ag_servers, ag_Sched->get_tasks_vector_ready(), ag_Sched->get_tasks_vector_to_release());
                logger->print(Logger::EveryInfoPlus, "step tmp_t_end: "+ std::to_string(tmp_t_end));
                if(tmp_t_end == -1) {
                    tmp_t_end = t_arrivalTime_delta + infinite_ddl;
                    logger->print(Logger::EveryInfo, "step tmp_t_end: "+ std::to_string(t_arrivalTime_delta)
                    +" + "+ std::to_string(infinite_ddl) +" = "+ std::to_string(tmp_t_end));
                }
                else {
                    logger->print(logger_level, "step tmp_t_end: "+ std::to_string(t_arrivalTime_delta)
                    +" + "+ std::to_string(tmp_t_end) +" = "+ std::to_string(t_arrivalTime_delta + tmp_t_end));
                    tmp_t_end = t_arrivalTime_delta + tmp_t_end;
                }
                if(isTaskInAlreadyActiveServer) { // ADDED: 4/01/21 to manage APERIODIC TASKS that are linked to an already active server!
                    logger->print(logger_level, "2. isTaskInAlreadyActiveServer:["+ ag_support->boolToString(isTaskInAlreadyActiveServer)
                            +"], tmp_t_end = ("+ std::to_string(tmp_t_end) +" - "+ std::to_string(t_arrivalTime_delta) +") = "+ std::to_string(tmp_t_end - t_arrivalTime_delta));
                    tmp_t_end = tmp_t_end - t_arrivalTime_delta;
                }
                t_action_end = tmp_t_end;
                t_end = std::max(t_end, tmp_t_end);

                logger->print(Logger::EveryInfo, "c- t_start:["+ std::to_string(t_start) +"], t_end:["+ std::to_string(t_end) +"]");
            }
            else if(task->getTaskExecutionType() == Task::EXEC_SEQUENTIAL)
            {
                // another batch, update t_start and t_end and Up
                Up = 0;
                goal_arrivalTime = 0;
                tmp_t_end = ag_support->getTaskDeadline_based_on_nexec(task, isTaskInAlreadyActiveServer, server_original_ddl, ag_servers, ag_Sched->get_tasks_vector_ready(), ag_Sched->get_tasks_vector_to_release());
                logger->print(Logger::EveryInfo, "tmp_t_end: "+ std::to_string(tmp_t_end));
                if(tmp_t_end == -1) {
                    t_end = t_start + infinite_ddl;
                    logger->print(Logger::EveryInfo, "t_end: "+ std::to_string(t_start)
                    +" + "+ std::to_string(infinite_ddl) +" = "+ std::to_string(t_end));
                }
                else {
                    t_end = t_start + tmp_t_end;
                    logger->print(Logger::EveryInfo, "t_end: "+ std::to_string(t_start)
                    +" + "+ std::to_string(tmp_t_end) +" = "+ std::to_string(t_end));
                }
                if(isTaskInAlreadyActiveServer) { // manage APERIODIC TASKS that are linked to an already active server!
                    logger->print(logger_level, "3. isTaskInAlreadyActiveServer:["+ ag_support->boolToString(isTaskInAlreadyActiveServer)
                            +"], t_end = ("+ std::to_string(t_end) +" - "+ std::to_string(t_start) +") = "+ std::to_string(t_end - t_start));
                    t_end = t_end - t_start;
                }
                t_action_end = t_end;
                logger->print(Logger::EveryInfo, "+ t_start: "+ std::to_string(t_start) +", t_end: "+ std::to_string(t_end) +", t_action_end: "+ std::to_string(t_action_end));
            }

            logger->print(logger_level, " -> Task:["+ std::to_string(task->getTaskId())
            +"], plan:["+ std::to_string(task->getTaskPlanId())
            +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId())
            +"], SERVER:["+ std::to_string(task->getTaskServer()) +"] <-");

            report_value = "    Checking tasks OVERLAP: "+ std::to_string(t_start)+ " < "+ std::to_string(end_batch);
            report_value += " && "+ std::to_string(t_action_end) +" > "+ std::to_string(start_batch);
            report_value += " --> Task:["+ std::to_string(task->getTaskId()) +"], OriginalPlan:["+ std::to_string(task->getTaskOriginalPlanId());
            report_value += "] = "+ ag_support->boolToString(t_start != t_action_end && t_start < end_batch && t_action_end > start_batch);
            logger->print(logger_level, report_value);

            // write simulation events report
            write_debug_isschedulable_json_report(report_value);

            // Is the current task with interval start_batch, end_batch ?
            if(t_start > t_action_end)
            {
                logger->print(logger_level, " >> The analyzed task's start instant is greater than the end time. <<");
                logger->print(logger_level, " >> This may happen with already active servers that are already active before task's start time <<");
                logger->print(logger_level, " >> -> task.server=["+ std::to_string(task->get_server_id())
                +"], ("+ std::to_string(t_start) +" > "+ std::to_string(t_action_end) +") << ");
            }
            else if(t_start < end_batch && t_action_end > start_batch)
            {
                ag_support->checkIfServerAlreadyCounted(ag_Server_Handler, task, server_already_counted, servers_needed, Logger::EveryInfo);

                /* v2: to simulate worst case: multiple overlapping tasks linked to the same server are considered as individuals.
                 * If overlapping tasks linked to the same server only affect it once, then add "&& server_already_counted == false" to the if statement condition */
                if(t_start != t_action_end)
                { // batch PAR --> sum task's U
                    tmp_task_U = computeTaskU(task);
                    logger->print(logger_level, "still in block PAR: Update Up = "+ std::to_string(Up) +" + "+ std::to_string(tmp_task_U)
                    +" = "+ std::to_string(Up + tmp_task_U));
                    logger->print(Logger::EveryInfo, " t_end: "+ std::to_string(t_end)
                    +", t_action_end: "+ std::to_string(t_action_end));
                    logger->print(Logger::EveryInfo, " define_struct:("+ std::to_string(t_start)
                    +", "+ std::to_string(t_action_end) +")");

                    // "t_action_end" equal to the end of the current task
                    Up = manage_new_schedulable_interval(intervals_U, define_struct(t_start, t_action_end, tmp_task_U));
                }
            }
            else if(t_action_end < start_batch)
            {
                logger->print(logger_level, "The analyzed task fully executes before the interval \"start_batch, end_batch\" begins -> "+ std::to_string(t_action_end) +" < "+ std::to_string(start_batch));
            }
            else if(t_start > end_batch)
            {
                logger->print(logger_level, "The analyzed task fully executes after the interval \"start_batch, end_batch\" begins -> "+ std::to_string(t_start) +" > "+ std::to_string(end_batch) +". Stop checking...");
                Up = 0;
                break; // exit the loop
            }
            else if(t_action_end == start_batch)
            {   // the task analyzed is on the edge, let's check the next one.
                logger->print(logger_level, "The analyzed task has t_action_end == start_batch -> "+ std::to_string(t_action_end) +" == "+ std::to_string(start_batch) +". Let's check the next one...");
            }
            else { // if t_start == end_batch -> the tasks is just after the current interval -> stop analyzing
                logger->print(logger_level, "The analyzed task has t_start == end_batch -> "+ std::to_string(t_start) +" == "+ std::to_string(end_batch) +". Stop checking...");
                Up = 0;
                break; // exit the loop
            }
        }

        if(i + 1 < (int)body.size())
        {
            if(body[i + 1]->get_type() == "goal")
            {
                next_goal = std::dynamic_pointer_cast<Goal>(body[i + 1]);
                if(next_goal->getGoalExecutionType() == Goal::EXEC_PARALLEL)
                { // do nothing, we are still in the same PAR block
                    logger->print(logger_level, "Next is GOAL PAR:["+ next_goal->get_goal_name()
                            +"], arrivalTime:["+ std::to_string(next_goal->get_arrival_time())
                    +"], t_start:["+ std::to_string(t_start)
                    +"], t_end:["+ std::to_string(t_end)
                    +"],\n tmp_first_batch_delta_time:["+ std::to_string(tmp_first_batch_delta_time)
                    +"], first_batch_delta_time:["+ std::to_string(first_batch_delta_time)
                    +"], intention goal_arrivalTime:["+ std::to_string(goal_arrivalTime) +"]");
                    logger->print(Logger::EveryInfoPlus, " --- DEBUG --- ");
                }
                else if(next_goal->getGoalExecutionType() == Goal::EXEC_SEQUENTIAL)
                {
                    U = std::max(U, Up);
                    Up = 0;
                    batch += 1;

                    logger->print(logger_level, "t_arrivalTime_delta: "+ std::to_string(t_arrivalTime_delta)
                    +", t_end: "+ std::to_string(t_end)
                    +", task != null: "+ ag_support->boolToString(task != nullptr)
                    +", t_start: "+ std::to_string(t_start)
                    +", first_batch_delta_time: "+ std::to_string(t_start));
                    t_arrivalTime_delta = t_end;
                    t_start = t_arrivalTime_delta;
                    first_batch_delta_time = t_start;
                    goal_arrivalTime = 0;
                }
            }
            else if(body[i + 1]->get_type() == "task")
            {
                next_task = std::dynamic_pointer_cast<Task>(body[i + 1]);

                if(next_task->getTaskExecutionType() == Task::EXEC_PARALLEL) {
                    // do nothing, we are still in the same PAR block
                }
                else if(next_task->getTaskExecutionType() == Task::EXEC_SEQUENTIAL)
                {
                    logger->print(logger_level, " --> CURRENT U = std::max("+ std::to_string(U) +", "+ std::to_string(Up) +") = "+ std::to_string(std::max(U, Up)));
                    U = std::max(U, Up);
                    Up = 0;
                    batch += 1;

                    logger->print(logger_level, "t_arrivalTime_delta: "+ std::to_string(t_arrivalTime_delta)
                    +", t_end: "+ std::to_string(t_end)
                    +", tmp_t != null: "+ ag_support->boolToString(task != nullptr)
                    +", t_start: "+ std::to_string(t_start)
                    +", first_batch_delta_time: "+ std::to_string(t_start));

                    t_arrivalTime_delta = t_end;
                    t_start = t_arrivalTime_delta;
                    first_batch_delta_time = t_start;
                    goal_arrivalTime = 0;
                }
            }
        }
        else
        {
            if(batch == 1)
            { // the plan has only one batch of PAR actions
                if(index_internal_goal < MCT_plan->getInternalGoalsSelectedPlansSize()) {
                    logger->print(logger_level, "Last action[Batch: 1]: Plan:["+ std::to_string(debug_plan_id) +"], U: "+ std::to_string(U) +" + "+ std::to_string(Up) +" = "+ std::to_string(U + Up));
                }
                U = U + Up;
                Up = 0;
            }
            else
            { // the plan has 2,3...N batches. We are analyzing the last action of the last batch. This means that we are using Up to track the computational power in the batch. We must return the max U among all batches
                if(index_internal_goal < MCT_plan->getInternalGoalsSelectedPlansSize()) {
                    logger->print(logger_level, "Last action[Batch: "+ std::to_string(batch)
                    +"]: Plan:["+ std::to_string(debug_plan_id) +"], U: std::max("+ std::to_string(U) +", "+ std::to_string(Up) +") = "+ std::to_string(std::max(U, Up)));
                }
                U = std::max(U, Up);
                Up = 0;
            }
        }
        logger->print(logger_level, "U: "+ std::to_string(U));
        logger->print(logger_level, "t_start: "+ std::to_string(t_start) +", t_end: "+ std::to_string(t_end) +", t_arrivalTime_delta: "+ std::to_string(t_arrivalTime_delta));
    }

    logger->print(logger_level, "Update U before return: std::max("+ std::to_string(U) +", "+ std::to_string(Up) +") = "+ std::to_string(std::max(U, Up)));
    U = std::max(U, Up);
    if(index_internal_goal < MCT_plan->getInternalGoalsSelectedPlansSize()) {
        logger->print(logger_level, "Plan:["+ std::to_string(debug_plan_id) +"], name:["+ debug_goal_name +"] -> RETURN U: "+ std::to_string(U));
    }
    else {
        std::string warning_msg = "[WARNING] index '"+ std::to_string(index_internal_goal) +"' out of range '"+ std::to_string(MCT_plan->getInternalGoalsSelectedPlansSize()) +"'!";
        logger->print(Logger::Warning, warning_msg);
        ag_metric->addGeneratedWarning(sim_current_time(), "ag_reasoning_cycle", warning_msg);
    }
    logger->print(logger_level, "------------END of computeIntentionU------------");

    return U;
}

// -----------------------------------------------------------------------------------------------
double agent::computeTaskU(const std::shared_ptr<Task> t) {
    double U = 0;

    if(t->getTaskNExec() == 1 && t->getTaskisPeriodic() == false)
    { // task is APERIODIC --> Utask = server_budget/server_period
        int server_id = t->getTaskServer();
        int server_index = ag_support->getServerIndexInAg_servers(server_id, ag_servers);

        if(server_id >= 0 && server_index != -1)
        { // in order to be valid, server_id must be positive and must be present in the set of available servers (server_index != -1 means that)...
            std::shared_ptr<Server> s = ag_servers[server_index];

            // Note: budget must be less than period, or max will have value greater than 1
            U = s->get_budget() / s->get_period();
        }
        else {
            std::string msg = "[ERROR] Task:["+ std::to_string(t->getTaskId()) +"] is associated with a not available server:["+ std::to_string(server_id) + "]!";
            throwRuntimeException(true, glob_path, glob_user, msg, "agent", "computeTaskU", sim_current_time().dbl());
        }
    }
    else if(t->getTaskNExec() > 1 || t->getTaskNExec() == -1 || t->getTaskisPeriodic())
    { // task is PERIODIC --> Utask = compTime/period (of the task)
        U = t->getTaskCompTime() / t->getTaskPeriod();
    }

    return U;
}

std::shared_ptr<MaximumCompletionTime> agent::computeGoalDDL(const std::shared_ptr<Plan> plan,
        std::vector<std::shared_ptr<Plan>>& contained_plans, bool& loop_detected,
        double first_batch_delta_time, double activation_delay, bool verify_intention, bool skip_intentionset, int annidation_level)
{
    logger->print(Logger::EveryInfo, "----computeGoalDDL----");
    std::vector<std::shared_ptr<Task>> empty_release_queue(0); // declared only because required by function "checkAndManageIntentionFalseConditions"
    std::shared_ptr<MaximumCompletionTime> MCTgp = std::make_shared<MaximumCompletionTime>();
    std::shared_ptr<MaximumCompletionTime> MCTi = std::make_shared<MaximumCompletionTime>();
    std::shared_ptr<MaximumCompletionTime> MCTj = std::make_shared<MaximumCompletionTime>();
    std::shared_ptr<MaximumCompletionTime> tmp_mctgp;
    std::shared_ptr<MaximumCompletionTime> ddl_intention;
    std::shared_ptr<Task> task;
    std::shared_ptr<Goal> goal;
    std::shared_ptr<Plan> ig_plan = nullptr;
    std::vector<std::shared_ptr<Action>> body;
    std::string msgLoop = "";
    std::string conditions_check_return_msg = "";
    std::string warning_msg = "";
    std::string nesting_spaces = std::to_string(annidation_level) +":";

    Logger::Level logger_level = Logger::EssentialInfo;
    int index_intention = -1;
    int batch = 0;
    double taskDDL;
    double intention_lastExecution = sim_current_time().dbl();
    double server_original_ddl;
    double intention_overlap;
    double tmp_batch_delta_time;

    bool valid_preconditions = true;
    bool isGoalLinkedToIntention = false;
    bool doesInternalGoalHasPlan = false; // true if there is at least one valid plan achieving the internal goal
    bool optimisticApproach = (bool) par("MCTIdentity"); // true: optimistic; false: pessimistic
    bool isTaskInAlreadyActiveServer = false; // used when we find an APERIODIC task connected to an active server
    bool cont_cond_valid = true;
    bool skip_intention_internal_goal = skip_intentionset;

    if(verify_intention == true) { // reduce the number of printed values
        logger_level = Logger::EveryInfo;
    }

    // DEBUG printing purposes ----------------------------
    for(int i = 0; i < annidation_level; i++) {
        nesting_spaces += "    ";
    }
    // ----------------------------------------------------

    logger->print(logger_level, nesting_spaces +" Plan:["+ plan->get_goal_name()
            +"], first_batch_delta_time:["+ std::to_string(first_batch_delta_time)
    +"], skip_intention_internal_goal:["+ ag_support->boolToString(skip_intention_internal_goal)
    +"], verify_intention:["+ ag_support->boolToString(verify_intention) +"]");

    // initialize DDL values...
    MCTgp->setMCTValue(0);
    MCTi->setMCTValue(0);
    MCTgp->add_internal_goal_selectedPlan(plan, annidation_level); // save the chosen plan

    /* Note: "checkIfPlanWasAlreadyIntention" uses "original_plan_id" to check if "plan" is already an intention.
     * This means that internal goals in batch > 1 will never be linked to intentions. */
    if(ag_support->checkIfPlanWasAlreadyIntention(plan, index_intention, ag_intention_set)
            && skip_intention_internal_goal == false
            && check_intentions_fcfs == true) /* "check_intentions_fcfs" is only used with FCFS */
    {
        body = ag_intention_set[index_intention]->get_Actions();

        intention_lastExecution = ag_intention_set[index_intention]->get_lastExecution();
        first_batch_delta_time = ag_intention_set[index_intention]->get_batch_startTime();
        activation_delay = 0;

        logger->print(logger_level, nesting_spaces +" Debug > Intention:["+ std::to_string(ag_intention_set[index_intention]->get_id())
        +"] for plan:["+ std::to_string(ag_intention_set[index_intention]->get_planID())
        +"] original_plan:["+ std::to_string(ag_intention_set[index_intention]->get_original_planID())
        +"] name:["+ ag_intention_set[index_intention]->get_goal_name()
        +"], lastExecution:["+ std::to_string(intention_lastExecution)
        +"], batch_startTime:["+ std::to_string(ag_intention_set[index_intention]->get_batch_startTime())
        +"], first_batch_delta_time:["+ std::to_string(first_batch_delta_time) +"]");

        if((int)body.size() == 0)
        {
            logger->print(logger_level, nesting_spaces +" - Body.size:["+ std::to_string(body.size())
            +"], waiting for completion:["+ ag_support->boolToString(ag_intention_set[index_intention]->get_waiting_for_completion())
            +"] -> Final MCTi:["+ std::to_string(ag_intention_set[index_intention]->get_scheduled_completion_time()) +"] --");

            // Note: do not return MCTgp immediately or intentions that are "waiting for completion" are not considered for loop detection
            MCTgp->setMCTValue(ag_intention_set[index_intention]->get_scheduled_completion_time());
        }
        else {
            ag_support->checkIntentionStartBatchTime(ag_intention_set[index_intention], ag_Sched);
            first_batch_delta_time = ag_intention_set[index_intention]->get_batch_startTime();
            logger->print(logger_level, nesting_spaces +" Updated first_batch_delta_time:["+ std::to_string(first_batch_delta_time) +"]");
        }
    }
    else {
        // to be able to update variables 'task' and 'goal' and reflect the changes on the plan as well
        body = MCTgp->getInternalGoalsSelectedPlans()[0].first->get_body();

        logger->print(logger_level, nesting_spaces +" Debug > Plan:["+ std::to_string(plan->get_id())
        +"] for plan:["+ std::to_string(plan->get_id())
        +"] name:["+ plan->get_goal_name()
        +"] first_batch_delta_time:["+ std::to_string(first_batch_delta_time)
        +" + "+ std::to_string(activation_delay)
        +"] = "+ std::to_string(first_batch_delta_time + activation_delay));
        first_batch_delta_time = first_batch_delta_time + activation_delay;
    }

    logger->print(logger_level, "----------------------------");
    logger->print(logger_level, nesting_spaces +" > Internal goal -> Plan:["+ std::to_string(plan->get_id())
    +"], goal_name:["+ plan->get_goal_name()
    +"], nesting level:["+ std::to_string(annidation_level)
    +"], first_batch_delta_time: "+ std::to_string(first_batch_delta_time));

    if(ag_support->checkIfPlanAlreadyCounted(plan, contained_plans, msgLoop, nesting_spaces)) {
        loop_detected = true;
        MCTgp = nullptr;
        warning_msg = nesting_spaces +" [WARNING] Loop detected! "+ msgLoop;
        logger->print(Logger::Warning, warning_msg);
        ag_metric->addGeneratedWarning(sim_current_time(), "computeGoalDDL", warning_msg);
    }
    else {
        contained_plans.push_back(plan);

        for(int i = 0; i < (int)body.size(); i++)
        {
            logger->print(Logger::EveryInfo, nesting_spaces +" >>>>> first_batch_delta_time: "+ std::to_string(first_batch_delta_time));
            // understand if the action is a task or a goal before moving on...
            if(body[i]->get_type() == "task") {
                task = std::dynamic_pointer_cast<Task>(body[i])->makeCopyOfTask();
                goal = nullptr;
            }
            else if(body[i]->get_type() == "goal") {
                goal = std::dynamic_pointer_cast<Goal>(body[i]);
                task = nullptr;
            }

            if((task != nullptr && task->getTaskExecutionType() == Task::EXEC_SEQUENTIAL)
                    || (goal != nullptr && goal->getGoalExecutionType() == Goal::EXEC_SEQUENTIAL))
            {
                // ... update the DDL of the main plan:
                if(MCTgp->getMCTValue() == -1 || MCTi->getMCTValue() == -1) {
                    logger->print(Logger::EveryInfo, nesting_spaces +" Updating MCTgp:["+ std::to_string(MCTgp->getMCTValue()) +"] to [-1]");
                    MCTgp->setMCTValue(-1); // infinite DDL
                }
                else {
                    logger->print(Logger::EveryInfo, nesting_spaces +" Updating MCTgp:["+ std::to_string(MCTgp->getMCTValue())
                    +" + "+ std::to_string(MCTi->getMCTValue())
                    +"] to ["+ std::to_string(MCTgp->getMCTValue() + MCTi->getMCTValue()) +"]");
                    MCTgp->setMCTValue(MCTgp->getMCTValue() + MCTi->getMCTValue());
                }
                MCTgp->taskSet_push_back(MCTi->getTaskSet());  // update the taskset
                MCTi->setMCTValue(0);       // restore default value
                MCTi->taskSet_clear();      // restore default value
                batch += 1;                 // update the batch index (both for SEQ goals and tasks)

                // DEBUG purpose -------------------------------
                if(annidation_level == 0) {
                    logger->print(logger_level, nesting_spaces +" ::: Batch:["+ std::to_string(batch) +"] +++ Plan:["+ std::to_string(plan->get_id())
                    +"], goal_name:["+ plan->get_goal_name() +"] -- MCTi:["+ std::to_string(MCTi->getMCTValue())
                    +"] -- New Batch:["+ std::to_string(MCTgp->getMCTValue() + MCTi->getMCTValue()) +"] +++ ");
                    logger->print(Logger::EveryInfoPlus, " --- DEBUG --- ");
                }
                // ----------------------------------------------

                /* if batch > 1, and all the actions in the previous batches where goals,
                 * and where all already completed...
                 * Or, if there are no parallel actions before i-th*/
                if((batch > 1 || i > 0))
                {   // UPDATE 'first_batch_delta_time'
                    logger->print(Logger::EveryInfo, nesting_spaces +" Batch:["+ std::to_string(batch)
                    +"], set 'first_batch_delta_time' = 0");
                    first_batch_delta_time = 0;
                }
                // ------------------------------------------------------------------------------------------------------

                if(task != nullptr)
                { /* it's an task
                 *  if APERIODIC -> server
                 *       if new server
                 *       if already active server
                 *   if PERIODIC */

                    int task_id = -1;
                    taskDDL = ag_support->getTaskDeadline_based_on_nexec(task,
                            isTaskInAlreadyActiveServer, server_original_ddl,
                            ag_servers, ag_Sched->get_tasks_vector_ready(), ag_Sched->get_tasks_vector_to_release());

                    logger->print(Logger::EveryInfo, "Task:["+ std::to_string(task->getTaskId())
                    +"], plan:["+ std::to_string(task->getTaskPlanId())
                    +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId())
                    +"], ddl:["+ std::to_string(taskDDL) +"]");

                    if(task->getTaskisPeriodic())
                    { // PERIODIC task -> save the deadline of the task
                        task_id = task->getTaskId();
                        if(task->getTaskNExecInitial() == -1) {
                            taskDDL = infinite_ddl;
                        }

                        logger->print(Logger::EveryInfo, nesting_spaces +" PERIODIC > Task:["+ std::to_string(task->getTaskId())
                        +"], plan:["+ std::to_string(task->getTaskPlanId())
                        +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId())
                        +"], DDL: ("+ std::to_string(taskDDL)
                        +" + "+ std::to_string(first_batch_delta_time)
                        +") --> "+ std::to_string(first_batch_delta_time + taskDDL));
                        MCTi->setMCTValue(first_batch_delta_time + taskDDL);
                    }
                    else { // APERIODIC task
                        task_id = task->getTaskId();
                        if(isTaskInAlreadyActiveServer)
                        { // the selected server is already active
                            logger->print(Logger::EveryInfo, nesting_spaces +" CHECK: "+ std::to_string(intention_lastExecution)
                            +" + "+ std::to_string(MCTgp->getMCTValue())
                            +" > "+ std::to_string(taskDDL)
                            +" --> "+ ag_support->boolToString(intention_lastExecution + MCTgp->getMCTValue() > taskDDL));

                            if(intention_lastExecution + MCTgp->getMCTValue() > taskDDL)
                            {   // This means that the server absolute ddl is lower then the current analyzed time
                                // Thus, use the default server period as deadline
                                logger->print(Logger::EveryInfo, nesting_spaces +" (Used server A) > Server:["+ std::to_string(task->getTaskServer())
                                +"], Task:["+ std::to_string(task->getTaskId())
                                +"], plan:["+ std::to_string(task->getTaskPlanId())
                                +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId())
                                +"], DDL: "+ std::to_string(server_original_ddl));

                                MCTi->setMCTValue(server_original_ddl);
                            }
                            else { // This means that the server overlaps this batch
                                /* ---------------------   taskDDL
                                 *              ****        batch   --> MCT = interval (batch to DDL)
                                 *                    ****  batch   --> MCT = interval (batch to DDL) */
                                logger->print(Logger::EveryInfo, nesting_spaces +" (Used server B) > Server:["+ std::to_string(task->getTaskServer())
                                +"], Task:["+ std::to_string(task->getTaskId())
                                +"], plan:["+ std::to_string(task->getTaskPlanId())
                                +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId())
                                +"], DDL:("+ std::to_string(taskDDL)
                                +" - "+ std::to_string(MCTgp->getMCTValue())
                                +") --> "+ std::to_string(taskDDL - MCTgp->getMCTValue()));

                                MCTi->setMCTValue(taskDDL - MCTgp->getMCTValue());
                            }
                        }
                        else { // not already instantiated server
                            logger->print(Logger::EveryInfo, nesting_spaces +" (NOT Used server) > Server:["+ std::to_string(task->getTaskServer())
                            +"], Task:["+ std::to_string(task->getTaskId())
                            +"], plan:["+ std::to_string(task->getTaskPlanId())
                            +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId())
                            +"], DDL: ("+ std::to_string(taskDDL)
                            +" + "+ std::to_string(first_batch_delta_time)
                            +") --> "+ std::to_string(first_batch_delta_time + taskDDL));
                            MCTi->setMCTValue(first_batch_delta_time + taskDDL);
                        }
                    }

                    MCTi->taskSet_push_back(task);
                    logger->print(logger_level, nesting_spaces +" -- Task:["+ std::to_string(task_id)
                    +"], Plan:["+ std::to_string(task->getTaskPlanId())
                    +"], original plan:["+ std::to_string(task->getTaskOriginalPlanId())
                    +"], server:["+ std::to_string(task->getTaskServer())
                    +"], name:["+ plan->get_goal_name()
                    +"] -> Final MCTi:["+ std::to_string(MCTi->getMCTValue()) +"] --");
                }
                else if(goal != nullptr)
                { // it's an internal goal
                    doesInternalGoalHasPlan = false;
                    isGoalLinkedToIntention = false;
                    loop_detected = false;
                    valid_preconditions = true;
                    cont_cond_valid = true;
                    ig_plan = nullptr;
                    conditions_check_return_msg = "";

                    if(batch > 1) {
                        skip_intention_internal_goal = true;
                    }
                    else {
                        skip_intention_internal_goal = false;
                    }

                    if(ag_support->checkIfGoalIsLinkedToValidSkill(goal, ag_skill_set) == false)
                    {
                        warning_msg = "[WARNING] (SEQ) INVALID INTERNAL GOAL:["+ goal->get_goal_name() +"] NOT linked to any skill! -> Can not be activated!";
                        logger->print(Logger::Warning, warning_msg);
                        ag_metric->addGeneratedWarning(sim_current_time(), "computeGoalDDL", warning_msg);
                        return nullptr; // there are no plans compatible with the internal goal. The whole plan has to be discarded
                    }
                    else
                    {
                        /* If the goal is already linked to an intention, get the plan from which we derived the intention and check if the cont_conditions are valid.
                         *  Note: only goals in the first batch can be linked to intention during this phase of the cycle */
                        if(ag_support->checkIfGoalIsLinkedToIntention(goal, ag_intention_set, index_intention) && batch == 1 && check_intentions_fcfs == true) /* "check_intentions_fcfs" is only used with FCFS */
                        {
                            intentions_fcfs.push_back(std::make_shared<Intention>(*ag_intention_set[index_intention])); /* only used with FCFS */
                            ig_plan = ag_intention_set[index_intention]->getTopInternalSelectedPlan();
                            ddl_intention = computeGoalDDL(ig_plan, contained_plans, loop_detected,
                                    ag_intention_set[index_intention]->get_batch_startTime(),
                                    0, // intention delay = 0 because already active before "now"
                                    true, // true = use EveryInfo as logger status
                                    false,  // skip_intentionset
                                    0); // annidation level

                            if(loop_detected)
                            { // there where no suitable plan for an internal goal(sub-sub-goal) of this goal(sub-goal)
                                logger->print(logger_level, nesting_spaces +" computeGoalDDL for plan:["+ std::to_string(ig_plan->get_id())
                                +"] return null. Loop detected!");
                            }
                            else if(ddl_intention != nullptr)
                            {
                                logger->print(Logger::EveryInfo, nesting_spaces +" goal:["+ goal->get_goal_name()
                                        +"], activation_time:["+ std::to_string(goal->get_activation_time())
                                +"], arrival_time:["+ std::to_string(goal->get_arrival_time())
                                +"], first_batch_delta_time:["+ std::to_string(first_batch_delta_time)
                                +"], tot: "+ std::to_string(goal->get_arrival_time() + first_batch_delta_time));

                                logger->print(Logger::EveryInfo, nesting_spaces +" ig_plan:["+ ig_plan->get_goal_name()
                                        +"], ddl_intention:["+ std::to_string(ddl_intention->getMCTValue())
                                +"], batch_startTime:["+ std::to_string(ag_intention_set[index_intention]->get_batch_startTime())
                                +"], last_ExecutionTime:["+ std::to_string(ag_intention_set[index_intention]->get_lastExecution())
                                +"], tot: "+ std::to_string(ddl_intention->getMCTValue() + ag_intention_set[index_intention]->get_lastExecution()));

                                if(ag_intention_set[index_intention]->get_waiting_for_completion()) {
                                    // intention is waiting for completion, thus, its deadline is equal to the completion_time
                                    intention_overlap = ag_intention_set[index_intention]->get_scheduled_completion_time();
                                }
                                else {
                                    intention_overlap = ddl_intention->getMCTValue();
                                }

                                logger->print(logger_level, nesting_spaces +" t: "+ sim_current_time().str() +", name:["+ goal->get_goal_name()
                                        +"], Overlap = (intention lastExecution <= updated goal_arrivalTime) = ("+ std::to_string(intention_overlap)
                                +" <= "+ std::to_string(goal->get_arrival_time() + first_batch_delta_time)
                                +") = "+ ag_support->boolToString(intention_overlap <= (goal->get_arrival_time() + first_batch_delta_time)));

                                if(intention_overlap <= (goal->get_arrival_time() + first_batch_delta_time))
                                {   /* if the last execution + deadline of the intention is greater than then initial time of the plan selected for the goal,
                                 * then they overlap. Otherwise, the goal's arrivalTime is in the future and a plan.body must be selected for it */
                                    logger->print(logger_level, nesting_spaces +" (SEQ) Overlap = ("+ std::to_string(intention_overlap)
                                    +" <= "+ std::to_string(goal->get_arrival_time() + first_batch_delta_time)
                                    +") = "+ ag_support->boolToString(intention_overlap <= (goal->get_arrival_time() + first_batch_delta_time))
                                    +" --> the intention completes before the goal gets activated. Choose a plan for goal...");

                                    skip_intention_internal_goal = true;
                                }
                                else {
                                    logger->print(logger_level, nesting_spaces +" (SEQ) Overlap use intention values...");
                                    isGoalLinkedToIntention = true;

                                    if((int)ag_intention_set.size() > index_intention
                                            && (int)ag_intention_set[index_intention]->getMainPlan()->getInternalGoalsSelectedPlans().size() > 0)
                                    {
                                        ig_plan = ag_intention_set[index_intention]->getTopInternalSelectedPlan();

                                        cont_cond_valid = ag_support->checkPlanOrDesireConditions(ag_belief_set, ig_plan->get_cont_conditions(),
                                                conditions_check_return_msg, "plan", ig_plan->get_id(),
                                                ag_intention_set[index_intention]->get_expression_constants(),
                                                ag_intention_set[index_intention]->get_expression_variables());

                                        logger->print(Logger::EveryInfo, nesting_spaces +" cont_cond_valid:["+ ag_support->boolToString(cont_cond_valid)
                                                +"] AND message:["+ conditions_check_return_msg +"]");

                                        if(cont_cond_valid == false)
                                        {
                                            logger->print(logger_level, nesting_spaces +" Cont_conditions: "+ ag_support->dump_expression(
                                                    ig_plan->get_cont_conditions(), ag_belief_set,
                                                    ag_intention_set[index_intention]->get_expression_constants(),
                                                    ag_intention_set[index_intention]->get_expression_variables()));

                                            checkAndManageIntentionFalseConditions(ig_plan, ag_selected_plans, ag_goal_set, ag_intention_set, ag_belief_set, empty_release_queue);
                                        }
                                        else {
                                            tmp_mctgp = computeGoalDDL(ig_plan, contained_plans, loop_detected,
                                                    ag_intention_set[index_intention]->get_batch_startTime(),
                                                    0,
                                                    verify_intention, // in order to print data
                                                    skip_intention_internal_goal, // batch 1, we want to analyze the intention-set
                                                    annidation_level + 1);

                                            logger->print(Logger::EveryInfo, nesting_spaces +" 1. > Plan:["+ std::to_string(ig_plan->get_id())
                                            +"], tmp_mctgp is null:["+ ag_support->boolToString(tmp_mctgp == nullptr)
                                            +"], loop detected:["+ ag_support->boolToString(loop_detected) +"]");
                                            if(loop_detected)
                                            { // there where no suitable plan for an internal goal(sub-sub-goal) of this goal(sub-goal)
                                                logger->print(logger_level, nesting_spaces +" computeGoalDDL for plan:["+ std::to_string(ig_plan->get_id())
                                                +"] return null. Loop detected!");
                                            }
                                            else if(tmp_mctgp == nullptr)
                                            { // there where no suitable plan for an internal goal(sub-sub-goal) of this goal(sub-goal)
                                                logger->print(logger_level, nesting_spaces +" computeGoalDDL for plan:["+ std::to_string(plan->get_id())
                                                +"] return null. An internal goal can't be satisfy!");
                                            }
                                            else {
                                                doesInternalGoalHasPlan = true;

                                                logger->print(Logger::EveryInfo, nesting_spaces +" (SEQ) Check before MCTSelect...");
                                                logger->print(Logger::EveryInfo, nesting_spaces +" Intention:["+ std::to_string(ag_intention_set[index_intention]->get_id())
                                                +"], plan:["+ std::to_string(ag_intention_set[index_intention]->get_planID())
                                                +"], original plan:["+ std::to_string(ag_intention_set[index_intention]->get_original_planID())
                                                +"], mctgp:["+ std::to_string(tmp_mctgp->getMCTValue())
                                                +"], lastExecution: ("+ std::to_string(tmp_mctgp->getMCTValue()) // ag_intention_set[index_intention]->get_lastExecution())
                                                +" < "+ std::to_string(MCTgp->getMCTValue())
                                                +") -> "+ ag_support->boolToString(tmp_mctgp->getMCTValue() < MCTgp->getMCTValue()));

                                                if(tmp_mctgp->getMCTValue() < MCTgp->getMCTValue())
                                                {   /* this means that we are considering an internal goal that has deadline lower
                                                 * than the start of the currently analyzed batch;
                                                 * + the plan for the goal was already active due to another plan
                                                 * -> we are not going to use it because its deadline won't influence the MCTgp (ddl) of this batch */

                                                    logger->print(Logger::EveryInfo, nesting_spaces +" [IMPORTANT] (SEQ) Intention:["+ std::to_string(ag_intention_set[index_intention]->get_id())
                                                    +"], plan:["+ std::to_string(ag_intention_set[index_intention]->get_planID())
                                                    +"], original plan:["+ std::to_string(ag_intention_set[index_intention]->get_original_planID())
                                                    +"], mctgp:["+ std::to_string(tmp_mctgp->getMCTValue())
                                                    +"], lastExecution: ("+ std::to_string(tmp_mctgp->getMCTValue())
                                                    +" < "+ std::to_string(MCTgp->getMCTValue())
                                                    +") -> "+ ag_support->boolToString(tmp_mctgp->getMCTValue() < MCTgp->getMCTValue()));

                                                    tmp_mctgp->setMCTValue(0);
                                                    MCTi = ag_support->MCTSelect(MCTi, tmp_mctgp, false);
                                                }
                                                else if(batch > 1)
                                                {
                                                    logger->print(logger_level, nesting_spaces +" [IMPORTANT] (SEQ) Batch: "+ std::to_string(batch)
                                                    +", plan:["+ std::to_string(ag_intention_set[index_intention]->get_planID())
                                                    +"], original plan:["+ std::to_string(ag_intention_set[index_intention]->get_original_planID())
                                                    +"] -> mctgp: "+ std::to_string(tmp_mctgp->getMCTValue())
                                                    +" - MCTgp: "+ std::to_string(MCTgp->getMCTValue())
                                                    +" -> ("+ std::to_string(tmp_mctgp->getMCTValue() - MCTgp->getMCTValue()) +")");

                                                    if(tmp_mctgp->getMCTValue() - MCTgp->getMCTValue() >= 0)
                                                    {   /* If true, it means that we are analyzing the 2-th, N-th batch of the plan
                                                     * + the batch contains an internal goal
                                                     * + the goal was already active do to another intention
                                                     * + the intention of the goal partially executes within the current batch
                                                     * use (tmp_mctgp - MCTgp allows) to correctly compute the deadline of plan
                                                     * and therefore update the MCTgp */
                                                        tmp_mctgp->setMCTValue(tmp_mctgp->getMCTValue() - MCTgp->getMCTValue());
                                                    }
                                                    else { // Same as the above "if()" statement
                                                        tmp_mctgp->setMCTValue(0);
                                                    }

                                                    MCTi = ag_support->MCTSelect(MCTi, tmp_mctgp, false);
                                                }
                                                else {
                                                    MCTi = ag_support->MCTSelect(MCTi, tmp_mctgp, optimisticApproach);
                                                }
                                                logger->print(Logger::EveryInfo, nesting_spaces +" 2. Set MCTi to:["+ std::to_string(MCTi->getMCTValue()) +"]\n----------------------------");
                                                // ---------------------------------
                                            }
                                        }
                                    }
                                }
                            }
                        } // -- end if 'goal linked to intention' and 'batch == 1'

                        if(isGoalLinkedToIntention == false)
                        {   /* IF THE GOAL IS NOT ALREADY LINKED TO AN INTENTION:
                         * loop over all possible plans in the plan library (ag_plan_set),
                         * looking for those that might achieve the internal goal 'goal' */

                            logger->print(Logger::EveryInfo, nesting_spaces +" --> (SEQ) Choosing best plan for internal goal:["+ goal->get_goal_name() +"] <-- ");
                            logger->print(Logger::EveryInfo, "----------------------------");

                            for(std::shared_ptr<Plan> plan_internalgoal : ag_plan_set)
                            {
                                valid_preconditions = true;

                                if(plan_internalgoal->get_goal_name() == goal->get_goal_name())
                                {
                                    logger->print(logger_level, nesting_spaces +" Internal goal:["+ std::to_string(goal->get_id())
                                    +"], belief:("+ goal->get_goal_name()
                                    +"), plan:["+ std::to_string(plan_internalgoal->get_id())
                                    +"], loop_detected:["+ ag_support->boolToString(loop_detected)
                                    +"] is not linked to any intention! Use now = "+ sim_current_time().str());

                                    logger->print(logger_level, nesting_spaces +"Batch:["+ std::to_string(batch)
                                    +"], first_batch_delta_time:["+ std::to_string(first_batch_delta_time)
                                    +"] + goal.arrivalTime:["+ std::to_string(goal->get_arrival_time())
                                    +"] = abosulte_arrivalTime:["+ std::to_string(first_batch_delta_time + goal->get_arrival_time())
                                    +"] is == now:["+ sim_current_time().str()
                                    +"]? --> "+ ag_support->boolToString(batch == 1 && ag_support->checkIfDoublesAreEqual(first_batch_delta_time + goal->get_arrival_time(), sim_current_time().dbl(), ag_Sched->EPSILON)));

                                    if(batch == 1 && ag_support->checkIfDoublesAreEqual(first_batch_delta_time + goal->get_arrival_time(), sim_current_time().dbl(), ag_Sched->EPSILON))
                                    { /* the currently analyzed internal goal is in the first batch and has arrival time = NOW
                                     * we need to check its preconditions before proceeding... */
                                        if(ag_support->checkIfPreconditionsAreValid(plan_internalgoal, ag_belief_set, "computeGoalDDL A") == false)
                                        {
                                            valid_preconditions = false;
                                            logger->print(logger_level, nesting_spaces +"Plan:["+ std::to_string(plan->get_id())
                                            +"], name:["+ plan->get_goal_name() +"] has NOT valid preconditions");
                                        }
                                        else {
                                            logger->print(logger_level, nesting_spaces +"Plan:["+ std::to_string(plan->get_id())
                                            +"], name:["+ plan->get_goal_name() +"] has valid preconditions");
                                        }
                                    }

                                    if(valid_preconditions)
                                    {
                                        loop_detected = false;
                                        if(goal->getGoalforceReasoningCycle() == true) {
                                            /* if an internal goal forced a reasoning cycle,
                                             * we cannot start from "first_batch_delta_time" because it would be in the past
                                             * (and set equal to the activation of the intention containing the goal).
                                             * Instead, use now as internal goal activation */
                                            tmp_batch_delta_time = sim_current_time().dbl();
                                        }
                                        else {
                                            tmp_batch_delta_time = first_batch_delta_time;
                                        }

                                        tmp_mctgp = computeGoalDDL(plan_internalgoal, contained_plans, loop_detected,
                                                tmp_batch_delta_time,
                                                goal->get_arrival_time(),
                                                verify_intention,
                                                skip_intention_internal_goal,
                                                annidation_level + 1);

                                        if(loop_detected) {
                                            // LOOP DETECTION -------------------------------------------------------------------------
                                            ag_support->removePlanFromCounted(plan, contained_plans);
                                            // END LOOP DETECTION -------------------------------------------------------------------------
                                        }
                                        else {
                                            logger->print(Logger::EveryInfo, nesting_spaces +" 2. > Plan:["+ std::to_string(plan_internalgoal->get_id()) +"]");
                                            if(tmp_mctgp == nullptr)
                                            { // there where no suitable plan for an internal goal(sub-sub-goal) of this goal(sub-goal)
                                                logger->print(logger_level, nesting_spaces +" computeGoalDDL for plan:["+ std::to_string(plan_internalgoal->get_id()) +"] return null. An internal goal can't be satisfy!");
                                            }
                                            else {
                                                doesInternalGoalHasPlan = true;
                                                logger->print(logger_level, nesting_spaces +" contained_plans in: "+ std::to_string(plan->get_id()));
                                                MCTi = ag_support->MCTSelect(MCTi, tmp_mctgp, optimisticApproach);
                                                logger->print(Logger::EveryInfo, nesting_spaces +" 3. Set MCTi to:["+ std::to_string(MCTi->getMCTValue()) +"]\n----------------------------");
                                            }
                                        }
                                    }
                                }
                            }

                            if(MCTi->getInternalGoalsSelectedPlansSize() > 0)
                            {
                                // update the id of the plan chosen for the goal
                                goal->set_selected_plan_id(MCTi->getInternalGoalsSelectedPlans()[0].first->get_id());

                                logger->print(logger_level, nesting_spaces +" --> (SEQ) Best plan selected for internal goal:["+ goal->get_goal_name()
                                        +"] is plan:["+ std::to_string(goal->get_selected_plan_id()) +"] <-- ");
                                logger->print(logger_level, "----------------------------");
                            }
                        }

                        // The entire library of plans has been analyzed, is there a valid plan for the internal goal?
                        if(doesInternalGoalHasPlan) {
                            logger->print(Logger::EveryInfo, "MCTgp.taskset.size: "+ std::to_string(MCTgp->getTaskSet().size()));
                            MCTgp->add_internal_goals_selectedPlans(MCTi);
                        }
                        else {
                            logger->print(logger_level, nesting_spaces +" computeGoalDDL[SEQ]: Goal:["+ std::to_string(goal->get_id())
                            +"], name:["+ goal->get_goal_name() +"] has no valid plans!");
                            // LOOP DETECTION -------------------------------------------------------------------------
                            ag_support->removePlanFromCounted(plan, contained_plans);
                            // END LOOP DETECTION ---------------------------------------------------------------------
                            return nullptr; // there are no plans compatible with the internal goal. The whole plan has to be discarded
                        }
                    }
                }
            }
            else if((task != nullptr && task->getTaskExecutionType() == Task::EXEC_PARALLEL)
                    || (goal != nullptr && goal->getGoalExecutionType() == Goal::EXEC_PARALLEL))
            {
                if(i == 0 && batch == 0)
                {/* 1. when the function is computing the deadline of a plan,
                 * this part of the code is always i > 0 and batch >= 1, because there must have been at least a SEQ action that preceed this one
                 * 2. when we use computeGoalDDL of an intention, the first item may be PAR.
                 * In both cases, we must increase the batch counter before using it */
                    batch += 1; // the first batch of the plan begins with a PAR action
                }

                logger->print(Logger::EveryInfo, nesting_spaces +" Current batch ddl:["+ std::to_string(MCTi->getMCTValue()) +"]");
                if(task != nullptr)
                {
                    MCTi->taskSet_push_back(task);
                    taskDDL = ag_support->getTaskDeadline_based_on_nexec(task,
                            isTaskInAlreadyActiveServer, server_original_ddl,
                            ag_servers, ag_Sched->get_tasks_vector_ready(), ag_Sched->get_tasks_vector_to_release());

                    if(task->getTaskisPeriodic())
                    { // PERIODIC task -> save the ddl of the task
                        if(task->getTaskNExecInitial() == -1) {
                            taskDDL = infinite_ddl;
                        }

                        logger->print(logger_level, nesting_spaces +" PERIODIC > Task:["+ std::to_string(task->getTaskId())
                        +"], plan:["+ std::to_string(task->getTaskPlanId())
                        +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId())
                        +"], DDL: ("+ std::to_string(taskDDL)
                        +" + "+ std::to_string(first_batch_delta_time)
                        +") --> "+ std::to_string(first_batch_delta_time + taskDDL));

                        logger->print(logger_level, nesting_spaces +" MCTi = std::max("+ std::to_string(MCTi->getMCTValue())
                        +", "+ std::to_string(first_batch_delta_time + taskDDL)
                        +") --> "+ std::to_string(std::max(MCTi->getMCTValue(), first_batch_delta_time + taskDDL)));

                        MCTi->setMCTValue(std::max(MCTi->getMCTValue(), first_batch_delta_time + taskDDL));
                    }
                    else
                    { // APERIODIC task
                        if(isTaskInAlreadyActiveServer)
                        { // the selected server is already active
                            logger->print(Logger::EveryInfo, nesting_spaces +" CHECK: "+ std::to_string(intention_lastExecution)
                            +" + "+ std::to_string(MCTgp->getMCTValue())
                            +" > "+ std::to_string(taskDDL)
                            +" --> "+ ag_support->boolToString(intention_lastExecution + MCTgp->getMCTValue() > taskDDL));

                            if(intention_lastExecution + MCTgp->getMCTValue() > taskDDL)
                            {   /* This means that the server absolute ddl is lower then the current analyzed time
                             * Thus, use the default server period as deadline */
                                logger->print(logger_level, nesting_spaces +" (Used server A) > Server:["+ std::to_string(task->getTaskServer())
                                +"], Task:["+ std::to_string(task->getTaskId())
                                +"], plan:["+ std::to_string(task->getTaskPlanId())
                                +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId())
                                +"], DDL: "+ std::to_string(server_original_ddl));

                                logger->print(logger_level, nesting_spaces +" MCTi = std::max("+ std::to_string(MCTi->getMCTValue())
                                +", "+ std::to_string(server_original_ddl)
                                +") --> "+ std::to_string(std::max(MCTi->getMCTValue(), server_original_ddl)));

                                MCTi->setMCTValue(std::max(MCTi->getMCTValue(), server_original_ddl));
                            }
                            else { // This means that the server overlaps this batch
                                /* ---------------------   taskDDL
                                 *              ****        batch   --> MCT = interval (batch to DDL)
                                 *                    ****  batch   --> MCT = interval (batch to DDL) */
                                logger->print(logger_level, nesting_spaces +" (Used server B) > Server:["+ std::to_string(task->getTaskServer())
                                +"], Task:["+ std::to_string(task->getTaskId())
                                +"], plan:["+ std::to_string(task->getTaskPlanId())
                                +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId())
                                +"], DDL:("+ std::to_string(taskDDL)
                                +" - "+ std::to_string(MCTgp->getMCTValue())
                                +") --> "+ std::to_string(taskDDL - MCTgp->getMCTValue()));

                                logger->print(logger_level, nesting_spaces +" MCTi = std::max("+ std::to_string(MCTi->getMCTValue())
                                +", "+ std::to_string(taskDDL - MCTgp->getMCTValue())
                                +") --> "+ std::to_string(std::max(MCTi->getMCTValue(), taskDDL - MCTgp->getMCTValue())));

                                MCTi->setMCTValue(std::max(MCTi->getMCTValue(), taskDDL - MCTgp->getMCTValue()));
                            }
                        }
                        else
                        { // not an instantiated server yet
                            logger->print(logger_level, nesting_spaces +" (NOT Used server) > Server:["+ std::to_string(task->getTaskServer())
                            +"], Task:["+ std::to_string(task->getTaskId())
                            +"], plan:["+ std::to_string(task->getTaskPlanId())
                            +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId())
                            +"], DDL: ("+ std::to_string(taskDDL)
                            +" + "+ std::to_string(first_batch_delta_time)
                            +") --> "+ std::to_string(first_batch_delta_time + taskDDL));

                            logger->print(logger_level, nesting_spaces +" MCTi = std::max("+ std::to_string(MCTi->getMCTValue())
                            +", "+ std::to_string(first_batch_delta_time + taskDDL)
                            +") --> "+ std::to_string(std::max(MCTi->getMCTValue(), first_batch_delta_time + taskDDL)));

                            MCTi->setMCTValue(std::max(MCTi->getMCTValue(), first_batch_delta_time + taskDDL));
                        }
                    }

                    logger->print(logger_level, nesting_spaces +" -- Task:["+ std::to_string(task->getTaskId())
                    +"], Plan:["+ std::to_string(task->getTaskPlanId())
                    +"], original plan:["+ std::to_string(task->getTaskOriginalPlanId())
                    +"], server:["+ std::to_string(task->getTaskServer())
                    +"], name:["+ plan->get_goal_name()
                    +"] -> Final MCTi:["+ std::to_string(MCTi->getMCTValue()) +"] --");
                }
                else if(goal != nullptr)
                { // it's an internal goal
                    doesInternalGoalHasPlan = false;
                    isGoalLinkedToIntention = false;
                    loop_detected = false;
                    valid_preconditions = true;
                    cont_cond_valid = true;
                    ig_plan = nullptr;
                    conditions_check_return_msg = "";
                    MCTj = std::make_shared<MaximumCompletionTime>(); // not in the paper... used to manage the case where 2 or more goals are in PAR

                    if(batch > 1) {
                        skip_intention_internal_goal = true;
                    }
                    else {
                        skip_intention_internal_goal = false;
                    }

                    if(ag_support->checkIfGoalIsLinkedToValidSkill(goal, ag_skill_set) == false)
                    {
                        warning_msg = "[WARNING] (PAR) INVALID INTERNAL GOAL:["+ goal->get_goal_name() +"] NOT linked to any skill! -> Can not be activated!";
                        logger->print(Logger::Warning, warning_msg);
                        ag_metric->addGeneratedWarning(sim_current_time(), "computeGoalDDL", warning_msg);
                        return nullptr; // there are no plans compatible with the internal goal. The whole plan has to be discarded
                    }
                    else
                    {
                        /* If the goal is already linked to an intention, get the plan from which we derived the intention and check if the cont_conditions are valid.
                         * Note: only goals in the first batch can be linked to intention during this phase of the cycle */
                        if(ag_support->checkIfGoalIsLinkedToIntention(goal, ag_intention_set, index_intention) && batch == 1 && check_intentions_fcfs == true) /* "check_intentions_fcfs" is only used with FCFS */
                        {   // compute the deadline of the plan chosen for the internal goal (if already an intention, use its body)

                            intentions_fcfs.push_back(std::make_shared<Intention>(*ag_intention_set[index_intention])); /* only used with FCFS */
                            ig_plan = ag_intention_set[index_intention]->getTopInternalSelectedPlan();
                            ddl_intention = computeGoalDDL(ig_plan, contained_plans, loop_detected,
                                    ag_intention_set[index_intention]->get_batch_startTime(), 0, true, skip_intention_internal_goal, 0);

                            if(loop_detected)
                            { // there where no suitable plan for an internal goal(sub-sub-goal) of this goal(sub-goal)
                                logger->print(logger_level, nesting_spaces +" computeGoalDDL for plan:["+ std::to_string(ig_plan->get_id())
                                +"] return null. Loop detected!");
                            }
                            else if(ddl_intention != nullptr)
                            {
                                logger->print(Logger::EveryInfo, nesting_spaces +" goal:["+ goal->get_goal_name()
                                        +"], activation_time:["+ std::to_string(goal->get_activation_time())
                                +"], arrival_time:["+ std::to_string(goal->get_arrival_time())
                                +"], first_batch_delta_time:["+ std::to_string(first_batch_delta_time)
                                +"], tot: "+ std::to_string(goal->get_arrival_time() + first_batch_delta_time));

                                logger->print(Logger::EveryInfo, nesting_spaces +" ig_plan:["+ ig_plan->get_goal_name()
                                        +"], ddl_intention:["+ std::to_string(ddl_intention->getMCTValue())
                                +"], batch_startTime:["+ std::to_string(ag_intention_set[index_intention]->get_batch_startTime())
                                +"], last_ExecutionTime:["+ std::to_string(ag_intention_set[index_intention]->get_lastExecution())
                                +"], tot: "+ std::to_string(ddl_intention->getMCTValue() + ag_intention_set[index_intention]->get_lastExecution()));

                                if(ag_intention_set[index_intention]->get_waiting_for_completion()) {
                                    // intention is waiting for completion, thus, its deadline is equal to the completion_time
                                    intention_overlap = ag_intention_set[index_intention]->get_scheduled_completion_time();
                                }
                                else {
                                    intention_overlap = ddl_intention->getMCTValue();
                                }

                                logger->print(logger_level, nesting_spaces +" t: "+ sim_current_time().str() +", name:["+ goal->get_goal_name()
                                        +"], Overlap = (intention lastExecution <= updated goal_arrivalTime) = ("+ std::to_string(intention_overlap)
                                +" <= "+ std::to_string(goal->get_arrival_time() + first_batch_delta_time)
                                +") = "+ ag_support->boolToString(intention_overlap <= (goal->get_arrival_time() + first_batch_delta_time)));

                                if(intention_overlap <= (goal->get_arrival_time() + first_batch_delta_time))
                                {   /* if the last execution + deadline of the intention is greater than then initial time of the plan selected for the goal,
                                 * then they overlap. Otherwise, the goal's arrivalTime is in the future and a plan.body must be selected for it */
                                    logger->print(logger_level, nesting_spaces +" (PAR) Overlap = ("+ std::to_string(intention_overlap)
                                    +" <= "+ std::to_string(goal->get_arrival_time() + first_batch_delta_time)
                                    +") = "+ ag_support->boolToString(intention_overlap <= (goal->get_arrival_time() + first_batch_delta_time))
                                    +" --> the intention completes before the goal gets activated. Choose a plan for goal...");

                                    skip_intention_internal_goal = true;
                                }
                                else {
                                    logger->print(logger_level, nesting_spaces +" (PAR) Overlap use intention values...");
                                    isGoalLinkedToIntention = true;

                                    if((int)ag_intention_set.size() > index_intention && (int)ag_intention_set[index_intention]->getMainPlan()->getInternalGoalsSelectedPlans().size() > 0)
                                    {
                                        ig_plan = ag_intention_set[index_intention]->getTopInternalSelectedPlan();

                                        cont_cond_valid = ag_support->checkPlanOrDesireConditions(ag_belief_set, ig_plan->get_cont_conditions(),
                                                conditions_check_return_msg, "plan", ig_plan->get_id(),
                                                ag_intention_set[index_intention]->get_expression_constants(),
                                                ag_intention_set[index_intention]->get_expression_variables());

                                        logger->print(logger_level, nesting_spaces +" Plan:["+ std::to_string(ig_plan->get_id())
                                        +"], cont_cond_valid:["+ ag_support->boolToString(cont_cond_valid)
                                        +"] AND message:["+ conditions_check_return_msg +"]");
                                        logger->print(Logger::EveryInfoPlus, " --- DEBUG --- ");

                                        if(cont_cond_valid == false)
                                        {
                                            logger->print(logger_level, nesting_spaces +" Cont_conditions: "+ ag_support->dump_expression(
                                                    ig_plan->get_cont_conditions(), ag_belief_set,
                                                    ag_intention_set[index_intention]->get_expression_constants(),
                                                    ag_intention_set[index_intention]->get_expression_variables()));

                                            // internal goal was an intention. cont-conditions not valid anymore....
                                            checkAndManageIntentionFalseConditions(ig_plan, ag_selected_plans, ag_goal_set, ag_intention_set, ag_belief_set, empty_release_queue);
                                        }
                                        else {
                                            logger->print(logger_level, nesting_spaces +" ag_intention_set[index_intention]->get_batch_startTime(): "+ std::to_string(ag_intention_set[index_intention]->get_batch_startTime()));

                                            tmp_mctgp = computeGoalDDL(ig_plan, contained_plans, loop_detected,
                                                    ag_intention_set[index_intention]->get_batch_startTime(),
                                                    0,
                                                    verify_intention, // in order to print data
                                                    skip_intention_internal_goal, // batch 1, we want to analyze the intention-set
                                                    annidation_level + 1);

                                            logger->print(Logger::EveryInfo, nesting_spaces +" 3. > Plan:["+ std::to_string(ig_plan->get_id())
                                            +"], tmp_mctgp is null:["+ ag_support->boolToString(tmp_mctgp == nullptr)
                                            +"], loop detected:["+ ag_support->boolToString(loop_detected) +"]");
                                            if(loop_detected)
                                            {
                                                logger->print(logger_level, nesting_spaces +" computeGoalDDL for plan:["+ std::to_string(ig_plan->get_id())
                                                +"] return null. Loop detected!");
                                                // LOOP DETECTION -------------------------------------------------------------------------
                                                ag_support->removePlanFromCounted(plan, contained_plans);
                                                // END LOOP DETECTION -------------------------------------------------------------------------
                                            }
                                            else if(tmp_mctgp == nullptr)
                                            {   // there where no suitable plan for an internal goal(sub-sub-goal) of this goal(sub-goal)
                                                logger->print(logger_level, nesting_spaces +" computeGoalDDL for plan:["+ std::to_string(plan->get_id())
                                                +"] return null. An internal goal can't be satisfy!");
                                            }
                                            else
                                            {
                                                doesInternalGoalHasPlan = true;

                                                logger->print(Logger::EveryInfo, nesting_spaces +" (PAR) Check before MCTSelect...");
                                                logger->print(Logger::EveryInfo, nesting_spaces +" Intention:["+ std::to_string(ag_intention_set[index_intention]->get_id())
                                                +"], plan:["+ std::to_string(ag_intention_set[index_intention]->get_planID())
                                                +"], original plan:["+ std::to_string(ag_intention_set[index_intention]->get_original_planID())
                                                +"], mctgp:["+ std::to_string(tmp_mctgp->getMCTValue())
                                                +"], lastExecution: ("+ std::to_string(tmp_mctgp->getMCTValue())
                                                +" < "+ std::to_string(MCTgp->getMCTValue())
                                                +") -> "+ ag_support->boolToString(tmp_mctgp->getMCTValue() < MCTj->getMCTValue()));

                                                if(tmp_mctgp->getMCTValue() < MCTgp->getMCTValue())
                                                {   /* this means that we are considering an internal goal that has deadline lower
                                                 * than the start of the currently analyzed batch;
                                                 * + the plan for the goal was already active due to another plan
                                                 * -> we are not going to use it because its deadline won't influence the MCTgp (ddl) of this batch */

                                                    logger->print(Logger::EveryInfo, nesting_spaces +" [IMPORTANT] (PAR) Intention:["+ std::to_string(ag_intention_set[index_intention]->get_id())
                                                    +"], plan:["+ std::to_string(ag_intention_set[index_intention]->get_planID())
                                                    +"], original plan:["+ std::to_string(ag_intention_set[index_intention]->get_original_planID())
                                                    +"], mctgp:["+ std::to_string(tmp_mctgp->getMCTValue())
                                                    +"], lastExecution: ("+ std::to_string(tmp_mctgp->getMCTValue())
                                                    +" < "+ std::to_string(MCTgp->getMCTValue())
                                                    +") -> "+ ag_support->boolToString(tmp_mctgp->getMCTValue() < MCTj->getMCTValue()));

                                                    tmp_mctgp->setMCTValue(0);
                                                    MCTj = ag_support->MCTSelect(MCTj, tmp_mctgp, false);
                                                }
                                                else if(batch > 1) {
                                                    logger->print(logger_level, nesting_spaces +" [IMPORTANT] (PAR) Batch: "+ std::to_string(batch)
                                                    +", plan:["+ std::to_string(ag_intention_set[index_intention]->get_planID())
                                                    +"], original plan:["+ std::to_string(ag_intention_set[index_intention]->get_original_planID())
                                                    +"] -> mctgp: "+ std::to_string(tmp_mctgp->getMCTValue())
                                                    +" - MCTgp: "+ std::to_string(MCTgp->getMCTValue())
                                                    +" -> ("+ std::to_string(tmp_mctgp->getMCTValue() - MCTgp->getMCTValue()) +")");

                                                    if(tmp_mctgp->getMCTValue() - MCTgp->getMCTValue() >= 0)
                                                    {
                                                        /* If true, it means that we are analyzing the 2-th, N-th batch of the plan
                                                         * + the batch contains an internal goal
                                                         * + the goal was already active do to another intention
                                                         * + the intention of the goal partially executes within the current batch
                                                         * use (tmp_mctgp - MCTgp allows) to correctly compute the deadline of plan
                                                         * and therefore update the MCTgp */
                                                        tmp_mctgp->setMCTValue(tmp_mctgp->getMCTValue() - MCTgp->getMCTValue());
                                                    }
                                                    else { // Same as the above "if()" statement
                                                        tmp_mctgp->setMCTValue(0);
                                                    }

                                                    MCTi = ag_support->MCTSelect(MCTi, tmp_mctgp, false);
                                                }
                                                else {
                                                    MCTj = ag_support->MCTSelect(MCTj, tmp_mctgp, optimisticApproach);

                                                }
                                                logger->print(Logger::EveryInfo, nesting_spaces +" 4. Set MCTj to:["+ std::to_string(MCTj->getMCTValue()) +"]\n----------------------------");
                                            }
                                        }
                                    }
                                } // -- end else statement: goal overlaps intention linked to it
                            } // -- end else (ddl_intention != nullptr)
                        } // -- end goal linked to intention

                        if(isGoalLinkedToIntention == false)
                        { /* If the goal is not already linked to an intention:
                         * loop over all possible plans in the plan library (ag_plan_set),
                         * looking for those that might achieve the internal goal 'goal' */

                            logger->print(Logger::EveryInfo, nesting_spaces +" --> (PAR) Choosing best plan for internal goal:["+ goal->get_goal_name() +"] <-- ");
                            logger->print(Logger::EveryInfo, "----------------------------");

                            for(std::shared_ptr<Plan> plan_internalgoal : ag_plan_set)
                            {
                                if(plan_internalgoal->get_goal_name() == goal->get_goal_name())
                                {
                                    valid_preconditions = true;

                                    logger->print(Logger::EveryInfo, nesting_spaces +" Internal goal:["+ std::to_string(goal->get_id())
                                    +"], belief:("+ goal->get_goal_name()
                                    +"), plan:["+ std::to_string(plan_internalgoal->get_id())
                                    +"], loop_detected:["+ ag_support->boolToString(loop_detected)
                                    +"] is not linked to any intention! Use now = "+ sim_current_time().str());

                                    logger->print(logger_level, nesting_spaces +"Batch:["+ std::to_string(batch)
                                    +"], first_batch_delta_time:["+ std::to_string(first_batch_delta_time)
                                    +"] + goal.arrivalTime:["+ std::to_string(goal->get_arrival_time())
                                    +"] = abosulte_arrivalTime:["+ std::to_string(first_batch_delta_time + goal->get_arrival_time())
                                    +"] is == now:["+ sim_current_time().str()
                                    +"]? --> "+ ag_support->boolToString(batch == 1 && ag_support->checkIfDoublesAreEqual(first_batch_delta_time + goal->get_arrival_time(), sim_current_time().dbl(), ag_Sched->EPSILON)));

                                    if(batch == 1 && ag_support->checkIfDoublesAreEqual(first_batch_delta_time + goal->get_arrival_time(), sim_current_time().dbl(), ag_Sched->EPSILON))
                                    { /* the currently analyzed internal goal is in the first batch and has arrival time = NOW
                                     * we need to check its preconditions before proceeding... */
                                        if(ag_support->checkIfPreconditionsAreValid(plan_internalgoal, ag_belief_set, "computeGoalDDL B") == false)
                                        {
                                            valid_preconditions = false;
                                            logger->print(logger_level, nesting_spaces +"Plan:["+ std::to_string(plan->get_id())
                                            +"], name:["+ plan->get_goal_name() +"] has NOT valid preconditions");
                                        }
                                        else {
                                            logger->print(logger_level, nesting_spaces +"Plan:["+ std::to_string(plan->get_id())
                                            +"], name:["+ plan->get_goal_name() +"] has valid preconditions");
                                        }
                                    }

                                    if(valid_preconditions)
                                    {
                                        loop_detected = false;
                                        if(goal->getGoalforceReasoningCycle() == true) {
                                            /* if an internal goal forced a reasoning cycle,
                                             * we cannot start from "first_batch_delta_time" because it would be in the past
                                             * (set = to the activation of the intention containing the goal).
                                             * But rather, use now as internal goal activation */
                                            tmp_batch_delta_time = sim_current_time().dbl();
                                        }
                                        else {
                                            tmp_batch_delta_time = first_batch_delta_time;
                                        }

                                        tmp_mctgp = computeGoalDDL(plan_internalgoal, contained_plans, loop_detected,
                                                tmp_batch_delta_time,
                                                goal->get_arrival_time(),
                                                verify_intention,
                                                skip_intention_internal_goal,
                                                annidation_level + 1);

                                        if(loop_detected) {
                                            // LOOP DETECTION ---------------------------------------------------------------------
                                            ag_support->removePlanFromCounted(plan, contained_plans);
                                            // END LOOP DETECTION -----------------------------------------------------------------
                                        }
                                        else {
                                            if(tmp_mctgp == nullptr)
                                            {   // there where no suitable plan for an internal goal(sub-sub-goal) of this goal(sub-goal)
                                                logger->print(logger_level, nesting_spaces +" CalcDDL for plan:["+ std::to_string(plan_internalgoal->get_id()) +"] return null. An internal goal can't be satisfy!");
                                            }
                                            else {
                                                doesInternalGoalHasPlan = true;
                                                MCTj = ag_support->MCTSelect(MCTj, tmp_mctgp, optimisticApproach);
                                                logger->print(Logger::EveryInfo, nesting_spaces +" Set MCTj to:["+ std::to_string(MCTj->getMCTValue()) +"]");
                                            }
                                        }
                                    }
                                }
                            } // -- end for loop

                            if(MCTj->getInternalGoalsSelectedPlansSize() > 0)
                            {
                                goal->set_selected_plan_id(MCTj->getInternalGoalsSelectedPlans()[0].first->get_id());
                                logger->print(logger_level, nesting_spaces +" --> (PAR) Best plan selected for internal goal:["+ goal->get_goal_name()
                                        +"] is plan:["+ std::to_string(goal->get_selected_plan_id()) +"] <-- ");

                                logger->print(logger_level, "----------------------------");
                            }
                        }

                        if(doesInternalGoalHasPlan)
                        {
                            MCTi->taskSet_push_back(MCTj->getTaskSet());
                            logger->print(Logger::EveryInfo, nesting_spaces +" MCTi.taskset.size: "+ std::to_string(MCTi->getTaskSet().size()));
                            MCTgp->add_internal_goals_selectedPlans(MCTj);
                            if(MCTi->getMCTValue() == -1 || MCTj->getMCTValue() == -1) {
                                logger->print(Logger::EveryInfo, nesting_spaces +" Updating MCTi:["+ std::to_string(MCTi->getMCTValue()) +"] to [-1]");
                                MCTi->setMCTValue(-1); // infinite DDL
                            }
                            else {
                                logger->print(Logger::EveryInfo, nesting_spaces +" [j] Updating MCTi = std::max("+ std::to_string(MCTi->getMCTValue())
                                +", "+ std::to_string(MCTj->getMCTValue())
                                +") = ["+ std::to_string(std::max(MCTi->getMCTValue(), MCTj->getMCTValue())) +"]");
                                MCTi->setMCTValue(std::max(MCTi->getMCTValue(), MCTj->getMCTValue()));
                            }
                        }
                        else {
                            logger->print(logger_level, nesting_spaces +" computeGoalDDL[PAR]: Goal:["+ std::to_string(goal->get_id())
                            +"], name:["+ goal->get_goal_name() +"] has no valid plans!");
                            // LOOP DETECTION -------------------------------------------------------------------------
                            ag_support->removePlanFromCounted(plan, contained_plans);
                            // END LOOP DETECTION -------------------------------------------------------------------------
                            return nullptr; // there are no plans compatible with the internal goal. The whole plan has to be discarded
                        }
                    }
                }
            }
        }

        MCTgp->taskSet_push_back(MCTi->getTaskSet());
        if(MCTgp->getMCTValue() == -1 || MCTi->getMCTValue() == -1) {
            logger->print(Logger::EveryInfo, nesting_spaces +" Updating MCTgp:["+ std::to_string(MCTgp->getMCTValue()) +"] to [-1]");
            MCTgp->setMCTValue(-1); // infinite DDL
        }
        else {
            logger->print(Logger::EveryInfo, nesting_spaces +" Updating MCTgp:["+ std::to_string(MCTgp->getMCTValue())
            +" + "+ std::to_string(MCTi->getMCTValue())
            +"] to ["+ std::to_string(MCTgp->getMCTValue() + MCTi->getMCTValue()) +"]");
            // LOOP DETECTION -------------------------------------------------------------------------
            ag_support->removePlanFromCounted(plan, contained_plans);
            // END LOOP DETECTION -------------------------------------------------------------------------
            MCTgp->setMCTValue(MCTgp->getMCTValue() + MCTi->getMCTValue());
        }

        // the DDL compute up to now is relative w.r.t. the actions in the plan. The absolute DDL must also count the time between intention.start and intetion.lastExec
        MCTgp->setMCTValue(MCTgp->getMCTValue());

        logger->print(logger_level, nesting_spaces +" -- Plan:["+ std::to_string(plan->get_id())
        +"], name:["+ plan->get_goal_name()
        +"] -> Final MCTgp:["+ std::to_string(MCTgp->getMCTValue()) +"] --"); // relative MCTgp
        logger->print(logger_level, "----------------------------");
    }
    logger->print(Logger::EveryInfo, nesting_spaces +" -----------END of computeGoalDDL------------");

    return MCTgp;
    // End of function 'computeGoalDDL'
}
// *******************************************************************************************************

/******** PHII + support functions *********************************************************************/
/* Function that selects from each intention the task to execute */
void agent::phiI(agentMSG* agMsg)
{
    logger->print(Logger::Default, "----phiI----");

    std::shared_ptr<Task> task, next_task;
    std::shared_ptr<Goal> goal, next_goal;

    std::vector<std::shared_ptr<Action>> intention_stack;
    std::shared_ptr<Action> top; // intention's stack top action
    std::vector<int> annidation_level_prec;
    std::shared_ptr<MaximumCompletionTime> MCT_main_p;
    std::vector<std::pair<std::shared_ptr<Plan>, int>> internal_goals_plans;
    std::vector<std::shared_ptr<Effect>> effects_at_begin, effects_at_end;
    std::shared_ptr<MaximumCompletionTime> MCTplan;
    std::shared_ptr<Plan> ig_plan;
    std::shared_ptr<Plan> plan_copy; // make a copy of the selected plan for this goal. Note: the action in the body are shared_ptr that are pass as reference in the new plan!

    std::string warning_msg = "";
    json post_conditions;

    int index = 0;
    int tmp_cnt; // number of beliefs update due to effects_at_begin
    int tmp_counter = 0;
    int ig_plan_annidation = 0;
    int next_intention_original_plan_id;
    double goal_arrival_time;
    bool updated_beliefset = false; // if at least one effects_at_begin changes the beliefset, updated_beliefset = true and a new reasoning cycle must be scheduled immediately
    bool checkNextAction = true; // used to check if following action has ExecutionType = PARALLEL
    bool get_next_internal_goal = false;
    bool manage_internal_goal = true;

    int log_level = logger->get_level(); // Debug
    // /* xx */logger->set_level(Logger::EveryInfo);

    tasks_to_be_release_phiI.clear();

    //  1. Select the 'top' action from the stack of each intention.
    for(int i = 0; i < (int)ag_intention_set.size(); i++)
    {
        annidation_level_prec.clear();
        annidation_level_prec.push_back(0);

        if(ag_intention_set[i]->get_waiting_for_completion() == true)
        { // do nothing ....
            logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                    +", Intention:["+ std::to_string(ag_intention_set[i]->get_id())
            +"], plan:["+ std::to_string(ag_intention_set[i]->get_planID())
            +"], original_plan:["+ std::to_string(ag_intention_set[i]->get_original_planID())
            +"] waiting for completion:["+ std::to_string(ag_intention_set[i]->get_scheduled_completion_time()) +"]");
            logger->print(Logger::EveryInfoPlus, " --- DEBUG --- ");
        }
        else if(ag_intention_set[i]->get_Actions().size() == 0)
        {
            // No action available for this intention + NOT waiting for completion! Should never reach this point
            warning_msg = "**** (1) Intention:["+ std::to_string(ag_intention_set[i]->get_id());
            warning_msg += "], plan:["+ std::to_string(ag_intention_set[i]->get_planID());
            warning_msg += "], original_plan:["+ std::to_string(ag_intention_set[i]->get_original_planID());
            warning_msg += "], name:["+ ag_intention_set[i]->get_goal_name();
            warning_msg += "] has no more actions. AND NOT WAITINIG FOR COMPLETION! ****";
            logger->print(Logger::Warning, warning_msg);
            ag_metric->addGeneratedWarning(sim_current_time(), "phiI", warning_msg);

            // Thus, intention completed: *********************************
            intention_has_been_completed(ag_intention_set[i]->get_goalID(), ag_intention_set[i]->get_planID(), ag_intention_set[i]->get_original_planID(), i);
            i = -1; // we do not know how many and which intentions have been removed, we must restart the loop from 0
            // ************************************************************
        }
        else {
            checkNextAction = true;
            index = 0;

            /* IDEA: always check the next action in order to check if its ExecutionType is PARALLEL
             * If it is, it means that the next action must be executed alongside the one on top of the stack.
             * Two cases:
             * - action is a Task: add it to vector-to-release. Add to the vector every other consecutive next task s.t. it's PARALLEL,
             * - action is a Goal: replace the first action execution type with PAR, replace selected plan's body to the goal in the intention, check the new top action*/
            while(checkNextAction)
            {
                checkNextAction = false;
                intention_stack = ag_intention_set[i]->get_Actions();
                if(intention_stack.size() == 0) {
                    i -= 1; // re-analyze this intention
                    break;  // exit while
                }
                else if(index < (int)intention_stack.size())
                {
                    top = intention_stack[index];

                    // 2. if it's a task, add it to the -to-be-released-vector:
                    if(top->get_type() == "task")
                    {
                        task = std::dynamic_pointer_cast<Task>(top);

                        if(index + 1 < (int)intention_stack.size())
                        {
                            if(intention_stack[index + 1]->get_type() == "task") {
                                next_task = std::dynamic_pointer_cast<Task>(intention_stack[index + 1]);
                                if(next_task->getTaskExecutionType() == Task::EXEC_PARALLEL) {
                                    checkNextAction = true;
                                }
                            }
                            else if(intention_stack[index + 1]->get_type() == "goal") {
                                goal = std::dynamic_pointer_cast<Goal>(intention_stack[index + 1]);
                                if(goal->getGoalExecutionType() == Goal::EXEC_PARALLEL) {
                                    checkNextAction = true;
                                }
                            }
                        }

                        logger->print(Logger::EveryInfo, "+++ Task:["+ std::to_string(task->getTaskId())
                        +"], plan:["+ std::to_string(task->getTaskPlanId())
                        +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId())
                        +"], activated:["+ ag_support->boolToString(task->getTaskisBeenActivated())
                        +"], status:["+ task->getTaskStatus_as_string() +"]");

                        /* Add a task to to-be-released vector only it doesn't already contains it
                         * The problem may arise if during the execution of a task, a new intention gets instantiated.
                         * If it happens, the script must not add the 'top' element of every intentions, but ONLY of the new ones.
                         * SOL 1 (08/04) --> To do so, use the field 'isBeenActivated' of Task's class*/
                        if(task->getTaskisBeenActivated() == false)
                        {
                            task->setTaskisBeenActivated(true); // update the Activation status

                            /* update the status of the task only if it has arrivalTime 0.
                             * This allows to go directly to the execution of the task by calling 'else if(fabs...)' in the schedule_schedule_taskset_rt function.
                             * For those task with arrivalTime != 0, we have to schedule their arrival at the correct time,
                             *  we can't execute the task directly.And we have to leave their status=IDLE */
                            if(ag_support->checkIfDoublesAreEqual(task->getTaskArrivalTime(), 0)) {
                                task->setTaskStatus(Task::TSK_ACTIVE);
                            }

                            /* ------------------------------------------------------------------------------------
                             * update the Arrival time of the task, based on the execution instant:
                             * original: task->setTaskArrivalTime(sim_current_time().dbl() + task->getTaskArrivalTime());
                             * Use the Initial Arrival Time of the task.
                             * This allows to use the original value and avoid using "old" data that has not been "cleaned" */
                            task->setTaskArrivalTime(sim_current_time().dbl() + task->getTaskArrivalTimeInitial());
                            // ------------------------------------------------------------------------------------

                            // update the 'firstActivation' field of the intention
                            if(ag_intention_set[i]->get_firstActivation() == -1) {
                                ag_intention_set[i]->set_firstActivation(sim_current_time().dbl());
                            }

                            // update the 'batch_startTime' field of the intention (if needed)
                            if(task->getTaskExecutionType() == Task::EXEC_SEQUENTIAL) {
                                ag_intention_set[i]->set_batch_startTime(sim_current_time().dbl());
                            }

                            /* convert the task from shared_pointer(std::shared_ptr<Task>) to classic pointer (std::shared_ptr<Task> );
                             * and then add it to the -to-be-released-vector */
                            tasks_to_be_release_phiI.push_back(task);
                        }
                    }
                    else if(top->get_type() == "goal")
                    {   // We have to find the best plan able to achieve it and replace the internal goal with plan.body
                        // it enters here when task is Task:SEQ before the internal goal...
                        goal = std::dynamic_pointer_cast<Goal>(top);

                        logger->print(Logger::EveryInfo, "Internal goal:["+ goal->get_goal_name() +"], id: "+ std::to_string(goal->get_id())
                        +", plan: "+ std::to_string(goal->get_plan_id())
                        +", original_plan: "+ std::to_string(goal->get_original_plan_id())
                        +", execution_type: "+ goal->getGoalExecutionType_as_string());

                        if(index + 1 < (int)intention_stack.size()) {
                            if(intention_stack[index + 1]->get_type() == "task") {
                                next_task = std::dynamic_pointer_cast<Task>(intention_stack[index + 1]);
                                if(next_task->getTaskExecutionType() == Task::EXEC_PARALLEL) {
                                    checkNextAction = true;
                                }
                            }
                            else if(intention_stack[index + 1]->get_type() == "goal") {
                                next_goal = std::dynamic_pointer_cast<Goal>(intention_stack[index + 1]);
                                if(next_goal->getGoalExecutionType() == Goal::EXEC_PARALLEL) {
                                    checkNextAction = true;
                                }
                            }
                        }

                        if(goal->getGoalisBeenActivated() == false)
                        {
                            if(ag_support->checkIfGoalIsLinkedToValidSkill(goal, ag_skill_set) == false)
                            { /* Note: the execution should never reach this point, a goal/internal goal with invalid skill_name gets detected in 'computeGoalDDL' */
                                warning_msg = "[WARNING] INTERNAL GOAL:["+ goal->get_goal_name() +"] NOT linked to any skill! -> Can not be activated!";
                                logger->print(Logger::Warning, warning_msg);
                                ag_metric->addGeneratedWarning(sim_current_time(), "phiI", warning_msg);

                                checkAndManageIntentionFalseConditions(ag_intention_set[i]->getTopInternalSelectedPlan(),
                                        ag_selected_plans, ag_goal_set, ag_intention_set, ag_belief_set, tasks_to_be_release_phiI);

                                i = -1; // we DO NOT know how many intentions have been removed from "checkAndManageIntentionFalseConditions" nor which ones. Thus, restart from the first element in the set
                                checkNextAction = false;
                                break;
                            }
                            else { // internal goal has a valid "goal_name"
                                // 1. find the plan that satisfy the goal and that has the highest PREF value
                                MCT_main_p = ag_intention_set[i]->getOriginalMainPlan();
                                internal_goals_plans = MCT_main_p->getInternalGoalsSelectedPlans();
                                get_next_internal_goal = false;
                                manage_internal_goal = true;

                                // update the 'batch_startTime' field of the intention (if needed)
                                // absolute_arrival_time() == -1 means that the goal has not been already managed by another phiI, and that is not already waiting for its activation
                                if(goal->getGoalExecutionType() == Goal::EXEC_SEQUENTIAL && goal->get_absolute_arrival_time() == -1) {
                                    ag_intention_set[i]->set_batch_startTime(sim_current_time().dbl());
                                }

                                // --------------------------------------------------------------------------
                                /* Before managing the internal goal, we must check the "arrivalTime".
                                 * - if arrivalTime = 0: move on as always and execute the 'for' loop (manage_internal_goal = true)
                                 * - if arrivalTime < 0: this will never happen, negative goal.arrivalTime are set to 0 in "jsonfile_handler"
                                 * - if arrivalTime > 0: activation of the goal is postponed */
                                if(goal->get_arrival_time() > ag_Sched->EPSILON)
                                {   // the goal is not ready to be activated. It's arrivalTime is schedule for the future...
                                    manage_internal_goal = false;

                                    if(goal->get_absolute_arrival_time() == -1)
                                    {   // the arrivalTime for the goal is not been scheduled yet...
                                        goal_arrival_time = sim_current_time().dbl() + goal->get_arrival_time();
                                        goal->set_absolute_arrival_time(goal_arrival_time);

                                        logger->print(Logger::Default, "t: "+ sim_current_time().str()
                                                +", internal goal id:["+ std::to_string(goal->get_id())
                                        +"], goal name:["+ goal->get_goal_name()
                                        +"], plan_id:["+ std::to_string(goal->get_plan_id())
                                        +"], original_plan_id:["+ std::to_string(goal->get_original_plan_id())
                                        +"] is linked to intention:["+ std::to_string(i)
                                        +"]\n intention original plan id:["+ std::to_string(ag_intention_set[i]->get_original_planID())
                                        +"] ---> arrival time relative:["+ std::to_string(goal->get_arrival_time())
                                        +"] ---> arrival time absolute:["+ std::to_string(goal->get_absolute_arrival_time()) +"]");

                                        simtime_t at_time = convert_double_to_simtime_t(goal_arrival_time);
                                        scheduleAt(at_time, set_internal_goal_arrival(goal, ag_intention_set[i]->get_original_planID()));
                                        scheduled_msg += 1;
                                    }
                                    else
                                    { // the arrivalTime for the goal is already been scheduled for the future. Ignore it...
                                        logger->print(Logger::Default, "t: "+ sim_current_time().str()
                                                +", internal goal id:["+ std::to_string(goal->get_id())
                                        +"], goal name:["+ goal->get_goal_name()
                                        +"], plan_id:["+ std::to_string(goal->get_plan_id())
                                        +"], original_plan_id:["+ std::to_string(goal->get_original_plan_id())
                                        +"] is linked to intention:["+ std::to_string(i)
                                        +"], arrival time relative:["+ std::to_string(goal->get_arrival_time())
                                        +"] ALREADY SCHEDULED at absolute time t: "+ std::to_string(goal->get_absolute_arrival_time()));

                                        // if the arrivalTime of the goal has been scheduled for time <= now, then we must analyze the goal...
                                        if(goal->get_absolute_arrival_time() != -1 && goal->get_absolute_arrival_time() <= sim_current_time().dbl())
                                        {
                                            logger->print(Logger::EssentialInfo, " arrivalTime of the goal has been scheduled for time <= now, then we must analyze the goal... "+ std::to_string(goal->get_absolute_arrival_time())
                                            +" <= "+ std::to_string(sim_current_time().dbl()));

                                            manage_internal_goal = true;
                                        }
                                    }
                                }
                                else {
                                    // goal.arrivaltime is set to 0. Hence, we can update the absolute arrival time and move on with the execution
                                    goal_arrival_time = sim_current_time().dbl() + goal->get_arrival_time();
                                    goal->set_absolute_arrival_time(goal_arrival_time);
                                }

                                if(manage_internal_goal == true)
                                {
                                    logger->print(Logger::EveryInfo, "Internal goal:["+ goal->get_goal_name() +"] has 'arrivalTime' set to 0 -> activate the goal!");
                                    // -------------------------------------------------------------------------

                                    /* [IMPORTANT] At this point of the execution, the plans for the internal goals have already been chosen by ag_reasoning_cycle.
                                     * However, the plans are selected only based on their overall ddl and preconditions are not considered until igs are ready to be activated.
                                     * This means that if we have more than one plan for the same internal goal and the one with the highest ddl has invalid preconditions,
                                     * the agent has NO WAY to achieve such goal.
                                     * 03/06/21: This is of course a problem, but the agent needs a plan for every ig in order to perform the schedulability test
                                     * + it can not check the preconditions of all plans during (IsSchedulable) because the state of the beliefset may change before igs are activated.
                                     * IMPLEMENTED SOL: add a flag to the invalid plan, immediately call ag_reasoning_cycle and check if there are other ways to achieve the goal
                                     * in practice: in function 'ag_internal_goal_activation' (responsible for managing the activation of an internal goal)
                                     * if the preconditions of the plan 'p' picked for the goal to activate are not valid,
                                     * a new reasoning cycle is called an plan 'p' will be ignored for the goal.
                                     * To do so, in 'computeGoalDDL', if the goal is not linked to an intention,
                                     * we exploits statement:
                                     * if(goal->get_selected_plan_id() == plan_internalgoal->get_id()
                                     *   && goal->get_has_arrival_time_zero() == false).
                                     *   When it's false, the plan_internalgoal is ignored. */
                                    for(int p = 1; p < (int)internal_goals_plans.size() && get_next_internal_goal == false; p++)
                                    { // loop over all internal goals of the main plan
                                        if(ag_support->checkIfPlanWasAlreadyIntention(internal_goals_plans[p].first, ag_intention_set))
                                        {   // internal goal is already an intention....
                                            goal->setGoalisBeenActivated(true);
                                            logger->print(Logger::EveryInfo, " (PhiI) Plan:["+ std::to_string(internal_goals_plans[p].first->get_id())
                                            +"], name:["+ internal_goals_plans[p].first->get_goal_name() +"] is already an intention! -> ignore it...");
                                        }
                                        else
                                        {
                                            logger->print(Logger::EveryInfo, "check names: "+ internal_goals_plans[p].first->get_goal_name() +" == "+ goal->get_goal_name());
                                            if(goal->get_goal_name() == internal_goals_plans[p].first->get_goal_name())
                                            {
                                                if(internal_goals_plans[p].second == 1)
                                                { // plans with annidation level = 1 are the actual internal goals of the main plan...
                                                    if(ag_support->checkIfGoalIsInGoalset(goal, ag_goal_set) == true)
                                                    {
                                                        logger->print(Logger::EssentialInfo, "Internal goal:["+ std::to_string(goal->get_id()) +"] is already in Goalset...");
                                                        goal->setActivation_time(sim_current_time().dbl());
                                                        goal->setGoalisBeenActivated(true);
                                                    }
                                                    else
                                                    {
                                                        // Added: check if internal goal plan has valid preconditions before converting it to an intention...
                                                        if(ag_support->checkIfPreconditionsAreValid(internal_goals_plans[p].first, ag_belief_set, "phiI") == false)
                                                        { // internal goal plan's preconditions are not valid...
                                                            checkAndManageIntentionFalseConditions(internal_goals_plans[p].first, ag_selected_plans, ag_goal_set,
                                                                    ag_intention_set, ag_belief_set, tasks_to_be_release_phiI);

                                                            i = -1; // we DO NOT know how many intentions have been removed from "checkAndManageIntentionFalseConditions" nor which ones. Thus, restart from the first element in the set
                                                            checkNextAction = false;
                                                            break;
                                                        }
                                                        else
                                                        {
                                                            goal->setActivation_time(sim_current_time().dbl());
                                                            goal->setGoalisBeenActivated(true);
                                                            effects_at_begin = internal_goals_plans[p].first->get_effects_at_begin();
                                                            effects_at_end = internal_goals_plans[p].first->get_effects_at_end();
                                                            post_conditions = internal_goals_plans[p].first->get_post_conditions();

                                                            MCTplan = std::make_shared<MaximumCompletionTime>();
                                                            tmp_counter = 0;
                                                            ig_plan_annidation = 0;

                                                            MCTplan->setMCTValue(goal->get_DDL());

                                                            for(int x = p; x < (int)internal_goals_plans.size(); x++)
                                                            { // stay in here as long as the next .second is greater than 1. [*]
                                                                ig_plan = internal_goals_plans[x].first;
                                                                ig_plan_annidation = internal_goals_plans[x].second;
                                                                plan_copy = ig_plan->makeACopyOfPlan(); // make a copy of the selected plan for this goal

                                                                if(x == p)
                                                                {
                                                                    logger->print(Logger::EssentialInfo, "New internal goal: chosen plan:["+ std::to_string(plan_copy->get_id()) +"]");
                                                                    ag_intention_set[i]->push_internal_goal(goal, plan_copy->get_id(), plan_copy->get_body().size());
                                                                    next_intention_original_plan_id = plan_copy->get_id();
                                                                }

                                                                // internal_goal_plan_valid_conditions = true;
                                                                std::vector<std::shared_ptr<Action>> body_actions = plan_copy->get_body();
                                                                logger->print(Logger::EveryInfo, "plan:["+ std::to_string(plan_copy->get_id()) +"], body.size:["+ std::to_string(body_actions.size()) +"]");

                                                                // update the tasks in of the selected plan 'ig_plan' and add them to 'MCTplan'...
                                                                for(int k = 0; k < (int)body_actions.size(); k++)
                                                                {
                                                                    logger->print(Logger::EveryInfoPlus, " -- Action["+ std::to_string(k)
                                                                    +"] is of type: "+ body_actions[k]->get_type());

                                                                    // if it's a task, we have to update it's t_intention_id
                                                                    if(body_actions[k]->get_type() == "task")
                                                                    {
                                                                        task = std::dynamic_pointer_cast<Task>(body_actions[k]);
                                                                        task->setTaskPlanId(ag_intention_set[i]->get_planID());
                                                                        task->setTaskOriginalPlanId(plan_copy->get_id());

                                                                        logger->print(Logger::EveryInfo, "task original plan id: "+ std::to_string(plan_copy->get_id())
                                                                        +", set to: "+ std::to_string(task->getTaskOriginalPlanId()));

                                                                        task->setTaskisBeenActivated(false);
                                                                    }
                                                                }
                                                                MCTplan->add_internal_goal_selectedPlan(plan_copy, internal_goals_plans[x].second - 1);

                                                                tmp_counter += 1;

                                                                // if the next plan has .second = 1, it means that it's an internal goal of the main plan
                                                                if(x + 1 < (int)internal_goals_plans.size() && internal_goals_plans[x + 1].second == 1) {
                                                                    get_next_internal_goal = true;
                                                                    break;
                                                                }
                                                            }
                                                            p += tmp_counter; // move forward the 'p' index as well, in order to be sure it's aligned and pointing to correct next tmp_plan
                                                            // --------------------------------------------------

                                                            // create a new intention for the internal goal, using the selected (and valid) plan 'ig_plan'...
                                                            int new_intention_id = 0;
                                                            if(ag_intention_set.size() > 0) {
                                                                new_intention_id = ag_intention_set[ag_intention_set.size() - 1]->get_id() + 1; // id of the last store intention + 1
                                                            }

                                                            // compute "goal" priority using the specified expression ...............................
                                                            ag_support->computeGoalPriority(goal, ag_belief_set);
                                                            // ..........................................................................................

                                                            ag_goal_set.push_back(goal); // add internal goal to goal_set
                                                            ag_metric->addActivatedGoal(goal, sim_current_time().dbl(), ag_intention_set[i]->get_planID(),
                                                                    next_intention_original_plan_id, "internal goal: "+ std::to_string(ig_plan_annidation));

                                                            logger->print(Logger::EveryInfo, "print sizes: "+ std::to_string(ag_intention_set.size())
                                                            +", "+ std::to_string(ag_intention_set[ag_intention_set.size() - 1]->getInternalGoalsSelectedPlansSize()));

                                                            ag_selected_plans.push_back(MCTplan);

                                                            logger->print(Logger::EssentialInfo, "A NEW INTENTION FROM GOAL:["+ std::to_string(goal->get_id()) +"], goal name:["+ goal->get_goal_name() +"]");
                                                            reasoning_cycle_new_activated_goals.push_back(goal->get_goal_name()); // list of goals activated during the current instant of time and that will be send to che client
                                                            ag_intention_set.push_back(
                                                                    std::make_shared<Intention>(new_intention_id,
                                                                            goal,
                                                                            ag_intention_set[i]->get_planID(),
                                                                            MCTplan, ag_belief_set,
                                                                            effects_at_begin, effects_at_end,
                                                                            post_conditions,
                                                                            sim_current_time().dbl())
                                                            );

                                                            // update the 'firstActivation' field of the intention
                                                            if(ag_intention_set[i]->get_firstActivation() == -1) {
                                                                ag_intention_set[i]->set_firstActivation(sim_current_time().dbl());
                                                            }
                                                            // --------------------------------------------------------------------------

                                                            // --------------------------------------------------------------------------
                                                            // check and apply (if present) EFFECTS_AT_BEGIN of this intention...
                                                            tmp_cnt = ag_support->updateBeliefsetDueToEffect(ag_belief_set,
                                                                    ag_intention_set[ag_intention_set.size()-1],
                                                                    ag_intention_set[ag_intention_set.size()-1]->get_effects_at_begin());

                                                            logger->print(Logger::EssentialInfo, " Updated beliefs: "+ std::to_string(tmp_cnt));
                                                            if(tmp_cnt > 0) {
                                                                updated_beliefset = true;
                                                            }
                                                            /* Clear list of "effects_at_begin" as soon as they have been applied.
                                                             * The reason is that if they update the beliefset,
                                                             * a reasoning cycle must be performed immediately.
                                                             * Thus, clearing the list guarantees that no reasoning cycle loops get activated */
                                                            ag_intention_set[ag_intention_set.size()-1]->clear_effects_at_begin();
                                                            // --------------------------------------------------------------------------

                                                            ag_metric->addActivatedIntentions(ag_intention_set[ag_intention_set.size() - 1], sim_current_time().dbl());
                                                            logger->print(Logger::EveryInfo, "original: "+ std::to_string(ag_intention_set[ag_intention_set.size() - 1]->get_original_planID())
                                                            + ", will be update to: "+ std::to_string(next_intention_original_plan_id));

                                                            ag_intention_set[ag_intention_set.size() - 1]->set_original_planID(next_intention_original_plan_id);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else { // should never reach this point
                        warning_msg = "[WARNING] Intention:[original_plan_id="+ std::to_string(ag_intention_set[i]->get_original_planID()) +"] contains invalid action(s)!";
                        logger->print(Logger::Warning, warning_msg);
                        ag_metric->addGeneratedWarning(sim_current_time(), "phiI", warning_msg);
                    }
                    index += 1;
                }
            } // end while
        }
    } // end for

    // [*] copy the "tasks_to_be_release_phiI" to the actual release queue:
    for(int i = 0; i < (int)tasks_to_be_release_phiI.size(); i++) {
        ag_Sched->ag_add_task_in_vector_to_release(tasks_to_be_release_phiI[i]->cloneTask()); // added clone to manage shared_ptr
    }

    // update statistics....
    ag_metric->addPhiIStats(ag_goal_set.size(), ag_intention_set.size(), sim_current_time());

    /* ---------------------------------------------------------------------------------------------------------------------------------
     * TODO Da decidere: se un goal è collegato ad un piano con conditions invalide, ha senso fare un nuovo ciclo di ragionamento?
     * Permetterebbe di utilizzare la U del piano rimosso per qualcos'altro?
     * --------------------------------------------------------------------------------------------------------------------------------- */
    if(updated_beliefset)
    { // Immediately repeat a new reasoning cycle to deal with updated beliefset (due to effects)
        scheduleAt(sim_current_time(), set_reasoning_cycle(true));
        scheduled_msg += 1;
        ag_support->setScheduleInFuture("PHII - repeat reasoning cycle due to 'effects_begin'"); // METRICS purposes

        logger->print(Logger::EssentialInfo, "->-> BELIEFSET CHANGED DUE TO EFFECTS -> SCHEDULE NEW REASONING CYCLE... <-<-");
    }
    else {
        // manage the case when the head of the ready queue has changed, and the new top element is in "READY" state instead of "EXEC"
        if(ag_support->manageReadyQueue(ag_Sched)) {
            scheduleAt(sim_current_time(), set_task_activation());
            scheduled_msg += 1;
        }

        // Once we have setup the -to-be-released-vector, schedule the execution of the tasks
        scheduleAt(sim_current_time(), set_ag_scheduler());
        scheduled_msg += 1;
        ag_support->setScheduleInFuture("SCHEDULE - phiI EDF"); // METRICS purposes
    }

    /********* DEBUG:phiI results **********************/
    if(logger->get_level() >= Logger::EssentialInfo) {
        logger->print(Logger::Default, "PhiI Result ------------------------------------");
        ag_support->printIntentions(ag_intention_set, ag_goal_set);
        logger->print(Logger::Default, "-----------------------------");
        ag_Sched->ev_ag_tasks_vector_to_release();
        logger->print(Logger::Default, "-----------------------------");
        ag_Sched->ev_ag_tasks_vector_ready();
        // logger->print(Logger::Default, "-----------------------------");
        // ag_support->printWaitingCompletionIntentions(ag_intention_set, ag_goal_set, false);
        logger->print(Logger::Default, "PhiI Ended -------------------------------------");
    }
    /***************************************************/

    logger->set_level(log_level); // Debug
    // END of phiI --------------------------------------------------------------------
}

/* Takes each intention, one by one, execute all the actions and then moves to the next intention */
void agent::phiI_FCFS(agentMSG* agMsg) {
    logger->print(Logger::EssentialInfo, "--------phiI_FCFS--------");

    std::shared_ptr<Task> next_task;
    std::shared_ptr<Task> tmp_top_task;
    std::shared_ptr<Goal> tmp_goal;
    std::shared_ptr<Action> top; // intention's stack top action
    std::vector<std::shared_ptr<Action>> intention_stack;
    std::string warning_msg = "";
    const int index_intention = 0; // refers to the i-th intention we are analyzing
    bool updated_beliefset = false; // if at least one effects_at_begin changes the beliefset, updated_beliefset = true and a new reasoning cycle must be scheduled immediately

    tasks_to_be_release_phiI.clear(); // always clear vector before using it

    if((int)ag_intention_set.size() > index_intention)
    {
        if(ag_intention_set[index_intention]->get_waiting_for_completion() == true)
        { // do nothing ....
            logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                    +", Intention:["+ std::to_string(ag_intention_set[index_intention]->get_id())
            +"], plan:["+ std::to_string(ag_intention_set[index_intention]->get_planID())
            +"], original_plan:["+ std::to_string(ag_intention_set[index_intention]->get_original_planID())
            +"] waiting for completion:["+ std::to_string(ag_intention_set[index_intention]->get_scheduled_completion_time()) +"]");
            logger->print(Logger::EveryInfoPlus, " --- DEBUG --- ");
        }
        else if(ag_intention_set[index_intention]->get_Actions().size() == 0)
        {
            // No action available for this intention + NOT waiting for completion! Should never reach this point
            warning_msg = "**** (2) Intention:["+ std::to_string(ag_intention_set[index_intention]->get_id());
            warning_msg += "], plan:["+ std::to_string(ag_intention_set[index_intention]->get_planID());
            warning_msg += "], name:["+ ag_intention_set[index_intention]->get_goal_name() +"] has no more actions. AND NOT WAITINIG FOR COMPLETION! ****";
            logger->print(Logger::Warning, warning_msg);
            ag_metric->addGeneratedWarning(sim_current_time(), "phiI_FCFS", warning_msg);

            // Thus, intention completed: *********************************
            intention_has_been_completed(ag_intention_set[index_intention]->get_goalID(),
                    ag_intention_set[index_intention]->get_planID(),
                    ag_intention_set[index_intention]->get_original_planID(), index_intention);
            // ************************************************************
        }
        else {
            removeFCFSInvalidTasksFromSchedulerVectors(ag_intention_set);
            tasks_to_be_release_phiI = recursiveFCFSPhiI(index_intention, ag_intention_set[index_intention]->get_goalID(), updated_beliefset);

            if(tasks_to_be_release_phiI.size() > 1) {
                std::string msg = " FCFS algorithm can only schedule one task at the time ("+ std::to_string(tasks_to_be_release_phiI.size()) +" where found).";
                logger->print(Logger::Error, msg);
                throwRuntimeException(true, glob_path, glob_user, msg, "agent", "phiI_FCFS", sim_current_time().dbl());
            }
            else if(tasks_to_be_release_phiI.size() == 1) {
                // add the task to the release queue
                ag_Sched->ag_add_task_in_vector_to_release(tasks_to_be_release_phiI[0]->cloneTask()); // added clone to manage shared_ptr
            }
        }
    }

    // update statistics....
    ag_metric->addPhiIStats(ag_goal_set.size(), ag_intention_set.size(), sim_current_time());

    // Note: if all active intentions get removed due to invalid conditions, force a new reasoning cycle
    if(updated_beliefset || ag_intention_set.size() == 0)
    { // Immediately repeat a new reasoning cycle to deal with updated beliefset (due to effects)
        scheduleAt(sim_current_time(), set_reasoning_cycle(true));
        scheduled_msg += 1;
        ag_support->setScheduleInFuture("PHII_FCFS - repeat reasoning cycle due to 'effects_begin'"); // METRICS purposes

        logger->print(Logger::EssentialInfo, "->-> BELIEFSET CHANGED DUE TO EFFECTS or REMOVED INTENTIONs -> SCHEDULE NEW REASONING CYCLE... <-<-");
    }
    else {
        // manage the case when the head of the ready queue has changed, and the new top element is in "READY" state instead of "EXEC"
        if(ag_support->manageReadyQueue(ag_Sched)) {
            scheduleAt(sim_current_time(), set_task_activation());
            scheduled_msg += 1;
        }
        // ------------------------
        // Once we have setup the -to-be-released-vector, schedule the execution of the tasks
        scheduleAt(sim_current_time(), set_ag_scheduler());
        ag_support->setScheduleInFuture("SCHEDULE - phiI FCFS"); // METRICS purposes
    }

    /********* DEBUG:phiI results **********************/
    if(logger->get_level() >= Logger::EssentialInfo) {
        logger->print(Logger::Default, "PhiI_FCFS Result ------------------------------------");
        ag_support->printIntentions(ag_intention_set, ag_goal_set);
        logger->print(Logger::Default, "-----------------------------");
        ag_Sched->ev_ag_tasks_vector_to_release();
        logger->print(Logger::Default, "-----------------------------");
        ag_Sched->ev_ag_tasks_vector_ready();
        logger->print(Logger::Default, "PhiI_FCFS Ended -------------------------------------");
    }
    /***************************************************/
}

std::vector<std::shared_ptr<Task>> agent::recursiveFCFSPhiI(int index_intention, int goal_id, bool& updated_beliefset) {
    std::vector<std::shared_ptr<Task>> selectedTasks;
    std::shared_ptr<Task> task;
    std::shared_ptr<Goal> goal;
    std::vector<std::shared_ptr<Action>> intention_stack;
    std::shared_ptr<Action> top; // intention's stack top action
    std::shared_ptr<MaximumCompletionTime> MCT_main_p;
    std::vector<std::pair<std::shared_ptr<Plan>, int>> internal_goals_plans;
    std::vector<std::shared_ptr<Effect>> effects_at_begin, effects_at_end;
    std::shared_ptr<MaximumCompletionTime> MCTplan;
    std::shared_ptr<Plan> ig_plan;
    std::shared_ptr<Plan> plan_copy; // make a copy of the selected plan for this goal. Note: the action in the body are shared_ptr that are pass as reference in the new plan!

    std::string warning_msg = "";
    json post_conditions;

    int index_next_internal_goal = 0;
    int tmp_cnt; // number of beliefs update due to effects_at_begin
    int next_intention_original_plan_id;
    int tmp_counter = 0;
    double goal_arrival_time;
    bool stopPlansLoop = false;
    bool manage_internal_goal = true;

    if(index_intention < (int)ag_intention_set.size())
    {
        intention_stack = ag_intention_set[index_intention]->get_Actions();

        if(ag_intention_set[index_intention]->get_goalID() != goal_id) {
            return selectedTasks;
        }
        else if(ag_intention_set[index_intention]->get_waiting_for_completion() == true)
        { // do nothing ....
            logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                    +", Intention:["+ std::to_string(ag_intention_set[index_intention]->get_id())
            +"], plan:["+ std::to_string(ag_intention_set[index_intention]->get_planID())
            +"], original_plan:["+ std::to_string(ag_intention_set[index_intention]->get_original_planID())
            +"] waiting for completion:["+ std::to_string(ag_intention_set[index_intention]->get_scheduled_completion_time()) +"]");
            logger->print(Logger::EveryInfoPlus, " --- DEBUG --- ");
        }
        else if(ag_intention_set[index_intention]->get_Actions().size() == 0)
        {
            // No action available for this intention + NOT waiting for completion! Should never reach this point
            warning_msg = "**** (3) Intention:["+ std::to_string(ag_intention_set[index_intention]->get_id());
            warning_msg += "], plan:["+ std::to_string(ag_intention_set[index_intention]->get_planID());
            warning_msg += "], name:["+ ag_intention_set[index_intention]->get_goal_name() +"] has no more actions. AND NOT WAITINIG FOR COMPLETION! ****";
            logger->print(Logger::Warning, warning_msg);
            ag_metric->addGeneratedWarning(sim_current_time(), "recursiveFCFSPhiI", warning_msg);

            // Thus, intention completed: *********************************
            intention_has_been_completed(ag_intention_set[index_intention]->get_goalID(),
                    ag_intention_set[index_intention]->get_planID(),
                    ag_intention_set[index_intention]->get_original_planID(),
                    index_intention);
            // ************************************************************
        }
        else {
            int i = 0;
            top = intention_stack[i];

            if(top->get_type() == "task")
            {
                task = std::dynamic_pointer_cast<Task>(top);

                logger->print(Logger::EveryInfo, "+++ Task:["+ std::to_string(task->getTaskId())
                +"], plan:["+ std::to_string(task->getTaskPlanId())
                +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId())
                +"], activated:["+ ag_support->boolToString(task->getTaskisBeenActivated())
                +"], status:["+ task->getTaskStatus_as_string() +"]");

                if(task->getTaskisBeenActivated() == false)
                {   // update the Activation status:
                    task->setTaskisBeenActivated(true);

                    if(task->getTaskStatus() == Task::TSK_IDLE) {
                        task->setTaskStatus(Task::TSK_ACTIVE);
                    }

                    /* ------------------------------------------------------------------------------------
                     * update the Arrival time of the task, based on the execution instant:
                     * original: task->setTaskArrivalTime(sim_current_time().dbl() + task->getTaskArrivalTime());
                     * Use the Initial Arrival Time of the task.
                     * This allows to use the original value and avoid using "old" data that has not been "cleaned" */
                    task->setTaskArrivalTime(sim_current_time().dbl() + task->getTaskArrivalTimeInitial());

                    // update the 'firstActivation' field of the intention
                    if(ag_intention_set[index_intention]->get_firstActivation() == -1) {
                        ag_intention_set[index_intention]->set_firstActivation(sim_current_time().dbl());
                    }

                    // update the 'batch_startTime' field of the intention (if needed)
                    if(task->getTaskExecutionType() == Task::EXEC_SEQUENTIAL) {
                        ag_intention_set[i]->set_batch_startTime(sim_current_time().dbl());
                    }

                    /* convert the task from shared_pointer(std::shared_ptr<Task>) to classic pointer (std::shared_ptr<Task> );
                     * and then add it to the -to-be-released-vector */
                    selectedTasks.push_back(task);
                }
            }
            else if(top->get_type() == "goal")
            {
                goal = std::dynamic_pointer_cast<Goal>(top);
                logger->print(Logger::EveryInfo, "Internal goal: "+ goal->get_goal_name()
                        +", execution_type: "+ goal->getGoalExecutionType_as_string());

                int tmp_index = -1;
                if(ag_support->checkIfGoalIsLinkedToIntention(goal, ag_intention_set, tmp_index))
                {  // it is, perform phiI on that intention...
                    selectedTasks = recursiveFCFSPhiI(tmp_index, ag_intention_set[tmp_index]->get_goalID(), updated_beliefset);
                    goal->setGoalisBeenActivated(true);

                    logger->print(Logger::EveryInfo, "selected tasks: "+ std::to_string(selectedTasks.size()));
                    logger->print(Logger::EveryInfoPlus, " --- DEBUG --- ");
                }
                else {
                    if(goal->getGoalisBeenActivated() == false)
                    {   // 1. find the plan that satisfy the goal and that has the highest PREF value
                        index_next_internal_goal += 1;
                        MCT_main_p = ag_intention_set[index_intention]->getOriginalMainPlan();
                        internal_goals_plans = MCT_main_p->getInternalGoalsSelectedPlans();
                        stopPlansLoop = false;
                        manage_internal_goal = true;

                        /* Before managing the internal goal, we must check the "arrivalTime".
                         * - if arrivalTime = 0: move on as always and execute the 'for' loop
                         * - if arrivalTime < 0: this will never happen, negative goal.arrivalTime are set to 0 in "jsonfile_handler"
                         * - if arrivalTime > 0: activation of the goal is postponed */
                        if(fabs(goal->get_arrival_time()) > ag_Sched->EPSILON)
                        {   // the goal is not ready to be activated. It's arrivalTime is schedule for the future...
                            manage_internal_goal = false;

                            if(goal->get_absolute_arrival_time() == -1)
                            {   // the arrivalTime for the goal is not been scheduled yet...
                                goal_arrival_time = sim_current_time().dbl() + goal->get_arrival_time();
                                goal->set_absolute_arrival_time(goal_arrival_time);

                                logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                                        +", internal goal id:["+ std::to_string(goal->get_id())
                                +"], goal name:["+ goal->get_goal_name()
                                +"], plan_id:["+ std::to_string(goal->get_plan_id())
                                +"], original_plan_id:["+ std::to_string(goal->get_original_plan_id())
                                +"] is linked to intention:["+ std::to_string(i)
                                +"]\n intention original plan id:["+ std::to_string(ag_intention_set[i]->get_original_planID())
                                +"] ---> arrival time relative:["+ std::to_string(goal->get_arrival_time())
                                +"] ---> arrival time absolute:["+ std::to_string(goal->get_absolute_arrival_time()) +"]");

                                simtime_t at_time = convert_double_to_simtime_t(goal_arrival_time);
                                scheduleAt(at_time, set_internal_goal_arrival(goal, ag_intention_set[i]->get_original_planID()));
                                scheduled_msg += 1;

                                // check if the goal for which we are scheduling the arrival is already an intention...
                                if(ag_support->checkIfGoalIsLinkedToIntention(goal, ag_intention_set, index_intention))
                                {  // it is, perform phiI on that intention...
                                    selectedTasks = recursiveFCFSPhiI(index_intention, ag_intention_set[index_intention]->get_goalID(), updated_beliefset);

                                    logger->print(Logger::EveryInfo, "selected tasks: "+ std::to_string(selectedTasks.size()));
                                    logger->print(Logger::EveryInfoPlus, " --- DEBUG --- ");
                                }
                            }
                            else
                            { // the arrivalTime for the goal is already been scheduled. Ignore it...
                                logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                                        +", internal goal id:["+ std::to_string(goal->get_id())
                                +"], goal name:["+ goal->get_goal_name()
                                +"], plan_id:["+ std::to_string(goal->get_plan_id())
                                +"], original_plan_id:["+ std::to_string(goal->get_original_plan_id())
                                +"] is linked to intention:["+ std::to_string(i)
                                +"], arrival time relative:["+ std::to_string(goal->get_arrival_time())
                                +"] ALREADY SCHEDULED at absolute time t: "+ std::to_string(goal->get_absolute_arrival_time()));

                                // if the arrivalTime of the goal has been scheduled for time <= now, then we must analyze the goal...
                                if(goal->get_absolute_arrival_time() != -1 && goal->get_absolute_arrival_time() <= sim_current_time().dbl())
                                {
                                    manage_internal_goal = true;
                                }
                            }
                        }
                        else {
                            // goal.arrivaltime is set to 0. Hence, we can update the absolute arrival time and move on with the execution
                            goal_arrival_time = sim_current_time().dbl() + goal->get_arrival_time();
                            goal->set_absolute_arrival_time(goal_arrival_time);
                        }

                        if(manage_internal_goal == true)
                        {
                            logger->print(Logger::EveryInfo, std::to_string(index_next_internal_goal) +" "+ std::to_string(internal_goals_plans.size()));
                            if(index_next_internal_goal < (int)internal_goals_plans.size())
                            {
                                for(int p = 1; p < (int)internal_goals_plans.size() && stopPlansLoop == false; p++)
                                { // loop over all internal goals of the main plan searching the ones with .second = 1
                                    if(ag_support->checkIfPlanWasAlreadyIntention(internal_goals_plans[p].first, ag_intention_set))
                                    {   // internal goal is already an intention....
                                        logger->print(Logger::EveryInfo, " (PhiI_FCFS) Plan:["+ std::to_string(internal_goals_plans[p].first->get_id()) +"] is already an intention! -> ignore it...");
                                    }
                                    else
                                    {
                                        if(goal->get_goal_name() == internal_goals_plans[p].first->get_goal_name())
                                        {
                                            if(internal_goals_plans[p].second == 1)
                                            { // plans with annidation level = 1 are the actual internal goals of the main plan...
                                                if(ag_support->checkIfGoalIsInGoalset(goal, ag_goal_set) == false)
                                                {
                                                    // Added: check if internal goal plan has valid preconditions before converting it to an intention...
                                                    if(ag_support->checkIfPreconditionsAreValid(internal_goals_plans[p].first, ag_belief_set, "phiI_FCFS"))
                                                    {
                                                        goal->setActivation_time(sim_current_time().dbl());
                                                        goal->setGoalisBeenActivated(true);
                                                        effects_at_begin = internal_goals_plans[p].first->get_effects_at_begin();
                                                        effects_at_end = internal_goals_plans[p].first->get_effects_at_end();
                                                        post_conditions = internal_goals_plans[p].first->get_post_conditions();

                                                        ag_goal_set.insert(ag_goal_set.begin() + index_intention + 1, goal); // add internal goal to goal_set
                                                        ag_metric->addActivatedGoal(goal, sim_current_time().dbl(), internal_goals_plans[p].first->get_id(), internal_goals_plans[p].first->get_id(), "internal goal");

                                                        MCTplan = std::make_shared<MaximumCompletionTime>();
                                                        next_intention_original_plan_id = internal_goals_plans[p].first->get_id();
                                                        tmp_counter = 0;

                                                        MCTplan->setMCTValue(goal->get_DDL());

                                                        for(int x = p; x < (int)internal_goals_plans.size(); x++)
                                                        { // stay in here as long as the next .second is greater than 1. [*]
                                                            ig_plan = internal_goals_plans[x].first;
                                                            plan_copy = ig_plan->makeACopyOfPlan(); // make a copy of the selected plan for this goal

                                                            logger->print(Logger::EveryInfo, "plan:["+ std::to_string(plan_copy->get_id()) +"]");

                                                            // update the tasks in of the selected plan 'ig_plan' and add them to 'MCTplan'...
                                                            for(int k = 0; k < (int)plan_copy->get_body().size(); k++) {
                                                                logger->print(Logger::EveryInfoPlus, " -- Action["+ std::to_string(k)
                                                                +"] is of type: "+ plan_copy->get_body()[k]->get_type());
                                                                // if it's a task, we have to update it's t_intention_id
                                                                if(plan_copy->get_body()[k]->get_type() == "task") {
                                                                    task = std::dynamic_pointer_cast<Task>(plan_copy->get_body()[k]);
                                                                    task->setTaskPlanId(ag_intention_set[index_intention]->get_planID());
                                                                    task->setTaskOriginalPlanId(plan_copy->get_id());

                                                                    logger->print(Logger::EveryInfo, "task original plan id: "+ std::to_string(plan_copy->get_id())
                                                                    +", set to: "+ std::to_string(task->getTaskOriginalPlanId()));

                                                                    task->setTaskisBeenActivated(false);
                                                                }
                                                            }
                                                            MCTplan->add_internal_goal_selectedPlan(plan_copy, internal_goals_plans[x].second - 1);

                                                            tmp_counter += 1;
                                                        }
                                                        p += tmp_counter; // move forward the 'p' index as well, in order to be sure it's aligned and pointing to correct next tmp_plan
                                                        // --------------------------------------------------

                                                        // compute "tmp_goal" priority using the specified expression ...............................
                                                        ag_support->computeGoalPriority(goal, ag_belief_set);
                                                        // ..........................................................................................

                                                        ag_selected_plans.insert(ag_selected_plans.begin() + index_intention + 1, MCTplan);

                                                        logger->print(Logger::EssentialInfo, "A NEW INTENTION FROM GOAL:["+ std::to_string(goal->get_id()) +"], goal name:["+ goal->get_goal_name() +"]");
                                                        ag_intention_set.insert(ag_intention_set.begin() + index_intention + 1,
                                                                std::make_shared<Intention>(phiIFCFS_index, goal,
                                                                        ag_intention_set[index_intention]->get_planID(),
                                                                        MCTplan, ag_belief_set,
                                                                        effects_at_begin, effects_at_end,
                                                                        post_conditions,
                                                                        sim_current_time().dbl()
                                                                )
                                                        );

                                                        // update the 'firstActivation' field of the intention
                                                        if(ag_intention_set[i]->get_firstActivation() == -1) {
                                                            ag_intention_set[i]->set_firstActivation(sim_current_time().dbl());
                                                        }

                                                        // --------------------------------------------------------------------------
                                                        // check and apply (if present) EFFECTS_AT_BEGIN of this intention...
                                                        tmp_cnt = ag_support->updateBeliefsetDueToEffect(ag_belief_set,
                                                                ag_intention_set[ag_intention_set.size()-1],
                                                                ag_intention_set[ag_intention_set.size()-1]->get_effects_at_begin());

                                                        logger->print(Logger::EssentialInfo, " Updated beliefs: "+ std::to_string(tmp_cnt));

                                                        if(tmp_cnt > 0) {
                                                            updated_beliefset = true;
                                                        }
                                                        /* Clear list of "effects_at_begin" as soon as they have been applied.
                                                         * The reason is that if they update the beliefset,
                                                         * a reasoning cycle must be performed immediately.
                                                         * Thus, clearing the list guarantees that no reasoning cycle loops get activated */
                                                        ag_intention_set[index_intention + 1]->clear_effects_at_begin();
                                                        // --------------------------------------------------------------------------

                                                        ag_metric->addActivatedIntentions(ag_intention_set[index_intention + 1], sim_current_time().dbl());

                                                        ag_intention_set[index_intention + 1]->set_original_planID(next_intention_original_plan_id);
                                                        // ---------------------------

                                                        // used to correctly compute LATENESS whenever a plan begins with an INTERNAL GOAL
                                                        if(ag_intention_set[index_intention]->get_firstActivation() == -1) {
                                                            ag_intention_set[index_intention]->set_firstActivation(sim_current_time().dbl());

                                                            logger->print(Logger::EveryInfoPlus, "now: "+ sim_current_time().str()
                                                                    +", t_DDL: "+ std::to_string(task->getTaskDeadline())
                                                            +", intention_startTime: "+ std::to_string(ag_intention_set[index_intention]->get_startTime())
                                                            +", intention_firstActivation: "+ std::to_string(ag_intention_set[index_intention]->get_firstActivation()));
                                                            logger->print(Logger::EveryInfoPlus, "**************************************************");
                                                        }
                                                        selectedTasks = recursiveFCFSPhiI(index_intention + 1, ag_intention_set[index_intention + 1]->get_goalID(), updated_beliefset);
                                                        // ----------------------------------------------------------------------------------------
                                                    }
                                                    else {
                                                        checkAndManageIntentionFalseConditions(internal_goals_plans[p].first, ag_selected_plans, ag_goal_set,
                                                                ag_intention_set, ag_belief_set, selectedTasks);

                                                        if(index_intention >= (int)ag_intention_set.size()) {
                                                            index_intention = 0;
                                                        }

                                                        selectedTasks = recursiveFCFSPhiI(index_intention, ag_intention_set[index_intention]->get_goalID(), updated_beliefset);
                                                        stopPlansLoop = true; // we found the intention dedicated to the internal goal. There is no need to check the rest of the plans in internal_goals_plans
                                                        break;

                                                    }
                                                }
                                                else { // if the goal is already in goalset, it MUST be linked to an intention.
                                                    logger->print(Logger::EssentialInfo, "Internal goal:["+ std::to_string(goal->get_id())
                                                    +"] is already in Goal-set...");
                                                    goal->setActivation_time(sim_current_time().dbl());
                                                    goal->setGoalisBeenActivated(true);

                                                    // if already in goalset, it must be linked to an active intention
                                                    for(int j = 0; j < (int)ag_intention_set.size(); j++) {
                                                        if(goal->get_goal_name() == ag_intention_set[j]->get_goal_name())
                                                        {
                                                            selectedTasks = recursiveFCFSPhiI(j, ag_intention_set[j]->get_goalID(), updated_beliefset);
                                                            stopPlansLoop = true; // we found the intention dedicated to the internal goal. There is no need to check the rest of the plans in internal_goals_plans
                                                            break;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            // ---------------------------------------------------
                        } // end if
                    } // end if
                } // end else of statement if(goal is linked to intention)
            } // end else if
        }
    }

    return selectedTasks;
}

void agent::phiI_RR(agentMSG* agMsg)
{
    logger->print(Logger::EssentialInfo, "----phiI_RR----");

    std::shared_ptr<Task> task, next_task;
    std::shared_ptr<Goal> goal;
    std::vector<std::shared_ptr<Action>> intention_stack;
    std::shared_ptr<Action> top; // intention's stack top action
    std::shared_ptr<MaximumCompletionTime> MCTplan;
    std::shared_ptr<Plan> ig_plan;
    std::shared_ptr<Plan> plan_copy; // make a copy of the selected plan for this goal. Note: the action in the body are shared_ptr that are pass as reference in the new plan!

    /* vector of pairs, one for each intention.
     * - Bool is used to tell phiI if for that intention we are in a PAR block or not
     * - Int refers to the next task to analyze
     * - Int index of the next internal goal */
    std::vector<std::tuple<bool, int, int>> checkNextActionIntention_ith(ag_intention_set.size(), std::tuple<bool, int, int>(true, 0, 0));
    std::shared_ptr<MaximumCompletionTime> MCT_main_p;
    std::vector<std::pair<std::shared_ptr<Plan>, int>> internal_goals_plans;
    std::vector<std::shared_ptr<Effect>> effects_at_begin, effects_at_end;

    std::string warning_msg = "";
    json post_conditions;

    bool get_next_internal_goal = false;
    bool manage_internal_goal = true;
    bool checkIntentions = true; // used to tell phiI that there is at least one more intention that has a PAR task to analyze
    bool updated_beliefset = false; // if at least one effects_at_begin changes the beliefset, updated_beliefset = true and a new reasoning cycle must be scheduled immediately
    int tmp_cnt; // number of beliefs update due to effects_at_begin
    int action_index = 0;
    int index_next_internal_goal = 0;
    int next_intention_original_plan_id;
    int tmp_counter = 0;
    double goal_arrival_time;

    tasks_to_be_release_phiI.clear(); // always clear vector before using it
    // -------------------------------------------------------------------

    while(checkIntentions)
    {
        checkIntentions = false;

        //  1. Select the 'top' action from the stack of each intention.
        for(int i = 0; i < (int)ag_intention_set.size(); i++)
        {
            if(ag_intention_set[i]->get_waiting_for_completion() == true)
            { // do nothing ....
                logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                        +", Intention:["+ std::to_string(ag_intention_set[i]->get_id())
                +"], plan:["+ std::to_string(ag_intention_set[i]->get_planID())
                +"], original_plan:["+ std::to_string(ag_intention_set[i]->get_original_planID())
                +"] waiting for completion:["+ std::to_string(ag_intention_set[i]->get_scheduled_completion_time()) +"]");
                logger->print(Logger::EveryInfoPlus, " --- DEBUG --- ");
            }
            else if(ag_intention_set[i]->get_Actions().size() == 0)
            {
                // No action available for this intention + NOT waiting for completion! Should never reach this point
                warning_msg = "**** (4) Intention:["+ std::to_string(ag_intention_set[i]->get_id());
                warning_msg += "], plan:["+ std::to_string(ag_intention_set[i]->get_planID());
                warning_msg += "], name:["+ ag_intention_set[i]->get_goal_name() +"] has no more actions. AND NOT WAITINIG FOR COMPLETION! ****";
                logger->print(Logger::Warning, warning_msg);
                ag_metric->addGeneratedWarning(sim_current_time(), "phiI_RR", warning_msg);

                // Thus, intention completed: *********************************
                intention_has_been_completed(ag_intention_set[i]->get_goalID(), ag_intention_set[i]->get_planID(), ag_intention_set[i]->get_original_planID(), i);
                checkNextActionIntention_ith.erase(checkNextActionIntention_ith.begin() + i);
                i = -1; // we do not know how many and which intentions have been removed, we must restart the loop from 0
                // ************************************************************
            }
            else if(i < (int)checkNextActionIntention_ith.size())
            {
                action_index = std::get<1>(checkNextActionIntention_ith[i]);
                intention_stack = ag_intention_set[i]->get_Actions();
                if(action_index < (int)intention_stack.size())
                {
                    top = intention_stack[action_index];

                    if(std::get<0>(checkNextActionIntention_ith[i]))
                    {
                        // 2. if it's a task, add it to the -to-be-released-vector:
                        if(top->get_type() == "task") {
                            task = std::dynamic_pointer_cast<Task>(top);

                            if(action_index + 1 < (int)intention_stack.size()) {
                                if(intention_stack[action_index + 1]->get_type() == "task") {
                                    next_task = std::dynamic_pointer_cast<Task>(intention_stack[action_index + 1]);
                                    if(next_task->getTaskExecutionType() == Task::EXEC_PARALLEL) {
                                        std::get<0>(checkNextActionIntention_ith[i]) = true;
                                    }
                                    else {
                                        std::get<0>(checkNextActionIntention_ith[i]) = false;
                                    }
                                    std::get<1>(checkNextActionIntention_ith[i]) = action_index + 1;
                                    checkIntentions = true;
                                }
                                else if(intention_stack[action_index + 1]->get_type() == "goal") {
                                    goal = std::dynamic_pointer_cast<Goal>(intention_stack[action_index + 1]);
                                    if(goal->getGoalExecutionType() == Goal::EXEC_PARALLEL) {
                                        std::get<0>(checkNextActionIntention_ith[i]) = true;
                                    }
                                    else {
                                        std::get<0>(checkNextActionIntention_ith[i]) = false;
                                    }
                                    std::get<1>(checkNextActionIntention_ith[i]) = action_index + 1;
                                    checkIntentions = true;
                                }
                            }

                            logger->print(Logger::EveryInfo, "+++ Task:["+ std::to_string(task->getTaskId())
                            +"], plan:["+ std::to_string(task->getTaskPlanId())
                            +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId())
                            +"], activated:["+ ag_support->boolToString(task->getTaskisBeenActivated())
                            +"], status:["+ task->getTaskStatus_as_string() +"]");

                            if(task->getTaskisBeenActivated() == false)
                            {
                                task->setTaskisBeenActivated(true);

                                /* update the status of the task only if it has arrivalTime 0.
                                 * This allows to go directly to the execution of the task by calling 'else if(fabs...)' in the schedule_schedule_taskset_rt function.
                                 * For those task with arrivalTime != 0, we have to schedule their arrival at the correct time,
                                 *  we can't execute the task directly.And we have to leave their status=IDLE */
                                if(ag_support->checkIfDoublesAreEqual(task->getTaskArrivalTime(), 0)) {
                                    task->setTaskStatus(Task::TSK_ACTIVE);
                                }

                                /* ------------------------------------------------------------------------------------
                                 * update the Arrival time of the task, based on the execution instant:
                                 * original: task->setTaskArrivalTime(sim_current_time().dbl() + task->getTaskArrivalTime());
                                 * Use the Initial Arrival Time of the task.
                                 * This allows to use the original value and avoid using "old" data that has not been "cleaned" */
                                task->setTaskArrivalTime(sim_current_time().dbl() + task->getTaskArrivalTimeInitial());
                                // ------------------------------------------------------------------------------------

                                // update the 'firstActivation' field of the intention
                                if(ag_intention_set[i]->get_firstActivation() == -1) {
                                    ag_intention_set[i]->set_firstActivation(sim_current_time().dbl());
                                }

                                // update the 'batch_startTime' field of the intention (if needed)
                                if(task->getTaskExecutionType() == Task::EXEC_SEQUENTIAL) {
                                    ag_intention_set[i]->set_batch_startTime(sim_current_time().dbl());
                                }

                                tasks_to_be_release_phiI.push_back(task);
                            }
                        }
                        else if(top->get_type() == "goal")
                        {   // We have to find the best plan able to achieve it and replace the internal goal with plan.body.
                            // Execution enters this 'if' when we have a Task:SEQ just before an internal goal
                            if((int)ag_intention_set[i]->get_Actions().size() > action_index + 1)
                            { // after the internal goal there is at least another action...
                                if(ag_intention_set[i]->get_Actions()[action_index + 1]->get_type() == "task")
                                {
                                    task = std::dynamic_pointer_cast<Task>(ag_intention_set[i]->get_Actions()[action_index + 1]);
                                    std::get<0>(checkNextActionIntention_ith[i]) = (task->getTaskExecutionType() == Task::EXEC_PARALLEL);
                                    std::get<1>(checkNextActionIntention_ith[i]) = action_index + 1;
                                    checkIntentions = true;
                                }
                                else if(ag_intention_set[i]->get_Actions()[action_index + 1]->get_type() == "goal") {
                                    goal = std::dynamic_pointer_cast<Goal>(ag_intention_set[i]->get_Actions()[action_index + 1]);
                                    std::get<0>(checkNextActionIntention_ith[i]) = (goal->getGoalExecutionType() == Goal::EXEC_PARALLEL);
                                    std::get<1>(checkNextActionIntention_ith[i]) = action_index + 1;
                                    checkIntentions = true;
                                }
                            }

                            goal = std::dynamic_pointer_cast<Goal>(top);
                            logger->print(Logger::EveryInfo, "Internal goal: "+ goal->get_goal_name()
                                    +", execution_type: "+ goal->getGoalExecutionType_as_string());

                            if(goal->getGoalisBeenActivated() == false)
                            {
                                // 1. find the plan that satisfy the goal and that has the highest PREF value
                                index_next_internal_goal = std::get<2>(checkNextActionIntention_ith[i]) + 1;
                                MCT_main_p = ag_intention_set[i]->getOriginalMainPlan();
                                internal_goals_plans = MCT_main_p->getInternalGoalsSelectedPlans();
                                get_next_internal_goal = false;

                                manage_internal_goal = true;

                                /* Before managing the internal goal, we must check the "arrivalTime".
                                 * - if arrivalTime = 0: move on as always and execute the 'for' loop
                                 * - if arrivalTime < 0: this will never happen, negative goal.arrivalTime are set to 0 in "jsonfile_handler"
                                 * - if arrivalTime > 0: activation of the goal is postponed */
                                if(fabs(goal->get_arrival_time()) > ag_Sched->EPSILON)
                                {   // the goal is not ready to be activated. It's arrivalTime is schedule for the future...
                                    manage_internal_goal = false;

                                    if(goal->get_absolute_arrival_time() == -1)
                                    {   // the arrivalTime for the goal is not been scheduled yet...
                                        goal_arrival_time = sim_current_time().dbl() + goal->get_arrival_time();
                                        goal->set_absolute_arrival_time(goal_arrival_time);

                                        logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                                                +", internal goal id:["+ std::to_string(goal->get_id())
                                        +"], goal name:["+ goal->get_goal_name()
                                        +"], plan_id:["+ std::to_string(goal->get_plan_id())
                                        +"], original_plan_id:["+ std::to_string(goal->get_original_plan_id())
                                        +"] is linked to intention:["+ std::to_string(i)
                                        +"]\n intention original plan id:["+ std::to_string(ag_intention_set[i]->get_original_planID())
                                        +"] ---> arrival time relative:["+ std::to_string(goal->get_arrival_time())
                                        +"] ---> arrival time absolute:["+ std::to_string(goal->get_absolute_arrival_time()) +"]");

                                        simtime_t at_time = convert_double_to_simtime_t(goal_arrival_time);
                                        scheduleAt(at_time, set_internal_goal_arrival(goal, ag_intention_set[i]->get_original_planID()));
                                        scheduled_msg += 1;
                                    }
                                    else
                                    { // the arrivalTime for the goal is already been scheduled. Ignore it...
                                        logger->print(Logger::EssentialInfo, "t: "+ sim_current_time().str()
                                                +", internal goal id:["+ std::to_string(goal->get_id())
                                        +"], goal name:["+ goal->get_goal_name()
                                        +"], plan_id:["+ std::to_string(goal->get_plan_id())
                                        +"], original_plan_id:["+ std::to_string(goal->get_original_plan_id())
                                        +"] is linked to intention:["+ std::to_string(i)
                                        +"], arrival time relative:["+ std::to_string(goal->get_arrival_time())
                                        +"] ALREADY SCHEDULED at absolute time t: "+ std::to_string(goal->get_absolute_arrival_time()));

                                        // if the arrivalTime of the goal has been scheduled for time <= now, then we must analyze the goal...
                                        if(goal->get_absolute_arrival_time() != -1 && goal->get_absolute_arrival_time() <= sim_current_time().dbl())
                                        {
                                            manage_internal_goal = true;
                                        }
                                    }
                                }
                                else {
                                    // goal.arrivaltime is set to 0. Hence, we can update the absolute arrival time and move on with the execution
                                    goal_arrival_time = sim_current_time().dbl() + goal->get_arrival_time();
                                    goal->set_absolute_arrival_time(goal_arrival_time);
                                }

                                if(manage_internal_goal == true)
                                {
                                    logger->print(Logger::EveryInfo, "check indexes: "+ std::to_string(index_next_internal_goal) +" "+ std::to_string(internal_goals_plans.size()));
                                    for(int p = 1; p < (int)internal_goals_plans.size() && get_next_internal_goal == false; p++)
                                    { // loop over all internal goals of the main plan
                                        if(ag_support->checkIfPlanWasAlreadyIntention(internal_goals_plans[p].first, ag_intention_set))
                                        {   // internal goal is already an intention....
                                            goal->setGoalisBeenActivated(true);
                                            logger->print(Logger::EveryInfo, " (PhiI_RR) Plan:["+ std::to_string(internal_goals_plans[p].first->get_id()) +"] is already an intention! -> ignore it...");
                                        }
                                        else
                                        {
                                            logger->print(Logger::EveryInfo, "check names: "+ internal_goals_plans[p].first->get_goal_name() +" == "+ goal->get_goal_name());
                                            if(goal->get_goal_name() == internal_goals_plans[p].first->get_goal_name())
                                            {
                                                if(internal_goals_plans[p].second == 1)
                                                { // plans with annidation level = 1 are the actual internal goals of the main plan...
                                                    if(ag_support->checkIfGoalIsInGoalset(goal, ag_goal_set) == false)
                                                    {
                                                        // Added: check if internal goal plan has valid preconditions before converting it to an intention...
                                                        if(ag_support->checkIfPreconditionsAreValid(internal_goals_plans[p].first, ag_belief_set, "phiI_RR"))
                                                        {
                                                            goal->setActivation_time(sim_current_time().dbl());
                                                            goal->setGoalisBeenActivated(true);
                                                            effects_at_begin = internal_goals_plans[p].first->get_effects_at_begin();
                                                            effects_at_end = internal_goals_plans[p].first->get_effects_at_end();
                                                            post_conditions = internal_goals_plans[p].first->get_post_conditions();

                                                            ag_goal_set.push_back(goal); // add internal goal to goal_set
                                                            ag_metric->addActivatedGoal(goal, sim_current_time().dbl(), internal_goals_plans[p].first->get_id(), internal_goals_plans[p].first->get_id(), "internal goal");

                                                            MCTplan = std::make_shared<MaximumCompletionTime>();
                                                            tmp_counter = 0;

                                                            MCTplan->setMCTValue(goal->get_DDL());

                                                            for(int x = p; x < (int)internal_goals_plans.size(); x++)
                                                            { // stay in here as long as the next .second is greater than 1. [*]
                                                                ig_plan = internal_goals_plans[x].first;
                                                                plan_copy = ig_plan->makeACopyOfPlan(); // make a copy of the selected plan for this goal

                                                                if(x == p) {
                                                                    logger->print(Logger::Default, "New internal goal: chosen plan:["+ std::to_string(plan_copy->get_id()) +"]");
                                                                    ag_intention_set[i]->push_internal_goal(goal, plan_copy->get_id(), plan_copy->get_body().size());
                                                                    next_intention_original_plan_id = plan_copy->get_id();
                                                                }

                                                                // internal_goal_plan_valid_conditions = true;
                                                                plan_copy = internal_goals_plans[x].first;
                                                                logger->print(Logger::EveryInfo, "plan:["+ std::to_string(plan_copy->get_id()) +"]");

                                                                // update the tasks in of the selected plan 'ig_plan' and add them to 'MCTplan'...
                                                                for(int k = 0; k < (int)plan_copy->get_body().size(); k++) {
                                                                    logger->print(Logger::EveryInfoPlus, " -- Action["+ std::to_string(k)
                                                                    +"] is of type: "+ plan_copy->get_body()[k]->get_type());

                                                                    // if it's a task, we have to update it's t_intention_id
                                                                    if((plan_copy->get_body()[k])->get_type() == "task") {
                                                                        task = std::dynamic_pointer_cast<Task>(plan_copy->get_body()[k]);
                                                                        task->setTaskPlanId(ag_intention_set[i]->get_planID());
                                                                        task->setTaskOriginalPlanId(plan_copy->get_id());
                                                                        logger->print(Logger::EveryInfo, "task original plan id: "+ std::to_string(plan_copy->get_id())
                                                                        +", set to: "+ std::to_string(task->getTaskOriginalPlanId()));

                                                                        task->setTaskisBeenActivated(false);
                                                                    }
                                                                }
                                                                MCTplan->add_internal_goal_selectedPlan(plan_copy, internal_goals_plans[x].second - 1);
                                                                tmp_counter += 1;

                                                                // if the next plan has .second = 1, it means that it's an internal goal of the main plan
                                                                if(x + 1 < (int)internal_goals_plans.size() && internal_goals_plans[x + 1].second == 1) {
                                                                    get_next_internal_goal = true;
                                                                    break;
                                                                }
                                                            }
                                                            p += tmp_counter; // move forward the 'p' index as well, in order to be sure it's aligned and pointing to correct next tmp_plan
                                                            // --------------------------------------------------

                                                            // create a new intention for the internal goal, using the selected (and valid) plan 'ig_plan'...
                                                            int new_intention_id = 0;
                                                            if(ag_intention_set.size() > 0) {
                                                                new_intention_id = ag_intention_set[ag_intention_set.size() - 1]->get_id() + 1; // id of the last store intention + 1
                                                            }

                                                            // compute "goal" priority using the specified expression ...............................
                                                            ag_support->computeGoalPriority(goal, ag_belief_set);
                                                            // ..........................................................................................

                                                            logger->print(Logger::EveryInfo, "print sizes: "+ std::to_string(ag_intention_set.size())
                                                            +", "+ std::to_string(ag_intention_set[ag_intention_set.size() - 1]->getInternalGoalsSelectedPlansSize()));

                                                            ag_selected_plans.push_back(MCTplan);

                                                            logger->print(Logger::EssentialInfo, "A NEW INTENTION FROM GOAL:["+ std::to_string(goal->get_id())
                                                            +"], goal name:["+ goal->get_goal_name() +"]");
                                                            ag_intention_set.push_back(
                                                                    std::make_shared<Intention>(new_intention_id,
                                                                            goal, ag_intention_set[i]->get_planID(),
                                                                            MCTplan, ag_belief_set,
                                                                            effects_at_begin, effects_at_end,
                                                                            post_conditions,
                                                                            sim_current_time().dbl()
                                                                    )
                                                            );

                                                            // update the 'firstActivation' field of the intention
                                                            if(ag_intention_set[i]->get_firstActivation() == -1) {
                                                                ag_intention_set[i]->set_firstActivation(sim_current_time().dbl());
                                                            }

                                                            // --------------------------------------------------------------------------
                                                            // check and apply (if present) EFFECTS_AT_BEGIN of this intention...
                                                            tmp_cnt = ag_support->updateBeliefsetDueToEffect(ag_belief_set,
                                                                    ag_intention_set[ag_intention_set.size()-1],
                                                                    ag_intention_set[ag_intention_set.size()-1]->get_effects_at_begin());

                                                            logger->print(Logger::EssentialInfo, " Updated beliefs: "+ std::to_string(tmp_cnt));

                                                            if(tmp_cnt > 0) {
                                                                updated_beliefset = true;
                                                            }
                                                            /* Clear list of "effects_at_begin" as soon as they have been applied.
                                                             * The reason is that if they update the beliefset,
                                                             * a reasoning cycle must be performed immediately.
                                                             * Thus, clearing the list guarantees that no reasoning cycle loops get activated */
                                                            ag_intention_set[ag_intention_set.size()-1]->clear_effects_at_begin();
                                                            // --------------------------------------------------------------------------

                                                            ag_metric->addActivatedIntentions(ag_intention_set[ag_intention_set.size() - 1], sim_current_time().dbl());

                                                            logger->print(Logger::EveryInfo, "original: "+ std::to_string(ag_intention_set[ag_intention_set.size() - 1]->get_original_planID())
                                                            + ", will be update to: "+ std::to_string(next_intention_original_plan_id));

                                                            ag_intention_set[ag_intention_set.size() - 1]->set_original_planID(next_intention_original_plan_id);

                                                            std::get<2>(checkNextActionIntention_ith[i]) = index_next_internal_goal;
                                                            logger->print(Logger::EveryInfo, "index_next_internal_goal: "+ std::to_string(index_next_internal_goal));
                                                            checkNextActionIntention_ith.push_back(std::tuple<bool, int, int>(true, 0, 0));
                                                        }
                                                        else {
                                                            checkAndManageIntentionFalseConditions_RR(internal_goals_plans[p].first, ag_selected_plans, ag_goal_set, ag_intention_set, ag_belief_set, tasks_to_be_release_phiI, checkNextActionIntention_ith);
                                                            i = -1;
                                                            get_next_internal_goal = false;
                                                            break;
                                                        }
                                                    }
                                                    else {
                                                        logger->print(Logger::EssentialInfo, "Internal goal:["+ std::to_string(goal->get_id()) +"] is already in Goalset...");
                                                        goal->setActivation_time(sim_current_time().dbl());
                                                        goal->setGoalisBeenActivated(true);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else {
                            warning_msg = "[WARNING] Intention:[plan_id="+ std::to_string(ag_intention_set[i]->get_planID()) +"] contains invalid action(s)!";
                            logger->print(Logger::Warning, warning_msg);
                            ag_metric->addGeneratedWarning(sim_current_time(), "phiI_RR", warning_msg);
                        }
                    }
                }
            } // end else
        }
    }

    // copy the "tasks_to_be_release_phiI" to the actual release queue:
    for(int i = 0; i < (int)tasks_to_be_release_phiI.size(); i++) {
        ag_Sched->ag_add_task_in_vector_to_release(tasks_to_be_release_phiI[i]->cloneTask()); // added clone to manage shared_ptr
    }

    // update statistics....
    ag_metric->addPhiIStats(ag_goal_set.size(), ag_intention_set.size(), sim_current_time());

    // Note: if all active intentions get removed due to invalid conditions, force a new reasoning cycle
    if(updated_beliefset || ag_intention_set.size() == 0)
    { // Immediately repeat a new reasoning cycle to deal with updated beliefset (due to effects)
        scheduleAt(sim_current_time(), set_reasoning_cycle(true));
        scheduled_msg += 1;
        ag_support->setScheduleInFuture("PHII_RR - repeat reasoning cycle due to 'effects_begin'"); // METRICS purposes

        logger->print(Logger::EssentialInfo, "->-> BELIEFSET CHANGED DUE TO EFFECTS or REMOVED INTENTIONs -> SCHEDULE NEW REASONING CYCLE... <-<-");
    }
    else {
        // manage the case when the head of the ready queue has changed, and the new top element is in "READY" state instead of "EXEC"
        if(ag_support->manageReadyQueue(ag_Sched)) {
            scheduleAt(sim_current_time(), set_task_activation());
            scheduled_msg += 1;
        }

        // Once we have setup the -to-be-released-vector, schedule the execution of the tasks
        scheduleAt(sim_current_time(), set_ag_scheduler());
        scheduled_msg += 1;
        ag_support->setScheduleInFuture("SCHEDULE - phiI RR"); // METRICS purposes
    }

    /********* DEBUG:phiI results **********************/
    if(logger->get_level() >= Logger::EssentialInfo) {
        logger->print(Logger::Default, "PhiI_RR Result ------------------------------------");
        ag_support->printIntentions(ag_intention_set, ag_goal_set);
        logger->print(Logger::Default, "-----------------------------");
        ag_Sched->ev_ag_tasks_vector_to_release();
        logger->print(Logger::Default, "-----------------------------");
        ag_Sched->ev_ag_tasks_vector_ready();
        logger->print(Logger::Default, "PhiI_RR Ended -------------------------------------");
    }
}

// When the last task of an intention gets completed, this function is called
int agent::intention_has_been_completed(int intention_goal_id, int intention_plan_id,
        int intention_original_plan_id,
        int index_intention,
        int nested_level, bool scheduleNewReasoningCycle)
{
    logger->print(Logger::EssentialInfo, "----intention_has_been_completed----");
    ag_support->setSimulationLastExecution("intention_has_been_completed");
    /* If there are no more actions to be performed for the selected intention:
     * 1. the plan is completed, remove it from selected plans
     * 2. remove the goal from the goal set
     * 3. remove it from the intention set
     * 4. update belief
     * 5. check if there are sensor's reading associated to intention's plan
     * 6. check if the update has triggered desires to goal conversion */

    std::vector<std::shared_ptr<Task>> empty_release_queue(0); // declared only because required by function "checkAndManageIntentionFalseConditions"
    std::shared_ptr<Intention> achieved_intention;
    std::vector<std::shared_ptr<Action>> actions;
    std::vector<std::shared_ptr<Intention>> intention_set_copy;
    // used with post_conditions: contains the list of intentions that contain the achieved intention as active internal goal
    std::vector<std::shared_ptr<Intention>> affected_intentions;
    std::shared_ptr<Goal> achievedGoal, tmp_goal;
    std::variant<int, double, bool, std::string> tmp_variant;
    int tmp_plan_id = -1;
    int satisfiedIntentions = 0;
    bool valid_post_conditions = false;

    std::string plan_goal_name; // it's the belief after the plan has been successfully completed
    std::string report_value = "";
    std::string msg_false_post_cond = "";

    // step 1: -----------------------------------------------------------
    logger->print(Logger::EveryInfo, " - Step 1: Remove Selected Plan for this intention...");
    int tmp_plan_p = 0;
    for(int i = 0; i < (int)ag_selected_plans.size(); i++) {
        if(ag_selected_plans[i]->getInternalGoalsSelectedPlans()[tmp_plan_p].first->get_id() == intention_original_plan_id) {
            logger->print(Logger::EveryInfoPlus, " -->> plan goal index: "+ std::to_string(i));

            plan_goal_name = ag_selected_plans[i]->getInternalGoalsSelectedPlans()[tmp_plan_p].first->get_goal_name();
            tmp_plan_id = ag_selected_plans[i]->getInternalGoalsSelectedPlans()[tmp_plan_p].first->get_id();

            ag_selected_plans.erase(ag_selected_plans.begin() + i);
            i -= 1;
            break;
        }
    }
    // -------------------------------------------------------------------

    // step 2: ------------------------------------------------------------
    if(plan_goal_name.empty() == false)
    {
        logger->print(Logger::EveryInfo, " - Intention goal_name:["+ plan_goal_name +"]");
        logger->print(Logger::EveryInfo, " - Step 2: Remove Goal...");

        for(int i = 0; i < (int)ag_goal_set.size(); i++) {
            logger->print(Logger::EveryInfoPlus, " -> Goal:["+ std::to_string(ag_goal_set[i]->get_id())
            +"] goal name = "+ ag_goal_set[i]->get_goal_name()
            +", compare to: "+ plan_goal_name +"?");

            if(ag_goal_set[i]->get_goal_name() == plan_goal_name) {
                achievedGoal = ag_goal_set[i];
                intention_goal_id = achievedGoal->get_id();
                ag_metric->setActivatedGoalCompletion(ag_goal_set[i], sim_current_time().dbl(), "Intention completed!");
                ag_goal_set.erase(ag_goal_set.begin() + i);
                break;
            }
        }
        logger->print(Logger::EveryInfo, " Achieved -> Goal:["+ std::to_string(achievedGoal->get_id())
        +"] goal name = "+ achievedGoal->get_goal_name());

        // step 3: ------------------------------------------------------------
        /* index_intention can not always be used directly!
         * When this function is called from "wrapper_intention_has_been_completed",
         * it may happen that ag_intention_set has changed between when the intention was set to waiting_for_completion
         * and when this function was called */
        if(index_intention == -1) {
            for(int i = 0; i < (int)ag_intention_set.size(); i++) {
                if(ag_intention_set[i]->get_goalID() == intention_goal_id
                        && ag_intention_set[i]->get_original_planID() == intention_original_plan_id
                        && ag_intention_set[i]->get_goal_name() == plan_goal_name)
                {
                    index_intention = i;
                    break;
                }
            }
        }

        if(index_intention != -1) {
            logger->print(Logger::EssentialInfo, " - Step 3: Remove Intention: index["+ std::to_string(ag_intention_set[index_intention]->get_id())
            +"], goal name:["+ ag_intention_set[index_intention]->get_goal_name()
            +"], plan_id["+ std::to_string(ag_intention_set[index_intention]->get_planID())
            +"], original_plan_id["+ std::to_string(ag_intention_set[index_intention]->get_original_planID()) +"]");

            achieved_intention = std::make_shared<Intention>(*ag_intention_set[index_intention]);
            affected_intentions.push_back(std::make_shared<Intention>(*ag_intention_set[index_intention])); // debug
            ag_metric->setActivatedIntentionCompletion(ag_intention_set[index_intention], sim_current_time().dbl(), "intention_has_been_completed");
            ag_metric->addGoalCompleted(ag_intention_set[index_intention], sim_current_time().dbl(), "intention_has_been_completed");
            ag_intention_set.erase(ag_intention_set.begin() + index_intention);
            satisfiedIntentions += 1;
        }
        // -------------------------------------------------------------------
    }

    // step 4: ------------------------------------
    if(achieved_intention != nullptr)
    {
        if(logger->get_level() >= Logger::EveryInfoPlus)
        {
            ag_support->printBeliefset(ag_belief_set);
            ag_support->printBeliefset(achieved_intention->get_expression_constants(), " List of Constants [tot = "+ std::to_string(achieved_intention->get_expression_constants().size()) +"]:");
            ag_support->printBeliefset(achieved_intention->get_expression_variables(), " List of Variables [tot = "+ std::to_string(achieved_intention->get_expression_variables().size()) +"]:");
            logger->print(Logger::Default, " ******************************** ");
        }

        logger->print(Logger::EssentialInfo, " Apply Effects_at_end of Intention:["+ std::to_string(achieved_intention->get_id())
        +"], from goal:["+ std::to_string(achieved_intention->get_goalID())
        +"], name:["+ achieved_intention->get_goal_name()
        +"], plan_id:["+ std::to_string(achieved_intention->get_planID())
        +"], original_plan_id:["+ std::to_string(achieved_intention->get_original_planID()) +"]");

        // apply (if present) EFFECTS_AT_END of this intention...
        ag_support->updateBeliefsetDueToEffect(ag_belief_set, achieved_intention,
                achieved_intention->get_effects_at_end());

        if(logger->get_level() >= Logger::EveryInfoPlus) { // DEBUG
            ag_support->printBeliefset(ag_belief_set);
            logger->print(Logger::Default, " ******************************** ");
        }
    }
    // -------------------------------------------

    // step 5 check if post_conditions are valid: --------------------
    if(achieved_intention != nullptr)
    {
        valid_post_conditions = ag_support->checkIfPostconditionsAreValid(achieved_intention, ag_belief_set,
                achieved_intention->get_expression_constants(), achieved_intention->get_expression_variables());

        if(valid_post_conditions == false)
        {
            completed_intention_invalid_postconditions = true;
            // compute the reason why the post conditions failed
            msg_false_post_cond = ag_support->dump_expression(achieved_intention->get_post_conditions(), ag_belief_set, achieved_intention->get_expression_constants(), achieved_intention->get_expression_variables());
            // store the invalid post condition (that will be then sent to the client)
            goals_with_invalid_post_cond.push_back(std::make_pair(achieved_intention->get_goal_name(), msg_false_post_cond));

            checkAndManageIntentionFalseConditions(achieved_intention->getTopInternalSelectedPlan(), ag_selected_plans,
                    ag_goal_set, ag_intention_set, ag_belief_set, empty_release_queue);
        }
    }
    else {
        logger->print(Logger::Error, "SOMETHING WRON: achieved_intention is NULL!");
        ag_metric->addGeneratedError(sim_current_time(), "intention_has_been_completed", "SOMETHING WRON: achieved_intention is NULL!");
    }
    // -------------------------------------------------------------------

    // step 6: -----------------------------------------------------------
    if(valid_post_conditions)
    {
        // Add the goal to the list of completed ones (data for the client)
        main_goals_completed.push_back(ag_intention_set[index_intention]->get_goal_name());

        // check if this intention was derived from an internal goal
        logger->print(Logger::EveryInfo, " - Step 4: tmp_plan_id: "+ std::to_string(tmp_plan_id));

        if(achievedGoal == nullptr) {
            logger->print(Logger::Error, "achievedGoal is null!");
            ag_metric->addGeneratedError(sim_current_time(), "intention_has_been_completed", "achievedGoal is null!");
        }

        for(int i = 0; i < (int)ag_intention_set.size(); i++)
        {   // loop over all actions, check if the completed intention was an internal goal:
            ag_support->checkAndManageIntentionDerivedFromIG(tmp_plan_id, tmp_plan_p, ag_intention_set[i]->get_goalID(), ag_belief_set, achievedGoal, ag_intention_set[i]);

            // if some intention has been completed, repeat this function
            if((ag_intention_set[i]->get_mainPlanActions().size() == 0 // used by EDF
                    || ag_intention_set[i]->get_Actions().size() == 0)
                    && ag_intention_set[i]->get_waiting_for_completion() == false)
            {
                logger->print(Logger::EssentialInfo, "**** Intention:["+ std::to_string(ag_intention_set[i]->get_id())
                +"], goal:["+ ag_intention_set[i]->get_goal_name()
                +"], plan:["+ std::to_string(ag_intention_set[i]->get_planID())
                +"], original_plan:["+ std::to_string(ag_intention_set[i]->get_original_planID()) +"] has no more actions. ****");

                affected_intentions.push_back(std::make_shared<Intention>(*ag_intention_set[i])); // debug

                // Thus, intention completed:
                // Note: call "intention_has_been_completed" but DO NOT schedule reasoning cycle again
                intention_has_been_completed(ag_intention_set[i]->get_goalID(), ag_intention_set[i]->get_planID(), ag_intention_set[i]->get_original_planID(), i, nested_level + 1, false);
                i = -1; // we do not know how many and which intentions have been removed, we must restart the loop from 0
            }
        }
    }

    // step 7: -----------------------------------------------------------
    if(scheduleNewReasoningCycle) {
        logger->print(Logger::EssentialInfo, " - Scheduled new reasoning cycle at time: "+ sim_current_time().str());
        scheduleAt(sim_current_time(), set_reasoning_cycle());
        scheduled_msg += 1;
        ag_support->setScheduleInFuture("AG_REASONING_CYCLE - intention completed"); // METRICS purposes
    }
    // -------------------------------------------------------------------

    // DEBUG: ---------------------
    if(nested_level == 0) {
        ag_support->printCompletedIntentions(affected_intentions, ag_goal_set);
        logger->print(Logger::Default, "-----------------------------");
    }
    ag_support->printIntentions(ag_intention_set, ag_goal_set);
    logger->print(Logger::Default, "-----------------------------");
    ag_Sched->ev_ag_tasks_vector_to_release();
    logger->print(Logger::Default, "-----------------------------");
    ag_Sched->ev_ag_tasks_vector_ready();
    logger->print(Logger::Default, "-----------------------------");
    // ----------------------------------

    return satisfiedIntentions;
}

void agent::wrapper_intention_has_been_completed(agentMSG* msg) {
    logger->print(Logger::Default, "----wrapper_intention_has_been_completed----");

    logger->print(Logger::EssentialInfo, "**** t: "+ sim_current_time().str()
            +", Intention:["+ std::to_string(msg->getAg_intention_goal_id())
    +"], from goal id:["+ std::to_string(msg->getAg_intention_plan_id())
    +"], name:["+ msg->getAg_intention_goal_name()
    +"], plan_id:["+ std::to_string(msg->getAg_intention_plan_id())
    +"], original_plan_id:["+ std::to_string(msg->getAg_intention_original_plan_id())
    +"] has no more actions. ****");

    if(logger->get_level() >= Logger::EveryInfo) {
        logger->print(Logger::EssentialInfo, " List of intention before appling completion procedure...");
        ag_support->printIntentions(ag_intention_set, ag_goal_set);
    }

    intention_has_been_completed(msg->getAg_intention_goal_id(),
            msg->getAg_intention_plan_id(),
            msg->getAg_intention_original_plan_id(),
            -1, // meaning that intention_has_been_completed needs to compute index_intention
            0, // this intention has not been completed due to another one (it is not a sub-plan)
            msg->getAg_scheduleNewReasoningCycle());

    logger->print(Logger::Default, "----ended wrapper_intention_has_been_completed----");
}

bool agent::checkAndManageIntentionFalseConditions(std::shared_ptr<Plan> plan,
        std::vector<std::shared_ptr<MaximumCompletionTime>>& selected_plans,
        std::vector<std::shared_ptr<Goal>>& goal_set,
        std::vector<std::shared_ptr<Intention>>& intention_set,
        const std::vector<std::shared_ptr<Belief>>& belief_set,
        std::vector<std::shared_ptr<Task>>& queue)
{ // plan has either invalid preconditions, cont_conditions or post_conditions

    // use to know which intentions contain "plan" has active internal goal. Structure: <intention, ID>
    std::vector<std::pair<std::shared_ptr<Intention>, int>> intentions_false;
    std::vector<std::shared_ptr<Action>> actions;
    std::shared_ptr<Goal> goal;
    std::shared_ptr<Task> task;
    std::string warning_msg = "";
    bool checkNextAction = true;
    int plan_id = -1;

    // loop over all intentions and check if plan is linked to one...
    for(int i = 0; i < (int)intention_set.size(); i++)
    {
        checkNextAction = true;

        logger->print(Logger::EveryInfoPlus, "intention.original_id == plan.id? "+ std::to_string(intention_set[i]->get_original_planID())
        +" == "+ std::to_string(plan->get_id()) +"? "+ ag_support->boolToString(intention_set[i]->get_original_planID() == plan->get_id()));

        logger->print(Logger::EveryInfo, "intention.plan_id: "+ std::to_string(intention_set[i]->get_planID())
        +", intention.original_id: "+ std::to_string(intention_set[i]->get_original_planID()));

        if(intention_set[i]->get_original_planID() == plan->get_id())
        {   // then the plan was already an active intention. Remove it...
            warning_msg = " X Intention:["+ std::to_string(intention_set[i]->get_id());
            warning_msg += "], goal:["+ intention_set[i]->get_goal_name();
            warning_msg += "] plan:["+ std::to_string(intention_set[i]->get_planID());
            warning_msg += "] original plan:["+ std::to_string(intention_set[i]->get_original_planID());
            warning_msg += "] HAS TO BE REMOVED do to false conditions. Remove entire intention:["+ std::to_string(i) +"] X";

            logger->print(Logger::Warning, warning_msg);
            ag_metric->addGeneratedWarning(sim_current_time(), "checkAndManageIntentionFalseConditions", warning_msg);

            ag_metric->setActivatedIntentionCompletion(intention_set[i], sim_current_time().dbl(), "No valid conditions for internal goal!");
            ag_metric->setActivatedGoalCompletion(intention_set[i]->get_goalID(), intention_set[i]->get_goal_name(), sim_current_time().dbl(), "No valid conditions for internal goal!");

            plan_id = intention_set[i]->get_original_planID();
            reasoning_cycle_stopped_goals.push_back(intention_set[i]->get_goal_name()); // data for the client

            ag_support->removeGoalAndSelectedPlanFromPhiI(selected_plans, goal_set, intention_set[i]);
            removeInternalgoalTasksFromSchedulerVectors(intention_set[i]->get_original_planID());

            // cancel message activation for this intention...
            cancel_message_internal_goal_arrival(msg_name_internal_goal_arrival, intention_set[i]->get_planID(), intention_set[i]->get_original_planID());
            cancel_message_intention_completion(msg_name_intention_completion,
                    intention_set[i]->get_goal_name(),
                    intention_set[i]->get_waiting_for_completion(),
                    intention_set[i]->get_goalID(),
                    intention_set[i]->get_planID(),
                    intention_set[i]->get_original_planID());

            // cancel intention activation for the active internal goal in the first batch of the intention...
            logger->print(Logger::EveryInfo, "Schedule internal goal arrival: "+ std::to_string(intention_set[i]->get_original_planID()) +", "+ std::to_string(intention_set[i]->get_original_planID()));
            cancel_message_internal_goal_arrival(msg_name_internal_goal_arrival, intention_set[i]->get_original_planID(), intention_set[i]->get_original_planID());

            /* if a plan gets removed due to an internal goal with invalid "goal_name"
             * then, check and remove every task belonging to the same batch and that preceed the goal
             * --> we need to do this, because "checkAndManageIntentionFalseConditions" does what described only on the ready and release queue,
             * but in phiI new tasks are added to a tmp_queue. */
            ag_Sched->ag_remove_task_to_release_phiI(queue, plan_id);

            intention_set.erase(intention_set.begin() + i);
            i -= 1;
        }
        else { // check the first batch of the intention's body. Checking internal goals...
            actions = intention_set[i]->get_Actions();

            for(int j = 0; j < (int)actions.size() && checkNextAction; j++)
            {
                if(actions[j]->get_type() == "task") {
                    // DO NOTHING...
                }
                else if(actions[j]->get_type() == "goal")
                {
                    goal = std::dynamic_pointer_cast<Goal>(actions[j]);

                    logger->print(Logger::EveryInfo, "checkAndManageIntentionFalsePostconditions 2: "+ goal->get_goal_name() +" == "+ plan->get_goal_name());

                    // check if the internal goal is the same the plan referres to...
                    if(goal->get_goal_name() == plan->get_goal_name())
                    { // if yes, add current intention to "intentions_false"
                        intentions_false.push_back(std::make_pair(std::make_shared<Intention>(*intention_set[i]), i));
                    }
                }

                checkNextAction = false;
                if(j + 1 < (int)actions.size()) {
                    if(actions[j + 1]->get_type() == "task") {
                        task = std::dynamic_pointer_cast<Task>(actions[j + 1]);
                        if(task->getTaskExecutionType() == Task::EXEC_PARALLEL) {
                            checkNextAction = true;
                        }
                    }
                    else if(actions[j + 1]->get_type() == "goal") {
                        goal = std::dynamic_pointer_cast<Goal>(actions[j + 1]);
                        if(goal->getGoalExecutionType() == Goal::EXEC_PARALLEL) {
                            checkNextAction = true;
                        }
                    }
                }
            }
        }
    }

    // for all "intention_false", check if they were internal goals...
    // if yes, repeat function
    // if no, remove intention
    for(int i = 0; i < (int)intentions_false.size(); i++)
    {
        logger->print(Logger::EveryInfo, " > intentions_false: "+ std::to_string(intentions_false[i].first->getTopInternalSelectedPlan()->get_id()));
        checkAndManageIntentionFalseConditions(intentions_false[i].first->getTopInternalSelectedPlan(),
                selected_plans, goal_set, intention_set, belief_set, queue);
    }

    return ((int)intentions_false.size() == 0);
}

bool agent::checkAndManageIntentionFalseConditions_RR(std::shared_ptr<Plan> plan,
        std::vector<std::shared_ptr<MaximumCompletionTime>>& selected_plans,
        std::vector<std::shared_ptr<Goal>>& goal_set,
        std::vector<std::shared_ptr<Intention>>& intention_set,
        const std::vector<std::shared_ptr<Belief>>& belief_set,
        std::vector<std::shared_ptr<Task>>& queue,
        std::vector<std::tuple<bool, int, int>>& checkNextActionIntention_ith)
{ // plan has either invalid preconditions, cont_conditions or post_conditions

    // use to know which intentions contain "plan" has active internal goal. Structure: <intention, ID>
    std::vector<std::pair<std::shared_ptr<Intention>, int>> intentions_false;
    std::vector<std::shared_ptr<Action>> actions;
    std::shared_ptr<Goal> goal;
    std::shared_ptr<Task> task;
    std::string warning_msg = "";
    bool checkNextAction = true;
    int plan_id = -1;

    // loop over all intentions and check if plan is linked to one...
    for(int i = 0; i < (int)intention_set.size(); i++)
    {
        checkNextAction = true;

        logger->print(Logger::EveryInfoPlus, "intention.original_id == plan.id? "+ std::to_string(intention_set[i]->get_original_planID())
        +" == "+ std::to_string(plan->get_id()) +"? "+ ag_support->boolToString(intention_set[i]->get_original_planID() == plan->get_id()));

        logger->print(Logger::EveryInfo, "intention.plan_id: "+ std::to_string(intention_set[i]->get_planID())
        +", intention.original_id: "+ std::to_string(intention_set[i]->get_original_planID()));

        if(intention_set[i]->get_original_planID() == plan->get_id())
        {   // then the plan was already an active intention. Remove it...
            warning_msg = " X Intention:["+ std::to_string(intention_set[i]->get_id());
            warning_msg += "], goal:["+ intention_set[i]->get_goal_name();
            warning_msg += "] plan:["+ std::to_string(intention_set[i]->get_planID());
            warning_msg += "] original plan:["+ std::to_string(intention_set[i]->get_original_planID());
            warning_msg += "] HAS TO BE REMOVED do to false conditions. Remove entire intention:["+ std::to_string(i) +"] X";

            logger->print(Logger::Warning, warning_msg);
            ag_metric->addGeneratedWarning(sim_current_time(), "checkAndManageIntentionFalseConditions", warning_msg);

            ag_metric->setActivatedIntentionCompletion(intention_set[i], sim_current_time().dbl(), "No valid conditions for internal goal!");
            ag_metric->setActivatedGoalCompletion(intention_set[i]->get_goalID(), intention_set[i]->get_goal_name(), sim_current_time().dbl(), "No valid conditions for internal goal!");

            plan_id = intention_set[i]->get_original_planID();
            reasoning_cycle_stopped_goals.push_back(intention_set[i]->get_goal_name()); // data for the client

            ag_support->removeGoalAndSelectedPlanFromPhiI(selected_plans, goal_set, intention_set[i]);
            removeInternalgoalTasksFromSchedulerVectors(intention_set[i]->get_original_planID());

            // cancel message activation for this intention...
            cancel_message_internal_goal_arrival(msg_name_internal_goal_arrival, intention_set[i]->get_planID(), intention_set[i]->get_original_planID());
            cancel_message_intention_completion(msg_name_intention_completion,
                    intention_set[i]->get_goal_name(),
                    intention_set[i]->get_waiting_for_completion(),
                    intention_set[i]->get_goalID(),
                    intention_set[i]->get_planID(),
                    intention_set[i]->get_original_planID());

            // cancel intention activation for the active internal goal in the first batch of the intention...
            logger->print(Logger::EveryInfo, "Schedule internal goal arrival: "+ std::to_string(intention_set[i]->get_original_planID()) +", "+ std::to_string(intention_set[i]->get_original_planID()));
            cancel_message_internal_goal_arrival(msg_name_internal_goal_arrival, intention_set[i]->get_original_planID(), intention_set[i]->get_original_planID());

            /* if a plan gets removed due to an internal goal with invalid "goal_name"
             * then, check and remove every task belonging to the same batch and that preceed the goal
             * --> we need to do this, because "checkAndManageIntentionFalseConditions" does what described only on the ready and release queue,
             * but in phiI new tasks are added to a tmp_queue. */
            ag_Sched->ag_remove_task_to_release_phiI(queue, plan_id);

            checkNextActionIntention_ith.erase(checkNextActionIntention_ith.begin() + i);
            intention_set.erase(intention_set.begin() + i);
            i -= 1;
        }
        else { // check the first batch of the intention's body. Checking internal goals...
            actions = intention_set[i]->get_Actions();

            for(int j = 0; j < (int)actions.size() && checkNextAction; j++)
            {
                if(actions[j]->get_type() == "task") {
                    // DO NOTHING...
                }
                else if(actions[j]->get_type() == "goal")
                {
                    goal = std::dynamic_pointer_cast<Goal>(actions[j]);

                    logger->print(Logger::EveryInfo, "checkAndManageIntentionFalsePostconditions 2: "+ goal->get_goal_name() +" == "+ plan->get_goal_name());

                    // check if the internal goal is the same the plan referres to...
                    if(goal->get_goal_name() == plan->get_goal_name())
                    { // if yes, add current intention to "intentions_false"
                        intentions_false.push_back(std::make_pair(std::make_shared<Intention>(*intention_set[i]), i));
                    }
                }

                checkNextAction = false;
                if(j + 1 < (int)actions.size()) {
                    if(actions[j + 1]->get_type() == "task") {
                        task = std::dynamic_pointer_cast<Task>(actions[j + 1]);
                        if(task->getTaskExecutionType() == Task::EXEC_PARALLEL) {
                            checkNextAction = true;
                        }
                    }
                    else if(actions[j + 1]->get_type() == "goal") {
                        goal = std::dynamic_pointer_cast<Goal>(actions[j + 1]);
                        if(goal->getGoalExecutionType() == Goal::EXEC_PARALLEL) {
                            checkNextAction = true;
                        }
                    }
                }
            }
        }
    }

    // for all "intention_false", check if they were internal goals...
    // if yes, repeat function
    // if no, remove intention
    for(int i = 0; i < (int)intentions_false.size(); i++)
    {
        logger->print(Logger::EveryInfo, " > intentions_false: "+ std::to_string(intentions_false[i].first->getTopInternalSelectedPlan()->get_id()));
        checkAndManageIntentionFalseConditions_RR(intentions_false[i].first->getTopInternalSelectedPlan(),
                selected_plans, goal_set, intention_set, belief_set, queue, checkNextActionIntention_ith);
    }

    return ((int)intentions_false.size() == 0);
}


bool agent::checkIfAllMessagesAreInFuture(const simtime_t present)
{
    cObject* event = nullptr;
    const char* class_name = "agentMSG";
    const char* full_name = "endsimulation";
    std::string error_msg = "";

    logger->print(Logger::EveryInfo, "---checkIfAllMessagesAreInFuture---");
    logger->print(Logger::EveryInfo, "TOT MSG: "+ std::to_string(this->getSimulation()->getFES()->getLength()));

    for (int k = 0; k < this->getSimulation()->getFES()->getLength(); k++)
    {
        try
        {
            event = this->getSimulation()->getFES()->get(k);

            // check if (event->getClassName() == class_name) AND (event->getFullName() != full_name), then enter the statement
            if(strcmp(event->getClassName(), class_name) == 0 && strcmp(event->getFullName(), full_name) != 0)
            {
                agentMSG* msg = check_and_cast<agentMSG*>(event);
                if (msg->getSenderModule()->getFullName() == getFullName())
                {
                    logger->print(Logger::EveryInfo, std::to_string(k)
                    +", Info: "+ std::to_string(msg->getInformative())
                    +", Order: "+ std::to_string(msg->getInsertOrder())
                    +", Time: "+ std::to_string(msg->getArrivalTime().dbl())
                    +", Priority: "+ std::to_string(msg->getSchedulingPriority())
                    +", Type:["+ msg->getFullName()
                    +"], goal name:["+ msg->getAg_intention_goal_name()
                    +"], plan_id:["+ std::to_string(msg->getAg_intention_plan_id())
                    +"], original plan_id:["+ std::to_string(msg->getAg_intention_original_plan_id()) +"]");

                    if(present > msg->getArrivalTime())
                    {
                        return false; // there is at least one more message scheduled for time t: now
                    }
                }
                logger->print(Logger::EveryInfoPlus, " --- DEBUG --- ");
            }
        }
        catch(cRuntimeError const& ex) {
            error_msg = ex.what();
            logger->print(Logger::Error, "[cRuntimeError] "+ error_msg);
            ag_metric->addGeneratedError(present, "checkIfAllMessagesAreInFuture", error_msg);
        }
        catch(std::runtime_error const& ex) {
            error_msg = ex.what();
            logger->print(Logger::Error, "[runtime_error] "+ error_msg);
            ag_metric->addGeneratedError(present, "checkIfAllMessagesAreInFuture", error_msg);
        }
    }

    return true; // either we do not have message, or they are all scheduled for the future
}

bool agent::checkIfDesireWasAGoalAndRemoveIt(const std::shared_ptr<Desire> desire)
{
    bool isGoal = false;

    // check if the desire was a goal...
    for(int g = 0; g < (int)ag_goal_set.size(); g++)
    {
        logger->print(Logger::EveryInfoPlus, "checkIfDesireWasAGoal: "+ std::to_string(ag_goal_set[g]->get_id())
        +" == "+ std::to_string(desire->get_id())
        +" && "+ ag_goal_set[g]->get_goal_name()
        +" == "+ desire->get_goal_name());

        if(ag_goal_set[g]->get_id() == desire->get_id() // Note: this condition should be enough, due to the fact that goal IDs are taken from desires's
                && ag_goal_set[g]->get_goal_name() == desire->get_goal_name())
        { // yes, the desire was a goal...
            isGoal = true;
            logger->print(Logger::EveryInfo, " -> Yes, the desire was already goal:["+ std::to_string(ag_goal_set[g]->get_id()) +"]");

            // check if it had a plan associated...
            for(int p = 0; p < (int)ag_selected_plans.size(); p++)
            {
                if(ag_selected_plans[p]->getInternalGoalsSelectedPlans()[0].first->get_goal_name() == desire->get_goal_name())
                { // yes, the goal was associated with a plan
                    // check if it was already an intention...
                    for(int i = 0; i < (int)ag_intention_set.size(); i++)
                    {
                        logger->print(Logger::EveryInfo, std::to_string(ag_intention_set[i]->get_original_planID())
                        +" == "+ std::to_string(ag_selected_plans[p]->getInternalGoalsSelectedPlans()[0].first->get_id()));
                        logger->print(Logger::EveryInfo, "ORIGINAL_PLAN_ID: "+ std::to_string(ag_intention_set[i]->get_original_planID()));

                        if(ag_intention_set[i]->get_original_planID() == ag_selected_plans[p]->getInternalGoalsSelectedPlans()[0].first->get_id())
                        { // yes, it was already an intention
                            logger->print(Logger::EveryInfo, " ->->-> Yes, the plan was already an intention:["+ std::to_string(ag_intention_set[i]->get_id()) +"]");
                            // if it's tasks were already in to-be-release or ready-vector, we have to remove them, we don't want to execute them...
                            // Use original_planID in order to remove only the tasks belonging to the selected_plan[p] and leave already active internal goals unchanged

                            reasoning_cycle_stopped_goals.push_back(ag_intention_set[i]->get_goal_name()); // data for the client

                            removeInternalgoalTasksFromSchedulerVectors(ag_selected_plans[p]->getInternalGoalsSelectedPlans()[0].first->get_id());

                            cancel_message_internal_goal_arrival(msg_name_internal_goal_arrival, ag_intention_set[i]->get_planID(), ag_intention_set[i]->get_original_planID());
                            cancel_message_intention_completion(msg_name_intention_completion,
                                    ag_intention_set[i]->get_goal_name(),
                                    ag_intention_set[i]->get_waiting_for_completion(),
                                    ag_intention_set[i]->get_goalID(),
                                    ag_intention_set[i]->get_planID(),
                                    ag_intention_set[i]->get_original_planID());

                            // remove it from intention set:
                            ag_metric->setActivatedIntentionCompletion(ag_intention_set[i], sim_current_time().dbl(), "checkIfDesireWasAGoal - goal removed");
                            ag_intention_set.erase(ag_intention_set.begin() + i);

                            break;
                        }
                    }
                    // remove it from selected plans set:
                    ag_selected_plans.erase(ag_selected_plans.begin() + p);
                    break;
                }
            }

            // remove it from goal set:
            ag_metric->setActivatedGoalCompletion(ag_goal_set[g], sim_current_time().dbl(), "Desire has invalid preconditions!");
            ag_goal_set.erase(ag_goal_set.begin() + g);
            break;
        }
    }

    return isGoal;
}

bool agent::checkIfGoalsPlanIsStillValid(const std::shared_ptr<Desire> desire, const std::shared_ptr<Goal> goal)
{
    bool isPlanValid = true; // is the plan associated with this goal still valid?
    bool valid_conditions = false;
    int intention_index = -1;
    std::string conditions_check_return_msg = "";
    std::vector<std::shared_ptr<Task>> empty_release_queue(0); // declared only because required by function "checkAndManageIntentionFalseConditions"

    // check the selected plans...
    for(int p = 0; p < (int)ag_selected_plans.size(); p++)
    {
        if(ag_selected_plans[p] != nullptr)
        {
            if(ag_selected_plans[p]->getInternalGoalsSelectedPlans()[0].first->get_goal_name() == goal->get_goal_name())
            {   // if the p-th plan is the one assigned to the goal...
                logger->print(Logger::EveryInfoPlus, ag_selected_plans[p]->getInternalGoalsSelectedPlans()[0].first->get_goal_name()
                        +" == " + goal->get_goal_name() + " ?");

                if(ag_support->checkIfGoalIsLinkedToIntention(goal, ag_intention_set, intention_index))
                {
                    valid_conditions = ag_support->checkPlanOrDesireConditions(ag_belief_set,
                            ag_selected_plans[p]->getInternalGoalsSelectedPlans()[0].first->get_cont_conditions(),
                            conditions_check_return_msg, "plan", ag_selected_plans[p]->getInternalGoalsSelectedPlans()[0].first->get_id(),
                            ag_intention_set[intention_index]->get_expression_constants(),
                            ag_intention_set[intention_index]->get_expression_variables());

                    logger->print(Logger::EssentialInfo, " Cont_conditions: "+ ag_support->dump_expression(ag_selected_plans[p]->getInternalGoalsSelectedPlans()[0].first->get_cont_conditions(),
                            ag_belief_set,
                            ag_intention_set[intention_index]->get_expression_constants(),
                            ag_intention_set[intention_index]->get_expression_variables()));
                }
                else {
                    valid_conditions = ag_support->checkPlanOrDesireConditions(ag_belief_set,
                            ag_selected_plans[p]->getInternalGoalsSelectedPlans()[0].first->get_cont_conditions(),
                            conditions_check_return_msg, "plan", ag_selected_plans[p]->getInternalGoalsSelectedPlans()[0].first->get_id());

                    logger->print(Logger::EssentialInfo, " Cont_conditions: "+ ag_support->dump_expression(ag_selected_plans[p]->getInternalGoalsSelectedPlans()[0].first->get_cont_conditions(), ag_belief_set));
                }

                logger->print(Logger::EveryInfo, "valid_conditions:["+ ag_support->boolToString(valid_conditions)
                        +"] AND message:["+ conditions_check_return_msg +"]");
                logger->print(Logger::EveryInfoPlus, " --- DEBUG --- ");

                if(valid_conditions == false)
                { // then, this context_cond doesn't hold anymore....
                    isPlanValid = false;

                    checkAndManageIntentionFalseConditions(ag_selected_plans[p]->getInternalGoalsSelectedPlans()[0].first,
                            ag_selected_plans, ag_goal_set, ag_intention_set, ag_belief_set, empty_release_queue);
                }
                break;
            }
        }
        else {
            logger->print(Logger::EveryInfo, " Goal:["+ std::to_string(goal->get_id())
            +"] is not associated with any plan...");
        }
    }
    return isPlanValid;
}

void agent::checkIfIntentionHadActiveInternalGoals(const int removed_intention_id, std::vector<std::shared_ptr<Intention>>& intentionset) {
    std::shared_ptr<Goal> tmp_g;
    std::shared_ptr<Task> tmp_t;
    int updated_plan_id = -1;

    for(int i = 0; i < (int)intentionset.size(); i++) {
        if(intentionset[i]->get_planID() == removed_intention_id) {
            std::vector<std::shared_ptr<Action>> actions = intentionset[i]->get_Actions();
            intentionset[i]->set_planID(intentionset[i]->get_original_planID());

            for(int y = 0; y < (int)actions.size(); y++) {
                if(actions[y]->get_type() == "goal") {
                    tmp_g = std::dynamic_pointer_cast<Goal>(actions[y]);

                    if(tmp_g->get_plan_id() != tmp_g->get_original_plan_id()) {
                        tmp_g->set_plan_id(tmp_g->get_original_plan_id());
                    }
                }
                else if(actions[y]->get_type() == "task") {
                    tmp_t = std::dynamic_pointer_cast<Task>(actions[y]);

                    if(tmp_t->getTaskPlanId() != tmp_t->getTaskOriginalPlanId()) {
                        updated_plan_id = tmp_t->getTaskOriginalPlanId();

                        if(tmp_t->getTaskisBeenActivated()) {
                            // if the task isBeenActivated, update the planId of its copies in RELEASE and READY
                            ag_Sched->update_tasks_planid_ready(tmp_t->getTaskId(), tmp_t->getTaskPlanId(), tmp_t->getTaskOriginalPlanId(), updated_plan_id);
                            ag_Sched->update_tasks_planid_release(tmp_t->getTaskId(), tmp_t->getTaskPlanId(), tmp_t->getTaskOriginalPlanId(), updated_plan_id);
                        }

                        tmp_t->setTaskPlanId(updated_plan_id);
                    }
                }
            }
        }
    }
}
/* ------------------------------------------------------------------------------------------------------------ */

void agent::checkIfContconditionsAreValid(std::vector<std::shared_ptr<Goal>>& goalset,
        std::vector<std::shared_ptr<MaximumCompletionTime>>& selected_plans,
        const std::vector<std::shared_ptr<Belief>>& beliefset,
        std::vector<std::shared_ptr<Intention>>& intentionset)
{
    int index_intention = -1;
    bool cont_cond_valid = false;
    std::string return_msg = "";
    std::vector<std::shared_ptr<Task>> empty_release_queue(0); // declared only because required by function "checkAndManageIntentionFalseConditions"

    for(int g = 0; g < (int)goalset.size(); g++)
    {
        for(int p = 0; p < (int)selected_plans.size(); p++)
        {
            std::shared_ptr<Plan> plan = selected_plans[p]->getInternalGoalsSelectedPlans()[0].first;

            // If the plan is already an intention,
            // then combine the plan's cont_conditions with the list of constants and variables defined in the intention.plan.effects_at_begin
            if(ag_support->checkIfPlanWasAlreadyIntention(plan, index_intention, intentionset))
            {
                cont_cond_valid = ag_support->checkPlanOrDesireConditions(beliefset,
                        plan->get_cont_conditions(),
                        return_msg, "plan", plan->get_id(),
                        intentionset[index_intention]->get_expression_constants(),
                        intentionset[index_intention]->get_expression_variables());

                logger->print(Logger::EveryInfo, "cont_cond_valid:["+ ag_support->boolToString(cont_cond_valid)
                        +"] AND message:["+ return_msg +"]");

                if(cont_cond_valid == false)
                {  // remove this plan from the list of filtered ones:

                    logger->print(Logger::Warning, " Intention:["+ std::to_string(intentionset[index_intention]->get_id())
                    +"], plan id:["+ std::to_string(intentionset[index_intention]->get_planID())
                    +"], original plan id:["+ std::to_string(intentionset[index_intention]->get_original_planID())
                    +"], Cont_conditions: "+ ag_support->dump_expression(plan->get_cont_conditions(), beliefset,
                            intentionset[index_intention]->get_expression_constants(),
                            intentionset[index_intention]->get_expression_variables()));

                    logger->print(Logger::EveryInfo, " -> Removed Plan [id=" + std::to_string(plan->get_id())
                    + "], name:["+ plan->get_goal_name() +"] because cont_conditions do not hold! Details: "+ return_msg);

                    checkAndManageIntentionFalseConditions(plan, selected_plans, goalset, intentionset, beliefset, empty_release_queue);
                    g = -1; // start main loop from the beginning (because we do not know what "checkAndManageIntentionFalseConditions" did)
                    break;
                }
            }
        }
    }
}

void agent::update_belief_set_time(agentMSG* agMsg)
{
    int sensor_id = agMsg->getAg_sensor_id();
    int sensor_index = -1;
    bool variant_update_succeed = true;
    bool belief_updated = false;
    std::variant<int, double, bool, std::string> tmp_variant, original_value;
    std::string report_value = "";
    std::string warning_msg = "";

    logger->print(Logger::EssentialInfo, "[!] UPDATE BELIEF t: "+ sim_current_time().str() +" due to sensor:["+ std::to_string(sensor_id) +"]");

    // for every 'time' sensor...
    for(int i = 0; i < (int)ag_sensors_time.size(); i++)
    {
        if(ag_sensors_time[i]->get_id() == sensor_id
                && fabs(ag_sensors_time[i]->get_time() - sim_current_time().dbl()) < ag_Sched->EPSILON)
        {
            sensor_index = i;

            // foreach belief, find the one with the sensor is interested into (sensor goal name == belief.name)
            for(int b = 0; b < (int)ag_belief_set.size(); b++)
            {
                if(ag_belief_set[b]->get_name() == ag_sensors_time[i]->get_belief_name())
                {
                    belief_updated = true;
                    original_value = ag_belief_set[b]->get_value();

                    // when found, update belief's vale with the value of simulated by the sensor:
                    tmp_variant = ag_support->updateBeliefValueBasedOnSensorRead(ag_belief_set[b]->get_value(), ag_sensors_time[i]->get_value(), ag_sensors_time[i]->get_mode(), variant_update_succeed);
                    if(variant_update_succeed == false)
                    {
                        logger->print(Logger::EssentialInfo, " Sensor (time) can not update Belief '"+ ag_belief_set[b]->get_name()
                                +"' due to incompatible type read from the sensor! belief:("+ ag_belief_set[b]->get_name()
                                +", "+ ag_support->variantToString(ag_belief_set[b]->get_value())
                                +"), sensor.belief.value:["+ ag_support->variantToString(ag_sensors_time[i]->get_value()) +"].");
                    }
                    else {
                        ag_belief_set[b]->set_value(tmp_variant);

                        report_value = "t: "+ sim_current_time().str() +" Belief:["+ ag_belief_set[b]->get_name();
                        report_value += "] value has been updated from:["+ ag_support->variantToString(original_value) +"] to:["+ ag_support->variantToString(ag_belief_set[b]->get_value()) +"]";
                        logger->print(Logger::EssentialInfo, report_value);

                        // write simulation events report
                        write_simulation_timeline_json_report(report_value);
                    }

                    // in both cases, remove the reading from the list, we have completed it...
                    ag_sensors_time.erase(ag_sensors_time.begin() + i);
                    break;
                }
            }

            // If no belief with the specified name has been found,
            // and if we are dealing with a SET sensor,
            // then add a new belief to the beliefset of the agent
            if(belief_updated == false && ag_sensors_time[i]->get_mode() == Sensor::SET)
            {
                belief_updated = true;
                ag_belief_set.push_back(std::make_shared<Belief>(ag_sensors_time[i]->get_belief_name(), ag_sensors_time[i]->get_value()));

                logger->print(Logger::EssentialInfo, " - A NEW Belief:["+ ag_sensors_time[i]->get_belief_name()
                        +"], value:["+ ag_support->variantToString(ag_sensors_time[i]->get_value())
                        +"] has been added to the knowledge-base of the agent!");

                report_value = "t: "+ sim_current_time().str() +" A NEW Belief:["+ ag_sensors_time[i]->get_belief_name();
                report_value += "], value:["+ ag_support->variantToString(ag_sensors_time[i]->get_value());
                report_value += "] has been added to the knowledge-base of the agent!";

                // write simulation events report
                write_simulation_timeline_json_report(report_value);

                // remove the reading from the list, we have completed it...
                ag_sensors_time.erase(ag_sensors_time.begin() + i);
                break;
            }
            break; // a sensor has been analyzed. Stop checking...
        }
    }

    if(belief_updated == false) {
        warning_msg = "[WARNING] There is no belief with name: "+ ag_sensors_time[sensor_index]->get_belief_name();
        logger->print(Logger::Warning, warning_msg);
        ag_metric->addGeneratedWarning(sim_current_time(), "update_belief_set_time", warning_msg);

        ag_sensors_time.erase(ag_sensors_time.begin() + sensor_index);
    }
    else { // the belief has been updated. Schedule a new reasoning cycle
        // Note: if multiple sensors are activated at the same time, use function 'cancel_next_reasoning_cycle_msg' to only schedule one reasoning cycle
        cancel_next_reasoning_cycle_msg(msg_name_reasoning_cycle, sim_current_time());
        scheduleAt(sim_current_time(), set_reasoning_cycle());
    }
}

// *****************************************************************************
// *******                     Support Functions:                        *******
// *****************************************************************************
void agent::removeFCFSInvalidTasksFromSchedulerVectors(const std::vector<std::shared_ptr<Intention>>& intentions)
{
    /* Before actually apply phiI, let's validate the intentions and both ready and realease queue
     * Reason: At t: 10 we have [4] intentions such that (see. _test-phiI_only_period-INF.8.1_seed_164 (starting from 1))
     * Intention[0], goal:[2], plan:[7], original_plan:[7] P7 Main plan > contains P8, whom body contains P9 (es *)
     * Intention[1], goal:[6], plan:[7], original_plan:[8]
     * Intention[2], goal:[1], plan:[8], original_plan:[9]
     * Intention[3], goal:[0], plan:[6], original_plan:[6]
     * Release: [Task:[0], plan:[8], original_plan:[9]]; Ready: [].
     * At t: 10 a sensor satisfies P8. Thus the intentionset becomes:
     * Intention[0], goal:[2], plan:[7], original_plan:[7] P7 Main plan
     * Intention[2], goal:[1], plan:[*9*], original_plan:[9]
     * Intention[3], goal:[0], plan:[6], original_plan:[6]
     * Release: [Task:[0], plan:[*9*], original_plan:[9]]; Ready: [].
     * Using FCFS means that intentions that are not linked (due to internal-goal) are executed sequentially.
     * Thus, in Release/Ready we can not have tasks of P9 until P7 is fully completed!
     * SOL: remove  tasks from release and ready if do not belong to first intention or a direct "child" (es *) */
    /* Version 1: TODO does not work correctly.
     * ----------------------------------------------
     * Time t: 17 - Given an intentionset such that:
     * *
     * List of Intentions[tot = 4]:
     *  Intention index:[0], from goal:[0], goal:(skill_batteryLevel), plan:[6], original_plan:[6], num. of actions: 5, startTime: 0.000000, firstActivation: 0.000000, lastExecution: 0.000000, batch_startTime: 16.000000, current completion time: -1.000000, set completion time: -1.000000, waiting for completion: false
     *  - Goal:[id=7], name:[skill_isFronDoorLocked], plan:[6], original_plan:[6], belief:(skill_isFronDoorLocked), arrivalTime:[0.000000], absolute arrivalTime:[0.000000], DDL:[100.000000], ExecType:[SEQUENTIAL], forceReasoning:[false], isBeenActivated:[true]
     *  - Task:[0], plan:[6], original_plan:[6], N_exec left: 1 of 1, compTime: 6.000000, DDL: 20.000000, SERVER:[-1], arrivalTimeInitial: 0.000000, arrivalTime: 0.000000, Execution_type:[SEQUENTIAL] isBeenActivated:[false]
     *  - Task:[1], plan:[6], original_plan:[6], N_exec left: 1 of 1, compTime: 2.000000, DDL: 25.000000, SERVER:[-1], arrivalTimeInitial: 0.000000, arrivalTime: 0.000000, Execution_type:[SEQUENTIAL] isBeenActivated:[false]
     *  - Task:[2], plan:[6], original_plan:[6], N_exec left: 1 of 1, compTime: 4.000000, DDL: 20.000000, SERVER:[-1], arrivalTimeInitial: 0.000000, arrivalTime: 0.000000, Execution_type:[SEQUENTIAL] isBeenActivated:[false]
     *  - Goal:[id=8], name:[skill_isGardenIrrigationOn], plan:[6], original_plan:[6], belief:(skill_isGardenIrrigationOn), arrivalTime:[0.000000], absolute arrivalTime:[-1.000000], DDL:[200.000000], ExecType:[SEQUENTIAL], forceReasoning:[false], isBeenActivated:[false]
     *  Intention index:[3], from goal:[7], goal:(skill_isFronDoorLocked), plan:[6], original_plan:[9], num. of actions: 1, startTime: 16.000000, firstActivation: 0.000000, lastExecution: 16.000000, batch_startTime: 0.000000, current completion time: -1.000000, set completion time: -1.000000, waiting for completion: false
     *  - Task:[3], plan:[6], original_plan:[9], N_exec left: 1 of 1, compTime: 4.000000, DDL: 20.000000, SERVER:[-1], arrivalTimeInitial: 0.000000, arrivalTime: 16.000000, Execution_type:[SEQUENTIAL] isBeenActivated:[true]
     *  Intention index:[1], from goal:[1], goal:(skill_isOvenOn), plan:[0], original_plan:[0], num. of actions: 6, startTime: 0.000000, firstActivation: -1.000000, lastExecution: 0.000000, batch_startTime: 0.000000, current completion time: -1.000000, set completion time: -1.000000, waiting for completion: false
     *  - Task:[0], plan:[0], original_plan:[0], N_exec left: 1 of 1, compTime: 7.000000, DDL: 20.000000, SERVER:[-1], arrivalTimeInitial: 0.000000, arrivalTime: 0.000000, Execution_type:[SEQUENTIAL] isBeenActivated:[false]
     *  - Task:[1], plan:[0], original_plan:[0], N_exec left: 1 of 1, compTime: 1.000000, DDL: 20.000000, SERVER:[-1], arrivalTimeInitial: 0.000000, arrivalTime: 0.000000, Execution_type:[SEQUENTIAL] isBeenActivated:[false]
     *  - Task:[2], plan:[0], original_plan:[0], N_exec left: 1 of 1, compTime: 1.000000, DDL: 25.000000, SERVER:[-1], arrivalTimeInitial: 0.000000, arrivalTime: 0.000000, Execution_type:[SEQUENTIAL] isBeenActivated:[false]
     *  - Task:[3], plan:[0], original_plan:[0], N_exec left: 1 of 1, compTime: 6.000000, DDL: 20.000000, SERVER:[-1], arrivalTimeInitial: 0.000000, arrivalTime: 0.000000, Execution_type:[SEQUENTIAL] isBeenActivated:[false]
     *  - Goal:[id=3], name:[skill_needsWeatherForecast], plan:[0], original_plan:[0], belief:(skill_needsWeatherForecast), arrivalTime:[0.000000], absolute arrivalTime:[-1.000000], DDL:[75.000000], ExecType:[SEQUENTIAL], forceReasoning:[false], isBeenActivated:[false]
     *  - Task:[4], plan:[0], original_plan:[0], N_exec left: 1 of 1, compTime: 2.000000, DDL: 18.000000, SERVER:[-1], arrivalTimeInitial: 0.000000, arrivalTime: 0.000000, Execution_type:[SEQUENTIAL] isBeenActivated:[false]
     *  Intention index:[2], from goal:[2], goal:(skill_isRoomAtRightTemperature), plan:[2], original_plan:[2], num. of actions: 3, startTime: 0.000000, firstActivation: -1.000000, lastExecution: 0.000000, batch_startTime: 0.000000, current completion time: -1.000000, set completion time: -1.000000, waiting for completion: false
     *  - Goal:[id=4], name:[skill_isMusicOn], plan:[2], original_plan:[2], belief:(skill_isMusicOn), arrivalTime:[0.000000], absolute arrivalTime:[-1.000000], DDL:[300.000000], ExecType:[SEQUENTIAL], forceReasoning:[false], isBeenActivated:[false]
     *  - Task:[0], plan:[2], original_plan:[2], N_exec left: 2 of 2, compTime: 1.000000, DDL: 24.000000, SERVER:[-1], arrivalTimeInitial: 0.000000, arrivalTime: 0.000000, Execution_type:[SEQUENTIAL] isBeenActivated:[false]
     *  - Goal:[id=5], name:[skill_isLightOn], plan:[2], original_plan:[2], belief:(skill_isLightOn), arrivalTime:[0.000000], absolute arrivalTime:[-1.000000], DDL:[200.000000], ExecType:[SEQUENTIAL], forceReasoning:[false], isBeenActivated:[false]
     * -----------------------------
     * Release queue[tot = 0]:
     * ---------------------------
     * Ready queue[tot = 1]:
     * - Task:[3], plan:[6], original_plan:[9], deadline:[20.000000], arrivalTime:[16.000000], releaseTime:[16.000000], N_exec left:[0 of 1], tskCres:[4.000000], tskStatus:[EXEC], tskExecType:[SEQUENTIAL], isBeenAct:[true]
     * ---------------------------
     * The function removes "Task:[3], plan:[6], original_plan:[9]" from the queue. Then, recursiveFCFS insert it back but with updated "releaseTime".
     * The problem is that the execution of the task in interval 16-17 is skipped and the task basically executes for 1 + 4.
     */
    int indexIntention = -1;
    int mainPlanID = -1;
    bool checkNext = true;
    std::shared_ptr<Task> task;
    std::shared_ptr<Goal> goal;

    if(intentions.size() > 1) { // if we have just 1 intention, do nothing. Otherwise...
        mainPlanID = intentions[0]->get_planID();
        indexIntention = 1;
        for(int i = 1; i < (int)intentions.size() && checkNext; i++) {
            checkNext = false;
            if(intentions[i]->get_original_planID() == mainPlanID) {
                mainPlanID = intentions[i]->get_original_planID();
                indexIntention += 1;
                checkNext = true;
            }
        }

        for(int i = indexIntention; i < (int)intentions.size(); i++) {
            mainPlanID = intentions[i]->get_original_planID();

            for(int j = 0; j < (int)intentions[i]->get_Actions().size(); j++) {
                if(intentions[i]->get_Actions()[j]->get_type() == "task") {
                    task = std::dynamic_pointer_cast<Task>(intentions[i]->get_Actions()[j]);
                    task->setTaskisBeenActivated(false);
                    task->setTaskArrivalTime(intentions[i]->get_lastExecution());
                }
                else if(intentions[i]->get_Actions()[j]->get_type() == "goal") {
                    goal = std::dynamic_pointer_cast<Goal>(intentions[i]->get_Actions()[j]);
                    goal->setGoalisBeenActivated(false);
                }
            }

            logger->print(Logger::EveryInfo, " Checking if there are tasks to remove from READY vector, due to intention not active anymore...");
            ag_Sched->ag_remove_task_in_ready_based_on_original_plan_id(mainPlanID);

            logger->print(Logger::EveryInfo, " Checking if there are tasks to remove from RELEASE vector, due to intention not active anymore...");
            ag_Sched->ag_remove_task_to_release_based_on_original_plan_id(mainPlanID);

            logger->print(Logger::EveryInfo, " Remove every scheduled message belonging to 'original_planID = "+ std::to_string(mainPlanID) +"'");
            cancel_message_removed_task(mainPlanID, false);
        }
    }

    // DEBUG -----------------------------------------------------
    logger->print(Logger::EssentialInfo, "-----------------------------");
    ag_Sched->ev_ag_tasks_vector_to_release();
    logger->print(Logger::EssentialInfo, "-----------------------------");
    ag_Sched->ev_ag_tasks_vector_ready();
    logger->print(Logger::EssentialInfo, "-----------------------------");
    // -----------------------------------------------------------
}

void agent::removeInternalgoalTasksFromSchedulerVectors(const int original_planID) {

    logger->print(Logger::EveryInfo, " Checking if there are tasks to remove from READY vector, due to intention not active anymore...");
    ag_Sched->ag_remove_task_in_ready_based_on_original_plan_id(original_planID);

    logger->print(Logger::EveryInfo, " Checking if there are tasks to remove from RELEASE vector, due to intention not active anymore...");
    ag_Sched->ag_remove_task_to_release_based_on_original_plan_id(original_planID);

    logger->print(Logger::EveryInfo, " Remove every scheduled message belonging to 'original_planID = "+ std::to_string(original_planID) +"'");
    cancel_message_removed_task(original_planID, false);
}

void agent::removeInvalidIntentions(std::vector<std::shared_ptr<Goal>>& goal_set, const std::vector<std::shared_ptr<MaximumCompletionTime>>& selected_plans) {
    logger->print(Logger::EveryInfo, "----* removeInvalidIntentions *----");
    bool isIntentionLinkedToPlan = false;
    std::shared_ptr<Goal> goal;

    for(int i = 0; i < (int)ag_intention_set.size(); i++)
    {
        isIntentionLinkedToPlan = false;

        logger->print(Logger::EveryInfo, " Intention:["+ std::to_string(ag_intention_set[i]->get_id())
        +"], plan:["+ std::to_string(ag_intention_set[i]->get_planID())
        +"], original_plan:["+ std::to_string(ag_intention_set[i]->get_original_planID())
        +"], first_int_goal_plan:["+ std::to_string(ag_intention_set[i]->getTopInternalSelectedPlan()->get_id()) +"]");

        for(int p = 0; p < (int)selected_plans.size(); p++)
        {
            logger->print(Logger::EveryInfo, "removeInvalidIntentions: "+ std::to_string(ag_intention_set[i]->get_original_planID())
            +" == "+ std::to_string(selected_plans[p]->getInternalGoalsSelectedPlans()[0].first->get_id()));

            if(ag_intention_set[i]->get_original_planID() == selected_plans[p]->getInternalGoalsSelectedPlans()[0].first->get_id())
            { // the best selected plan for the goal[p] is linked to an active intention...
                logger->print(Logger::EveryInfo, " - Valid original_planID");
                isIntentionLinkedToPlan = true;
            }

            if(isIntentionLinkedToPlan)
            {
                logger->print(Logger::EveryInfo, " - Plan:["+ std::to_string(ag_intention_set[i]->get_planID())
                +"], Original Plan:["+ std::to_string(ag_intention_set[i]->get_original_planID())
                +"], goal name:["+ ag_intention_set[i]->get_goal_name()
                +"] still active, keep the Intention");

                break;
            }
        }

        if(isIntentionLinkedToPlan == false)
        {
            logger->print(Logger::EssentialInfo, " >>> Intention:["+ std::to_string(ag_intention_set[i]->get_id())
            +"] plan:["+ std::to_string(ag_intention_set[i]->get_planID())
            +"] original_plan:["+ std::to_string(ag_intention_set[i]->get_original_planID())
            +"], goal_name:["+ ag_intention_set[i]->get_goal_name()
            +"] is NOT active anymore -> keep goal, remove intention... <<<");

            // FCFS: check if the removed intention had been activated due to an internal goal of a different intention
            for(int x = 0; x < (int)ag_intention_set.size(); x++) {
                for(int y = 0; y < (int)ag_intention_set[x]->get_Actions().size(); y++)
                {
                    if(ag_intention_set[x]->get_Actions()[y]->get_type() == "goal")
                    {
                        goal = std::dynamic_pointer_cast<Goal>(ag_intention_set[x]->get_Actions()[y]);

                        if(goal->get_goal_name()  == ag_intention_set[i]->get_goal_name()
                                && goal->getGoalisBeenActivated())
                        {
                            goal->setGoalisBeenActivated(false);
                        }
                    }
                }
            }
            // --------------------------------------------------------------------------------

            reasoning_cycle_stopped_goals.push_back(ag_intention_set[i]->get_goal_name()); // data for the client

            // remove intention's task from execution
            removeInternalgoalTasksFromSchedulerVectors(ag_intention_set[i]->get_original_planID());

            cancel_message_internal_goal_arrival(msg_name_internal_goal_arrival, ag_intention_set[i]->get_planID(), ag_intention_set[i]->get_original_planID());
            cancel_message_intention_completion(msg_name_intention_completion,
                    ag_intention_set[i]->get_goal_name(),
                    ag_intention_set[i]->get_waiting_for_completion(),
                    ag_intention_set[i]->get_goalID(),
                    ag_intention_set[i]->get_planID(),
                    ag_intention_set[i]->get_original_planID());

            // remove intention from intention_set
            // i.e. because internal goal of an intention not active anymore
            ag_metric->setActivatedIntentionCompletion(ag_intention_set[i], sim_current_time().dbl(), "removeInvalidIntentions");
            ag_intention_set.erase(ag_intention_set.begin() + i);

            i -= 1;
        }
    }
}

double agent::compute_MsgUtil_sever() {
    // add msg_server util if needed
    bool msg_server_mode = (bool) par("msg_server_mode");
    double msg_util = 0;
    if (msg_server_mode) {
        double server_budget = (double) par("server_budget");
        double server_period = (double) par("server_period");
        msg_util = server_budget / server_period * 2; // count both read and write msg servers
    }

    return msg_util;
}

// Initialize the knowledge-base of the agent (beliefset, desireset...) without sending messages
void agent::initialize_knowledgbase() {
    // v2: setup knowledge-base directly
    std::pair<bool, json> set_knowledge = std::make_pair(true, json::object());

    // read BELIEFSET file
    set_knowledge = set_ag_beliefset(true, read_from_file);
    validate_knowledgebase(set_knowledge);

    // read the SKILLS file
    set_knowledge = set_ag_skillset(true, read_from_file);
    validate_knowledgebase(set_knowledge);

    // read DESIRESET file
    set_knowledge = set_ag_desireset(true, read_from_file);

    // read SERVER file
    set_knowledge = set_ag_servers(true, read_from_file);
    validate_knowledgebase(set_knowledge);

    // read the list of PLANS available for each agent
    set_knowledge = set_ag_planset(true, read_from_file);
    validate_knowledgebase(set_knowledge);

    // read the SENSORS file
    set_knowledge = set_ag_sensors(true, read_from_file);
    validate_knowledgebase(set_knowledge);

    // read the UPDATED values for the BELIEF
    set_knowledge = set_ag_update_beliefset(true, read_from_file);
    validate_knowledgebase(set_knowledge);
}

int agent::mod(int a, int b) {
    // [!!!] The problem with % is that if "a" is negative, (a % b) returns a negative value or 0, due to how C++ implements operator '%'.
    // https://stackoverflow.com/questions/11630321/why-does-c-output-negative-numbers-when-using-modulo
    int res = a % b;
    return (res >=0) ? res: (res + b);
}

simtime_t agent::sim_current_time() {
    /*[!] IMPORTANT! replaced every "scheduleAt(sim_current_time())"
     * with scheduleAt(sim_current_time().trunc(SIMTIME_MS)) in order to avoid messages scheduled in the past
     * simtime_t t = sim_current_time().trunc(SIMTIME_MS);                       // 9682256000000000
     * simtime_t t_converted = static_cast<simtime_t>(sim_current_time().dbl()); // 9682256000000084 */

    return simTime().trunc(sim_time_unit);
}

simtime_t agent::convert_double_to_simtime_t(double time)
{
    /* ***************************************************************************************
     * Note: se continuasse a dare errori di scheduleAt nel passato di 0.001, una cosa da provare potrebbe essere:
     * if(fabs(sim_current_time() - t) < EPSILON) { then use sim_current_time() per schedulare il messaggio }
     * ****************************************************************************************/

    // round numbers on the third decimal number.
    double value = round(time * 1000.0) / 1000.0;
    simtime_t t = value;
    return t.trunc(sim_time_unit);  // truncate at third digit, without round the number
}


// *****************************************************************************
// *******                        HANDLE MESSAGE                         *******
// *****************************************************************************
void agent::handleMessage(cMessage* msg)
{
    try {
        json ack_msg = json::object();

        /* *****************************************************************************************
         * ****                                Thread management                                ****
         * *****************************************************************************************/
        if(read_from_file == false)
        {
            /* ************************************************************************
             * if the current time is greater than the RUN until instant,
             * and the scheduled messages are all scheduled for the future,
             * and no response as been sent to the client until now,
             * THEN sent it now. */
            if(proceed_simulation && sim_current_time() >= run_until)
            {
                logger->print(Logger::EveryInfo, "checkIfAllMessagesAreInFuture: time t: "+ sim_current_time().str());
                if(checkIfAllMessagesAreInFuture(sim_current_time()))
                {
                    if(is_client_still_waiting_response)
                    {
                        if(send_new_solution_msg)
                        {
                            if(no_solution_found) { // during the execution, the code entered ag_reasoning_cycle and then stopped
                                ack_msg = ack_no_solution(reasoning_cycle_stopped_goals, reasoning_cycle_new_activated_goals, main_goals_completed, goals_with_invalid_post_cond);
                            }
                            else { // during the execution, the code entered ag_reasoning_cycle and phiI
                                ack_msg = ack_new_solution(reasoning_cycle_stopped_goals, reasoning_cycle_new_activated_goals, main_goals_completed, goals_with_invalid_post_cond);
                            }
                        }
                        else
                        { // during the execution, the code performed DO NOT entered ag_reasoning_cycle. It may have entered phiI
                            ack_msg = ack_run_until(reasoning_cycle_new_activated_goals);
                        }

                        logger->print(Logger::EssentialInfo, " - ACK --->>> "+ ack_msg.dump());

                        // message sent to the client, reset variable
                        send_new_solution_msg = false;
                        completed_intention_invalid_postconditions = false;
                        main_goals_completed.clear(); // achieved desire and internal goals between two consecutive RUN command
                        goals_with_invalid_post_cond.clear(); // completed intentions with invalid post conditions
                        // v2: Clear vectors containing new and stopped goals...
                        reasoning_cycle_new_activated_goals.clear(); // list of goals activated during reasoning cycle
                        reasoning_cycle_stopped_goals.clear(); // list of goals removed during reasoning cycle

                        // actually send message
                        send_msg_to_client(ack_msg);
                        logger->print(Logger::EveryInfoPlus, "-- Message sent to client...");
                    }
                }
            }

            logger->print(Logger::EveryInfo, " *-*-*- > cond_var run before: "+ ag_support->boolToString((proceed_simulation && sim_current_time() < run_until) || exit_simulation));

            // wait until the condition is true before moving on with the code execution
            cond_var.wait(lk, [this] {
                return ((proceed_simulation && this->sim_current_time() < run_until)
                        || exit_simulation);
            });
            // --------------------------------------------

            logger->print(Logger::EveryInfo, " *-*-*- > cond_var run after: "+ ag_support->boolToString((proceed_simulation && sim_current_time() < run_until) || exit_simulation));
            logger->print(Logger::EssentialInfo, " - Resume simulation, execute until time t: "+ run_until.str());
        }
        /* ****************************************************************************************************************** */

        if(exit_simulation)
        {
            cancel_all_messages();
            finish();
        }
        else if (selected_sched_type == FCFS || selected_sched_type == RR) {
            handleMessageGP(msg);
        }
        else if(selected_sched_type == EDF) {
            handleMessageRT(msg);

            logger->print(Logger::EveryInfo, "B scheduled:["+ std::to_string(scheduled_msg)
            +"], cancelled:["+ std::to_string(executed_msg)
            +"], pending:["+ (std::to_string(pending_scheduled_messages()) +"]"));

        }
    }
    catch(std::exception const& ex) {
        logger->print(Logger::Error, ex.what());
        ag_metric->finishCrash(true, glob_path, glob_user, ex.what(), "agent", "handleMessage", sim_current_time().dbl());
        ag_metric->addGeneratedError(sim_current_time(), "handleMessage", ex.what());
        exit(EXIT_FAILURE);
    }
}

// General purpose message handling:
void agent::handleMessageGP(cMessage* msg) {
    agentMSG* agMsg = check_and_cast<agentMSG*>(msg);

    // Handle messages sent to 'self'
    if (agMsg->isSelfMessage())
    {
        switch (agMsg->getInformative()) {
        case UPDATE_BELIEF_TIME: { // update the agent belief set when a sensor reading occurs
            update_belief_set_time(agMsg);
            break;
        }
        case AG_REASONING_CYCLE: { // activate BDI agent reasoning cycle
            ag_reasoning_cycle(agMsg);
            break;
        }
        case SCHEDULE: {
            schedule_taskset(agMsg);
            break;
        }
        case AG_TASK_SELECTION_PHII: {
            if(selected_sched_type == FCFS) {
                phiI_FCFS(agMsg);
            }
            else if(selected_sched_type == RR) {
                phiI_RR(agMsg);
            }
            break;
        }
        case ACTIVATE_TASK: {
            activate_task(agMsg);
            break;
        }
        case CHECK_TASK_TERMINATED: {
            check_task_completion(agMsg);
            break;
        }
        case INTERNAL_GOAL_ARRIVAL: { // used whenever phiI needs to manage an internal goal with "arrivalTime" greater than 0
            ag_internal_goal_activation(agMsg);
            break;
        }
        case INTENTION_COMPLETED: { // an intention as been completed...
            wrapper_intention_has_been_completed(agMsg);
            break;
        }
        case RUN_UNTIL_T: { // read the updated values for the belief
            manage_ag_run_until_t(agMsg);
            break;
        }
        default:
            logger->print(Logger::EssentialInfo, "Event with informative: '"+ std::to_string(agMsg->getInformative()) +"' not supported");
            break;
        }
    } // chiude isSelfMessage

    if(agMsg != nullptr) {
        cancelAndDelete(agMsg);
        executed_msg += 1;
    }
}

// Real time message handling
void agent::handleMessageRT(cMessage* msg) {
    agentMSG* agMsg = check_and_cast<agentMSG* >(msg);
    std::pair<bool, json> set_knowledge = std::make_pair(true, json::object());

    //Handle messages sent to 'self'
    if (agMsg->isSelfMessage())
    {
        switch (agMsg->getInformative()) {
        case UPDATE_BELIEF_TIME: { // update the agent belief set when a sensor reading occurs
            update_belief_set_time(agMsg);
            break;
        }
        case AG_REASONING_CYCLE: { // activate BDI agent reasoning cycle
            ag_reasoning_cycle(agMsg);
            break;
        }
        case SCHEDULE: {
            // activation 1: in order to set up the scheduler
            // activation 2: called by the agent in order to manage the execution of intention's tasks
            schedule_taskset_rt(agMsg);
            break;
        }
        case AG_TASK_SELECTION_PHII: {
            // uses EDF to choose which tasks (among the 'top' elements of the available intentions) activate
            phiI(agMsg);
            break;
        }
        case ACTIVATE_TASK: {
            activate_task(agMsg);
            break;
        }
        case PREEMPT: {
            preempt(agMsg);
            break;
        }
        case CHECK_TASK_TERMINATED: {
            check_task_completion_rt(agMsg);
            break;
        }
        case INTERNAL_GOAL_ARRIVAL: { // used whenever phiI needs to manage an internal goal with "arrivalTime" greater than 0
            ag_internal_goal_activation(agMsg);
            break;
        }
        case INTENTION_COMPLETED: { // an intention as been completed...
            wrapper_intention_has_been_completed(agMsg);
            break;
        }
        case RUN_UNTIL_T: { // read the updated values for the belief
            manage_ag_run_until_t(agMsg);
            break;
        }
        default:
            logger->print(Logger::EssentialInfo, "Event with informative: "+ std::to_string(agMsg->getInformative()) +" not supported!");
            break;
        }
    } // end isSelfMessage


    // if the setup of the agent generates an error, terminate the entire simulation
    if(set_knowledge.first == false) {
        if(set_knowledge.second.find("status") != set_knowledge.second.end()) {
            if(set_knowledge.second["status"] == "error") {
                exit(EXIT_FAILURE);
            }
        }
    }

    if(agMsg != nullptr) {
        cancelAndDelete(agMsg);
        executed_msg += 1;
    }
}

// *****************************************************************************
// *******                           FINISH                              *******
// *****************************************************************************
void agent::finish() {
    if(strcmp("DF", getName()) != 0)
    {
        if(function_finish_already_performed == false)
        {
            function_finish_already_performed = true;

            // stop server thread
            stop_background_thread = true;

            // cancel all pending messages
            cancel_all_messages();

            // save metrics...
            ag_metric->finishCrash(false, glob_path, glob_user);
            ag_metric->finishForceSimulationEnd(exit_simulation, glob_path, glob_user, "Force simulation end", "agent", "listening_thread", sim_current_time().dbl());

            // UPDATE 21/07/21: This should never happen, ag_Sched is set up in function 'initialize'
            if(ag_Sched != nullptr) {
                supportFinish();
            }

            // send one last message to the client
            json ack_msg = ack_simulation_completed(sim_current_time().dbl(), simulation_limit_reached(), get_metrics_data());
            send_msg_to_client(ack_msg);

            // Cancel every pending message (if any) ---------------------------------------------
            cancel_all_messages();
            // -----------------------------------------------------------------------------------

            // -----------------------------------------------------------------------------------
            ag_support->computeSimulationEllapsedTime(begin_sim_time);
            // -----------------------------------------------------------------------------------
        }
    }
}

void agent::supportFinish()
{
    ag_metric->finishGeneratedDeadlineMiss(ag_tasks_lateness, getIndex(), selected_sched_type, ag_Sched->get_sched_type_as_string(), par("phiI_prioritize_internal_goals"));
    ag_metric->finishResponseTime(getIndex());
    ag_metric->finishReasoningCyclesStats(getIndex());
    ag_metric->finishPhiIStats(getIndex());

    // Check if simulation time limit has been reached ------------------------------------------------
    cConfigOption simTimeConfig("sim-time-limit", true, cConfigOption::Type::CFG_DOUBLE, "s", "10000", "");
    double maxSimulationTimeLimit = cSimulation::getActiveSimulation()->getEnvir()->getConfig()->getAsDouble(&simTimeConfig);
    bool reached_simulation_time_limit = simulation_limit_reached();

    std::vector<std::shared_ptr<Intention>> invalid_intentions;
    std::vector<std::shared_ptr<Task>> invalid_vector_to_release = ag_Sched->get_tasks_vector_to_release();
    std::vector<std::shared_ptr<Task>> invalid_vector_ready = ag_Sched->get_tasks_vector_ready();

    std::shared_ptr<MaximumCompletionTime> mct;
    std::vector<std::shared_ptr<Plan>> contained_plans;
    std::shared_ptr<Plan> plan;
    std::string msg;
    double first_batch_delta_time, intention_ddl;
    int index_intention = -1;
    bool loop_detected;
    int log_level = logger->get_level();
    /** In FCFS, if the first plan has deadline grater than the simulation limit, then all the other still active intentions are considered "valid".
     * Also valid if the first intention is waiting for completion with time > simulation time.
     * false: for EDF and RR. true: only in FCFS if the first intention is not valid. */
    bool fcfs_first_intention_valid = false; /* Only used with FCFS */

    logger->print(Logger::EssentialInfo, " Check simulation time limit...");
    for(int i = 0; i < (int)ag_intention_set.size(); i++)
    {
        if(ag_selected_plans[i]->getInternalGoalsSelectedPlansSize() > 0)
        {
            plan = ag_selected_plans[i]->getInternalGoalsSelectedPlans()[0].first;

            if(ag_support->checkIfPlanWasAlreadyIntention(plan, index_intention, ag_intention_set))
            {
                first_batch_delta_time = ag_intention_set[index_intention]->get_batch_startTime();

                logger->print(Logger::EveryInfo, "first_batch_delta_time: "+ std::to_string(first_batch_delta_time));
                logger->set_level(Logger::PrintNothing);
                mct = computeGoalDDL(plan, contained_plans, loop_detected, first_batch_delta_time, 0, false, false);
                logger->set_level(log_level);

                if(mct == nullptr) {
                    msg = "[WARNING] Plan:["+ std::to_string(plan->get_id());
                    msg += "], goal:["+ plan->get_goal_name();
                    msg += +"] is NOT valid, no plans for the internal goals!";
                    logger->print(Logger::Warning, msg);

                    invalid_intentions.push_back(std::make_shared<Intention>(*ag_intention_set[index_intention]));
                }
                else {
                    if(ag_intention_set[index_intention]->get_waiting_for_completion()) { // for intention waiting for completion, the ddl is equal to the scheduled completion time
                        intention_ddl = ag_intention_set[index_intention]->get_scheduled_completion_time();
                    }
                    else {
                        intention_ddl = mct->getMCTValue();
                    }
                    msg = " -- Plan:["+ std::to_string(plan->get_id());
                    msg += "], goal:["+ plan->get_goal_name();
                    msg += "], waiting completion:["+ ag_support->boolToString(ag_intention_set[index_intention]->get_waiting_for_completion());
                    msg += "] has deadline: MCT: "+ std::to_string(intention_ddl);

                    if(intention_ddl <= maxSimulationTimeLimit && fcfs_first_intention_valid == false)  /* Only used with FCFS */
                    {
                        logger->print(Logger::EssentialInfo, msg + " --> invalid plan!");
                        invalid_intentions.push_back(std::make_shared<Intention>(*ag_intention_set[index_intention]));
                    }
                    else { // do not count tasks of the valid intentions...
                        logger->print(Logger::EssentialInfo, msg + " --> valid plan...");
                        fcfs_first_intention_valid = true; /* Only used with FCFS */

                        logger->print(Logger::EveryInfo, " Checking if there are tasks to remove from READY vector, due to intention not active anymore...");
                        ag_support->ag_Sched_remove_task_in_queue(invalid_vector_ready, ag_intention_set[index_intention]->get_original_planID());

                        logger->print(Logger::EveryInfo, " Checking if there are tasks to remove from RELEASE vector, due to intention not active anymore...");
                        ag_support->ag_Sched_remove_task_in_queue(invalid_vector_to_release, ag_intention_set[index_intention]->get_original_planID());
                    }
                }
            }
        }
    }
    // ...................

    ag_metric->finishStillActiveElements(invalid_vector_to_release, invalid_vector_ready, invalid_intentions,
            ag_goal_set, ag_belief_set, getIndex(), reached_simulation_time_limit, maxSimulationTimeLimit);
    ag_metric->finishGoalAchieved(getIndex());

    if(reached_simulation_time_limit)
    {
        ag_support->printGoalset(ag_goal_set);

        // Updated data:
        ag_support->printInvalidIntentions(invalid_intentions, ag_goal_set);
        ag_support->ag_Sched_ev_ag_tasks_in_queue(ag_Sched->get_tasks_vector_to_release(), "Release");
        ag_support->ag_Sched_ev_ag_tasks_in_queue(ag_Sched->get_tasks_vector_ready(), "Ready");
        logger->print(Logger::EssentialInfo, "------------------------------------");
    }

    ag_metric->finishUnexpectedTasksExecution(getIndex());
    ag_metric->finishGeneratedErrors(getIndex());
    ag_metric->finishGeneratedWarnings(getIndex());

    // WRITE SIMULATION REPORTS: ----------------------------------------------------
    write_stats_json_report(getIndex(), ag_settings->get_ddl_check(), ag_settings->get_ddl_miss(), glob_path, glob_user, ag_Sched->get_sched_type_as_string());
    write_simulation_timeline_json_report("Deadline miss: "+ std::to_string(ag_tasks_lateness.size()));
    ag_support->finishSaveToFile(glob_path, glob_user, ag_Sched->get_sched_type_as_string());
    ag_metric->finishSaveToFile(glob_path, glob_user, ag_Sched->get_sched_type_as_string());

    ag_metric->printFinish(" ******************************************** ");
}

void agent::throwRuntimeException(bool crashed, std::string folder, std::string user,
        std::string reason, std::string class_name, std::string function, double time)
{
    ag_metric->finishCrash(crashed, folder, user, reason, class_name, function, time);
    ag_metric->addGeneratedError(time, function, reason);
    supportFinish();
    throw std::runtime_error(class_name +", "+ function +", "+ reason);
}

bool agent::simulation_limit_reached()
{   // Check if simulation time limit has been reached ------------------------------------------------
    cConfigOption simTimeConfig("sim-time-limit", true, cConfigOption::Type::CFG_DOUBLE, "s", "300", "");
    double maxSimulationTimeLimit = cSimulation::getActiveSimulation()->getEnvir()->getConfig()->getAsDouble(&simTimeConfig);
    std::pair<simtime_t, std::string> latest_schedule_in_future = ag_support->getScheduleInFuture();

    if(latest_schedule_in_future.first >= maxSimulationTimeLimit) {
        return true;
    }

    return false;
}


