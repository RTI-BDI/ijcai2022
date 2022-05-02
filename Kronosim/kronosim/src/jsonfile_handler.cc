/*
 * jsonfile_handler.cc
 *
 *  Created on: Mar 9, 2020
 *      Author: francesco
 */

#include "../headers/jsonfile_handler.h"

json JsonfileHandler::read_beliefset_from_json(std::string path, std::vector<std::shared_ptr<Belief>>& ag_beliefset,
        int ag_id, std::string received_content, bool read_from_file)
{ // Note: Beliefs with the same name ARE NOT ALLOWED! They also need to be different from the name used for the SKILLSET

    json beliefset;
    json ret_val = json::array(); // list of analyzed data

    std::variant<int, double, bool, std::string> value;
    std::string name = "", warn_err_msg = "";
    std::string agent_id;
    bool alreadyExists = false;
    bool safe_to_read_data = false;

    try {
        agent_id = std::to_string(ag_id);
        if(read_from_file == false)
        {
            if(received_content.empty() == false) {
                beliefset = beliefset.parse(received_content);
                safe_to_read_data = true;
            }
            else {
                warn_err_msg = "[ERROR] Received content for Beliefset is EMPTY!";
                logger->print(Logger::Error, warn_err_msg);
                ret_val.push_back(generate_error_warning_json("error", warn_err_msg));
            }
        }
        else
        {
            std::ifstream i(path.c_str());

            if (!i.fail()) {
                i >> beliefset;
                safe_to_read_data = true;
            }
            else {
                throw std::runtime_error("Could not open file!");
            }
        }

        if(safe_to_read_data)
        {
            for (const auto& belief : beliefset[agent_id])
            {
                if(belief.find("name") != belief.end())
                {
                    name = belief["name"].get<std::string>();

                    alreadyExists = checkIfBeliefAlreadyExists(ag_beliefset, name);
                    if(alreadyExists) {
                        warn_err_msg = "[WARNING] Belief with same name:["+ name +"] already exists! -> ignore it!";
                        logger->print(Logger::Warning, warn_err_msg);
                        ag_metric->addGeneratedWarning(sim_current_time(), "read_beliefset_from_json", warn_err_msg);
                        ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                    }

                    if(belief.find("value") != belief.end())
                    {
                        // get the value stored in belief.value, based on its type
                        if(convertJsonValueToVariant(belief["value"], value) == "undefined")
                        {
                            warn_err_msg = "[WARNING] Belief:["+ name +"] value parameter has invalid type! -> ignore it!";
                            logger->print(Logger::Warning, warn_err_msg);
                            ag_metric->addGeneratedWarning(sim_current_time(), "read_beliefset_from_json", warn_err_msg);
                            ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                        }
                        else {
                            ag_beliefset.push_back(std::make_shared<Belief>(name, value));
                        }
                    }
                    else {
                        warn_err_msg = "[WARNING] Belief:["+ name +"] does not have parameter 'value'! -> ignore it!";
                        logger->print(Logger::Warning, warn_err_msg);
                        ag_metric->addGeneratedWarning(sim_current_time(), "read_beliefset_from_json", warn_err_msg);
                        ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                    }
                }
                else {
                    warn_err_msg = "[WARNING] Belief without parameter 'name'! -> ignore it!";
                    logger->print(Logger::Warning, warn_err_msg);
                    ag_metric->addGeneratedWarning(sim_current_time(), "read_beliefset_from_json", warn_err_msg);
                    ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                }
            }
        }
    }
    catch (std::exception& ex) {
        throw std::runtime_error("[ERROR] While reading file "+ path + ": " + ex.what());
    }

    return ret_val;
}

json JsonfileHandler::read_desireset_from_json(std::string path,
        const std::vector<std::shared_ptr<Belief>>& beliefset,
        const std::vector<std::shared_ptr<Skill>>& skillset,
        std::vector<std::shared_ptr<Desire>>& ag_desireset, int ag_id, std::string received_content, bool read_from_file)
{   // Desires with the same skill ARE ALLOWED! Desires linked to invalid beliefs or skill ARE NOT!

    json desireset;
    json ret_val = json::array(); // list of analyzed data
    std::string agent_id;

    bool correctly_setup = true;
    bool safe_to_read_data = false;
    double ddl;
    std::string beliefType_message = "";
    std::string goal_name = "";
    std::string warn_err_msg = "";
    std::shared_ptr<Desire> new_desire;
    std::shared_ptr<Skill> skill;
    json preconditions, cont_conditions, formula;
    // Reference table variables ----------------------
    std::shared_ptr<Belief> rt_belief;
    std::string rt_belief_name;
    std::variant<int, double, bool, std::string> rt_threshold;
    double rt_priority;
    Operator::Type rt_op;
    std::vector<std::shared_ptr<Reference_table>> reference_table;
    // ------------------------------------------------

    try {
        agent_id = std::to_string(ag_id);
        if(read_from_file == false)
        {
            if(received_content.empty() == false) {
                desireset = desireset.parse(received_content);
                safe_to_read_data = true;
            }
            else {
                warn_err_msg = "[ERROR] Received content for Desireset is EMPTY!";
                logger->print(Logger::Error, warn_err_msg);
                ret_val.push_back(generate_error_warning_json("error", warn_err_msg));
            }
        }
        else
        {
            std::ifstream i(path.c_str());

            if (!i.fail()) {
                i >> desireset;
                safe_to_read_data = true;
            }
            else {
                throw std::runtime_error("Could not open file!");
            }
        }

        if(safe_to_read_data)
        {
            for (const auto& desire : desireset[agent_id])
            {
                beliefType_message = "";
                correctly_setup = true;
                reference_table.clear();

                if(desire.find("goal_name") == desire.end()) {
                    warn_err_msg = "[WARNING] Desire does not have parameter 'goal_name'! -> delete desire";
                    logger->print(Logger::Warning, warn_err_msg);
                    ag_metric->addGeneratedWarning(sim_current_time(), "read_desireset_from_json", warn_err_msg);
                    ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                }
                else
                {
                    goal_name = desire["goal_name"].get<std::string>();

                    skill = findSkillWithGivenName(skillset, goal_name);
                    if(skill == nullptr)
                    {   // even if the desire is not linked to a valid skill, such skill may be added to the knowledge in the future -> thus keep it stored
                        warn_err_msg = "[WARNING] Desire:["+ goal_name +"], id:["+ std::to_string(desire_goal_id) +"] is linked to an invalid skill! -> keep it anyway...";
                        logger->print(Logger::Warning, warn_err_msg);
                        ag_metric->addGeneratedWarning(sim_current_time(), "read_desireset_from_json", warn_err_msg);
                        ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                    }

                    // Desire.preconditions-----------------------------------------------------------------
                    if(desire.find("preconditions") != desire.end()) {
                        preconditions = desire["preconditions"];
                    }
                    else {
                        warn_err_msg = "[WARNING] Desire:["+ goal_name +"], Parameter 'preconditions' not found! Initialized as empty array:[]";
                        logger->print(Logger::Warning, warn_err_msg);
                        ag_metric->addGeneratedWarning(sim_current_time(), "read_desireset_from_json", warn_err_msg);
                        ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));

                        preconditions = json::array();
                    }
                    // -------------------------------------------------------------------------------------

                    // Desire.cont_conditions---------------------------------------------------------------
                    if(desire.find("cont_conditions") != desire.end()) {
                        cont_conditions = desire["cont_conditions"];
                    }
                    else {
                        warn_err_msg = "[WARNING] Desire:["+ goal_name +"], Parameter 'cont_conditions' not found! Initialized as empty array:[]";
                        logger->print(Logger::Warning, warn_err_msg);
                        ag_metric->addGeneratedWarning(sim_current_time(), "read_desireset_from_json", warn_err_msg);
                        ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));

                        cont_conditions = json::array();
                    }
                    // -------------------------------------------------------------------------------------

                    // Desire.priority, dynamic value ------------------------------------------------------
                    if(desire.find("priority") != desire.end())
                    {
                        if(desire["priority"].contains("formula")) // use contains for objects, find for arrays
                        {
                            formula = desire["priority"]["formula"];
                        }
                        else {
                            warn_err_msg = "[WARNING] Desire:["+ goal_name +"], Parameter 'priority.formula' not found! Initialized as empty array:[ 0 ]";
                            logger->print(Logger::Warning, warn_err_msg);
                            ag_metric->addGeneratedWarning(sim_current_time(), "read_desireset_from_json", warn_err_msg);
                            ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));

                            formula = json::array();
                            formula.push_back(0);
                        }
                    }
                    else {
                        warn_err_msg = "[WARNING] Desire:["+ goal_name +"], Parameter 'priority' not found for Desire:["+ goal_name +"] -> delete desire";
                        logger->print(Logger::Warning, warn_err_msg);
                        ag_metric->addGeneratedWarning(sim_current_time(), "read_desireset_from_json", warn_err_msg);
                        ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));

                        correctly_setup = false;
                    }
                    // -------------------------------------------------------------------------------------

                    if(correctly_setup)
                    {
                        // Desire.priority, reference table ----------------------------------------------------
                        if(desire["priority"].contains("reference_table") == false)
                        {
                            warn_err_msg = "[WARNING] Desire:["+ goal_name +"], Parameter 'priority.reference_table' not found! Initialized as empty array:[]";
                            logger->print(Logger::Warning, warn_err_msg);
                            ag_metric->addGeneratedWarning(sim_current_time(), "read_desireset_from_json", warn_err_msg);
                            ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));

                            reference_table.clear();
                        }
                        else {
                            if(desire["priority"]["reference_table"].size() > 0)
                            {
                                for(json element : desire["priority"]["reference_table"])
                                {
                                    rt_belief_name = element["belief_name"].get<std::string>();
                                    convertJsonValueToVariant(element["threshold"], rt_threshold);
                                    rt_priority = element["priority"].get<double>();
                                    rt_op = convertStringToOperator(element["op"].get<std::string>());

                                    try {
                                        rt_belief = findBeliefWithGivenName(beliefset, rt_belief_name);

                                        if(rt_belief == nullptr)
                                        {
                                            throw std::runtime_error("The specified belief:[id="+ rt_belief_name
                                                    +"], for Desire:[id="+ std::to_string(desire_goal_id)
                                            +"] is not valid. Can not create a record in the reference_table. -> delete desire");
                                        }
                                        else {
                                            // the belief is present in the beliefset...
                                            reference_table.push_back(std::make_shared<Reference_table>(
                                                    rt_belief->get_name(), rt_threshold, rt_priority, rt_op
                                            ));
                                        }
                                    }
                                    catch (std::exception& ex) {
                                        throw std::runtime_error("The specified 'operator' is not valid. -> delete desire");
                                    }
                                }
                            }
                        }
                        // -------------------------------------------------------------------------------------

                        if(desire.find("deadline") != desire.end())
                        {
                            ddl = desire["deadline"].get<double>();
                            if(ddl >= 0) {
                                new_desire = std::make_shared<Desire>(desire_goal_id, goal_name, preconditions, cont_conditions, ddl);
                            }
                            else { // ddl set to default value: -1
                                new_desire = std::make_shared<Desire>(desire_goal_id, goal_name, preconditions, cont_conditions);
                            }

                            // add few more elements to the desire:
                            if(desire["priority"].contains("computedDynamically") == false)
                            {
                                warn_err_msg = "[WARNING] Desire:["+ goal_name +"], Parameter 'priority.computedDynamically' not found! Set to false.";
                                logger->print(Logger::Warning, warn_err_msg);
                                ag_metric->addGeneratedWarning(sim_current_time(), "read_desireset_from_json", warn_err_msg);
                                ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));

                                new_desire->set_computedDynamically(false);
                            }
                            else {
                                new_desire->set_computedDynamically(desire["priority"]["computedDynamically"].get<bool>());
                            }

                            new_desire->set_priority_formula(formula);
                            new_desire->set_priority_reference_table(reference_table);
                            // -------------------------------------------------------------------------------------

                            ag_desireset.push_back(new_desire);
                            desire_goal_id += 1;
                        }
                        else {
                            warn_err_msg = "[WARNING] Desire:["+ goal_name +"], Parameter 'deadline' not found! -> delete desire";
                            logger->print(Logger::Warning, warn_err_msg);
                            ag_metric->addGeneratedWarning(sim_current_time(), "read_desireset_from_json", warn_err_msg);
                            ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                        }
                    }
                }
            }
        }
    }
    catch (std::exception& ex) {
        throw std::runtime_error("[ERROR] While reading file "+ path + ": " + ex.what());
    }

    return ret_val;
}

json JsonfileHandler::read_planset_from_json(std::string path,
        const std::vector<std::shared_ptr<Belief>>& beliefset,
        const std::vector<std::shared_ptr<Skill>>& skillset,
        std::vector<std::shared_ptr<Plan>>& ag_plans,
        const std::vector<std::shared_ptr<Server>>& servers,
        int ag_id, const std::string schedule_type_name, std::string received_content, bool read_from_file)
{   // Plans with the same skill ARE ALLOWED! Plans linked to invalid beliefs or skill ARE NOT!

    json plans;
    json ret_val = json::array(); // list of analyzed data
    std::string agent_id;

    bool safe_to_read_data = false;
    std::string beliefType_message = "";
    std::string goal_name = "";
    std::string warn_err_msg = "";
    int task_unique_id; // used to set up the task_id within plans
    json no_preconditions;
    json formula;
    json preconditions, cont_conditions, post_conditions;
    std::shared_ptr<Skill> skill;

    // Reference table variables ----------------------
    std::shared_ptr<Belief> rt_belief;
    std::string rt_belief_name;
    std::variant<int, double, bool, std::string> rt_threshold;
    double rt_pref;
    Operator::Type rt_op;
    std::vector<std::shared_ptr<Reference_table>> reference_table;
    // ------------------------------------------------

    // Plan.body variables ----------------------------
    std::vector<std::shared_ptr<Action>> body_actions;
    std::shared_ptr<Action> action;
    std::shared_ptr<Goal> body_goal;
    std::shared_ptr<Task> body_task;
    std::shared_ptr<Skill> action_skill;
    json action_priority;
    Goal::Status action_status;
    Goal::Execution goal_exec_type;
    bool isFirstActionInBody = true;
    int cnt_body = 0;
    double action_ddl, action_arrivalTime;
    std::string action_goal_name = "";
    // ------------------------------------------------

    try
    {
        agent_id = std::to_string(ag_id);
        if(read_from_file == false)
        {
            if(received_content.empty() == false) {
                plans = plans.parse(received_content);
                safe_to_read_data = true;
            }
            else {
                warn_err_msg = "[ERROR] Received content for Planset is EMPTY!";
                logger->print(Logger::Error, warn_err_msg);
                ret_val.push_back(generate_error_warning_json("error", warn_err_msg));
            }
        }
        else
        {
            std::ifstream i(path.c_str());

            if (!i.fail()) {
                i >> plans;
                safe_to_read_data = true;
            }
            else {
                throw std::runtime_error("Could not open file!");
            }
        }

        if(safe_to_read_data)
        {
            for (const auto& plan : plans[agent_id])
            {
                task_unique_id = 0;
                beliefType_message = "";
                reference_table.clear();
                body_actions.clear();

                // Check if the Plan is linked to an existing Skill -------------------------------------------------------------------------
                if(plan.find("goal_name") != plan.end())
                {
                    goal_name = plan["goal_name"].get<std::string>();

                    skill = findSkillWithGivenName(skillset, goal_name);
                    if(skill == nullptr) {
                        warn_err_msg = "[WARNING] Plan:["+ goal_name +"], id:["+ std::to_string(plan_id) +"] is linked to an invalid skill! -> keep it anyway...";
                        logger->print(Logger::Warning, warn_err_msg);
                        ag_metric->addGeneratedWarning(sim_current_time(), "read_planset_from_json", warn_err_msg);
                        ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                    }

                    // Plan.pref, dynamic value ------------------------------------------------------------
                    if(plan.find("preference") == plan.end())
                    {
                        warn_err_msg = "[WARNING] Plan:["+ goal_name +"] has no parameter 'preference'! -> deleted!";
                        logger->print(Logger::Warning, warn_err_msg);
                        ag_metric->addGeneratedWarning(sim_current_time(), "read_planset_from_json", warn_err_msg);
                        ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                    }
                    else
                    {
                        if(plan["preference"].contains("formula"))
                        {
                            formula = plan["preference"]["formula"];
                        }
                        else {
                            warn_err_msg = "[WARNING] Plan:["+ goal_name +"], Parameter 'preference.formula' not found! Initialized as empty array:[ 0 ]";
                            logger->print(Logger::Warning, warn_err_msg);
                            ag_metric->addGeneratedWarning(sim_current_time(), "read_planset_from_json", warn_err_msg);
                            ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));

                            formula = json::array();
                            formula.push_back(0);
                        }

                        // -------------------------------------------------------------------------------------

                        // Plan.preference, reference table ----------------------------------------------------
                        if(plan["preference"].contains("reference_table") == false) {
                            warn_err_msg = "[WARNING] Plan:["+ goal_name +"], Parameter 'preference.reference_table' not found! Initialized as empty array:[]";
                            logger->print(Logger::Warning, warn_err_msg);
                            ag_metric->addGeneratedWarning(sim_current_time(), "read_planset_from_json", warn_err_msg);
                            ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));

                            reference_table.clear();
                        }
                        else {
                            if(plan["preference"]["reference_table"].size() > 0)
                            {
                                for(json element : plan["preference"]["reference_table"])
                                {
                                    rt_belief_name = element["belief_name"].get<std::string>();
                                    convertJsonValueToVariant(element["threshold"], rt_threshold);
                                    rt_pref = element["pref"].get<double>();
                                    rt_op = convertStringToOperator(element["op"].get<std::string>());

                                    rt_belief = findBeliefWithGivenName(beliefset, rt_belief_name);

                                    if(rt_belief == nullptr) {
                                        throw std::runtime_error("Plan:["+ goal_name +"]: the specified belief:[name="+ rt_belief_name
                                                +"], for plan:[id="+ std::to_string(plan_id)
                                        +"] is not valid. Can not create a record in the reference_table.");
                                    }
                                    else {
                                        // the belief is present in the beliefset...
                                        reference_table.push_back(std::make_shared<Reference_table>(
                                                rt_belief->get_name(), rt_threshold, rt_pref, rt_op
                                        ));
                                    }
                                }
                            }
                        }
                        // -------------------------------------------------------------------------------------

                        // Plan.preconditions, cont_conditions and post_conditions -----------------------------
                        if(plan.find("preconditions") != plan.end()) {
                            preconditions = plan["preconditions"];
                        }
                        else {
                            warn_err_msg = "[WARNING] Plan:["+ goal_name +"], Parameter 'preconditions' not found! Initialized as empty array:[]";
                            logger->print(Logger::Warning, warn_err_msg);
                            ag_metric->addGeneratedWarning(sim_current_time(), "read_planset_from_json", warn_err_msg);
                            ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));

                            preconditions = json::array();
                        }

                        if(plan.find("cont_conditions") != plan.end()) {
                            cont_conditions = plan["cont_conditions"];
                        }
                        else {
                            warn_err_msg = "[WARNING] Plan:["+ goal_name +"], Parameter 'cont_conditions' not found! Initialized as empty array:[]";
                            logger->print(Logger::Warning, warn_err_msg);
                            ag_metric->addGeneratedWarning(sim_current_time(), "read_planset_from_json", warn_err_msg);
                            ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));

                            cont_conditions = json::array();
                        }

                        if(plan.find("post_conditions") != plan.end()) {
                            post_conditions = plan["post_conditions"];
                        }
                        else {
                            warn_err_msg = "[WARNING] Plan:["+ goal_name +"], Parameter 'post_conditions' not found! Initialized as empty array:[]";
                            logger->print(Logger::Warning, warn_err_msg);
                            ag_metric->addGeneratedWarning(sim_current_time(), "read_planset_from_json", warn_err_msg);
                            ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));

                            post_conditions = json::array();
                        }
                        // -------------------------------------------------------------------------------------

                        // Plan.effects_at_begin ---------------------------------------------------------------
                        std::vector<std::shared_ptr<Effect>> effects_at_begin;

                        if(plan.find("effects_at_begin") != plan.end()) {
                            if(plan["effects_at_begin"].size() > 0) {
                                for(json effect : plan["effects_at_begin"]) {
                                    effects_at_begin.push_back(std::make_shared<Effect>(effect));
                                }
                            }
                        }
                        else {
                            warn_err_msg = "[WARNING] Plan:["+ goal_name +"], Parameter 'effects_at_begin' not found! Initialized as empty array:[]";
                            logger->print(Logger::Warning, warn_err_msg);
                            ag_metric->addGeneratedWarning(sim_current_time(), "read_planset_from_json", warn_err_msg);
                            ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                        }
                        // -------------------------------------------------------------------------------------

                        // Plan.effects_at_end -----------------------------------------------------------------
                        std::vector<std::shared_ptr<Effect>> effects_at_end;

                        if(plan.find("effects_at_end") != plan.end()) {
                            if(plan["effects_at_end"].size() > 0) {
                                for(json effect : plan["effects_at_end"]) {
                                    effects_at_end.push_back(std::make_shared<Effect>(effect));
                                }
                            }
                        }
                        else {
                            warn_err_msg = "[WARNING] Plan:["+ goal_name +"], Parameter 'effects_at_end' not found! Initialized as empty array:[]";
                            logger->print(Logger::Warning, warn_err_msg);
                            ag_metric->addGeneratedWarning(sim_current_time(), "read_planset_from_json", warn_err_msg);
                            ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                        }
                        // -------------------------------------------------------------------------------------

                        // Plan.body ---------------------------------------------------------------------------
                        isFirstActionInBody = true;
                        cnt_body = 0;
                        if(plan.find("body") != plan.end())
                        {
                            if(plan["body"].size() > 0)
                            {
                                for(json action : plan["body"])
                                {
                                    std::string action_type = action["action_type"].get<std::string>();
                                    action_type = boost::to_upper_copy(action_type); // convert string to upper case

                                    if(action_type == "GOAL")
                                    {
                                        // Get the goal id. Then, get the belief linked to such goal
                                        action_goal_name = action["action"]["goal_name"].get<std::string>();
                                        action_skill = findSkillWithGivenName(skillset, action_goal_name);
                                        if(action_skill == nullptr)
                                        {
                                            warn_err_msg = "[WARNING] Plan:["+ goal_name +"], Internal goal:["+ std::to_string(cnt_body);
                                            warn_err_msg+= " of "+ std::to_string(plan["body"].size()) +"] is not valid, it is not linked to a valid Skill:["+ action_goal_name +"]! -> keep it anyway...";
                                            logger->print(Logger::Warning, warn_err_msg);
                                            ag_metric->addGeneratedWarning(sim_current_time(), "read_planset_from_json", warn_err_msg);
                                            ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                                        }

                                        // Internal goal deadline: ----------------------------------------
                                        action_ddl = action["action"]["deadline"].get<double>();
                                        // ----------------------------------------------------------------
                                        // Internal goal activation time: ---------------------------------
                                        action_arrivalTime = action["action"]["arrivalTime"].get<double>();

                                        if(action_arrivalTime < 0) {
                                            action_arrivalTime = 0;
                                            warn_err_msg = "[WARNING] Plan:["+ goal_name +"], Internal goal:["+ action_goal_name;
                                            warn_err_msg+= "] has invalid arrivalTime ("+ std::to_string(action_arrivalTime) +")! Set to 0.";
                                            logger->print(Logger::Warning, warn_err_msg);
                                            ag_metric->addGeneratedWarning(sim_current_time(), "read_planset_from_json", warn_err_msg);
                                            ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                                        }
                                        // ----------------------------------------------------------------

                                        // Check for loops within the plan: -------------------------------
                                        if(goal_name == action_goal_name) {
                                            throw std::runtime_error("Loop detected! Plan:["+ std::to_string(plan_id)
                                            +"] for goal:["+ goal_name
                                            +"] and internal goal:["+ action_goal_name
                                            +"] can not contain internal goals with same id:["+ std::to_string(desire_goal_id) +"]");
                                        }
                                        // ----------------------------------------------------------------

                                        // Priority expression ------------------------------
                                        action_priority = action["action"]["priority"];
                                        // --------------------------------------------------

                                        // Set task execution type --------------------------
                                        action_status = Goal::IDLE;
                                        if(isFirstActionInBody) {
                                            isFirstActionInBody = false;
                                            goal_exec_type = Goal::EXEC_SEQUENTIAL;
                                        }
                                        else {
                                            goal_exec_type = convertStringToGoalExecutionType(action["execution"].get<std::string>(), schedule_type_name);
                                        }
                                        // --------------------------------------------------

                                        body_goal = std::make_shared<Goal>(
                                                desire_goal_id,
                                                plan_id, plan_id,
                                                action_goal_name,       // skill.goal_name this internal goal referrs to
                                                no_preconditions,       // no preconditions for internal goals
                                                no_preconditions,       // no cont_conditions for internal goals
                                                -1,                     // goal priority not computed yet
                                                -1,                     // activation time
                                                action_arrivalTime,     // delay between activation and when it can become an intention
                                                action_status, action_ddl, goal_exec_type);
                                        body_goal->set_priority_formula(action_priority); // set the expression that will define the "priority" of this goal

                                        body_actions.push_back(body_goal);
                                        desire_goal_id += 1;
                                    }
                                    else if(action_type == "TASK")
                                    {
                                        json task = action["action"];
                                        int t_id = task_unique_id++; // automatically set task id
                                        int t_plan_id = plan_id;
                                        int t_ag_executor = 0;
                                        int t_ag_demander = 0;
                                        double t_comp = task["computationTime"].get<double>();
                                        double t_CRes = t_comp;
                                        double t_R = 0; // t_R = task "arrivalTime"
                                        double t_DDL = task["relativeDeadline"].get<double>();
                                        double t_period = task["period"].get<double>();
                                        int t_n_exec = task["n_exec"].get<int>();
                                        double t_first_activation = -1;
                                        double t_last_activation = -1;
                                        int t_server = task["server"].get<int>();
                                        Task::Execution execution_type;

                                        if(t_n_exec == 0 || t_n_exec < -1)
                                        {
                                            warn_err_msg = "[WARNING] In Plan:["+ std::to_string(plan_id)
                                            +"], name:["+ goal_name
                                            +"] task:["+ std::to_string(t_id)
                                            +"] has an invalid \"n_exec\" value ("+ std::to_string(t_n_exec)
                                            +")! Must be = -1 or >= 1!";
                                            logger->print(Logger::Warning, warn_err_msg);
                                            ag_metric->addGeneratedWarning(sim_current_time(), "read_planset_from_json", warn_err_msg);
                                            ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                                            break;
                                        }
                                        else
                                        {
                                            if((t_server == -1 || schedule_type_name.compare("EDF") != 0) && t_comp > t_DDL)
                                            {
                                                warn_err_msg = "[WARNING] Plan:["+ goal_name +"], Periodic task:["+ std::to_string(t_id)
                                                +"], plan:["+ std::to_string(t_plan_id)
                                                +"], goal name:["+ goal_name
                                                +"] has computationTime greater than period ("+ std::to_string(t_comp)
                                                +" > "+ std::to_string(t_DDL) +")!";
                                                logger->print(Logger::Warning, warn_err_msg);
                                                ag_metric->addGeneratedWarning(sim_current_time(), "read_planset_from_json", warn_err_msg);
                                                ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                                            }

                                            if(isFirstActionInBody) {
                                                isFirstActionInBody = false;
                                                execution_type = Task::EXEC_SEQUENTIAL;
                                            }
                                            else {
                                                execution_type = convertStringToTaskExecutionType(action["execution"].get<std::string>(), schedule_type_name);
                                            }

                                            if(schedule_type_name == "EventsGenerator")
                                            {   // simply copy the task without changing any value
                                                body_task = std::make_shared<Task>(t_id, t_plan_id, t_plan_id, t_ag_executor, t_ag_demander,
                                                        t_comp, t_CRes, t_R, t_R, t_DDL, t_first_activation,
                                                        t_last_activation, t_server, t_period,
                                                        t_n_exec, t_n_exec, false, false, Task::TSK_IDLE, execution_type);
                                                body_actions.push_back(body_task);
                                            }
                                            else if(schedule_type_name == "FCFS")
                                            {
                                                /* In FCFS SERVERs DO NOT exit! + Each task must be considered as SEQUENTIAL.
                                                 * Note: in FCFS each task is considered PERIODIC, even if N_exec = 1.
                                                 *      This allows us to ignore servers in computeGoalDDL and exploit the period of tasks as deadlines
                                                 *      THIS IS WHY nt_isPeriod = true. */

                                                t_server = -1; // in FCFS servers are not available
                                                body_task = std::make_shared<Task>(t_id, t_plan_id, t_plan_id, t_ag_executor, t_ag_demander,
                                                        t_comp, t_CRes, t_R, t_R, t_DDL, t_first_activation,
                                                        t_last_activation, t_server, t_period,
                                                        t_n_exec, t_n_exec, false, true, Task::TSK_IDLE, Task::EXEC_SEQUENTIAL);
                                                body_actions.push_back(body_task);
                                            }
                                            else if(schedule_type_name == "RR") {
                                                /* In RR SERVERs DO NOT exit! We use QUANTUM for both periodic and aperiodic tasks!
                                                 * Note: in RR each task is considered PERIODIC, even if N_exec = 1.
                                                 *      This allows us to ignore servers in usecomputeGoalDDL and exploit the period of tasks as deadlines
                                                 *      THIS IS WHY nt_isPeriod = true. */

                                                t_server = -1; // in FCFS servers are not available
                                                body_task = std::make_shared<Task>(t_id, t_plan_id, t_plan_id, t_ag_executor, t_ag_demander,
                                                        t_comp, t_CRes, t_R, t_R, t_DDL, t_first_activation,
                                                        t_last_activation, t_server, t_period,
                                                        t_n_exec, t_n_exec, false, true, Task::TSK_IDLE, execution_type);
                                                body_actions.push_back(body_task);
                                            }
                                            else
                                            { // schedule_type_name must be: EDF
                                                /* In order to make EDF behave correctly:
                                                 * - Periodic tasks(n_exec = -1 || n_exec > 1) MUST have server = -1
                                                 * - APeriodic tasks(n_exec = 1) MUST have server != -1 */
                                                if(t_server != -1)
                                                { // task is APERIODIC
                                                    if(t_n_exec == 1)
                                                    { // correctly set up
                                                        int tmp_server_id = getServerIndexInAg_servers(servers, t_server);

                                                        if(tmp_server_id != -1)
                                                        {
                                                            if(t_comp > servers[tmp_server_id]->get_budget())
                                                            {
                                                                warn_err_msg = "[WARNING] APERIODIC Task:["+ std::to_string(t_id)
                                                                +"], plan id:["+ std::to_string(t_plan_id)
                                                                +"], original plan id:["+ std::to_string(t_plan_id)
                                                                +"], compTime:["+ std::to_string(t_comp)
                                                                +"] is GREATER than server.budget:["+ std::to_string(servers[tmp_server_id]->get_budget())
                                                                +"] -> server reset mechanism might lead to deadline miss!";
                                                                logger->print(Logger::Warning, warn_err_msg);
                                                                ag_metric->addGeneratedWarning(sim_current_time(), "read_planset_from_json", warn_err_msg);
                                                                ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                                                            }

                                                            t_DDL = servers[tmp_server_id]->get_curr_ddl();

                                                            body_task = std::make_shared<Task>(t_id, t_plan_id, t_plan_id, t_ag_executor, t_ag_demander,
                                                                    t_comp, t_CRes, t_R, t_R, t_DDL, t_first_activation,
                                                                    t_last_activation, t_server, t_period,
                                                                    t_n_exec, t_n_exec, false, false, Task::TSK_IDLE, execution_type);
                                                            body_actions.push_back(body_task);
                                                        }
                                                        else {
                                                            warn_err_msg = "[WARNING] Plan:["+ goal_name +"], APERIODIC tasks:[id="+ std::to_string(t_id) +"], plan:[id="
                                                                    +std::to_string(plan_id) +"] must have a valid server! (id:"+ std::to_string(t_server) +" is not) -> deleted!";
                                                            logger->print(Logger::Warning, warn_err_msg);
                                                            ag_metric->addGeneratedWarning(sim_current_time(), "read_planset_from_json", warn_err_msg);
                                                            ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                                                        }
                                                    }
                                                    else {
                                                        warn_err_msg = "[WARNING] Plan:["+ goal_name +"], PERIODIC tasks:[id="+ std::to_string(t_id) +"], plan:[id="
                                                                +std::to_string(plan_id) +"] must have server = -1! -> deleted!";
                                                        logger->print(Logger::Warning, warn_err_msg);
                                                        ag_metric->addGeneratedWarning(sim_current_time(), "read_planset_from_json", warn_err_msg);
                                                        ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                                                    }
                                                }
                                                else
                                                { // task is PERIODIC
                                                    if(t_n_exec > 1 || t_n_exec == -1)
                                                    {
                                                        body_task = std::make_shared<Task>(t_id, t_plan_id, t_plan_id, t_ag_executor, t_ag_demander,
                                                                t_comp, t_CRes, t_R, t_R, t_DDL, t_first_activation,
                                                                t_last_activation, t_server, t_period,
                                                                t_n_exec, t_n_exec, false, true, Task::TSK_IDLE, execution_type);
                                                        body_actions.push_back(body_task);
                                                    }
                                                    else {
                                                        warn_err_msg = "[WARNING] Plan:["+ goal_name +"], Periodic tasks:[id="+ std::to_string(t_id) +"], plan:[id="
                                                                +std::to_string(plan_id) +"] must have server = -1! -> deleted!";
                                                        logger->print(Logger::Warning, warn_err_msg);
                                                        ag_metric->addGeneratedWarning(sim_current_time(), "read_planset_from_json", warn_err_msg);
                                                        ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                                                    }
                                                }
                                            }
                                        }
                                        // -------------------------------------------------------------------------------
                                    }
                                    cnt_body += 1;
                                }

                                if((int)body_actions.size() > 0)
                                {
                                    // Plan.cost_function ------------------------------------------------------------------
                                    double pref = 0; // it will be update to the correct value whenever the plan is selected as 'applicable  plan' in function 'unifyGoals'
                                    std::shared_ptr<Plan> new_plan = std::make_shared<Plan>(plan_id,
                                            goal_name,
                                            pref, preconditions, cont_conditions, post_conditions,
                                            effects_at_begin, effects_at_end,
                                            body_actions);

                                    // add few more elements to the plan:
                                    if(plan["preference"].contains("computedDynamically") == false)
                                    {
                                        warn_err_msg = "[WARNING] Plan:["+ goal_name +"], Parameter 'preference.computedDynamically' not found! Initialized as empty array:[]";
                                        logger->print(Logger::Warning, warn_err_msg);
                                        ag_metric->addGeneratedWarning(sim_current_time(), "read_planset_from_json", warn_err_msg);
                                        ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));

                                        new_plan->set_computedDynamically(false);
                                    }
                                    else {
                                        new_plan->set_computedDynamically(plan["preference"]["computedDynamically"].get<bool>());
                                    }
                                    new_plan->set_pref_formula(formula);
                                    new_plan->set_pref_reference_table(reference_table);

                                    ag_plans.push_back(new_plan);
                                    plan_id += 1;
                                }
                                else {
                                    warn_err_msg = "[WARNING] Plan:["+ goal_name +"], has empty body -> deleted!";
                                    logger->print(Logger::Warning, warn_err_msg);
                                    ag_metric->addGeneratedWarning(sim_current_time(), "read_planset_from_json", warn_err_msg);
                                    ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                                }
                            }
                            else
                            {
                                warn_err_msg = "[WARNING] Plan:["+ goal_name +"] HAS NO ACTIONS DEFINED! -> deleted!";
                                logger->print(Logger::Warning, warn_err_msg);
                                ag_metric->addGeneratedWarning(sim_current_time(), "read_planset_from_json", warn_err_msg);
                                ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                            }
                        }
                        else {
                            warn_err_msg = "[WARNING] Plan:["+ goal_name +"] HAS NO BODY DEFINED! -> deleted!";
                            logger->print(Logger::Warning, warn_err_msg);
                            ag_metric->addGeneratedWarning(sim_current_time(), "read_planset_from_json", warn_err_msg);
                            ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                        }
                    }
                }
                else {
                    warn_err_msg = "[WARNING] Plan:[id="+ std::to_string(plan_id) +"] does not have parameter 'goal_name'! -> deleted!";
                    logger->print(Logger::Warning, warn_err_msg);
                    ag_metric->addGeneratedWarning(sim_current_time(), "read_planset_from_json", warn_err_msg);
                    ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                }
            }
        }
    }
    catch (std::exception& ex) {
        throw std::runtime_error("[ERROR] While reading file "+ path + ": " + ex.what());
    }

    return ret_val;
}

json JsonfileHandler::read_sensors_from_json(std::string path,
        std::vector<std::shared_ptr<Sensor>>& ag_sensors_time, int ag_id, std::string received_content, bool read_from_file)
{
    json read_sensors; // sensors values
    json ret_val = json::array(); // list of analyzed data
    std::string agent_id, str_value;

    bool safe_to_read_data = false;
    std::string warn_err_msg = "";
    std::variant<int, double, bool, std::string> value;

    try {
        agent_id = std::to_string(ag_id);
        if(read_from_file == false)
        {
            if(received_content.empty() == false) {
                read_sensors = read_sensors.parse(received_content);
                safe_to_read_data = true;
            }
            else {
                warn_err_msg = "[ERROR] Received content for Sensors is EMPTY!";
                logger->print(Logger::Error, warn_err_msg);
                ret_val = generate_error_warning_json("error", warn_err_msg);
            }
        }
        else
        {
            std::ifstream i(path.c_str());

            if (!i.fail()) {
                i >> read_sensors;
                safe_to_read_data = true;
            }
            else {
                safe_to_read_data = false;
                warn_err_msg = "[WARNING] File 'sensors.json' not found! ";
                logger->print(Logger::Warning, warn_err_msg);
                ag_metric->addGeneratedWarning(sim_current_time(), "read_sensors_from_json", warn_err_msg);

                ret_val = generate_error_warning_json("warning", warn_err_msg);
            }
        }

        if(safe_to_read_data)
        {
            for (const auto& sensor : read_sensors[agent_id])
            {
                if(sensor.find("belief_name") != sensor.end())
                {
                    std::string belief_name = sensor["belief_name"].get<std::string>();
                    if(belief_name.empty() == false && boost::replace_all_copy(belief_name, " ", "").size() > 0)
                    {
                        if(sensor.find("value") != sensor.end())
                        {
                            // get the value stored in read_value, based on its type
                            convertJsonValueToVariant(sensor["value"], value);

                            if(sensor.find("time") != sensor.end())
                            {
                                double time = sensor["time"].get<double>();
                                if(time >= sim_current_time().dbl())
                                {
                                    if(sensor.find("mode") != sensor.end())
                                    {
                                        std::string mode_string = sensor["mode"].get<std::string>();
                                        Sensor::Mode mode = convertStringToSensorMode(mode_string);

                                        if(time >= 0) {
                                            ag_sensors_time.push_back(std::make_shared<Sensor>(sensor_read_id, belief_name, value, time, mode));
                                            sensor_read_id += 1;
                                        }
                                        else {
                                            warn_err_msg = "[WARNING] Sensor reading:[id="+ std::to_string(sensor_read_id) +"], name:["+ belief_name
                                                    +"] has invalid 'activation time' (t = "+ std::to_string(time) +")! -> ignored it!";
                                            logger->print(Logger::Warning, warn_err_msg);
                                            ag_metric->addGeneratedWarning(sim_current_time(), "read_sensors_from_json", warn_err_msg);

                                            ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                                        }
                                    }
                                    else {
                                        warn_err_msg = "[WARNING] Sensor:["+ belief_name +"] does not have parameter 'mode' -> ignored it!";
                                        logger->print(Logger::Warning, warn_err_msg);
                                        ag_metric->addGeneratedWarning(sim_current_time(), "read_sensors_from_json", warn_err_msg);

                                        ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                                    }
                                }
                                else {
                                    warn_err_msg = "[WARNING] t: "+ std::to_string(sim_current_time().dbl()) +" Sensor:["+ belief_name +"] has activation time:["+ std::to_string(time) +"] in the past! -> ignored it!";
                                    logger->print(Logger::Warning, warn_err_msg);
                                    ag_metric->addGeneratedWarning(sim_current_time(), "read_sensors_from_json", warn_err_msg);

                                    ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                                }
                            }
                            else {
                                warn_err_msg = "[WARNING] Sensor:["+ belief_name +"] does not have parameter 'time' -> ignored it!";
                                logger->print(Logger::Warning, warn_err_msg);
                                ag_metric->addGeneratedWarning(sim_current_time(), "read_sensors_from_json", warn_err_msg);
                                ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                            }
                        }
                        else {
                            warn_err_msg = "[WARNING] Sensor:["+ belief_name +"] does not have parameter 'value' -> ignored it!";
                            logger->print(Logger::Warning, warn_err_msg);
                            ag_metric->addGeneratedWarning(sim_current_time(), "read_sensors_from_json", warn_err_msg);
                            ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                        }
                    }
                    else {
                        warn_err_msg = "[WARNING] Sensor reading:[id="+ std::to_string(sensor_read_id) +"] has empty name -> ignored it!";
                        logger->print(Logger::Warning, warn_err_msg);
                        ag_metric->addGeneratedWarning(sim_current_time(), "read_sensors_from_json", warn_err_msg);
                        ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                    }
                }
                else {
                    warn_err_msg = "[WARNING] Sensor does not have parameter 'belief_name' -> ignored it!";
                    logger->print(Logger::Warning, warn_err_msg);
                    ag_metric->addGeneratedWarning(sim_current_time(), "read_sensors_from_json", warn_err_msg);
                    ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                }
            }
        }
    }
    catch (std::exception& ex) {
        throw std::runtime_error("[ERROR] While reading file "+ path + ": " + ex.what());
    }

    return ret_val;
}

json JsonfileHandler::read_servers_from_json(std::string path,
        std::shared_ptr<ServerHandler> ag_Server_Handler,
        std::vector<std::shared_ptr<Server>>& ag_servers,
        std::string schedule_type_name,
        int ag_id, std::string received_content, bool read_from_file)
{   // Servers with the same ID are NOT ALLOWED!
    json servers;
    json ret_val = json::array(); // list of analyzed data
    std::string agent_id;

    if(schedule_type_name.compare("EDF") == 0 || schedule_type_name.compare("EventsGenerator") == 0)
    {
        bool safe_to_read_data = false;
        bool server_already_exists = false, correctly_setup = true;
        std::string warn_err_msg = "";

        try {
            agent_id = std::to_string(ag_id);
            if(read_from_file == false)
            {
                if(received_content.empty() == false) {
                    servers = servers.parse(received_content);
                    safe_to_read_data = true;
                }
                else {
                    warn_err_msg = "[ERROR] Received content for Servers is EMPTY!";
                    logger->print(Logger::Error, warn_err_msg);
                    ret_val.push_back(generate_error_warning_json("error", warn_err_msg));
                }
            }
            else
            {
                std::ifstream i(path.c_str());

                if (!i.fail()) {
                    i >> servers;
                    safe_to_read_data = true;
                }
                else {
                    throw std::runtime_error("Could not open file!");
                }
            }

            if(safe_to_read_data)
            {
                for (const auto& server : servers[agent_id])
                {
                    server_already_exists = false;
                    correctly_setup = true;

                    if(server.find("id") == server.end())
                    {
                        warn_err_msg = "[WARNING] Server does not have parameter 'id'!";
                        logger->print(Logger::Warning, warn_err_msg);
                        ag_metric->addGeneratedWarning(sim_current_time(), "read_servers_from_json", warn_err_msg);
                        ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));

                        correctly_setup = false;
                    }
                    else {
                        int id = server["id"].get<int>();

                        if(server.find("period") == server.end())
                        {
                            warn_err_msg = "[WARNING] Server:["+ std::to_string(id) +"] does not have parameter 'period'!";
                            logger->print(Logger::Warning, warn_err_msg);
                            ag_metric->addGeneratedWarning(sim_current_time(), "read_servers_from_json", warn_err_msg);
                            ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                            correctly_setup = false;
                        }
                        if(server.find("budget") == server.end())
                        {
                            warn_err_msg = "[WARNING] Server:["+ std::to_string(id) +"] does not have parameter 'budget'!";
                            logger->print(Logger::Warning, warn_err_msg);
                            ag_metric->addGeneratedWarning(sim_current_time(), "read_servers_from_json", warn_err_msg);
                            ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                            correctly_setup = false;
                        }

                        if(correctly_setup)
                        {
                            double period = server["period"].get<double>();
                            double budget = server["budget"].get<double>();
                            ServerType type = CBS;

                            if(budget > period) {
                                warn_err_msg = "[WARNING] Server:["+ std::to_string(id)
                                +"] has budget greater than period ("+ std::to_string(budget)
                                +" > "+ std::to_string(period) +")!";
                                logger->print(Logger::Warning, warn_err_msg);
                                ag_metric->addGeneratedWarning(sim_current_time(), "read_servers_from_json", warn_err_msg);
                                ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                            }

                            for(int i = 0; i < (int)ag_servers.size(); i++) {
                                if(ag_servers[i]->get_server_id() == id) {
                                    server_already_exists = true;
                                }
                            }

                            if(server_already_exists) {
                                warn_err_msg = "[WARNING] Server with ID:["+ std::to_string(id) +"] already exists! -> ignore it!";
                                logger->print(Logger::Warning, warn_err_msg);
                                ag_metric->addGeneratedWarning(sim_current_time(), "read_servers_from_json", warn_err_msg);
                                ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                            }
                            else {
                                ag_servers.push_back(std::make_shared<Server>(id, period, budget, type));

                                if(ag_Server_Handler != nullptr) {
                                    ag_Server_Handler->add_server(std::make_shared<Server>(id, period, budget, type));
                                }
                            }
                        }
                    }
                }
            }
            else {
                throw std::runtime_error("No servers available...");
            }
        }
        catch (std::exception& ex) {
            throw std::runtime_error("[ERROR] While reading file "+ path + ": " + ex.what());
        }
    }

    return ret_val;
}

json JsonfileHandler::read_skillset_from_json(const std::string path,
        const std::vector<std::shared_ptr<Belief>>& beliefset,
        std::vector<std::shared_ptr<Skill>>& skillset, const int ag_id, std::string received_content, bool read_from_file)
{   // Skills with the same ID are NOT ALLOWED!  They also need to be different from the name used for the BELIEFSET

    json skills;
    json ret_val = json::array(); // list of analyzed data
    std::string agent_id;

    bool safe_to_read_data = false;
    bool valid_skill = true;
    std::string goal_name_no_spaces = "";
    std::string warn_err_msg = "";

    try {
        agent_id = std::to_string(ag_id);
        if(read_from_file == false)
        {
            if(received_content.empty() == false) {
                skills = skills.parse(received_content);
                safe_to_read_data = true;
            }
            else {
                warn_err_msg = "[ERROR] Received content for Skillset is EMPTY!";
                logger->print(Logger::Error, warn_err_msg);
                ret_val.push_back(generate_error_warning_json("error", warn_err_msg));
            }
        }
        else
        {
            std::ifstream i(path.c_str());

            if (!i.fail()) {
                i >> skills;
                safe_to_read_data = true;
            }
            else {
                throw std::runtime_error("Could not open file!");
            }
        }

        if(safe_to_read_data)
        {
            for (const auto& skill : skills[agent_id])
            {
                valid_skill = true;

                if(skill.find("goal_name") != skill.end())
                {
                    std::string goal_name = skill["goal_name"].get<std::string>();
                    goal_name_no_spaces = boost::replace_all_copy(goal_name, " ", "");

                    if(goal_name_no_spaces.size() == 0)
                    {
                        warn_err_msg = "[WARNING] Skill:["+ goal_name +"] has invalid name! -> ignore it!";
                        logger->print(Logger::Warning, warn_err_msg);
                        ag_metric->addGeneratedWarning(sim_current_time(), "read_skillset_from_json", warn_err_msg);
                        ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                        valid_skill = false;
                    }
                    else {
                        // check if "skill.goal_name" is UNIQUE WITHIN the SKILLSET...
                        for(int i = 0; i < (int)skillset.size(); i++) {
                            if(skillset[i]->get_goal_name() == goal_name) {
                                warn_err_msg = "[WARNING] A Skill with name:["+ goal_name +"] already exists! -> ignore it!";
                                logger->print(Logger::Warning, warn_err_msg);
                                ag_metric->addGeneratedWarning(sim_current_time(), "read_skillset_from_json", warn_err_msg);
                                ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                                valid_skill = false;
                            }
                        }

                        // check if "skill.goal_name" is UNIQUE WITHIN the BELIEFSET (in order to avoid confusing a skill with a belief, they must have different names)...
                        for(int i = 0; i < (int)beliefset.size(); i++) {
                            if(beliefset[i]->get_name() == goal_name) {
                                warn_err_msg = "[WARNING] A Belief with name:["+ goal_name +"] already exists! -> ignore it!";
                                logger->print(Logger::Warning, warn_err_msg);
                                ag_metric->addGeneratedWarning(sim_current_time(), "read_skillset_from_json", warn_err_msg);
                                ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                                valid_skill = false;
                            }
                        }

                        if(valid_skill) {
                            // Use valid data to instantiate a new skill
                            skillset.push_back(std::make_shared<Skill>(goal_name));
                        }
                    }
                }
                else {
                    warn_err_msg = "[WARNING] Skill does not have parameter 'goal_name' -> ignored it!";
                    logger->print(Logger::Warning, warn_err_msg);
                    ag_metric->addGeneratedWarning(sim_current_time(), "read_skillset_from_json", warn_err_msg);
                    ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                }
            }
        }
    }
    catch (std::exception& ex) {
        throw std::runtime_error("[ERROR] While reading file "+ path + ": " + ex.what());
    }

    return ret_val;
}

json JsonfileHandler::update_beliefset_from_json(std::string path,
        std::vector<std::shared_ptr<Belief>>& beliefset, int ag_id, std::string received_content, bool read_from_file)
{
    json read_beliefset; // updated belief
    json ret_val = json::array(); // list of analyzed data
    std::string agent_id;

    int update_result = 0;
    bool safe_to_read_data = true;
    std::string message = "";
    std::string warn_err_msg = "";

    try {
        agent_id = std::to_string(ag_id);
        if(read_from_file == false)
        {
            if(received_content.empty()) {
                safe_to_read_data = true;
                warn_err_msg = "[WARNING] Parameter 'update-beliefset' is empty! -> ignore it";
                logger->print(Logger::Warning, warn_err_msg);
                ag_metric->addGeneratedWarning(sim_current_time(), "update_beliefset_from_json", warn_err_msg);
                ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
            }
            else {
                if(received_content.empty() == false) {
                    read_beliefset = read_beliefset.parse(received_content);
                }
                else {
                    warn_err_msg = "[ERROR] Received content for Beliefset is EMPTY!";
                    logger->print(Logger::Error, warn_err_msg);
                    ret_val.push_back(generate_error_warning_json("error", warn_err_msg));
                }
            }
        }
        else
        {
            std::ifstream i(path.c_str());

            if (!i.fail()) {
                i >> read_beliefset;
                safe_to_read_data = true;
            }
            else {
                safe_to_read_data = false;
                warn_err_msg = "[WARNING] File 'update-beliefset.json' not found! ";
                logger->print(Logger::Warning, warn_err_msg);
                ag_metric->addGeneratedWarning(sim_current_time(), "update_beliefset_from_json", warn_err_msg);
                ret_val.push_back(generate_error_warning_json("error", warn_err_msg));
            }
        }

        if(safe_to_read_data)
        {
            for (const auto& belief : read_beliefset[agent_id])
            {
                if(belief.find("name") != belief.end())
                {
                    std::string name = belief["name"].get<std::string>();

                    // get the value stored in belief.value, based on its type
                    if(belief.find("value") != belief.end())
                    {
                        std::variant<int, double, bool, std::string> value;
                        if(convertJsonValueToVariant(belief["value"], value) == "undefined") {
                            warn_err_msg = "[WARNING] Belief with name:["+ name +"] has invalid value type! -> ignore it!";
                            logger->print(Logger::Warning, warn_err_msg);
                            ag_metric->addGeneratedWarning(sim_current_time(), "update_beliefset_from_json", warn_err_msg);
                            ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                        }
                        else {
                            update_result = updateBeliefIfPresent(beliefset, name, value, message);
                            if(update_result == 1) {
                                // belief correctly updated...
                            }
                            else if(update_result == 2) {
                                warn_err_msg = "[WARNING] Belief:["+ name +"] not found in the beliefset! -> ignore update!";
                                logger->print(Logger::Warning, warn_err_msg);
                                ag_metric->addGeneratedWarning(sim_current_time(), "update_beliefset_from_json", warn_err_msg);
                                ret_val.push_back(generate_error_warning_json("warning", warn_err_msg));
                            }
                            else if(update_result == 3) {
                                logger->print(Logger::Warning, message);
                                ag_metric->addGeneratedWarning(sim_current_time(), "update_beliefset_from_json", message);
                                ret_val.push_back(generate_error_warning_json("warning", message));
                            }
                        }
                    }
                    else {
                        message = "Update Belief:["+ name +"] does not have parameter 'value' -> ignore it!";
                        logger->print(Logger::Warning, message);
                        ret_val.push_back(generate_error_warning_json("warning", message));
                    }
                }
                else {
                    message = "Update Belief does not have parameter 'name' -> ignore it!";
                    logger->print(Logger::Warning, message);
                    ret_val.push_back(generate_error_warning_json("warning", message));
                }
            }
        }
    }
    catch (std::exception& ex) {
        throw std::runtime_error("[ERROR] While reading file "+ path + ": " + ex.what());
    }

    return ret_val;
}

// Support functions: -------------------------------------------------------------------------------------------
std::string JsonfileHandler::convertJsonValueToVariant(json json_value,
        std::variant<int, double, bool, std::string>& value)
{
    // Note: in json we do not have type "DOUBLE". We only have: boolean, number_unsigned, number_float, string
    if(json_value.type() == json::value_t::number_float) {
        value = json_value.get<double>();
        return "double";
    }
    else if(json_value.type() == json::value_t::number_unsigned || json_value.type() == json::value_t::number_integer) {
        value = json_value.get<int>();
        return "int";
    }
    else if(json_value.type() == json::value_t::boolean) {
        value = json_value.get<bool>();
        return "bool";
    }
    else if(json_value.type() == json::value_t::string) {
        value = json_value.get<std::string>();
        return "string";
    }

    return "undefined";
}

json JsonfileHandler::convertVariantToJson(std::variant<int, double, bool, std::string>& value)
{
    json ret_val;
    std::variant<int, double, bool, std::string> tmp;
    tmp = value;

    if(auto val = std::get_if<int>(&tmp))
    {
        int& s = *val;
        ret_val = s;
    }
    else if(auto val = std::get_if<double>(&tmp))
    {
        double& s = *val;
        ret_val = s;
    }
    else if(auto val = std::get_if<bool>(&tmp))
    {
        bool& s = *val;
        ret_val = s;
    }
    else if(auto val = std::get_if<std::string>(&tmp))
    {
        std::string& s = *val;
        ret_val = s;
    }

    return ret_val;
}

Goal::Status JsonfileHandler::convertStringToGoalStatus(std::string value) {
    value = boost::to_upper_copy(value);
    if(value == "IDLE") {
        return Goal::IDLE;
    }
    else if(value == "ACTIVE") {
        return Goal::ACTIVE;
    }
    else if(value == "INACTIVE") {
        return Goal::INACTIVE;
    }

    // should never reach this point...
    return Goal::IDLE;
}

Task::Execution JsonfileHandler::convertStringToTaskExecutionType(std::string value, std::string scheduler) {
    value = boost::to_upper_copy(value);
    if(scheduler == "FCFS") {
        return Task::EXEC_SEQUENTIAL;
    }
    else {
        if(value == "SEQUENTIAL" || value == "SEQ") {
            return Task::EXEC_SEQUENTIAL;
        }
        else if(value == "PARALLEL" || value == "PAR") {
            return Task::EXEC_PARALLEL;
        }

        // should never reach this point...
        return Task::EXEC_SEQUENTIAL;
    }
}

Goal::Execution JsonfileHandler::convertStringToGoalExecutionType(std::string value, std::string scheduler) {
    value = boost::to_upper_copy(value);
    if(scheduler == "FCFS") {
        return Goal::EXEC_SEQUENTIAL;
    }
    else {
        if(value == "SEQUENTIAL" || value == "SEQ") {
            return Goal::EXEC_SEQUENTIAL;
        }
        else if(value == "PARALLEL" || value == "PAR") {
            return Goal::EXEC_PARALLEL;
        }

        // should never reach this point...
        std::string warn_err_msg = "[WARNING] Goal was neither SEQ nor PAR! It's "+ value +"! Set to default SEQUENTIAL!";
        logger->print(Logger::Warning, warn_err_msg);
        ag_metric->addGeneratedWarning(sim_current_time(), "convertStringToGoalExecutionType", warn_err_msg);

        return Goal::EXEC_SEQUENTIAL;
    }
}

Operator::Type JsonfileHandler::convertStringToOperator(std::string value) {
    value = boost::to_upper_copy(value);
    if(value == "EQUAL") {
        return Operator::EQUAL;
    }
    else if(value == "GREATER") {
        return Operator::GREATER;
    }
    else if(value == "GREATER_EQUAL") {
        return Operator::GREATER_EQUAL;
    }
    else if(value == "LESS") {
        return Operator::LESS;
    }
    else if(value == "LESS_EQUAL") {
        return Operator::LESS_EQUAL;
    }

    // should never reach this point...
    throw std::runtime_error("Could not find the specified Operator:["+ value +"]!");
}

Sensor::Mode JsonfileHandler::convertStringToSensorMode(std::string value) {
    value = boost::to_upper_copy(value);
    if(value == "INCREASE") {
        return Sensor::INCREASE;
    }
    else if(value == "DECREASE") {
        return Sensor::DECREASE;
    }
    else {
        // set the default value...
        return Sensor::SET;
    }
}

json JsonfileHandler::generate_error_warning_json(std::string type, std::string description)
{
    json data = {
            { "status", type },
            { "description", description }
    };

    return data;
}

/* *******************************************************************
 * **************    Private Functions    ****************************
 * *******************************************************************/
bool JsonfileHandler::checkIfBeliefAlreadyExists(const std::vector<std::shared_ptr<Belief>>& beliefset, const std::string name)
{
    for(int i = 0; i < (int)beliefset.size(); i++) {
        if(beliefset[i]->get_name() == name) {
            return true;
        }
    }

    return false; // new belief
}

ServerType JsonfileHandler::convertIntToServerType(int value) {
    if(value == 0) {
        return CBS;
    }

    // should never reach this point...
    throw std::runtime_error("Could not find the specified Server Type!");
}

std::shared_ptr<Belief> JsonfileHandler::findBeliefWithGivenName(const std::vector<std::shared_ptr<Belief>>& beliefset,
        std::string prec_belief_name_ref)
{
    for(std::shared_ptr<Belief> belief : beliefset) {
        if(belief->get_name() == prec_belief_name_ref) {
            return belief;
        }
    }

    // belief not present...
    return nullptr;
}

std::shared_ptr<Skill> JsonfileHandler::findSkillWithGivenName(const std::vector<std::shared_ptr<Skill>>& skillset, std::string goal_name)
{
    for(std::shared_ptr<Skill> skill : skillset) {
        if(skill->get_goal_name() == goal_name) {
            return skill;
        }
    }

    // skill not present...
    return nullptr;
}

int JsonfileHandler::getServerIndexInAg_servers(const std::vector<std::shared_ptr<Server>>& servers, const int server_id) {
    for(int i = 0; i < (int)servers.size(); i++) {
        if (servers[i]->getTaskServer() == server_id) {
            return i;
        }
    }
    return -1;
}

simtime_t JsonfileHandler::sim_current_time() {
    return simTime().trunc(sim_time_unit);
}

int JsonfileHandler::updateBeliefIfPresent(std::vector<std::shared_ptr<Belief>>& beliefset,
        std::string name, std::variant<int, double, bool, std::string> new_value, std::string& message)
{   /* update belief only if original value and new one are of the same type family
 * @return 1 if belief correctly updated
 * @return 2 if belief not found
 * @return 3 if types do not match
 */
    std::variant<int, double, bool, std::string> current_value;

    // check if the belief exists...
    for(std::shared_ptr<Belief> belief : beliefset)
    {
        if(belief->get_name() == name)
        { // yes, then update its value...
            current_value = belief->get_value();

            if(std::get_if<int>(&current_value))
            {
                if(auto n_v = std::get_if<int>(&new_value)) {
                    int& db = *n_v;
                    belief->set_value(db);
                    return 1;
                }
                else if(auto n_v = std::get_if<double>(&new_value)) {
                    double& db = *n_v;
                    belief->set_value(db);
                    return 1;
                }
                else {
                    message = "[WARNING] Belief:["+ name +"] is numerical. Only numerical values can be assigned to it! --> UpdateBelief ignored!";
                    return 3;
                }
            }
            else if(std::get_if<double>(&current_value))
            {
                if(auto n_v = std::get_if<double>(&new_value)) {
                    double& db = *n_v;
                    belief->set_value(db);
                    return 1;
                }
                else if(auto n_v = std::get_if<int>(&new_value)) {
                    int& db = *n_v;
                    belief->set_value(db);
                    return 1;
                }
                else {
                    message = "[WARNING] Belief:["+ name +"] is numerical. Only numerical values can be assigned to it! --> UpdateBelief ignored!";
                    return 3;
                }
            }
            else if(std::get_if<bool>(&current_value))
            {
                if(auto n_v = std::get_if<bool>(&new_value)) {
                    bool& db = *n_v;
                    belief->set_value(db);
                    return 1;
                }
                else {
                    message = "[WARNING] Belief:["+ name +"] is boolean. Only boolean values can be assigned to it! --> UpdateBelief ignored!";
                    return 3;
                }
            }
            else if(std::get_if<std::string>(&current_value))
            {
                if(auto n_v = std::get_if<std::string>(&new_value)) {
                    std::string& db = *n_v;
                    belief->set_value(db);
                    return 1;
                }
                else {
                    message = "[WARNING] Belief:["+ name +"] is a string. Only string values can be assigned to it! --> UpdateBelief ignored!";
                    return 3;
                }
            }
        }
    }

    return 2;
}

/* *******************************************************************
 * **************        Constructor      ****************************
 * *******************************************************************/
JsonfileHandler::JsonfileHandler()
{ // constructor used by "EventsGenerator.cc"
    ag_metric = std::make_shared<Metric>();
    logger = std::make_shared<Logger>(Logger::EssentialInfo);
    update_iteration = 0;
    desire_goal_id = 0;
    sensor_read_id = 0;
}

JsonfileHandler::JsonfileHandler(std::shared_ptr<Logger> _logger, std::shared_ptr<Metric> _metric)
{  // constructor used by "agent.cc"
    ag_metric = _metric;
    logger = _logger;
    update_iteration = 0;
    desire_goal_id = 0;
    sensor_read_id = 0;
}

JsonfileHandler::~JsonfileHandler() { }
