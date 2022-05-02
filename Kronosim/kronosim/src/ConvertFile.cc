/*
 * ConvertFile.cc
 *
 *  Created on: Mar 8, 2021
 *      Author: francesco
 */

#include "../headers/ConvertFile.h"

ConvertFile::ConvertFile(std::string user, std::string path, std::string original_simulation_name, std::string destination_folder) {
    writer_inputs = std::make_unique<WriterInputs>();

    /* Create Skillset file automatically */
    //    read_desireset_from_json(user, path, beliefset, skillset, desireset, agent_id);
    //    read_planset_from_json(user, path, beliefset, skillset, planset, agent_id);
    //    create_skillset_json(user, path, destination_folder, original_simulation_name, skillset, agent_id);
    /* ---------------------------------- */

    convert_beliefset_from_json(user, path, destination_folder, original_simulation_name, beliefset, agent_id);
    // File skillset.json must have been created manually to use:
    convert_skillset_from_json(user, path, destination_folder, original_simulation_name, skillset, agent_id);

    convert_desireset_from_json(user, path, destination_folder, original_simulation_name, beliefset, skillset, desireset, agent_id);
    convert_sensors_from_json(user, path, destination_folder, original_simulation_name, sensors_time, sensors_plans, agent_id);
    convert_servers_from_json(user, path, destination_folder, original_simulation_name, servers, agent_id);
    convert_planset_from_json(user, path, destination_folder, original_simulation_name, beliefset, servers, skillset, planset, agent_id, "EDF");
    convert_update_beliefset_from_json(user, path, destination_folder, original_simulation_name, beliefset, agent_id);
}

ConvertFile::~ConvertFile() { }

void ConvertFile::convert_beliefset_from_json(std::string user, std::string path, std::string destination_folder, std::string ofileName,
        std::vector<std::shared_ptr<Belief>>& ag_beliefset, int ag_id)
{
    path = path + "beliefset.json";
    std::ifstream i(path.c_str());
    std::shared_ptr<Belief> new_belief;

    try {
        if (!i.fail()) {
            json beliefset;
            i >> beliefset;
            std::string id = std::to_string(ag_id);
            for (const auto& belief : beliefset[id]) {
                std::string name = belief["name"].get<std::string>();

                // get the value stored in belief.value, based on its type
                std::variant<int, double, bool, std::string> value;
                convertJsonValueToVariant(belief["value"], value);

                new_belief = std::make_shared<Belief>(name, value);

                ag_beliefset.push_back(new_belief);
                writer_inputs->write_beliefset_json(new_belief);
            }

            writer_inputs->save_beliefset_json(user, "", ofileName, destination_folder);
        }
        else {
            throw std::runtime_error("Could not open file!");
        }
    }
    catch (std::exception const& ex) {
        throw std::runtime_error("[ERROR] While reading file "+ path + ": " + ex.what());
    }
}

void ConvertFile::convert_desireset_from_json(std::string user, std::string path, std::string destination_folder, std::string ofileName,
        const std::vector<std::shared_ptr<Belief>>& ag_beliefset, const std::vector<std::shared_ptr<Skill>>& ag_skillset,
        std::vector<std::shared_ptr<Desire>>& ag_desireset, int ag_id)
{
    path = path + "desireset.json";
    std::ifstream i(path.c_str());
    std::string goal_name;
    std::string value_type = "";
    int desire_id = -1;

    try {
        if (!i.fail()) {
            json desireset;
            i >> desireset;
            std::string id = std::to_string(ag_id);
            for (const auto& desire : desireset[id]) {
                // valid_belief = false;

                goal_name = desire["goal_name"].get<std::string>();

                // Desire.preconditions-----------------------------------------------------------------
                json preconditions = desire["preconditions"];
                // ------------------------------------------------------------------------------------
                // Desire.cont_conditions-----------------------------------------------------------------
                json cont_conditions = desire["cont_conditions"];
                // ------------------------------------------------------------------------------------

                // Desire.priority, dynamic value ------------------------------------------------------
                json formula = desire["priority"]["formula"];
                // -------------------------------------------------------------------------------------

                // Desire.priority, reference table ----------------------------------------------------
                std::shared_ptr<Belief> rt_belief;
                std::string rt_belief_name;
                std::variant<int, double, bool, std::string> rt_threshold;
                double rt_priority;
                Operator::Type rt_op;
                std::vector<std::shared_ptr<Reference_table>> reference_table;

                if(desire["priority"]["reference_table"].size() > 0) {
                    for(json element : desire["priority"]["reference_table"]) {
                        rt_belief_name = element["belief_name"].get<std::string>();
                        convertJsonValueToVariant(element["threshold"], rt_threshold);
                        rt_priority = element["priority"].get<double>();
                        rt_op = convertStringToOperator(element["op"].get<std::string>());

                        try {
                            rt_belief = findBeliefWithGivenName(ag_beliefset, rt_belief_name);

                            if(rt_belief == nullptr) {
                                throw std::runtime_error("The specified belief:[id="+ rt_belief_name
                                        +"], for Desire:["+ goal_name
                                        +"] is not valid. Can not create a record in the reference_table.");
                            }
                            else {
                                // the belief is present in the ag_beliefset...
                                reference_table.push_back(std::make_shared<Reference_table>(
                                        rt_belief->get_name(), rt_threshold, rt_priority, rt_op
                                ));
                            }
                        }
                        catch (std::exception const& ex) {
                            throw std::runtime_error("The specified 'operator' is not valid.");
                        }
                    }
                }
                // -------------------------------------------------------------------------------------

                std::shared_ptr<Desire> new_desire;
                double ddl = desire["deadline"].get<double>();
                if(ddl >= 0) { // versione per far funzionare il DEBUG (nel file, deadline: -1 = inf)
                    new_desire = std::make_shared<Desire>(
                            desire_id, goal_name,
                            preconditions, cont_conditions, ddl);
                }
                else {
                    new_desire = std::make_shared<Desire>(
                            desire_id, goal_name,
                            preconditions, cont_conditions);
                }

                // add few more elements to the desire:
                new_desire->set_computedDynamically(desire["priority"]["computedDynamically"].get<bool>());
                new_desire->set_priority_formula(formula);
                new_desire->set_priority_reference_table(reference_table);
                // -------------------------------------------------------------------------------------

                ag_desireset.push_back(new_desire);
                writer_inputs->write_desireset_json(new_desire);
            }

            writer_inputs->save_desireset_json(user, "", ofileName, destination_folder);
        }
        else {
            throw std::runtime_error("Could not open file!");
        }
    }
    catch (std::exception const& ex) {
        throw std::runtime_error("[ERROR] While reading file "+ path + ": " + ex.what());
    }
}

void ConvertFile::convert_planset_from_json(std::string user, std::string path, std::string destination_folder, std::string ofileName,
        const std::vector<std::shared_ptr<Belief>>& ag_beliefset, const std::vector<std::shared_ptr<Server>>& ag_servers,
        const std::vector<std::shared_ptr<Skill>>& ag_skillset, std::vector<std::shared_ptr<Plan>>& ag_plans,
        int ag_id, std::string schedule_type_name)
{
    path = path + "planset.json";

    int task_unique_id; // used to set up the task_id within plans
    int plan_id = -1;
    int debug_index = 0;
    double action_arrival_time;
    std::string goal_name, action_goal_name;
    json no_preconditions;
    json preconditions, cont_conditions, post_conditions;
    std::vector<std::shared_ptr<Effect>> effects_at_begin, effects_at_end;

    std::ifstream i(path.c_str());
    try {
        if (!i.fail()) {
            json plans;
            i >> plans;
            std::string id = std::to_string(ag_id);

            for (const auto& plan : plans[id])
            {
                task_unique_id = 0;
                effects_at_begin.clear();
                effects_at_end.clear();

                goal_name = plan["goal_name"].get<std::string>();

                // Plan.pref, dynamic value ------------------------------------------------------------
                json formula = plan["preference"]["formula"];
                // -------------------------------------------------------------------------------------

                // Plan.pref, reference table ----------------------------------------------------
                std::shared_ptr<Belief> rt_belief;
                std::string rt_belief_name;
                std::variant<int, double, bool, std::string> rt_threshold;
                double rt_pref;
                Operator::Type rt_op;

                std::vector<std::shared_ptr<Reference_table>> reference_table;

                if(plan["preference"]["reference_table"].size() > 0) {
                    for(json element : plan["preference"]["reference_table"]) {
                        rt_belief_name = element["belief_name"].get<std::string>();
                        convertJsonValueToVariant(element["threshold"], rt_threshold);
                        rt_pref = element["pref"].get<double>();
                        rt_op = convertStringToOperator(element["op"].get<std::string>());

                        rt_belief = findBeliefWithGivenName(beliefset, rt_belief_name);

                        if(rt_belief == nullptr) {
                            throw std::runtime_error("The specified belief:[id="+ rt_belief_name
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
                // -------------------------------------------------------------------------------------

                // Plan conditions-----------------------------------------------------------------
                preconditions = plan["preconditions"];
                cont_conditions = plan["cont_conditions"];

                if(plan.find("post_conditions") != plan.end()) {
                    post_conditions = plan["post_conditions"];
                    std::cout << "post_conditions: " << post_conditions << std::endl;
                }
                else {
                    post_conditions = json::array();
                }
                // -------------------------------------------------------------------------------------

                // Plan.effects_at_begin -----------------------------------------------------------------
                if(plan.find("effects_at_begin") != plan.end()) {
                    if(plan["effects_at_begin"].size() > 0) {
                        for(int i = 0; i < (int)plan["effects_at_begin"].size(); i++) {
                            effects_at_begin.push_back(std::make_shared<Effect>(plan["effects_at_begin"][i]));
                        }
                    }
                }
                // -------------------------------------------------------------------------------------

                // Plan.effects_at_end -------------------------------------------------------------------
                if(plan.find("effects_at_end") != plan.end()) {
                    if(plan["effects_at_end"].size() > 0) {
                        for(int i = 0; i < (int)plan["effects_at_end"].size(); i++) {
                            effects_at_end.push_back(std::make_shared<Effect>(plan["effects_at_end"][i]));
                        }
                    }
                }
                // -------------------------------------------------------------------------------------

                // Plan.body ---------------------------------------------------------------------------
                std::vector<std::shared_ptr<Action>> body_actions;
                int action_id = -1;
                double action_ddl;
                std::string action_name;
                std::vector<std::shared_ptr<Action>> actions;
                std::shared_ptr<Goal> body_goal;
                std::shared_ptr<Task> body_task;

                json action_priority;
                Goal::Status action_status = Goal::IDLE;
                Goal::Execution goal_exec_type;
                std::shared_ptr<Action> action;

                bool isFirstActionInBody = true;
                if(plan["body"].size() > 0) {
                    for(json action : plan["body"]) {
                        std::string action_type = action["action_type"].get<std::string>();
                        action_type = boost::to_upper_copy(action_type); // convert string to upper case

                        if(action_type == "GOAL")
                        {
                            action_goal_name = action["action"]["goal_name"].get<std::string>();
                            action_ddl = action["action"]["deadline"].get<double>();
                            action_priority = action["action"]["priority"];
                            action_arrival_time = action["action"]["arrivalTime"];

                            if(isFirstActionInBody) {
                                isFirstActionInBody = false;
                                goal_exec_type = Goal::EXEC_SEQUENTIAL;
                            }
                            else {
                                goal_exec_type = convertStringToGoalExecutionType(action["execution"].get<std::string>(), schedule_type_name);
                            }

                            body_goal = std::make_shared<Goal>(action_id,
                                    plan_id, plan_id,
                                    action_goal_name,
                                    no_preconditions,
                                    no_preconditions,
                                    -1, // priority
                                    -1, // activation_time
                                    action_arrival_time, // arrivalTime
                                    action_status,
                                    action_ddl,
                                    goal_exec_type);
                            body_goal->set_priority_formula(action_priority);

                            body_actions.push_back(body_goal);
                        }
                        else if(action_type == "TASK") {
                            json task = action["action"];

                            int t_id = task_unique_id++;
                            int t_plan_id = plan_id;
                            double t_comp = task["computationTime"].get<double>();
                            double t_R = 0;
                            double t_DDL = task["relativeDeadline"].get<double>();
                            double t_period = task["period"].get<double>();
                            int t_n_exec = task["n_exec"].get<int>();
                            int t_server = task["server"].get<int>();
                            Task::Execution execution_type;

                            if(isFirstActionInBody) {
                                isFirstActionInBody = false;
                                execution_type = Task::EXEC_SEQUENTIAL;
                            }
                            else {
                                execution_type = convertStringToTaskExecutionType(action["execution"].get<std::string>(), schedule_type_name);
                            }

                            /* In order to make EDF behave correctly:
                             * - Periodic tasks(n_exec = -1 || n_exec > 1) MUST have server = -1
                             * - APeriodic tasks(n_exec = 1) MUST have server != -1 */
                            if(t_server != -1) { // task is APERIODIC
                                if(t_n_exec == 1) { // correctly set up
                                    int tmp_server_id = getServerIndexInAg_servers(servers, t_server);

                                    if(tmp_server_id != -1) {
                                        t_DDL = servers[tmp_server_id]->get_curr_ddl();
                                        body_task = std::make_shared<Task>(t_id, t_plan_id, t_plan_id,
                                                0, 0, t_comp, t_comp, t_R, t_R, t_DDL, -1,
                                                -1, t_server, t_period,
                                                t_n_exec, t_n_exec, false, false, Task::TSK_IDLE, execution_type);
                                        body_actions.push_back(body_task);
                                    }
                                    else {
                                        throw std::runtime_error("APERIODIC tasks:[id="+ std::to_string(t_id) +"], plan:[id="
                                                +std::to_string(plan_id) +"] must have a valid server! (id:"+ std::to_string(t_server) +" is not)");
                                    }
                                }
                                else {
                                    throw std::runtime_error("PERIODIC tasks:[id="+ std::to_string(t_id) +"], plan:[id="
                                            +std::to_string(plan_id) +"] must have server = -1!");
                                }
                            }
                            else { // task is PERIODIC
                                if(t_n_exec > 1 || t_n_exec == -1) { // correctly set up
                                    body_task = std::make_shared<Task>(t_id, t_plan_id, t_plan_id,
                                            0, 0, t_comp, t_comp, t_R, t_R, t_DDL, -1,
                                            -1, t_server, t_period, t_n_exec, t_n_exec, false, true, Task::TSK_IDLE, execution_type);
                                    body_actions.push_back(body_task);
                                }
                                else {
                                    throw std::runtime_error("Periodic tasks:[id="+ std::to_string(t_id) +"], plan:[id="
                                            +std::to_string(plan_id) +"] must have server = -1!");
                                }
                            }
                            // -------------------------------------------------------------------------------
                        }// end else
                    } // end for

                    // Plan.cost_function ------------------------------------------------------------------
                    double pref = 0; // it will be update to the correct value whenever the plan is selected as 'applicable  plan' in function 'unifyGoals'
                    // -------------------------------------------------------------------------------------

                    std::shared_ptr<Plan> new_plan = std::make_shared<Plan>(plan_id,
                            goal_name, pref,
                            preconditions, cont_conditions, post_conditions,
                            effects_at_begin, effects_at_end,
                            body_actions);

                    // add few more elements to the plan:
                    new_plan->set_computedDynamically(plan["preference"]["computedDynamically"].get<bool>());
                    new_plan->set_pref_formula(formula);
                    new_plan->set_pref_reference_table(reference_table);

                    ag_plans.push_back(new_plan);
                    writer_inputs->write_planset_json(new_plan, debug_index);
                    debug_index += 1;
                }
                else
                {
                    EV_WARN << "[WARNING] Plan:[id=" << plan_id << "] HAS NO ACTIONS DEFINED! -> deleted!";
                    std::cout << "[WARNING] The best plan:[id=" << plan_id << "] HAS NO ACTIONS DEFINED! -> deleted!";
                }
                // -------------------------------------------------------------------------------------
            }
            writer_inputs->save_planset_json(user, "", ofileName, destination_folder);
        }
        else {
            throw std::runtime_error("Could not open file!");
        }
    }
    catch (std::exception const& ex) {
        throw std::runtime_error("[ERROR] While reading file "+ path + ": " + ex.what());
    }
}

void ConvertFile::convert_sensors_from_json(std::string user, std::string path, std::string destination_folder, std::string ofileName,
        std::vector<std::shared_ptr<Sensor>>& ag_sensors_time, std::vector<std::shared_ptr<Sensor>>& ag_sensors, int ag_id)
{
    path = path + "sensors.json";
    std::ifstream i(path.c_str());
    std::shared_ptr<Sensor> current_sensor;
    int sensor_id = -1;

    try {
        if (!i.fail()) {
            json read_sensors; // sensors value read from file
            i >> read_sensors;
            std::string id = std::to_string(ag_id);
            for (const auto& sensor : read_sensors[id]) {

                std::string belief_name = sensor["belief_name"].get<std::string>();

                // get the value stored in read_value, based on its type
                std::variant<int, double, bool, std::string> value;
                convertJsonValueToVariant(sensor["value"], value);

                double time = sensor["time"].get<double>();

                std::string mode_string = sensor["mode"].get<std::string>();
                Sensor::Mode mode = convertStringToSensorMode(mode_string);

                current_sensor = std::make_shared<Sensor>(sensor_id, belief_name, value, time, mode);
                ag_sensors_time.push_back(current_sensor);
                writer_inputs->write_sensors_json(current_sensor);
                sensor_id += 1;
            }
            writer_inputs->save_sensors_json(user, "", ofileName, destination_folder);
        }
        else {
            throw std::runtime_error("Could not open file!");
        }
    }
    catch (std::exception const& ex) {
        throw std::runtime_error("[ERROR] While reading file "+ path + ": " + ex.what());
    }
}

void ConvertFile::convert_servers_from_json(std::string user, std::string path, std::string destination_folder,  std::string ofileName,
        std::vector<std::shared_ptr<Server>>& ag_servers, int ag_id)
{
    path = path + "servers.json";
    std::ifstream i(path.c_str());
    std::shared_ptr<Server> current_server;

    try {
        if (!i.fail()) {
            json servers;
            i >> servers;
            try {
                std::string id = std::to_string(ag_id);
                for (const auto& server : servers[id]) {
                    int server_id = server["id"].get<int>();
                    double period = server["period"].get<double>();
                    double budget = server["budget"].get<double>();

                    ag_servers.push_back(std::make_shared<Server>(server_id, period, budget));
                    writer_inputs->write_servers_json(std::make_shared<Server>(server_id, period, budget));
                }

                writer_inputs->save_servers_json(user, "", ofileName, destination_folder);
            }
            catch(std::exception& e) {
                throw std::runtime_error("No servers available...");
            }

        }
        else {
            throw std::runtime_error("Could not open file!");
        }
    }
    catch (std::exception const& ex) {
        throw std::runtime_error("[ERROR] While reading file "+ path + ": " + ex.what());
    }
}

void ConvertFile::convert_skillset_from_json(std::string user, std::string path, std::string destination_folder, std::string ofileName,
        std::vector<std::shared_ptr<Skill>>& ag_skills, int ag_id)
{
    path = path + "skillset.json";
    std::ifstream i(path.c_str());
    std::shared_ptr<Skill> current_skill;

    try {
        if (!i.fail()) {
            json skills;
            i >> skills;
            try {
                std::string agent_id = std::to_string(ag_id);
                for (const auto& skill : skills[agent_id]) {
                    std::string name = skill["goal_name"].get<std::string>();

                    current_skill = std::make_shared<Skill>(name);
                    ag_skills.push_back(current_skill);
                    writer_inputs->write_skillset_json(current_skill);
                }

                writer_inputs->save_skillset_json(user, "", ofileName, destination_folder);
            }
            catch(std::exception& e) {
                throw std::runtime_error("Unexpected error while performing 'convert_skills_from_json'...");
            }
        }
        else {
            throw std::runtime_error("Could not open file!");
        }
    }
    catch (std::exception const& ex) {
        throw std::runtime_error("[ERROR] While reading file "+ path + ": " + ex.what());
    }
}

void ConvertFile::convert_update_beliefset_from_json(std::string user, std::string path, std::string destination_folder, std::string ofileName,
        std::vector<std::shared_ptr<Belief>>& ag_beliefset, int ag_id)
{
    std::variant<int, double, bool, std::string> value;
    path = path + "update-beliefset.json";
    std::ifstream i(path.c_str());

    try {
        if (!i.fail()) {
            json read_beliefset; // updated belief data read from file
            i >> read_beliefset;
            std::string id = std::to_string(ag_id);
            for (const auto& belief : read_beliefset[id]) {
                std::string name = belief["name"].get<std::string>();

                // get the value stored in belief.value, based on its type
                convertJsonValueToVariant(belief["value"], value);

                writer_inputs->write_update_beliefset_json(std::make_shared<Belief>(name, value));
            }
            writer_inputs->save_update_beliefset_json(user, "", ofileName, destination_folder);
        }
        else {
            throw std::runtime_error("Could not open file!");
        }
    }
    catch (std::exception const& ex) {
        throw std::runtime_error("[ERROR] While reading file "+ path + ": " + ex.what());
    }
}

//// Read functions use to automatically create file skillset.json: -----------------------------------------------
//void ConvertFile::read_desireset_from_json(std::string user, std::string path, std::vector<std::shared_ptr<Belief>>& ag_beliefset,
//        std::vector<std::shared_ptr<Skill>>& ag_skillset, std::vector<std::shared_ptr<Desire>>& ag_desireset, int ag_id)
//{
//    path = path + "desireset.json";
//    std::ifstream i(path.c_str());
//    int skill_id = -1;
//    bool valid_belief = true;
//
//    try {
//        if (!i.fail()) {
//            json desireset;
//            i >> desireset;
//            std::string id = std::to_string(ag_id);
//            for (const auto& desire : desireset[id]) {
//                valid_belief = true;
//                std::string belief_name = desire["belief"]["name"].get<std::string>();
//
//                std::variant<int, double, bool, std::string> value;
//                std::string value_type = convertJsonValueToVariant(desire["belief"]["value"], value);
//
//                // Check if the (belief_name, value) already exists...
//                for(int i = 0; i < (int)ag_skillset.size(); i++) {
//                    if(ag_skillset[i]->get_belief_name() == belief_name)
//                    {
//                        valid_belief = false;
//                        break;
//                    }
//                }
//
//                if(valid_belief) {
//                    if(ag_skillset.size() > 0) {
//                        skill_id = ag_skillset[ag_skillset.size() - 1]->get_id() + 1;
//                    }
//                    else {
//                        skill_id = 0;
//                    }
//                    ag_skillset.push_back(std::make_shared<Skill>(skill_id, belief_name));
//                }
//            }
//        }
//        else {
//            throw std::runtime_error("Could not open file!");
//        }
//    }
//    catch (std::exception const& ex) {
//        throw std::runtime_error("[ERROR] While reading file "+ path + ": " + ex.what());
//    }
//}
//
//void ConvertFile::read_planset_from_json(std::string user, std::string path, std::vector<std::shared_ptr<Belief>>& ag_beliefset,
//        std::vector<std::shared_ptr<Skill>>& ag_skillset, std::vector<std::shared_ptr<Plan>>& ag_planset, int ag_id)
//{
//    path = path + "planset.json";
//    std::ifstream i(path.c_str());
//    int skill_id = -1;
//    bool valid_belief = true, valid_action = true;
//
//    try {
//        if (!i.fail()) {
//            json planset;
//            i >> planset;
//            std::string id = std::to_string(ag_id);
//            for (const auto& plan : planset[id]) {
//                valid_belief = true;
//                std::string belief_name = plan["belief"]["name"].get<std::string>();
//
//                std::variant<int, double, bool, std::string> value;
//                std::string value_type = convertJsonValueToVariant(plan["belief"]["value"], value);
//
//                // Check if the (belief_name, value) already exists...
//                for(int i = 0; i < (int)ag_skillset.size(); i++) {
//                    if(ag_skillset[i]->get_belief_name() == belief_name)
//                    {
//                        valid_belief = false;
//                        break;
//                    }
//                }
//
//                if(valid_belief) {
//                    if(ag_skillset.size() > 0) {
//                        skill_id = ag_skillset[ag_skillset.size() - 1]->get_id() + 1;
//                    }
//                    else {
//                        skill_id = 0;
//                    }
//
//                    ag_skillset.push_back(std::make_shared<Skill>(skill_id, belief_name));
//                }
//
//                // analyze internal goals > Plan.body ---------------------------------------------------------------------------
//                std::vector<std::shared_ptr<Action>> body_actions;
//                std::string action_name, goal_value_type;
//                std::variant<int, double, bool, std::string> action_value;
//                std::vector<std::shared_ptr<Action>> actions;
//                std::shared_ptr<Action> body_goal;
//                std::shared_ptr<Action> body_task;
//                std::shared_ptr<Belief> action_belief;
//                std::shared_ptr<Action> action;
//                Operator::Type goal_operator_type;
//
//                if(plan["body"].size() > 0) {
//                    for(json action : plan["body"]) {
//                        valid_action = true;
//                        std::string action_type = action["action_type"].get<std::string>();
//                        action_type = boost::to_upper_copy(action_type); // convert string to upper case
//
//                        if(action_type == "GOAL") {
//                            goal_operator_type = Operator::EQUAL;
//                            action_name = action["action"]["name"].get<std::string>();
//                            convertJsonValueToVariant(action["action"]["value"], action_value);
//
//                            // Check if 'action_name' exists as skill...
//                            for(int i = 0; i < (int)ag_skillset.size(); i++) {
//                                if(ag_skillset[i]->get_belief_name() == action_name)
//                                {
//                                    valid_action = false;
//                                    break;
//                                }
//                            }
//
//                            if(valid_action) {
//                                if(ag_skillset.size() > 0) {
//                                    skill_id = ag_skillset[ag_skillset.size() - 1]->get_id() + 1;
//                                }
//                                else {
//                                    skill_id = 0;
//                                }
//                                ag_skillset.push_back(std::make_shared<Skill>(skill_id, action_name));
//                            }
//                        }
//                    }
//                }
//            }
//        }
//        else {
//            throw std::runtime_error("Could not open file!");
//        }
//    }
//    catch (std::exception const& ex) {
//        throw std::runtime_error("[ERROR] While reading file "+ path + ": " + ex.what());
//    }
//}

bool ConvertFile::compareBeliefValues(std::variant<int, double, bool, std::string> belief_value, std::variant<int, double, bool, std::string> comparison_value)
{   // Example: if we have 'op = LESS', what the code will do is: belief < comparison_value
    bool res = false;

    if(auto b = std::get_if<int>(&belief_value))
    {
        if(auto value_b = std::get_if<int>(&comparison_value)) {
            int& ab = *b;
            int& db = *value_b;
            res = (ab == db);
        }
        else if(auto value_b = std::get_if<double>(&comparison_value)) {
            int& ab = *b;
            double& db = *value_b;
            res = (ab == db);
        }
    }
    else if(auto b = std::get_if<double>(&belief_value))
    {
        if(auto value_b = std::get_if<double>(&comparison_value)) {
            double& ab = *b;
            double& db = *value_b;
            res = (ab == db);
        }
        else if(auto value_b = std::get_if<int>(&comparison_value)) {
            double& ab = *b;
            int& db = *value_b;
            res = (ab == db);
        }
    }
    else if(auto b = std::get_if<bool>(&belief_value))
    {
        if(auto value_b = std::get_if<bool>(&comparison_value)) {
            bool& ab = *b;
            bool& db = *value_b;
            res = (ab == db);
        }
    }
    else if(auto b = std::get_if<std::string>(&belief_value))
    {
        if(auto value_b = std::get_if<std::string>(&comparison_value)) {
            std::string& ab = *b;
            std::string& db = *value_b;
            res = (ab == db);
        }
    }

    return res;
}

void ConvertFile::create_skillset_json(std::string user, std::string path, std::string destination_folder, std::string ofileName,
        std::vector<std::shared_ptr<Skill>>& ag_skills, int ag_id)
{
    path = path + "skillset.json";
    std::ifstream i(path.c_str());
    std::shared_ptr<Skill> current_skill;

    try {
        std::string id = std::to_string(ag_id);
        for (const auto& skill : ag_skills) {
            writer_inputs->write_skillset_json(skill);
        }

        writer_inputs->save_skillset_json(user, "", ofileName, destination_folder);
    }
    catch (std::exception const& ex) {
        throw std::runtime_error("[ERROR] While reading file "+ path + ": " + ex.what());
    }
}
// ---------------------------------------------------------------------------------------------------------------

// Support functions: -------------------------------------------------------------------------------------------
std::string ConvertFile::convertJsonValueToVariant(json json_value, std::variant<int, double, bool, std::string>& value) {
    // Note: in json we do not have type "DOUBLE". We only have: boolean, number_unsigned, number_float, string
    if(json_value.type() == json::value_t::number_float) {
        value = json_value.get<double>();
        return "double";
    }
    else if(json_value.type() == json::value_t::number_unsigned) {
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

Goal::Status ConvertFile::convertStringToGoalStatus(std::string value) {
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

Task::Execution ConvertFile::convertStringToTaskExecutionType(std::string value, std::string scheduler) {
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

Goal::Execution ConvertFile::convertStringToGoalExecutionType(std::string value, std::string scheduler) {
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
        std::cerr << "Goal was neither SEQ nor PAR...." << std::endl;
        EV_ERROR << "Goal was neither SEQ nor PAR...." << std::endl;
        return Goal::EXEC_SEQUENTIAL;
    }
}

Operator::Type ConvertFile::convertStringToOperator(std::string value) {
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
    throw std::runtime_error("Could not find the specified Operator!");
}

Sensor::Mode ConvertFile::convertStringToSensorMode(std::string value) {
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

std::string ConvertFile::checkIfFormulaIsSet(const std::string formula, const std::string msg) {
    std::string tmp_formula = boost::replace_all_copy(formula, " ", "");
    if(tmp_formula.size() == 0) { // set default value to "formula"
        std::cout << msg << " has invalid 'formula' parameter. Set it to default value = 0" << std::endl;
        EV_WARN << msg << " has invalid 'formula' parameter. Set it to default value = 0" << std::endl;
        return "0";
    }

    return formula;
}

bool ConvertFile::checkIfGoalHasCorrectBeliefName(const std::vector<std::shared_ptr<Belief>>& beliefset, const std::shared_ptr<Belief> action_belief)
{
    for(int i = 0; i < (int)beliefset.size(); i++) {
        if(beliefset[i]->get_name() == action_belief->get_name()) {
            return true;
        }
    }

    return false;
}

ServerType ConvertFile::convertIntToServerType(int value) {
    if(value == 0) {
        return CBS;
    }

    // should never reach this point...
    throw std::runtime_error("Could not find the specified Server Type!");
}

std::shared_ptr<Belief> ConvertFile::findBeliefWithGivenName(const std::vector<std::shared_ptr<Belief>>& beliefset,
        std::string belief_name_ref)
{
    for(std::shared_ptr<Belief> belief : beliefset) {
        if(belief->get_name() == belief_name_ref) {
            return belief;
        }
    }

    // belief not present...
    return nullptr;
}

int ConvertFile::getServerIndexInAg_servers(const std::vector<std::shared_ptr<Server>>& servers, const int server_id) {
    for(int i = 0; i < (int)servers.size(); i++) {
        if (servers[i]->getTaskServer() == server_id) {
            return i;
        }
    }
    return -1;
}

std::string ConvertFile::getSkillValueType_asString(std::variant<int, double, bool, std::string>& value)
{
    if(std::get_if<int>(&value))
    {
        return "int";
    }
    else if(std::get_if<double>(&value))
    {
        return "double";
    }
    else if(std::get_if<bool>(&value))
    {
        return "bool";
    }
    else if(std::get_if<std::string>(&value))
    {
        return "string";
    }

    return ""; // Type not supported (should not reach this point)
}


