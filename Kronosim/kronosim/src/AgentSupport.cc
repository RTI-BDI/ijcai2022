/*
 * AgentSupport.cc
 *
 *  Created on: Feb 2, 2021
 *      Author: francesco
 */

#include "../headers/AgentSupport.h"

AgentSupport::AgentSupport(int level) {
    logger = std::make_shared<Logger>(level);
    operator_entity = std::make_unique<Operator>();
}

AgentSupport::~AgentSupport() { }

void AgentSupport::ag_Sched_remove_task_in_queue(std::vector<std::shared_ptr<Task>>& queue_vector, int plan_id)
{
    for(int k = 0; k < (int)queue_vector.size(); k++)
    {
        if(queue_vector[k]->is_server())
        {
            // loop over the entire queue
            for(int q = 0; q < queue_vector[k]->queue_length(); q++) {
                if(queue_vector[k]->get_ith_task(q)->getTaskOriginalPlanId() == plan_id) {
                    queue_vector[k]->remove_element_from_queue(q);
                    q -= 1;
                }
            }
            if(queue_vector[k]->queue_length() == 0) {
                queue_vector.erase(queue_vector.begin() + k);
                k -= 1;
            }
            else {
                queue_vector[k]->setTaskId(queue_vector[k]->get_ith_task(0)->getTaskId());
                queue_vector[k]->setTaskPlanId(queue_vector[k]->get_ith_task(0)->getTaskPlanId());// BDI
                queue_vector[k]->setTaskOriginalPlanId(queue_vector[k]->get_ith_task(0)->getTaskOriginalPlanId());// BDI
                queue_vector[k]->setTaskDemander(queue_vector[k]->get_ith_task(0)->getTaskDemander());
                queue_vector[k]->setTaskExecuter(queue_vector[k]->get_ith_task(0)->getTaskExecuter());
                queue_vector[k]->setTaskReleaseTime(queue_vector[k]->get_ith_task(0)->getTaskReleaseTime());
                queue_vector[k]->setTaskStatus(queue_vector[k]->get_ith_task(0)->getTaskStatus()); // BDI
            }
        }
        else {
            if (queue_vector[k]->getTaskOriginalPlanId() == plan_id) {
                if (queue_vector.size() == 1) {
                    queue_vector.clear();
                } else {
                    queue_vector.erase(queue_vector.begin() + k);
                    k -= 1;
                }
            }
        }
    }
}

void AgentSupport::ag_Sched_ev_ag_tasks_in_queue(std::vector<std::shared_ptr<Task>> queue_vector, std::string name)
{
    logger->print(Logger::Default, name +"[tot = "+ std::to_string(queue_vector.size()) +"]:");

    for (auto task : queue_vector) {
        if (task->is_server()) {
            logger->print(Logger::Default, "- SERVER["+ std::to_string(task->getTaskServer()) +"]: curr_budget:["+ std::to_string(task->get_curr_budget())
            +"], curr_ddl:["+ std::to_string(task->get_curr_ddl())
            +"], queue.size:["+ std::to_string(task->queue_length())
            +"], deadline:["+ std::to_string(task->getTaskDeadline())
            +"], startTime:["+ std::to_string(task->get_startTime())
            +"], arrivalTime:["+ std::to_string(task->getTaskArrivalTime())
            +"], tskCres:["+ std::to_string(task->getTaskCompTimeRes())
            +"], tskStatus:["+ task->getTaskStatus_as_string()
            +"], tskExecType:["+ task->getTaskExecutionType_as_string()
            +"], isBeenAct:["+ boolToString(task->getTaskisBeenActivated())
            +"], task:["+ std::to_string(task->getTaskId())
            +"], plan:["+ std::to_string(task->getTaskPlanId())
            +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId()) +"]");

            for(int i = 0; i < task->queue_length(); i++) {
                std::shared_ptr<Task> tmp = task->get_ith_task(i);

                logger->print(Logger::Default, " -- task:["+ std::to_string(tmp->getTaskId())
                +"], plan:["+ std::to_string(tmp->getTaskPlanId())
                +"], original_plan:["+ std::to_string(tmp->getTaskOriginalPlanId())
                +"], deadline:["+ std::to_string(tmp->getTaskDeadline())
                +"], arrivalTime:["+ std::to_string(tmp->getTaskArrivalTime())
                +"], releaseTime:["+ std::to_string(tmp->getTaskReleaseTime())
                +"], N_exec left:["+ std::to_string(tmp->getTaskNExec())
                +" of "+ std::to_string(tmp->getTaskNExecInitial())
                +"], tskCres:["+ std::to_string(tmp->getTaskCompTimeRes())
                +"], tskStatus:["+ tmp->getTaskStatus_as_string()
                +"], tskExecType:["+ tmp->getTaskExecutionType_as_string()
                +"], isBeenAct:["+ boolToString(tmp->getTaskisBeenActivated()) +"]");
            }
        }
        else {
            logger->print(Logger::Default, "- Task:["+ std::to_string(task->getTaskId())
            +"], plan:["+ std::to_string(task->getTaskPlanId())
            +"], original_plan:["+ std::to_string(task->getTaskOriginalPlanId())
            +"], deadline:["+ std::to_string(task->getTaskDeadline())
            +"], arrivalTime:["+ std::to_string(task->getTaskArrivalTime())
            +"], releaseTime:["+ std::to_string(task->getTaskReleaseTime())
            +"], N_exec left:["+ std::to_string(task->getTaskNExec())
            +" of "+ std::to_string(task->getTaskNExecInitial())
            +"], tskCres:["+ std::to_string(task->getTaskCompTimeRes())
            +"], tskStatus:["+ task->getTaskStatus_as_string()
            +"], tskExecType:["+ task->getTaskExecutionType_as_string()
            +"], isBeenAct:["+ boolToString(task->getTaskisBeenActivated()) +"]");
        }
    }
}

std::string AgentSupport::boolToString(bool b) {
    return b ? "true" : "false";
}

bool AgentSupport::checkAndAddPlanToPlansInBatch(std::vector<std::shared_ptr<Plan>>& plans_in_batch, std::shared_ptr<Plan> plan) {
    logger->print(Logger::EveryInfo, " checkAndAddPlanToPlansInBatch: received:["+ std::to_string(plan->get_id())
    +"] Tot plans=["+ std::to_string(plans_in_batch.size()) +"]");

    // DEBUG purposes -------------------------------------------------------------------
    if(logger->get_level() >= Logger::EveryInfo) {
        for(int i = 0; i < (int)plans_in_batch.size(); i++) {
            logger->print(Logger::Default, " plans_in_batch:["+ std::to_string(i)
            +"], plan:["+ std::to_string(plans_in_batch[i]->get_id()) +"], name:["+ plans_in_batch[i]->get_goal_name() +"]");
        }
    }
    // ----------------------------------------------------------------------------------

    if(plans_in_batch.size() == 0) {
        plans_in_batch.push_back(plan);
        logger->print(Logger::EssentialInfo, " checkAndAddPlanToPlansInBatch: Plan:["+ std::to_string(plan->get_id())
        +"], name:["+ plan->get_goal_name()
        +"] has been added. Tot plans=["+ std::to_string(plans_in_batch.size()) +"]");
        return true;
    }
    else {
        for(int i = 0; i < (int)plans_in_batch.size(); i++) {
            if(plans_in_batch[i]->get_id() == plan->get_id()) {
                logger->print(Logger::EssentialInfo, " checkAndAddPlanToPlansInBatch: Plan:["+ std::to_string(plan->get_id())
                +"], goal:["+ plan->get_goal_name()
                +"] already analyzed in this batch (same plan.id)! -> ignore it");

                return false;
            }
            else if(plans_in_batch[i]->get_goal_name() == plan->get_goal_name())
                /* the original version also add && plans_in_batch[i].belief.value == plan.belief.value
                 * however, this means that 2 or more plans for the same belief may be activated simultaneously.
                 * we do not want this to happen.
                 * i.e. we can not have a robot re-charging the battery at 75 and 100 at the same time
                 * Thus, the AND condition has been removed -> && plans_in_batch[i]->get_belief()->get_value() == plan->get_belief()->get_value()) */
            {
                logger->print(Logger::EssentialInfo, " checkAndAddPlanToPlansInBatch: Plan:["+ std::to_string(plan->get_id())
                +"], goal:["+ plan->get_goal_name()
                +"] already analyzed in this batch (different Plan:["+ std::to_string(plans_in_batch[i]->get_id())
                +"] has same belief.name)! -> ignore it");

                return false;
            }
        }
    }

    plans_in_batch.push_back(plan);
    logger->print(Logger::EssentialInfo, " checkAndAddPlanToPlansInBatch: Plan:["+ std::to_string(plan->get_id())
    +"], goal:["+ plan->get_goal_name()
    +"] has been added. Tot plans=["+ std::to_string(plans_in_batch.size()) +"]");
    return true;
}

void AgentSupport::checkAndManageIntentionDerivedFromIG(int plan_id, int plan, int intention_goal_id,
        const std::vector<std::shared_ptr<Belief>>& beliefset, std::shared_ptr<Goal> achievedGoal,
        std::shared_ptr<Intention> intention)
{
    // step 5: -----------------------------------------------------------
    logger->print(Logger::EveryInfo, "plan_id: "+ std::to_string(plan_id));
    std::shared_ptr<Goal> goal, g_head;
    std::shared_ptr<Task> task, t_head;
    bool checkNextAction = true;
    std::string achievedGoal_name = "";
    int isPlanBeenRemoved = 0;
    int internal_goal_in_first_batch = 0;

    std::vector<std::shared_ptr<Action>> actions = intention->get_Actions();
    for(int i = 0; i < (int)actions.size() && checkNextAction; i++)
    {
        checkNextAction = false;
        if(actions[i]->get_type() == "goal")
        {
            goal = std::dynamic_pointer_cast<Goal>(actions[i]);
            internal_goal_in_first_batch += 1;

            logger->print(Logger::EveryInfo, "gisBeenAct: "+ boolToString(goal->getGoalisBeenActivated())
                    +", name: "+ goal->get_goal_name());

            // !!!!! ADDED: in order to correctly manage FCFS
            // !!!!! + in EDF and RR if I have the same goal "G" in the first batch of multiple intentions,
            // when "G" gets completed, its action is removed only from the intention that triggered its activation.
            // In the other intentions (x) the internal goal will not be considered accomplished.
            // If in (x) the internal goal had arrivalTime > 0 and s.t. activation + arrival time < accomplishment of the goal,
            // then ig in (x) is still considered not accomplished and will be activated as if arrivalTime was 0 (because absolute arrivalTime is actually in the past)
            if(goal->getGoalisBeenActivated())
            {
                logger->print(Logger::EveryInfo, "name: "+ goal->get_goal_name() +" == "+ achievedGoal->get_goal_name());

                if(goal->get_goal_name() == achievedGoal->get_goal_name())
                {
                    logger->print(Logger::EveryInfo, "IG absolute arrival time < now? ("+ std::to_string(goal->get_absolute_arrival_time())
                    +" < "+ std::to_string(sim_current_time().dbl()) +") = "+ boolToString(goal->get_absolute_arrival_time() < sim_current_time().dbl()));

                    if(goal->get_absolute_arrival_time() < sim_current_time().dbl())
                    {
                        intention->pop_Action_and_nothing_more(i, plan);
                        actions = intention->get_Actions();
                        i -= 1;
                        isPlanBeenRemoved += 1;

                        if((int)actions.size() > 0)
                        {
                            // check if the action that is now first in the intention has isBeenActivated = false
                            if(actions[0]->get_type() == "goal") {
                                g_head = std::dynamic_pointer_cast<Goal>(actions[0]);

                                logger->print(Logger::EveryInfoPlus, "> HEAD is GOAL:[alreadyActive="+ boolToString(g_head->getGoalisBeenActivated())
                                        +"] && different name: "+ g_head->get_goal_name() +" != "+ goal->get_goal_name());

                                if(g_head->getGoalisBeenActivated() == false
                                        && g_head->get_goal_name() != goal->get_goal_name())
                                {
                                    // This means that we executed the last action in the first batch,
                                    // and now the second batch has became the first one.
                                    intention->set_startEqualLastExecution(sim_current_time().dbl());

                                    if(g_head->getGoalExecutionType() == Goal::EXEC_SEQUENTIAL) {
                                        intention->set_batch_startTime(sim_current_time().dbl());
                                    }
                                }
                            }
                            else if(actions[0]->get_type() == "task") {
                                t_head = std::dynamic_pointer_cast<Task>(actions[0]);

                                logger->print(Logger::EveryInfoPlus, "> HEAD is TASK:[alreadyActive="+ boolToString(t_head->getTaskisBeenActivated()) +"]");
                                if(t_head->getTaskisBeenActivated() == false) {
                                    // This means that we executed the last action in the first batch,
                                    // and now the second batch has became the first one.
                                    intention->set_startEqualLastExecution(sim_current_time().dbl());

                                    if(t_head->getTaskExecutionType() == Task::EXEC_SEQUENTIAL) {
                                        intention->set_batch_startTime(sim_current_time().dbl());
                                    }
                                }
                            }
                        }
                        else { // if the intention is now empty (this function was called by the last task's execution), update lastExecution parameter
                            intention->set_startEqualLastExecution(sim_current_time().dbl());
                        }
                    }
                }
            }
        }

        if(i + 1 < (int)actions.size()) {
            if(actions[i + 1]->get_type() == "task") {
                task = std::dynamic_pointer_cast<Task>(actions[i + 1]);
                if(task->getTaskExecutionType() == Task::EXEC_PARALLEL) {
                    checkNextAction = true;
                }
            }
            else if(actions[i + 1]->get_type() == "goal") {
                goal = std::dynamic_pointer_cast<Goal>(actions[i + 1]);
                if(goal->getGoalExecutionType() == Goal::EXEC_PARALLEL) {
                    checkNextAction = true;
                }
            }
        }
    }

    if(isPlanBeenRemoved > 0) {
        // loop over all plans and remove the internal goal's plan (if present)
        std::shared_ptr<MaximumCompletionTime> mainPlan = intention->getMainPlan();
        std::vector<std::pair<std::shared_ptr<Plan>, int>> subplans = mainPlan->getInternalGoalsSelectedPlans();

        /* If 'tmp_plan_id' was present as an action in this intention,
         * we must remove such reference from the subplans and internal goals of the intention.
         * However, few different scenarios may occur:
         * 1. tmp_plan_id is not in the current first batch of this intention -> isPlanBeenRemoved will be = 0
         * -> we must not modify anything
         * 2. tmp_plan_id is in the first batch of this intention -> isPlanBeenRemoved > 0
         * -> remove relationship
         * 3. like 2. but we have more than tmp_plan_id in the first batch -> isPlanBeenRemoved = N > 0
         * -> remove all N relationships
         * 4. like 1. but we have more than tmp_plan_id in the intention (same batch/different batches)
         * -> isPlanBeenRemoved = 0 -> we must not modify anything
         * 5. combination of the above cases -> only remove plans from the first batch
         * -> if isPlanBeenRemoved > 0, they must all be in the first batch  */
        int copy_isPlanBeenRemoved = isPlanBeenRemoved;

        for(int i = 0; i < (int)subplans.size() && copy_isPlanBeenRemoved > 0; i++) {
            if(subplans[i].first->get_id() == plan_id) {
                intention->pop_internal_goal_plan(i);
                subplans.erase(subplans.begin() + i);
                i -= 1;
                copy_isPlanBeenRemoved -= 1;
            }
        }

        // loop over all internal_goals and remove the completed one (if present)
        std::vector<std::stack<std::tuple<std::shared_ptr<Goal>, int, int>>> internalgoals = intention->get_internal_goals();
        copy_isPlanBeenRemoved = isPlanBeenRemoved;

        for(int i = 0; i < (int)internalgoals.size() && copy_isPlanBeenRemoved > 0; i++) {
            if(std::get<1>(internalgoals[i].top()) == plan_id) {
                intention->pop_internal_goals(i);
                internalgoals.erase(internalgoals.begin() + i);
                i -= 1;
                copy_isPlanBeenRemoved -= 1;
            }
        }
    }
    else {
        std::shared_ptr<MaximumCompletionTime> mainPlan = intention->getMainPlan();
        std::vector<std::pair<std::shared_ptr<Plan>, int>> subplans = mainPlan->getInternalGoalsSelectedPlans();
        bool keep_checking = true;

        /* Check if the completed intention was a subplan with annidation level > 2
         * This means looking for subplans that are not linked to internal goals present in the body of this intention,
         * but rather of internal goal of the internal goal of the current intention
         * i.e. Intention[0].body = [G:P8, T, T] with P8 containing:
         * - P6 - annidation 1 (internal goal of P8)
         * - P5 - annidation 2 (internal goal of P6) <- we are looking for annidation 2 or bigger
         * - P1 - annidation 1 (internal goal of P8) */

        logger->print(Logger::EveryInfoPlus, "internal_goal_in_first_batch: "+ std::to_string(internal_goal_in_first_batch));
        for(int i = 0; i < (int)subplans.size() && (internal_goal_in_first_batch > 0 || keep_checking); i++) {
            keep_checking = false;
            logger->print(Logger::EveryInfoPlus, " subplan.belief.name:["+ (subplans[i].first->get_goal_name()) +"]");

            if(subplans[i].second == 1) {
                internal_goal_in_first_batch -= 1;
            }
            else if(subplans[i].first->get_id() == plan_id && subplans[i].second > 1) {
                logger->print(Logger::EveryInfo, " ==>> intention:["+ std::to_string(intention->get_id())
                +"], from goal:["+ std::to_string(intention->get_goalID())
                +"], goal:("+ intention->get_goal_name()
                +"), plan:["+ std::to_string(intention->get_planID())
                +"], original_plan:["+ std::to_string(intention->get_original_planID())
                +"], contained plan:["+ std::to_string(plan_id)
                +"], belief:["+ subplans[i].first->get_goal_name()
                +"] as subplan of level: "+ std::to_string(subplans[i].second)+ " <<== ");
                intention->pop_internal_goal_plan(i);
                subplans.erase(subplans.begin() + i);
                i -= 1;
            }

            if(internal_goal_in_first_batch == 0 && i + 1 < (int)subplans.size()) {
                if(subplans[i + 1].second > 1) {
                    // if we enter here, it means that we are analyzing the last goal in the PAR batch...
                    // thus, we must "keep_checking" as long as we find subplans with annidation level > 1
                    // meaning that are internal goals of the last goal in the PAR batch
                    keep_checking = true;
                }
            }
        }
    }
}

void AgentSupport::checkAndRemovePlanToPlansInBatch(std::vector<std::shared_ptr<Plan>>& plans_in_batch, std::shared_ptr<Plan> plan) {
    if(plans_in_batch.size() == 0) {
        logger->print(Logger::EssentialInfo, " checkAndRemovePlanToPlansInBatch: Plan:["+ std::to_string(plan->get_id())
        +"] not found. Tot plans=["+ std::to_string(plans_in_batch.size()) +"]");
    }
    else {
        for(int i = 0; i < (int)plans_in_batch.size(); i++) {
            if(plans_in_batch[i]->get_id() == plan->get_id()) {
                plans_in_batch.erase(plans_in_batch.begin() + i);
                logger->print(Logger::EssentialInfo, " checkAndRemovePlanToPlansInBatch: Plan:["+ std::to_string(plan->get_id()) +"] has been removed -> Tot plans=["+ std::to_string(plans_in_batch.size()) +"]");
            }
            else if(plans_in_batch[i]->get_goal_name() == plan->get_goal_name())
            {
                plans_in_batch.erase(plans_in_batch.begin() + i);
                logger->print(Logger::EssentialInfo, " checkAndRemovePlanToPlansInBatch: Plan:["+ std::to_string(plan->get_id()) +"] has been removed -> Tot plans=["+ std::to_string(plans_in_batch.size()) +"]");
            }
        }
    }
}

bool AgentSupport::checkIntentionStartBatchTime(std::shared_ptr<Intention> intention, std::shared_ptr<Ag_Scheduler> ag_Sched)
{ // Check if the parameter "batch_startTime" of the intention is up-to-date

    std::vector<std::shared_ptr<Action>> actions;
    std::shared_ptr<Task> task;
    std::shared_ptr<Goal> goal;
    bool task_in_ready = false, task_in_release = false;

    if(intention != nullptr)
    {
        actions = intention->get_Actions();
        if((int)actions.size() > 0)
        {
            if(actions[0]->get_type() == "task")
            {
                task = std::dynamic_pointer_cast<Task>(actions[0]);

                if(task->getTaskExecutionType() == Task::EXEC_SEQUENTIAL)
                {
                    if(logger->get_level() >= Logger::EveryInfo) { // EveryInfo
                        ag_Sched->ev_ag_tasks_vector_to_release();
                        logger->print(Logger::Default, "-----------------------------");
                        ag_Sched->ev_ag_tasks_vector_ready();
                        logger->print(Logger::Default, "-----------------------------");
                    }

                    // check if the task is in "ready" or "release"
                    task_in_ready = ag_Sched->check_is_task_in_ready(task);
                    task_in_release = ag_Sched->check_is_task_in_release(task);

                    // if the task is not active, and it is SEQUENTIAL, update the batch_startTime of the intetion (that was referring to the previous one)
                    if(task_in_ready == false && task_in_release == false)
                    {
                        // update only if now and startTime are different
                        if(checkIfDoublesAreEqual(intention->get_batch_startTime(), sim_current_time().dbl(), ag_Sched->EPSILON) == false)
                        {
                            logger->print(Logger::EveryInfo, " - (task) Updated intention:[id="+ std::to_string(intention->get_id())
                            +"] name:["+ intention->get_goal_name()
                            +"] batch_startTime:["+ std::to_string(intention->get_batch_startTime())
                            +"] to ["+ std::to_string(sim_current_time().dbl()) +"]");

                            intention->set_batch_startTime(sim_current_time().dbl());
                            return true;
                        }
                    }
                }
            }
            else if(actions[0]->get_type() == "goal")
            {
                goal = std::dynamic_pointer_cast<Goal>(actions[0]);

                if(goal->getGoalExecutionType() == Goal::EXEC_SEQUENTIAL)
                {
                    logger->print(Logger::EveryInfo, " Goal:["+ goal->get_goal_name() +"], absolute arrival time:["+ std::to_string(goal->get_absolute_arrival_time()) +"]");
                    // this means that the goal has not been activated nor scheduled to be activated,
                    // therefore, we must update the batch_startTime of the intention (that was referring to the previous one)
                    if(goal->get_absolute_arrival_time() == -1)
                    {
                        // update only if now and startTime are different
                        if(checkIfDoublesAreEqual(intention->get_batch_startTime(), sim_current_time().dbl(), ag_Sched->EPSILON) == false)
                        {
                            logger->print(Logger::EssentialInfo, " - (goal) Updated intention:[id="+ std::to_string(intention->get_id())
                            +"] name:["+ intention->get_goal_name()
                            +"] batch_startTime:["+ std::to_string(intention->get_batch_startTime())
                            +"] to ["+ std::to_string(sim_current_time().dbl()) +"]");

                            intention->set_batch_startTime(sim_current_time().dbl());
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

bool AgentSupport::checkPlanOrDesireConditions(const std::vector<std::shared_ptr<Belief>>& beliefset,
        const json conditions, std::string& msg, const std::string description, const int id,
        const std::vector<std::shared_ptr<Belief>>& expression_constants,
        const std::vector<std::shared_ptr<Belief>>& expression_variables)
{  /* ************************************************************************************************
 * Note: if the designer specifies "READ_VARIABLE" or "READ_CONSTANT" as part of the PRECONDITION expression,
 * the outcome will always be FALSE, the preconditions of the plan/desire do not hold!
 * ------------------------------------
 * if the designer specifies "READ_VARIABLE" or "READ_CONSTANT" as part of the CONT_CONDITION expression of a DESIRE,
 * the outcome will always be FALSE, the CONT_CONDITION of the desire do not hold!
 * ------------------------------------
 * if the designer specifies "READ_VARIABLE" or "READ_CONSTANT" as part of the CONT_CONDITION expression of a PLAN,
 * the value of both variables and constants will be searched in the list of variables and constants defined within the "effects_at_begin" linked to the PLAN
 * (if the specified variables or constants have not been defined, the outcome will be FALSE)
 * ************************************************************************************************
 * Valid structure(s):
 * - conditions:[] --> empty array
 * - conditions:[ bool value ]
 * - conditions:[ [ "operator", parameter1...N ] ] --> changes according to the "operator" selected. Operator is either a logical or a numerical operator
 * NOT valid structure(s):
 * - conditions:[ "operator", ["READ", "name"], parameter ]
 * - conditions:[ numerical value ]
 * ************************************************************************************************ */
    logger->print(Logger::EveryInfo, "----checkPlanOrDesireConditions----");

    AgentSupport::ExpressionBoolResult result;
    bool is_first_a_read_belief = false;
    std::string warning_msg = "";

    logger->print(Logger::EveryInfo, "Conditions: "+ conditions.dump());

    if(conditions.size() == 0) {
        msg = "ok: No Condition found! Therefore, valid!";
        logger->print(Logger::EveryInfo, " checkConditions for:["+ description +"], id:["+ std::to_string(id) +"]: No Condition found! Therefore, valid!");
        return true;
    }
    else if(conditions.size() == 1) {
        if(conditions[0].type() == json::value_t::boolean)
        { // "conditions": [ true ] or [ false ]
            msg = "ok";
            return conditions[0].get<bool>();
        }
        else {
            int log_level = logger->get_level(); // Debug
            // /* xx */logger->set_level(Logger::EveryInfoPlus);

            if(conditions.type() == json::value_t::array) {
                if(conditions[0].size() == 2) {
                    is_first_a_read_belief = true;
                    result = effectLogicExpression(conditions[0], beliefset, expression_variables, expression_constants, 1);
                }
            }
            if(is_first_a_read_belief == false) {
                result = effectLogicExpression(conditions[0], beliefset, expression_variables, expression_constants);
            }
            msg = result.msg;

            logger->print(Logger::EveryInfo, " checkConditions for:["+ description +"], id:["+ std::to_string(id) +"]: \n - errors: "+ boolToString(result.generated_errors)
                    +"\n - msg: " + result.msg
                    +"\n - value: " + boolToString(result.value));

            if(result.generated_errors) {
                warning_msg = " checkConditions for:["+ description +"], id:["+ std::to_string(id) +"]: "+ result.msg;
                warning_msg += ", expression:["+ dump_expression(conditions[0], beliefset, expression_constants, expression_variables) +"]";
                logger->print(Logger::Warning, warning_msg);
            }
            logger->print(Logger::EveryInfo, " are conditions valid: "+ boolToString(result.value));
            logger->print(Logger::EveryInfo, " -------------------------------------- ");

            logger->set_level(log_level);
            return result.value;
        }
    }
    else {
        msg = " checkConditions for:["+ description +"], id:["+ std::to_string(id) +"]: Condition must have size equal 0 or 1! It is:["+ std::to_string(conditions.size()) +"]!";
        logger->print(Logger::Warning, msg);
        return false;
    }
}

bool AgentSupport::checkDesireToAvoidDuplicatedGoals(const std::vector<std::shared_ptr<Goal>>& goalset, const std::shared_ptr<Goal> goal) {
    for(int i = 0; i < (int)goalset.size(); i++) {
        if(goalset[i]->get_goal_name() == goal->get_goal_name()) {
            return true;
        }
    }
    return false;
}

std::shared_ptr<Belief> AgentSupport::checkIfBeliefExists(const std::vector<std::shared_ptr<Belief>>& beliefset,
        const std::string name)
{
    for(std::shared_ptr<Belief> belief : beliefset) {
        if(belief->get_name() == name) {
            return belief;
        }
    }

    return nullptr;
}

bool AgentSupport::checkIfDesireIsGoal(const std::shared_ptr<Desire> desire, const std::vector<std::shared_ptr<Goal>>& goalset)
{
    for(int i = 0; i < (int)goalset.size(); i++)
    {
        if(desire->get_id() == goalset[i]->get_id()
                && desire->get_goal_name() == goalset[i]->get_goal_name())
        {
            return true;
        }
    }

    return false;
}

bool AgentSupport::checkIfDesireIsLinkedToValidSkill(std::shared_ptr<Desire> desire, const std::vector<std::shared_ptr<Skill>>& skillset)
{
    for(int i = 0; i < (int)skillset.size(); i++) {
        if(desire->get_goal_name() == skillset[i]->get_goal_name()) {
            return true;
        }
    }

    return false;
}

bool AgentSupport::checkIfDoublesAreEqual(const double v1, const double v2, const double tollerance) {
    return (v1 - tollerance) < v2 && (v1 + tollerance) > v2;
}

bool AgentSupport::checkIfGoalIsInGoalset(std::shared_ptr<Goal> goal, const std::vector<std::shared_ptr<Goal>>& goalset, bool isFCFSMode) {
    for(int i = 0; i < (int)goalset.size(); i++) {
        logger->print(Logger::EveryInfo, "checkIfGoalIsInGoalset: "+ std::to_string(goal->get_id())
        +", "+ goalset[i]->get_goal_name()
        +" "+ goal->get_goal_name());

        if(isFCFSMode == false) {
            // Note: using goalset[i]->'get_belief()' allows to manage both the case where the goal in the goalset is derived from an internal goal of a plan, both if it was a desire.
            if(goalset[i]->get_goal_name() == goal->get_goal_name())
                // [*] Even if we have goals with the same belief but different final state,
                // only one can be pursued at the time. Thus, this is not needed&& compareBeliefValues(goalset[i]->get_belief()->get_value(), goal->get_belief()->get_value(), Operator::EQUAL))
            {
                return true;
            }
        }
        else {
            /* in FCFS, if we have two intentions, such that the second is the desire of an internal goal of the first one,
             * we still want to activate the internal goal (and have two intention for the same belief) or otherwise, the program will "stall". */
            if(goalset[i]->get_id() == goal->get_id()
                    && goalset[i]->get_goal_name() == goal->get_goal_name())
                // same as [*] && compareBeliefValues(goalset[i]->get_belief()->get_value(), goal->get_belief()->get_value(), Operator::EQUAL))
            {
                return true;
            }
        }
    }
    return false;
}

bool AgentSupport::checkIfGoalIsLinkedToIntention(const std::shared_ptr<Goal> goal, const std::vector<std::shared_ptr<Intention>>& intentionset, int& index_intention)
{
    for(int i = 0; i < (int)intentionset.size(); i++)
    {
        if(goal->get_goal_name() == intentionset[i]->get_goal_name())
        {
            index_intention = i;
            return true;
        }
    }

    index_intention = -1;
    return false;
}


bool AgentSupport::checkIfGoalIsLinkedToValidSkill(std::shared_ptr<Goal> goal, const std::vector<std::shared_ptr<Skill>>& skillset)
{
    for(int i = 0; i < (int)skillset.size(); i++) {
        if(goal->get_goal_name() == skillset[i]->get_goal_name()) {
            return true;
        }
    }

    return false;
}

void AgentSupport::checkIfGoalWasActivatedInOtherIntentions(std::shared_ptr<Goal> g, std::vector<std::shared_ptr<Intention>>& intentionset) {
    std::shared_ptr<Goal> goal;

    for(int i = 0; i < (int)intentionset.size(); i++)
    {
        std::vector<std::shared_ptr<Action>> actions = intentionset[i]->get_Actions();
        for(int y = 0; y < (int)actions.size(); y++)
        {
            if(actions[y]->get_type() == "goal")
            {
                goal = std::dynamic_pointer_cast<Goal>(actions[y]);

                /* When the main intention from which we derived an internal goal is discarded or force a reasoning cycle,
                 * all the sub-intentions plan_id (as well as contained tasks and goals) gets updated and set = original_plan_id.
                 * However, the goal in goalset is not updated.
                 * Thus, something like this may happens:
                 * - es. "simulation _test-phiI_only_period-INF.8_seed_4" has goal = 4,9,9; while goal = 4,7,9.
                 * The result is that we can't compare goal->get_plan_id() == goal->get_plan_id()
                 * SOL: if(goal->get_belief()->get_name() == goal->get_belief()->get_name()) { Note: removed 'goal->get_id() == goal->get_id() && ' */
                if(goal->get_goal_name() == g->get_goal_name()) {
                    goal->setGoalisBeenActivated(false);
                    goal->setGoalforceReasoningCycle();
                }
            }
        }
    }
}

bool AgentSupport::checkIfPlanAlreadyCounted(const std::shared_ptr<Plan> plan, std::vector<std::shared_ptr<Plan>>& contained_plans, std::string& msg, const std::string nesting_spaces)
{
    std::string loop_order = "";

    for(int i = 0; i < (int)contained_plans.size(); i++) {
        loop_order += " > plan:["+ std::to_string(contained_plans[i]->get_id()) +"], goal:["+ contained_plans[i]->get_goal_name() +"]";
    }
    loop_order += " > plan:["+ std::to_string(plan->get_id()) +"], goal:["+ plan->get_goal_name() +"]";

    if(logger->get_level() >= Logger::EveryInfo)
    {
        logger->print(Logger::Default, "checkIfPlanAlreadyCounted:[tot="+ std::to_string(contained_plans.size()) +"]");
        logger->print(Logger::Default, loop_order);
    }

    for(int i = 0; i < (int)contained_plans.size(); i++) {
        logger->print(Logger::EveryInfo, "checkIfPlanAlreadyCounted: is '"+ std::to_string(plan->get_id())
        +"' already counted? "+ std::to_string(contained_plans[i]->get_id()));

        if(plan->get_goal_name() == contained_plans[i]->get_goal_name())
        {
            msg = "MainPlan:["+ std::to_string(contained_plans[0]->get_id());
            msg += "], goal:["+ contained_plans[0]->get_goal_name();
            msg += "].\n";
            msg += nesting_spaces + " Analyzed plan:["+ std::to_string(plan->get_id());
            msg += "] is NOT VALID, it contains internal goals that refer to the goal:["+ plan->get_goal_name();
            msg += "] of the main plan!\n";
            msg += nesting_spaces + " Order: "+ loop_order;

            return true;
        }
    }

    return false;
}

bool AgentSupport::checkIfPlanWasAlreadyIntention(const std::shared_ptr<Plan> plan, int& index_intention, const std::vector<std::shared_ptr<Intention>>& ag_intention_set) {
    for(int i = 0; i < (int)ag_intention_set.size(); i++)
    {
        if(ag_intention_set[i]->get_original_planID() == plan->get_id()
                && ag_intention_set[i]->get_goal_name() == plan->get_goal_name())
        {
            index_intention = i;
            return true;
        }
    }

    index_intention = -1;
    return false;
}

bool AgentSupport::checkIfPlanWasAlreadyIntention(const std::shared_ptr<Plan> plan, const std::vector<std::shared_ptr<Intention>>& ag_intention_set) {
    for(std::shared_ptr<Intention> intention : ag_intention_set)
    {
        if(intention->get_original_planID() == plan->get_id()
                && intention->get_goal_name() == plan->get_goal_name()) {
            return true;
        }
    }

    return false;
}

bool AgentSupport::checkIfPostconditionsAreValid(const std::shared_ptr<Intention> intention,
        const std::vector<std::shared_ptr<Belief>>& beliefset,
        const std::vector<std::shared_ptr<Belief>>& expression_constants,
        const std::vector<std::shared_ptr<Belief>>& expression_variables)
{
    bool valid_conditions = false;
    std::string return_msg = "";

    if(logger->get_level() >= Logger::EveryInfoPlus) {
        printBeliefset(beliefset);
        printBeliefset(intention->get_expression_variables(), " List of Variables [tot = "+ std::to_string(intention->get_expression_variables().size()) +"]:");
        logger->print(Logger::Default, " - Post_conditions: "+ intention->get_post_conditions().dump());
    }

    logger->print(Logger::EveryInfo, intention->get_goal_name() +", POST_CONDITION: "+ dump_expression(intention->get_post_conditions(), beliefset, expression_constants, expression_variables));

    valid_conditions = checkPlanOrDesireConditions(beliefset, intention->get_post_conditions(), return_msg, "plan", intention->get_original_planID(), expression_constants, expression_variables);

    logger->print(Logger::EveryInfo, "valid_conditions:["+ boolToString(valid_conditions)
            +"] AND message:["+ return_msg +"]");

    if(valid_conditions == false)
    {
        logger->print(Logger::Warning, " X (checkIfPostconditionsAreValid) -> Intention:["+ std::to_string(intention->get_id())
        +"], plan:["+ std::to_string(intention->get_planID())
        +"], original_plan:["+ std::to_string(intention->get_original_planID())
        +"], goal_name:["+ intention->get_goal_name()
        +"] postconditions DO NOT hold! Post_conditions: "+ dump_expression(intention->get_post_conditions(), beliefset, expression_constants, expression_variables) +" X");
    }

    return valid_conditions;
}

bool AgentSupport::checkIfPreconditionsAreValid(const std::shared_ptr<Plan> plan,
        const std::vector<std::shared_ptr<Belief>>& beliefset, const std::string debug)
{
    bool valid_conditions = false;
    std::string return_msg = "";

    logger->print(Logger::EveryInfo, "checkIfPrecConditionsAreValid: "+ plan->get_goal_name() +", PRECONDITION: "+ dump_expression(plan->get_preconditions(), beliefset));

    valid_conditions = checkPlanOrDesireConditions(beliefset, plan->get_preconditions(), return_msg, "plan", plan->get_id());

    logger->print(Logger::EveryInfo, "valid_conditions:["+ boolToString(valid_conditions)
            +"] AND message:["+ return_msg +"]");

    if(valid_conditions == false)
    {
        logger->print(Logger::EssentialInfo, " X (checkIfPrecConditionsAreValid) -> Plan:[" + std::to_string(plan->get_id())
        + "], goal_name:["+ plan->get_goal_name() +"] preconditions DO NOT hold! Preconditions: "+ dump_expression(plan->get_preconditions(), beliefset) +" X");
    }

    return valid_conditions;
}

void AgentSupport::checkIfRemovedIntentionsWaitingCompletion(const std::vector<std::shared_ptr<Goal>>& goalset, std::vector<std::shared_ptr<Goal>>& goalset_copy,
        std::vector<std::shared_ptr<MaximumCompletionTime>>& selected_plans, const std::vector<std::shared_ptr<MaximumCompletionTime>>& selected_plans_copy,
        std::vector<std::shared_ptr<Intention>>& intentionset, const std::vector<std::shared_ptr<Intention>>& intentionset_copy)
{
    int index_intention = -1;
    bool skip_goal = false;

    if(logger->get_level() >= Logger::EveryInfo)
    {
        printGoalset(goalset);
        printGoalset(goalset_copy);
        logger->print(Logger::Default, " *-*-*-*-*-*-*-*-*-* ");

        printSelectedPlans(selected_plans);
        printSelectedPlans(selected_plans_copy);
        logger->print(Logger::Default, " *-*-*-*-*-*-*-*-*-* ");

        printIntentions(intentionset, goalset);
        printIntentions(intentionset_copy, goalset_copy);
        logger->print(Logger::Default, " *-*-*-*-*-*-*-*-*-* ");
    }

    for(int i = 0; i < (int)goalset.size(); i++)
    {
        index_intention = -1;
        skip_goal = false;

        if(checkIfGoalIsLinkedToIntention(goalset[i], intentionset_copy, index_intention))
        {
            if(intentionset_copy[index_intention]->get_Actions().size() == 0 && intentionset_copy[index_intention]->get_waiting_for_completion())
            {
                // check if the goal is already active and present in the goalset_copy. If yes, do not add it...
                for(int j = 0; j < (int)goalset_copy.size(); j++) {
                    if(goalset_copy[j]->get_goal_name() == goalset[i]->get_goal_name()) {
                        skip_goal = true;
                        break;
                    }
                }

                if(skip_goal == false) {
                    intentionset.push_back(std::make_shared<Intention>(*intentionset_copy[index_intention]));
                    goalset_copy.push_back(std::make_shared<Goal>(*goalset[i]));

                    // unfortunately, selected_plans and intentions may not be aligned. Thus, to get the correct selectedPlan we must iterate on all available ones...
                    for(int j = 0; j < (int)selected_plans_copy.size(); j++)
                    {
                        if(goalset[i]->get_goal_name() == selected_plans_copy[j]->getInternalGoalsSelectedPlans()[0].first->get_goal_name()) {
                            selected_plans.push_back(std::make_shared<MaximumCompletionTime>(*selected_plans_copy[j]));
                            break;
                        }
                    }
                }
            }

        }
    }

    if(logger->get_level() >= Logger::EveryInfo)
    {
        printGoalset(goalset);
        printGoalset(goalset_copy);
        logger->print(Logger::Default, " *-*-*-*-*-*-*-*-*-* ");

        printSelectedPlans(selected_plans);
        printSelectedPlans(selected_plans_copy);
        logger->print(Logger::Default, " *-*-*-*-*-*-*-*-*-* ");

        printIntentions(intentionset, goalset);
        printIntentions(intentionset_copy, goalset_copy);
        logger->print(Logger::Default, " *-*-*-*-*-*-*-*-*-* ");
    }
}

bool AgentSupport::checkIfServerAlreadyCounted(const std::shared_ptr<ServerHandler> ag_Server_Handler, const std::shared_ptr<Task> task,
        bool& server_already_counted, std::vector<int>& servers_needed, const Logger::Level logger_level)
{
    server_already_counted = false;

    if(task->getTaskServer() != -1)
    { // APERIODIC
        logger->print(Logger::EveryInfo, " Task SERVER is: "+ std::to_string(task->getTaskServer()));

        if(IsServerAlreadyUsed(task, servers_needed) == -1) {
            logger->print(logger_level, " Task:["+ std::to_string(task->getTaskId())
            +"], plan:["+ std::to_string(task->getTaskPlanId())
            +"], original plan:["+ std::to_string(task->getTaskOriginalPlanId()) +"]");

            server_already_counted = false;
            servers_needed.push_back(task->getTaskServer());
        }
        else {
            server_already_counted = true;

            // V1: server already counted are not summed again
            //            logger->print(Logger::EveryInfo, "Task:["+ std::to_string(task->getTaskId())
            //                    +"], plan:["+ std::to_string(task->getTaskPlanId())
            //                    +"], NOT ADDED TO U.");
            // V2: overlapping tasks linked to the same server, all affect the overall U (18-08-21)
        }
    }

    return server_already_counted;
}

double AgentSupport::checkIfServerIsActiveAndGetDDL(const int server_id, const std::vector<std::shared_ptr<Task>> ready, const std::vector<std::shared_ptr<Task>> release) {
    // check if server is already "active" -> meaning that it's either in "ready" or "release" queue
    for(int i = 0; i < (int)ready.size(); i++)
    {
        if(ready[i]->is_server() && ready[i]->getTaskServer() == server_id) {
            logger->print(Logger::EveryInfo, "S["+ std::to_string(server_id)
            +"] in READY, absDDL:["+ std::to_string(ready[i]->get_curr_ddl())
            +"], activation: "+ std::to_string(ready[i]->get_startTime()));
            return ready[i]->get_curr_ddl(); // it is going to be summed back when function 'getTaskDeadline_based_on_nexec' gets called
        }
    }

    for(int i = 0; i < (int)release.size(); i++)
    {
        if(release[i]->is_server() && release[i]->getTaskServer() == server_id) {
            logger->print(Logger::EveryInfo, "S["+ std::to_string(server_id)
            +"] in RELEASE, absDDL:["+ std::to_string(release[i]->get_curr_ddl())
            +"], activation: "+ std::to_string(release[i]->get_startTime()));
            return release[i]->get_curr_ddl(); // it is going to be summed back when function 'getTaskDeadline_based_on_nexec' gets called
        }
    }

    return -2; // server not found
}

int AgentSupport::checkIfTaskAlreadyInServerQueue(std::shared_ptr<Task> server, std::shared_ptr<Task> task) {
    std::shared_ptr<Task> t_queue;

    if(server->get_server_id() != -1) {
        for(int i = 0; i < server->queue_length(); i++) {
            t_queue = server->get_ith_task(i);

            if(t_queue->getTaskId() == task->getTaskId()
                    && t_queue->getTaskPlanId() == task->getTaskPlanId()
                    && t_queue->getTaskOriginalPlanId() == task->getTaskOriginalPlanId())
            {
                return i;
            }
        }
    }

    return -1;
}

bool AgentSupport::checkIfTaskBelongsToIntentionInternalGoal(const std::shared_ptr<Task> task, const std::shared_ptr<Plan> plan,
        const std::vector<std::shared_ptr<Intention>>& ag_intention_set)
{
    if(task->getTaskOriginalPlanId() == plan->get_id()) {
        return false; // the task belongs to the main plan, from which the intention is derived...
    }
    else {
        for(int i = 0; i < (int)ag_intention_set.size(); i++) {
            if(task->getTaskOriginalPlanId() == ag_intention_set[i]->get_original_planID()) {
                return true;
            }
        }
    }

    return false;
}

bool AgentSupport::checkIfTasksInReadyOrRealeaseQueue(const std::shared_ptr<Task> task, const std::vector<std::shared_ptr<Task>> ag_tasks_vector_to_release, const std::vector<std::shared_ptr<Task>> ag_tasks_vector_ready) {
    for(int k = 0; k < (int)ag_tasks_vector_to_release.size(); k++)
    {
        if(ag_tasks_vector_to_release[k]->getTaskPlanId() == task->getTaskPlanId()
                && ag_tasks_vector_to_release[k]->getTaskOriginalPlanId() == task->getTaskOriginalPlanId())
        {
            return true;
        }
    }

    for(int k = 0; k < (int)ag_tasks_vector_ready.size(); k++)
    {
        if(ag_tasks_vector_ready[k]->getTaskPlanId() == task->getTaskPlanId()
                && ag_tasks_vector_ready[k]->getTaskOriginalPlanId() == task->getTaskOriginalPlanId())
        {
            return true;
        }
    }

    return false;
}

bool AgentSupport::checkIfTaskIsLinkedToIntention(const std::shared_ptr<Task> task, const std::vector<std::shared_ptr<Intention>>& intentionset, int& index_intention)
{
    for(int i = 0; i < (int)intentionset.size(); i++) {
        if(intentionset[i]->get_original_planID() == task->getTaskOriginalPlanId()) {
            index_intention = i;
            return true;
        }
    }

    index_intention = -1;
    return false;
}

bool AgentSupport::compareBeliefValues(std::variant<int, double, bool, std::string> belief_value,
        std::variant<int, double, bool, std::string> comparison_value, Operator::Type op)
{   // Example: if we have 'op = LESS', what the code will do is: belief < comparison_value
    bool res = false;

    switch(op) {
    case Operator::LESS:
        if(auto b = std::get_if<int>(&belief_value))
        {
            if(auto value_b = std::get_if<int>(&comparison_value)) {
                int& ab = *b;
                int& db = *value_b;
                res = (ab < db);
                logger->print(Logger::EveryInfoPlus, "LESS INT-INT: "+ std::to_string(ab) +" < "+ std::to_string(db) +" ? "+ std::to_string(res));
            }
            else if(auto value_b = std::get_if<double>(&comparison_value)) {
                int& ab = *b;
                double& db = *value_b;
                res = (ab < db);
                logger->print(Logger::EveryInfoPlus, "LESS INT-DOUBLE: "+ std::to_string(ab) +" < "+ std::to_string(db) +" ? " + std::to_string(res));
            }
        }
        else if(auto b = std::get_if<double>(&belief_value))
        {
            if(auto value_b = std::get_if<double>(&comparison_value)) {
                double& ab = *b;
                double& db = *value_b;
                res = (ab < db);
                logger->print(Logger::EveryInfoPlus, "LESS DOUBLE-DOUBLE: "+ std::to_string(ab) +" < "+ std::to_string(db) +" ? " + std::to_string(res));
            }
            else if(auto value_b = std::get_if<int>(&comparison_value)) {
                double& ab = *b;
                int& db = *value_b;
                res = (ab < db);
                logger->print(Logger::EveryInfoPlus, "LESS DOUBLE-INT: "+ std::to_string(ab) +" < "+ std::to_string(db) +" ? "+ std::to_string(res));
            }
        }
        break;
    case Operator::LESS_EQUAL:
        if(auto b = std::get_if<int>(&belief_value))
        {
            if(auto value_b = std::get_if<int>(&comparison_value)) {
                int& ab = *b;
                int& db = *value_b;
                res = (ab <= db);
                logger->print(Logger::EveryInfoPlus, "LESS_EQUAL INT-INT: "+ std::to_string(ab) +" <= "+ std::to_string(db) +" ? " + std::to_string(res));
            }
            else if(auto value_b = std::get_if<double>(&comparison_value)) {
                int& ab = *b;
                double& db = *value_b;
                res = (ab <= db);
                logger->print(Logger::EveryInfoPlus, "LESS_EQUAL INT-DOUBLE: "+ std::to_string(ab) +" <= "+ std::to_string(db) +" ? " + std::to_string(res));
            }
        }
        else if(auto b = std::get_if<double>(&belief_value))
        {
            if(auto value_b = std::get_if<double>(&comparison_value)) {
                double& ab = *b;
                double& db = *value_b;
                res = (ab <= db);
                logger->print(Logger::EveryInfoPlus, "LESS_EQUAL DOUBLE-DOUBLE: "+ std::to_string(ab) +" <= "+ std::to_string(db) +" ? " + std::to_string(res));
            }
            else if(auto value_b = std::get_if<int>(&comparison_value)) {
                double& ab = *b;
                int& db = *value_b;
                res = (ab <= db);
                logger->print(Logger::EveryInfoPlus, "LESS_EQUAL DOUBLE-INT: "+ std::to_string(ab) +" <= "+ std::to_string(db) +" ? " + std::to_string(res));
            }
        }
        break;
    case Operator::GREATER:
        if(auto b = std::get_if<int>(&belief_value))
        {
            if(auto value_b = std::get_if<int>(&comparison_value)) {
                int& ab = *b;
                int& db = *value_b;
                res = (ab > db);
                logger->print(Logger::EveryInfoPlus, "GREATER INT-INT: "+ std::to_string(ab) +" > "+ std::to_string(db) +" ? " + std::to_string(res));
            }
            else if(auto value_b = std::get_if<double>(&comparison_value)) {
                int& ab = *b;
                double& db = *value_b;
                res = (ab > db);
                logger->print(Logger::EveryInfoPlus, "GREATER INT-DOUBLE: "+ std::to_string(ab) +" > "+ std::to_string(db) +" ? " + std::to_string(res));
            }
        }
        else if(auto b = std::get_if<double>(&belief_value))
        {
            if(auto value_b = std::get_if<double>(&comparison_value)) {
                double& ab = *b;
                double& db = *value_b;
                res = (ab > db);
                logger->print(Logger::EveryInfoPlus, "GREATER DOUBLE-DOUBLE: "+ std::to_string(ab) +" > "+ std::to_string(db) +" ? " + std::to_string(res));
            }
            else if(auto value_b = std::get_if<int>(&comparison_value)) {
                double& ab = *b;
                int& db = *value_b;
                res = (ab > db);
                logger->print(Logger::EveryInfoPlus, "GREATER DOUBLE-INT: "+ std::to_string(ab) +" > "+ std::to_string(db) +" ? " + std::to_string(res));
            }
        }
        break;
    case Operator::GREATER_EQUAL:
        if(auto b = std::get_if<int>(&belief_value))
        {
            if(auto value_b = std::get_if<int>(&comparison_value)) {
                int& ab = *b;
                int& db = *value_b;
                res = (ab >= db);
                logger->print(Logger::EveryInfoPlus, "GREATER_EQUAL INT-INT: "+ std::to_string(ab) +" >= "+ std::to_string(db) +" ? " + std::to_string(res));
            }
            else if(auto value_b = std::get_if<double>(&comparison_value)) {
                int& ab = *b;
                double& db = *value_b;
                res = (ab >= db);
                logger->print(Logger::EveryInfoPlus, "GREATER_EQUAL INT-DOUBLE: "+ std::to_string(ab) +" >= "+ std::to_string(db) +" ? " + std::to_string(res));
            }
        }
        else if(auto b = std::get_if<double>(&belief_value))
        {
            if(auto value_b = std::get_if<double>(&comparison_value)) {
                double& ab = *b;
                double& db = *value_b;
                res = (ab >= db);
                logger->print(Logger::EveryInfoPlus, "GREATER_EQUAL DOUBLE-DOUBLE: "+ std::to_string(ab) +" >= "+ std::to_string(db) +" ? " + std::to_string(res));
            }
            else if(auto value_b = std::get_if<int>(&comparison_value)) {
                double& ab = *b;
                int& db = *value_b;
                res = (ab >= db);
                logger->print(Logger::EveryInfoPlus, "GREATER_EQUAL DOUBLE-INT: "+ std::to_string(ab) +" >= "+ std::to_string(db) +" ? " + std::to_string(res));
            }
        }
        break;
    case Operator::EQUAL:
        if(auto b = std::get_if<int>(&belief_value))
        {
            if(auto value_b = std::get_if<int>(&comparison_value)) {
                int& ab = *b;
                int& db = *value_b;
                res = (ab == db);
                logger->print(Logger::EveryInfoPlus, "EQUAL INT-INT: "+ std::to_string(ab) +" == "+ std::to_string(db) +" ? " + std::to_string(res));
            }
            else if(auto value_b = std::get_if<double>(&comparison_value)) {
                int& ab = *b;
                double& db = *value_b;
                res = (ab == db);
                logger->print(Logger::EveryInfoPlus, "EQUAL INT-DOUBLE: "+ std::to_string(ab) +" == "+ std::to_string(db) +" ? " + std::to_string(res));
            }
            else {
                logger->print(Logger::EveryInfoPlus, "INT-INT: Goal.belief:["+ variantToString(belief_value) +"], plan.belief:["+ variantToString(comparison_value) +"]. Plan belief and Goal belief has not the same type.");
            }
        }
        else if(auto b = std::get_if<double>(&belief_value))
        {
            if(auto value_b = std::get_if<double>(&comparison_value)) {
                double& ab = *b;
                double& db = *value_b;
                res = (ab == db);
                logger->print(Logger::EveryInfoPlus, "EQUAL DOUBLE-DOUBLE: "+ std::to_string(ab) +" == "+ std::to_string(db) +" ? " + std::to_string(res));
            }
            else if(auto value_b = std::get_if<int>(&comparison_value)) {
                double& ab = *b;
                int& db = *value_b;
                res = (ab == db);
                logger->print(Logger::EveryInfoPlus, "EQUAL DOUBLE-INT: "+ std::to_string(ab) +" == "+ std::to_string(db) +" ? " + std::to_string(res));
            }
        }
        else if(auto b = std::get_if<bool>(&belief_value))
        {
            if(auto value_b = std::get_if<bool>(&comparison_value)) {
                bool& ab = *b;
                bool& db = *value_b;
                res = (ab == db);
                logger->print(Logger::EveryInfoPlus, "BOOL: "+ std::to_string(ab) +" == "+ std::to_string(db) +" ? " + std::to_string(res));
            }
        }
        else if(auto b = std::get_if<std::string>(&belief_value))
        {
            if(auto value_b = std::get_if<std::string>(&comparison_value)) {
                std::string& ab = *b;
                std::string& db = *value_b;
                res = (ab == db);
                logger->print(Logger::EveryInfoPlus, "STRING: "+ ab +" == "+ db +" ? "+ std::to_string(res));
            }
        }
        else {
            logger->print(Logger::EveryInfoPlus, "Not valid type found for the specified operator");
        }
        break;
    default: break;
    }

    return res;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
// compute Priority and Pref value either using the specified formula, or using the reference_table specified by the user
void AgentSupport::computeDesirePriority(std::shared_ptr<Desire>& desire, const std::vector<std::shared_ptr<Belief>>& beliefset) {
    double goal_pr = 0;
    AgentSupport::ExpressionDoubleResult result;
    std::string warning_msg = "";
    json formula;

    if(desire->get_computedDynamically())
    { // use formula:
        formula = desire->get_priority_formula();
        logger->print(Logger::EveryInfoPlus, "- desire:["+ std::to_string(desire->get_id())
        +"], belief:["+ desire->get_goal_name() +"]");

        result = formulaExpression(formula, beliefset);

        logger->print(Logger::EveryInfo, " - Formula Desire Priority: "+ formula.dump());
        logger->print(Logger::EveryInfo, " - EVALUATION: \n - errors: "+ boolToString(result.generated_errors)
                +"\n - msg: " + result.msg
                +"\n - value: " + std::to_string(result.value));
        logger->print(Logger::EveryInfoPlus, " ************************************** ");

        if(result.generated_errors == false) {
            goal_pr = result.value;
        }
        else {
            warning_msg = "[WARNING] Compute DESIRE:["+ desire->get_goal_name();
            warning_msg += "] priority: "+ result.msg;
            warning_msg += ", formula:["+ dump_expression(formula, beliefset) +"]";
            logger->print(Logger::EveryInfo, warning_msg);
            logger->print(Logger::EssentialInfo, " Priority of Desire:["+ desire->get_goal_name()
                    +"] set to: "+ std::to_string(goal_pr));
        }
        logger->print(Logger::EveryInfo, "goal_pr: "+ std::to_string(goal_pr));
        logger->print(Logger::EveryInfo, " -------------------------------------- ");
    }
    else { //use reference_table:
        std::vector<std::shared_ptr<Reference_table>> reference_table = desire->get_priority_reference_table();
        for(int r = 0; r < (int)reference_table.size(); r++) {
            // 1. check if the belief exists + 2. check if have they have the same type + 3. check activation:
            std::shared_ptr<Belief> tmp_bel = checkIfBeliefExists(beliefset, reference_table[r]->get_belief_name());
            if(tmp_bel != nullptr && compareBeliefValues(tmp_bel->get_value(), reference_table[r]->get_threshold_value(), reference_table[r]->get_operator())) {
                goal_pr = reference_table[r]->get_pr_pref();
            }
        }
    }

    if(goal_pr > 1) {
        warning_msg = "[WARNING] Priority of Desire:["+ std::to_string(desire->get_id());
        warning_msg += "] must be between [0,1]. Set to 1, was: "+ std::to_string(goal_pr);
        logger->print(Logger::EveryInfo, warning_msg);

        goal_pr = 1;
    }
    else if(goal_pr < 0) {
        warning_msg = "[WARNING] Priority of Desire:["+ std::to_string(desire->get_id());
        warning_msg += "] must be between [0,1]. Set to 0, was: "+ std::to_string(goal_pr);
        logger->print(Logger::EveryInfo, warning_msg);

        goal_pr = 0;
    }
    else {
        logger->print(Logger::EveryInfo, " Priority of Desire:["+ desire->get_goal_name()
                +"] set to: "+ std::to_string(goal_pr));
    }

    desire->set_priority(goal_pr);
}

void AgentSupport::computeGoalPriority(std::shared_ptr<Goal>& goal, const std::vector<std::shared_ptr<Belief>>& beliefset) {
    double goal_pr = 0;
    AgentSupport::ExpressionDoubleResult result;
    std::string warning_msg = "";
    json formula;

    int log_level = logger->get_level(); // Debug
    // /* xx */logger->set_level(Logger::EveryInfoPlus);

    formula = goal->get_priority_formula();
    logger->print(Logger::EveryInfo, "- goal:["+ std::to_string(goal->get_id())
    +"], belief:["+ goal->get_goal_name() +"]");

    result = formulaExpression(formula, beliefset);

    logger->print(Logger::EveryInfo, " - Formula Goal Priority: "+ formula.dump());

    logger->print(Logger::EveryInfo, " EVALUATION: \n - errors: "+ boolToString(result.generated_errors)
            +"\n - msg: " + result.msg
            +"\n - value: " + std::to_string(result.value));
    logger->print(Logger::EveryInfoPlus, " ************************************** ");

    if(result.generated_errors == false) {
        goal_pr = result.value;
    }
    else {
        goal_pr = 0;
        warning_msg = "[WARNING] Compute GOAL:["+ goal->get_goal_name();
        warning_msg += "] priority: "+ result.msg;
        warning_msg += ", formula:["+ dump_expression(formula, beliefset) +"]";

        logger->print(Logger::Warning, warning_msg);
        logger->print(Logger::EssentialInfo, " Priority of Goal:["+ goal->get_goal_name() +"] set to: "+ std::to_string(goal_pr));
    }
    logger->print(Logger::EveryInfo, "goal_pr: "+ std::to_string(goal_pr));
    logger->print(Logger::EveryInfo, " -------------------------------------- ");

    if(goal_pr > 1) {
        warning_msg = "[WARNING] Priority of Goal:["+ std::to_string(goal->get_id());
        warning_msg += "] must be between [0,1]. Set to 1, was: "+ std::to_string(goal_pr);
        logger->print(Logger::Warning, warning_msg);

        goal_pr = 1;
    }
    else if(goal_pr < 0) {
        warning_msg = "[WARNING] Priority of Goal:["+ std::to_string(goal->get_id());
        warning_msg += "] must be between [0,1]. Set to 0, was: "+ std::to_string(goal_pr);
        logger->print(Logger::Warning, warning_msg);

        goal_pr = 0;
    }
    else {
        logger->print(Logger::EveryInfo, " Priority of Goal:["+ goal->get_goal_name()
                +"] set to: "+ std::to_string(goal_pr));
    }

    logger->set_level(log_level);
    goal->set_priority(goal_pr);
}

double AgentSupport::computePlanPref(std::shared_ptr<Plan>& plan,
        const std::vector<std::shared_ptr<Belief>>& beliefset)
{
    double plan_pref = 0;
    AgentSupport::ExpressionDoubleResult result;
    std::string warning_msg = "";
    json formula;

    int log_level = logger->get_level(); // Debug
    // /* xx */logger->set_level(Logger::EveryInfoPlus);

    if(plan->get_computedDynamically())
    { // use formula:
        formula = plan->get_pref_formula();

        result = formulaExpression(formula, beliefset);

        logger->print(Logger::EveryInfo, " - Formula Plan Pref: "+ formula.dump());
        logger->print(Logger::EveryInfo, " - EVALUATION: \n - errors: "+ boolToString(result.generated_errors)
                +"\n - msg: " + result.msg
                +"\n - value: " + std::to_string(result.value));

        if(result.generated_errors == false) {
            plan_pref = result.value;
        }
        else {
            warning_msg = "[WARNING] Compute Plan:["+ std::to_string(plan->get_id());
            warning_msg += "], name:["+ plan->get_goal_name() +"] preference: "+ result.msg;
            warning_msg += ", formula:["+ dump_expression(formula, beliefset) +"]";
            logger->print(Logger::Warning, warning_msg);
            logger->print(Logger::EssentialInfo, " Preference of Plan:["+ std::to_string(plan->get_id())
            +"] name:["+ plan->get_goal_name()
            +"] set to: "+ std::to_string(plan_pref));
        }
        logger->print(Logger::EveryInfo, "plan_pref: "+ std::to_string(plan_pref));
        logger->print(Logger::EveryInfo, " -------------------------------------- ");
    }
    else { //use reference_table:
        std::vector<std::shared_ptr<Reference_table>> reference_table = plan->get_pref_reference_table();
        for(int r = 0; r < (int)reference_table.size(); r++) {
            // 1. check if the belief exists + 2. check if have they have the same type + 3. check activation:
            std::shared_ptr<Belief> tmp_bel = checkIfBeliefExists(beliefset, reference_table[r]->get_belief_name());
            if(tmp_bel != nullptr && compareBeliefValues(tmp_bel->get_value(), reference_table[r]->get_threshold_value(), reference_table[r]->get_operator()))
            {
                // update plan's pref:
                plan_pref = reference_table[r]->get_pr_pref();
            }
        }
    }

    if(plan_pref > 1) {
        warning_msg = "[WARNING] Pref of Plan:["+ std::to_string(plan->get_id());
        warning_msg += "], name:["+ plan->get_goal_name();
        warning_msg += "], must be between [0,1]. Set to 1, was: "+ std::to_string(plan_pref);
        logger->print(Logger::Warning, warning_msg);
        plan_pref = 1;
    }
    else if(plan_pref < 0) {
        warning_msg = "[WARNING] Pref of Plan:["+ std::to_string(plan->get_id());
        warning_msg += "], name:["+ plan->get_goal_name();
        warning_msg += "] must be between [0,1]. Set to 0, was: "+ std::to_string(plan_pref);
        logger->print(Logger::Warning, warning_msg);
        plan_pref = 0;
    }
    else {
        logger->print(Logger::EveryInfo, " Preference of Plan:["+ plan->get_goal_name()
                +"] set to: "+ std::to_string(plan_pref));
    }

    logger->set_level(log_level);
    return plan_pref;
}

double AgentSupport::computeSimulationEllapsedTime(const std::chrono::steady_clock::time_point begin_sim_time)
{
    // DEBUG purposes: compute the time needed to execute the entire simulation....
    std::chrono::steady_clock::time_point end_sim_time = std::chrono::steady_clock::now();
    double tot_duration_seconds = std::chrono::duration_cast<std::chrono::seconds>(end_sim_time - begin_sim_time).count();
    int duration_minutes = (int)(tot_duration_seconds / 60);
    int duration_seconds = (int)(tot_duration_seconds - (duration_minutes * 60));

    std::string time_diff = "Current simulation took = "+ std::to_string(duration_minutes) +" min : "+ std::to_string(duration_seconds) +" sec";
    logger->print(Logger::Default, time_diff);

    return tot_duration_seconds;
}

std::string AgentSupport::convertOperatorToString(Operator::Type op) {
    switch(op) {
    case 0: return "EQUAL";
    case 1: return "GREATER";
    case 2: return "GREATER_EQUAL";
    case 3: return "LESS";
    case 4: return "LESS_EQUAL";
    default: return "EQUAL";
    }
}

std::string AgentSupport::convertSchedType_to_string(const int sched_type) {
    switch (sched_type) {
    case 0: return "FCFS"; break;
    case 1: return "RR"; break;
    case 2: return "EDF"; break;
    default: return "undefined"; break;
    }
}

int AgentSupport::countGoalsPlans(const std::vector<std::vector<std::shared_ptr<Plan>>>& goals_plans) {
    // Returns the sum of selected_plans for every goals
    int cnt = 0;

    for(int i = 0; i < (int)goals_plans.size(); i++) {
        cnt += goals_plans[i].size();
    }
    return cnt;
}

int AgentSupport::IsServerAlreadyUsed(std::shared_ptr<Task> task, std::vector<int>& servers_needed) {
    if(task != nullptr) {
        if(task->getTaskNExec() == 1) {
            for(int i = 0; i < (int)servers_needed.size(); i++) {
                if(servers_needed[i] == task->getTaskServer()) {
                    return i;
                }
            }
        }
    }
    return -1; // server non used
}

void AgentSupport::initializeAlreadyActiveIntention(std::vector<std::pair<int, bool>>& alreadyActive, const std::vector<std::shared_ptr<Intention>>& intentionset) {
    alreadyActive.clear();

    for(int i = 0; i < (int)intentionset.size(); i++) {
        alreadyActive.push_back(std::make_pair(intentionset[i]->get_original_planID(), false));
    }
}

void AgentSupport::manageForceReasoningCycle(std::vector<std::shared_ptr<Intention>>& intentionset,
        std::vector<std::shared_ptr<Plan>>& planset, const int removedIntentionPlanId)
{
    std::shared_ptr<Goal> goal, goal_second;
    std::shared_ptr<Task> task, task_second;
    std::vector<std::pair<std::shared_ptr<Plan>, int>> subPlans;
    std::vector<std::shared_ptr<Action>> actions;

    // Reset the body of subgoals that refers to this goal
    for(int x = 0; x < (int)intentionset.size(); x++) {
        subPlans = intentionset[x]->getMainPlan()->getInternalGoalsSelectedPlans();
        for(int y = 0; y < (int)subPlans.size(); y++) {
            if(subPlans[y].first->get_id() == removedIntentionPlanId) {
                // restore subplan's body
                for(int p = 0; p < (int)planset.size(); p++) {
                    if(planset[p]->get_id() == removedIntentionPlanId) {
                        subPlans[y].first->set_body(planset[p]->get_body());
                    }
                }
            }
        }
        // update intention startTime if needed...
        actions = intentionset[x]->get_Actions();

        if(actions.size() > 0) {
            if(actions[0]->get_type() == "goal") {
                goal = std::dynamic_pointer_cast<Goal>(actions[0]);

                // Case 1: actions = G:SEQ or G:PAR
                if(actions.size() > 1) {
                    if(actions[1]->get_type() == "task") {
                        task_second = std::dynamic_pointer_cast<Task>(actions[1]);
                        if(task_second->getTaskExecuter() == Task::EXEC_SEQUENTIAL) {
                            // Case 1.1 G:SEQ, T:SEQ
                            intentionset[x]->set_startEqualLastExecution(sim_current_time().dbl());
                        }
                        else {
                            // TODO Case 1.2 G:SEQ, T:PAR: DIFFICILE da gestire
                            // [*] > dovrei calcolare G partendo dal tmpo del force,
                            // e A:PAR dal valore originale startTime intention
                            // (NON POSSIBILE con i dati che ho adesso)
                        }
                    }
                    else if(actions[1]->get_type() == "goal") {
                        goal_second = std::dynamic_pointer_cast<Goal>(actions[1]);

                        if(goal_second->getGoalExecutionType() == Goal::EXEC_SEQUENTIAL) {
                            // Case 1.3 G:SEQ, SEQ
                            intentionset[x]->set_startEqualLastExecution(sim_current_time().dbl());
                        }
                        else {
                            // TODO Case 1.4 G:SEQ, G:PAR: DIFFICILE da gestire
                            // [*] > dovrei calcolare G partendo dal tmpo del force,
                            // e A:PAR dal valore originale startTime intention
                            // (NON POSSIBILE con i dati che ho adesso)
                        }
                    }
                }
            }
            else if(actions[0]->get_type() == "task") {
                task = std::dynamic_pointer_cast<Task>(actions[0]);
                // TODO Case 2: actions = T:SEQ or T:PAR
                // - se goal è SEQ, DO NOTHING because the goal is not active (and not in the first batch)
                // - se goal PAR nel 1° batch, vedi [*]
            }
        }
    }
}

bool AgentSupport::manageReadyQueue(std::shared_ptr<Ag_Scheduler> ag_Sched)
{
    /* 1. The following statement is used whenever a task T0,0 is removed from EXEC due to preemption,
     * AND the new activated task T0,1 gets removed (due to sensors)
     * before the scheduled check_completion_rt of the T0,0.
     * 2. The first task in ready has STATUS = READY.
     * The reason is that before this iteration it WAS NOT the head task in the vector and the head task has been removed.
     * This happens when a sensor satisfy a goal whose plan contained the head task.
     * Result: head task with STATUS = EXEC removed -> update new head std::shared_ptr<Task> */
    if(ag_Sched->get_tasks_vector_ready().size() > 0)
    {
        std::shared_ptr<Task> taskHeadReady = ag_Sched->get_tasks_vector_ready()[0];

        logger->print(Logger::EveryInfo, "taskHeadReady:["+ std::to_string(taskHeadReady->getTaskId())
        +"], plan:["+ std::to_string(taskHeadReady->getTaskPlanId())
        +"], original_plan:["+ std::to_string(taskHeadReady->getTaskOriginalPlanId())
        +"], status: "+ taskHeadReady->getTaskStatus_as_string());

        if(taskHeadReady->getTaskStatus() == Task::TSK_READY) {
            setScheduleInFuture("ACTIVATE_TASK - after preemption and intention completed, activate first task in ready"); // METRICS purposes
            return true;
        }
    }

    return false;
}

std::shared_ptr<MaximumCompletionTime> AgentSupport::MCTSelect(const std::shared_ptr<MaximumCompletionTime> MCTint_goal,
        const std::shared_ptr<MaximumCompletionTime> MCTfp, const bool optimisticApproach)
{   /* if the designer requires an optimistic agent, the function will select the plan with
 * the minimum MCT among all those achieving the internal goal.
 * In case of a pessimistic agent the function will take the plan with maximum MCT.
 * The chosen plan is then added to the task-set, and its deadline is summed to MCTg p. */
    double ig_mct = MCTint_goal->getMCTValue();
    double fp_mct = MCTfp->getMCTValue();

    if(ig_mct == -1)
        ig_mct = std::numeric_limits<double>::max();
    if(fp_mct == -1)
        fp_mct = std::numeric_limits<double>::max();

    if(optimisticApproach) { // optimistic -> take the plan with the LOWEST MCT(ddl)
        if(ig_mct < fp_mct && MCTint_goal->getTaskSet().size() > 0) {
            return MCTint_goal;
        }
        else {
            return MCTfp;
        }
    }
    else { // pessimistic  -> take the plan with the HIGHEST MCT(ddl)
        if(ig_mct > fp_mct && MCTint_goal->getTaskSet().size() > 0) {
            return MCTint_goal;
        }
        else {
            return MCTfp;
        }
    }
}

void AgentSupport::orderFeasiblePlansBasedOnDDL(std::vector<std::vector<std::shared_ptr<MaximumCompletionTime>>>& feasible_plans,
        const bool approach, const std::vector<std::shared_ptr<Intention>>& intentionset,
        const std::shared_ptr<Ag_Scheduler> ag_Sched)
{
    /* Prioritize plans according to their MCT value (their deadline)
     * But, plans linked to intention in 'waiting_for_completion' are always put before everything else
     * Note: among the list of feasible plans for a goal, ONLY ONE plan can be linked to an active intention!
     * (We can not have multiple active intentions for plans linked to the same goal) */

    bool sorted = true;
    bool optimistic = false, pessimistic = false, invert_intention = false;
    bool is_intention = false;
    int index_intention = -1;
    std::shared_ptr<MaximumCompletionTime> tmp_mct;

    for(int i = 0; i < (int)feasible_plans.size(); i++)
    {
        index_intention = -1;

        do {
            sorted = true;
            for(int y = 0; y < (int)feasible_plans[i].size() - 1; y++)
            {
                invert_intention = false;
                optimistic = false;
                pessimistic = false;

                is_intention = checkIfPlanWasAlreadyIntention(feasible_plans[i][y]->getInternalGoalsSelectedPlans()[0].first,
                        index_intention, intentionset);

                if(is_intention && intentionset[index_intention]->get_waiting_for_completion()) {
                    // skip this loop, move to y + 1
                    logger->print(Logger::EveryInfo, " Feasible plan analyzed is intention:["+ std::to_string(intentionset[index_intention]->get_id())
                    +"], in waiting for completion at:["+ std::to_string(intentionset[index_intention]->get_scheduled_completion_time()) +"]");
                }
                else
                {
                    is_intention = checkIfPlanWasAlreadyIntention(feasible_plans[i][y + 1]->getInternalGoalsSelectedPlans()[0].first,
                            index_intention, intentionset);

                    if(is_intention && intentionset[index_intention]->get_waiting_for_completion())
                    { // immediately invert
                        invert_intention = true;
                    }
                    else { // check selected approach
                        /* OPTIMISTIC: is plan[y] > plan[y+1] ?: (feasible_plans[i][y] > feasible_plans[i][y + 1] && is_intention == false && approach)
                         * NOTE: plans with same "getMCTValue" will **NOT** be inverted */
                        optimistic = ((feasible_plans[i][y]->getMCTValue() - feasible_plans[i][y + 1]->getMCTValue()) > ag_Sched->EPSILON);
                        optimistic = optimistic && approach;

                        /* PESSIMISTIC: is plan[y] < plan[y+1] ?: (feasible_plans[i][y] < feasible_plans[i][y + 1] && is_intention == false && approach == false)
                         * NOTE: plans with same "getMCTValue" will **NOT** be inverted */
                        pessimistic = ((feasible_plans[i][y]->getMCTValue() - feasible_plans[i][y + 1]->getMCTValue()) < ag_Sched->EPSILON);
                        pessimistic = pessimistic && (approach == false);
                        pessimistic = pessimistic && (fabs(feasible_plans[i][y]->getMCTValue() - feasible_plans[i][y + 1]->getMCTValue()) > ag_Sched->EPSILON); // in order to skip plans with same value
                    }

                    if(optimistic || pessimistic || invert_intention)
                    {
                        tmp_mct = feasible_plans[i][y];
                        feasible_plans[i][y] = feasible_plans[i][y+1];
                        feasible_plans[i][y+1] = tmp_mct;
                        sorted = false;
                    }
                }
            }
        } while (sorted == false);
    }
}

void AgentSupport::orderGoalsetBasedOnPriority(std::vector<std::shared_ptr<Goal>>& goalset) {
    // Order Goalset based on priority (outcome: [0]prMax > ... > [n]prMin)
    std::sort(goalset.begin(), goalset.end(), [](std::shared_ptr<Goal> a, std::shared_ptr<Goal> b) {
        return (a->get_priority() > b->get_priority());
    });
}

void AgentSupport::orderPlansBasedOnPref(std::vector<std::vector<std::shared_ptr<Plan>>>& applicable_plans) {
    // sort based on the cost value[0,1] of 'pref'
    for(int i = 0; i < (int)applicable_plans.size(); i++) {
        // for each goal, the plans are ordered as: [0]prefMax > ... > [n]prefMin)
        std::sort(applicable_plans[i].begin(), applicable_plans[i].end(), [](std::shared_ptr<Plan> a, std::shared_ptr<Plan> b) {
            return (a->get_pref() > b->get_pref());
        });
    }
}

AgentSupport::ExpressionResult AgentSupport::parseExpression(const json expression,
        const std::vector<std::shared_ptr<Belief>>& beliefset,
        std::vector<std::shared_ptr<Belief>>& expression_variables,
        std::vector<std::shared_ptr<Belief>>& expression_constants)
{
    AgentSupport::ExpressionResult result_parse;
    AgentSupport::ExpressionBoolResult result_bool;
    AgentSupport::ExpressionDoubleResult result_double;
    std::shared_ptr<Belief> belief;
    std::variant<int, double, bool, std::string> belief_value;
    std::variant<int, double, bool, std::string> ret_value;
    std::string keyword; // accepted: DEFINE_CONSTANT, DEFINE_VARIABLE, +, *, -, /, ^, AND, OR, NOT, >, >=, <, <=, ==, !=
    std::string name = "", no_space_name = "";
    json item;
    const int expected_parameters = 3;
    bool valid_belief = false;

    if(expression.size() == expected_parameters)
    {
        item = expression[0];

        // First element in any expression MUST be a string...
        if(item.type() == json::value_t::string)
        {
            keyword = boost::to_upper_copy(item.get<std::string>());
            logger->print(Logger::EveryInfo, " keyword: "+ keyword);

            if(keyword == "DEFINE_CONSTANT")
            {
                // the second parameter of each effect must be a string...
                if(expression[1].type() == json::value_t::string)
                {
                    name = expression[1].get<std::string>();
                    no_space_name = boost::replace_all_copy(name, " ", ""); // remove spaces from string..
                    logger->print(Logger::EveryInfo, "Name: "+ no_space_name);

                    if(name.empty() || no_space_name.empty()) {
                        result_parse.generated_errors = true;
                        result_parse.msg = "Expression DEFINE_CONSTANT: specified name is empty!";
                        return result_parse;
                    }

                    // check if new constant name already exists...
                    for(int i = 0; i < (int)expression_constants.size(); i++) {
                        if(name == expression_constants[i]->get_name())
                        {
                            result_parse.generated_errors = true;
                            result_parse.msg = "Expression DEFINE_CONSTANT: name:["+ name +"] already exists as constant!";
                            return result_parse;
                        }
                    }

                    // input parameters are valid...
                    result_parse = effectEvaluateExpression(expression[2], beliefset, expression_variables, expression_constants);
                    logger->print(Logger::EveryInfo, " Function EFFEC_DEFINE_CONSTANT correctly performed: " + boolToString(!result_parse.generated_errors));

                    if(result_parse.generated_errors == false) {
                        // the value of the constant has been correctly defined and computed.
                        // Save it in the list of constants...
                        expression_constants.push_back(std::make_shared<Belief>(name, result_parse.value));
                    }
                }
                else {
                    result_parse.generated_errors = true;
                    result_parse.msg = "Expression DEFINE_CONSTANT: second element must be a string!";
                    return result_parse;
                }
            }
            else if(keyword == "DEFINE_VARIABLE")
            {
                // the second parameter of each effect must be a string...
                if(expression[1].type() == json::value_t::string)
                {
                    name = expression[1].get<std::string>();
                    no_space_name = boost::replace_all_copy(name, " ", ""); // remove spaces from string..
                    logger->print(Logger::EveryInfo, "Name: "+ no_space_name);

                    if(name.empty() || no_space_name.empty()) {
                        result_parse.generated_errors = true;
                        result_parse.msg = "Expression DEFINE_VARIABLE: specified name is empty!";
                        return result_parse;
                    }

                    // check if new constant name already exists...
                    for(int i = 0; i < (int)expression_variables.size(); i++) {
                        if(name == expression_variables[i]->get_name())
                        {
                            result_parse.generated_errors = true;
                            result_parse.msg = "Expression DEFINE_VARIABLE: name:["+ name +"] already exists as variable!";
                            return result_parse;
                        }
                    }

                    // input parameters are valid...
                    result_parse = effectEvaluateExpression(expression[2], beliefset, expression_variables, expression_constants);
                    logger->print(Logger::EveryInfo, " Function EFFEC_DECLARE_VARIABLE correctly performed: " + boolToString(!result_parse.generated_errors));

                    if(result_parse.generated_errors == false) {
                        // the value of the variable has been correctly defined and computed.
                        // Save it in the list of variables...
                        expression_variables.push_back(std::make_shared<Belief>(name, result_parse.value));
                    }
                }
                else {
                    result_parse.generated_errors = true;
                    result_parse.msg = "Expression DEFINE_VARIABLE: second element must be a string!";
                    return result_parse;
                }
            }
            else if(keyword == "ASSIGN" || keyword == "INCREASE" || keyword == "DECREASE")
            {   // structure: "keyword", "belief_name", [ expression ]
                // check if second parameter has correct type (must be a string)...
                if(expression[1].type() == json::value_t::string)
                {
                    name = expression[1].get<std::string>();
                    no_space_name = boost::replace_all_copy(name, " ", ""); // remove spaces from string..
                    logger->print(Logger::EveryInfo, "Name: "+ no_space_name);

                    if(name.empty() || no_space_name.empty()) { // invalid belief name
                        result_parse.generated_errors = true;
                        result_parse.msg = "Expression "+ keyword +": specified name is empty!";
                        return result_parse;
                    }

                    // check if belief.name exists...
                    for(int i = 0; i < (int)beliefset.size(); i++) {
                        if(name == beliefset[i]->get_name()) {
                            belief = beliefset[i]; // store the reference to the belief
                            valid_belief = true;
                            break;
                        }
                    }

                    if(valid_belief)
                    {
                        // check and parse expression...
                        result_parse = effectEvaluateExpression(expression[2], beliefset, expression_variables, expression_constants);
                        logger->print(Logger::EveryInfo, " Function "+ keyword +" correctly performed: " + boolToString(!result_parse.generated_errors));

                        if(result_parse.generated_errors) {
                            return result_parse;
                        }

                        belief_value = belief->get_value();
                        logger->print(Logger::EveryInfo, " Updating Belief = ("+ belief->get_name()
                                +", "+ variantToString(belief_value) +"), mode: "+ keyword
                                +", expression_value: "+ variantToString(result_parse.value));

                        // Perform ASSIGN, DECREASE, INCREASE and compute the updated value for the belief...
                        updateSingleBeliefDueToEffect(keyword, belief_value, result_parse.value, result_parse);

                        logger->print(Logger::EveryInfo, " Belief = ("+ belief->get_name()
                                +", "+ variantToString(belief_value) +")");

                        // if the updated value has been correctly computed, apply the new value to the beliefset
                        if(result_parse.generated_errors == false) {
                            belief->set_value(belief_value);
                        }
                    }
                    else {
                        result_parse.generated_errors = true;
                        result_parse.msg = "Expression "+ keyword +": belief with name:["+ name +"] not found!";
                        return result_parse;
                    }
                }
                else {
                    result_parse.generated_errors = true;
                    result_parse.msg = "Expression "+ keyword +": second element must be a string!";
                    return result_parse;
                }
            }
            else {
                result_parse.generated_errors = true;
                result_parse.msg = "Keyword:["+ keyword +"] not valid!";
                logger->print(Logger::EveryInfoPlus, result_parse.msg);
                return result_parse;
            }
        }
        else {
            result_parse.generated_errors = true;
            result_parse.msg = "Expression first item must always be a string. Type not valid!";
            logger->print(Logger::EveryInfoPlus, result_parse.msg);
            return result_parse;
        }
    }
    else {
        result_parse.generated_errors = true;
        result_parse.msg = "Expression.size must be:["+ std::to_string(expected_parameters) +"]! It is:["+ std::to_string(expression.size()) +"]";
        logger->print(Logger::EveryInfoPlus, result_parse.msg);
        return result_parse;
    }


    return result_parse;
}

AgentSupport::ExpressionResult AgentSupport::effectEvaluateExpression(const json expression,
        const std::vector<std::shared_ptr<Belief>>& beliefset,
        std::vector<std::shared_ptr<Belief>>& expression_variables,
        std::vector<std::shared_ptr<Belief>>& expression_constants)
{   /* Json expression MUST be: (i) numerical_expression, or (ii) bool_expression */

    AgentSupport::ExpressionResult result;
    AgentSupport::ExpressionDoubleResult result_double;
    AgentSupport::ExpressionBoolResult result_bool;
    const int expected_params_lv0 = 1, expected_min_params_lv1 = 2;
    const int expected_inner_array_size = 2;
    std::string variable_name = "", tmp_no_space_name = "";
    std::string inner_keyword = "", belief_name = "";
    std::vector<std::shared_ptr<Belief>> inner_beliefs;

    logger->print(Logger::EveryInfoPlus, "check tot received parameters: "+ std::to_string(expression.size()));
    if(expression.size() == expected_params_lv0)
    {
        if(expression.empty()) {
            result.generated_errors = true;
            result.msg = "Expression EVALUATE_EXPRESSION: first element is null or empty!";
            return result;
        }
        else {
            // expression: number, bool, read_belief, read_constant, read_variable, bool_expression, numerical_expression
            if(expression.type() == json::value_t::number_float
                    || expression.type() == json::value_t::number_unsigned
                    || expression.type() == json::value_t::number_integer)
            {
                result.generated_errors = false;
                result.msg = "ok";
                result.value = expression.get<double>();
            }
            else if(expression.type() == json::value_t::boolean)
            {
                result.generated_errors = false;
                result.msg = "ok";
                result.value = expression.get<bool>();
            }
            else if(expression.type() == json::value_t::array) {
                if(expression.size() > 0) {
                    if(expression[0].empty()) {
                        result.generated_errors = true;
                        result.msg = "Expression EVALUATE_EXPRESSION: does not contain valid elements (empty pos 0 in array).";
                        return result;
                    }
                }

                result = effectEvaluateExpression(expression, beliefset, expression_variables, expression_constants);

                if(result.generated_errors) {
                    return result;
                }
            }
            else {
                result.generated_errors = true;
                result.msg = "Expression EVALUATE_EXPRESSION: first element has invalid type!";
                return result;
            }

            return result;
        }
    }
    else if(expression.size() >= expected_min_params_lv1)
    {
        if(expression.type() == json::value_t::array)
        {
            if(expression.size() >= expected_inner_array_size)
            {
                if(expression[0].type() == json::value_t::string)
                {
                    inner_keyword = boost::to_upper_copy(expression[0].get<std::string>());

                    if(inner_keyword == "+" || inner_keyword == "*" || inner_keyword == "-"
                            || inner_keyword == "/" || inner_keyword == "^")
                    {   // Numeric expression can only contain the specified inner_keyword
                        result_double = effectNumericalExpression(expression, beliefset, expression_variables, expression_constants);
                        convertExpressionDoubleToResult(result_double, result);
                        return result;
                    }
                    else if(inner_keyword == "AND" || inner_keyword == "OR" || inner_keyword == "NOT"
                            || inner_keyword == ">" || inner_keyword == ">=" || inner_keyword == "<"
                                    || inner_keyword == "<=" || inner_keyword == "==" || inner_keyword == "!=")
                    {   /* Boolean expression can contain both boolean and numerical values.
                     * Be careful about how numbers and booleans are related (i.e.: 5 > false generates an error)
                     * On the other hand: 5 > 3 AND ((7 + 4 * 3 / 2) == 75) return a valid result */
                        result_bool = effectLogicExpression(expression, beliefset, expression_variables, expression_constants);
                        convertExpressionBoolToResult(result_bool, result);
                        return result;
                    }
                    else if(inner_keyword == "READ_BELIEF") {
                        inner_beliefs = beliefset;
                    }
                    else if(inner_keyword == "READ_CONSTANT") {
                        inner_beliefs = expression_constants;
                    }
                    else if(inner_keyword == "READ_VARIABLE") {
                        inner_beliefs = expression_variables;
                    }
                    else {
                        result.generated_errors = true;
                        result.msg = "Expression EVALUATE_EXPRESSION: first element is an array. First element:["+ inner_keyword +"] is not a valid keyword!";
                        return result;
                    }

                    if(expression[1].type() == json::value_t::string)
                    {
                        belief_name = expression[1].get<std::string>();

                        // get the belief specified in the array
                        for(int i = 0; i < (int)inner_beliefs.size(); i++) {
                            // logger->print(Logger::EveryInfoPlus, "A: " + belief_name +" == "+ inner_beliefs[i]->get_name());
                            if(belief_name == inner_beliefs[i]->get_name())
                            {
                                result.generated_errors = false;
                                result.msg = "ok";
                                result.value = inner_beliefs[i]->get_value();
                                return result;
                            }
                        }

                        result.generated_errors = true;
                        result.msg = "Expression EVALUATE_EXPRESSION: specified "+ inner_keyword +":["+ belief_name +"] not found!";
                        return result;
                    }
                    else {
                        result.generated_errors = true;
                        result.msg = "Expression EVALUATE_EXPRESSION: first element is an array. Keyword:["+ inner_keyword +"] must be followed by a string!";
                        return result;
                    }
                }
                else {
                    result.generated_errors = true;
                    result.msg = "Expression EVALUATE_EXPRESSION: first element is an array. First element of the array must be a string!";
                    return result;
                }
            }
            else {
                result.generated_errors = true;
                result.msg = "Expression EVALUATE_EXPRESSION: first element is an array. Invalid number of parameters:["+ std::to_string(expression.size()) +"], must be >= "+ std::to_string(expected_inner_array_size) +"!";
                return result;
            }
        }
        else {
            result.generated_errors = true;
            result.msg = "Inner Expression has an invalid type! Must be array!";
            return result;
        }
    }
    else {
        result.generated_errors = true;
        result.msg = "Expression has an invalid number of parameters:["+ std::to_string(expression.size());
        result.msg += "], must either be = ("+ std::to_string(expected_params_lv0);
        result.msg += ", main level) or >= ("+ std::to_string(expected_min_params_lv1) +", nested levels)!";
        return result;
    }

    result.generated_errors = true;
    result.msg = "Unexpected error";
    return result;
}

AgentSupport::ExpressionDoubleResult AgentSupport::effectNumericalExpression(const json expression,
        const std::vector<std::shared_ptr<Belief>>& beliefset,
        const std::vector<std::shared_ptr<Belief>>& expression_variables,
        const std::vector<std::shared_ptr<Belief>>& expression_constants,
        int inner_level)
{
    logger->print(Logger::EveryInfo, " ----effectNumericalExpression---- ");
    /* Json expression MUST be in format:
     *  [0] "math operator", # string: +, *, -, /, ^
     *  [...]
     *  [N] args: number, array: read_belief, read_constant, read_variable, numerical_expression */

    AgentSupport::ExpressionDoubleResult result, tmp_result;
    std::variant<int, double, bool, std::string> belief_value;
    const int min_expected_params_expression = 2, min_expected_reads = 2;
    std::string operator_str = "", tmp_no_space_name = "";
    std::string inner_keyword = "", belief_name = "";
    std::string expression_type = "";
    std::vector<std::shared_ptr<Belief>> inner_beliefs;
    bool read_has_correct_type = false;

    if(expression.size() >= min_expected_params_expression && inner_level == 0)
    {
        // check if the first item is a string...
        if(expression[0].type() == json::value_t::string)
        {
            operator_str = expression[0].get<std::string>();
            tmp_no_space_name = boost::replace_all_copy(operator_str, " ", ""); // remove spaces from string..
            logger->print(Logger::EveryInfo, "Operator: "+ tmp_no_space_name);

            if(operator_str.empty() || tmp_no_space_name.empty()) {
                result.generated_errors = true;
                result.msg = "Expression Numerical: keyword is empty!";
                result.value = -1;
                return result;
            }

            // analyze every element in the array
            for(int i = 1; i < (int)expression.size(); i++)
            {
                // expression[i]: number, array: read_belief, read_constant, read_variable, nested_expression
                if(expression[i].type() == json::value_t::number_float
                        || expression[i].type() == json::value_t::number_integer
                        || expression[i].type() == json::value_t::number_unsigned)
                {
                    if(i == 1) { // expression first element (within simulated brackets)
                        result.generated_errors = false;
                        result.msg = "ok";
                        result.value = expression[i].get<double>();
                    }
                    else {
                        if(updateNumericalResult(operator_str, result.value, expression[i].get<double>()) == false) {
                            // updateNumericalResult has generated an error...

                            result.generated_errors = true;
                            result.msg = "Expression Numerical: can not apply 'updateNumericalResult', operator:["+ operator_str +"] not valid!";
                            result.value = -1;
                            return result;
                        }
                    }
                }
                else if(expression[i].type() == json::value_t::array)
                {   // nested_expression or read_something...
                    if(expression[i][0].type() == json::value_t::string)
                    {
                        inner_keyword = boost::replace_all_copy(expression[i][0].get<std::string>(), " ", ""); // remove spaces from string..
                        inner_keyword = boost::to_upper_copy(inner_keyword);
                        logger->print(Logger::EveryInfo, "Keyword: "+ inner_keyword);

                        if(inner_keyword == "+" || inner_keyword == "*" || inner_keyword == "-"
                                || inner_keyword == "/" || inner_keyword == "^")
                        {
                            tmp_result = effectNumericalExpression(expression[i], beliefset,
                                    expression_variables, expression_constants);
                        }
                        else { // "READ_BELIEF", "READ_CONSTANT", "READ_VARIABLE"
                            tmp_result = effectNumericalExpression(expression[i], beliefset,
                                    expression_variables, expression_constants, inner_level + 1);
                        }

                        if(tmp_result.generated_errors) {
                            return tmp_result;
                        }
                        else {
                            if(i == 1) {
                                result.generated_errors = false;
                                result.msg = "ok";
                                result.value = tmp_result.value;
                            }
                            else {
                                logger->print(Logger::EveryInfo, "  i > 1 --> result: " + std::to_string(result.value));
                                logger->print(Logger::EveryInfo, "  i > 1 --> tmp_result: " + std::to_string(tmp_result.value));
                                if(updateNumericalResult(operator_str, result.value, tmp_result.value)) {
                                    result.generated_errors = false;
                                    result.msg = "ok";
                                    logger->print(Logger::EveryInfo, "  i > 1 --> result: " + std::to_string(result.value));
                                }
                                else {
                                    result.generated_errors = true;
                                    result.msg = "Expression Numeric: can't perform operation:["+ inner_keyword +"] on inner array!";
                                    result.value = -1;
                                    return result;
                                }
                            }
                        }
                    }
                    else {
                        result.generated_errors = true;
                        result.msg = "Expression Numerical: element in pos:["+ std::to_string(i) +"][0] has invalid type!";
                        result.value = -1;
                        return result;
                    }
                }
                else {
                    expression_type = expression[i].type_name();
                    result.generated_errors = true;
                    result.msg = "Expression Numerical: element in pos:["+ std::to_string(i);
                    result.msg += "] = '"+ expression[i].dump();
                    result.msg += "' has invalid type:["+ expression_type +"]!";
                    result.value = -1;
                    return result;
                }
            }
            return result;
        }
        else {
            result.generated_errors = true;
            result.msg = "Expression Numerical: first element must be a string!";
            result.value = -1;
            return result;
        }
    }
    else if(expression.size() >= min_expected_reads && inner_level > 0)
    { // nested_expression or read_something...
        // check if the first item is a string...
        if(expression[0].type() == json::value_t::string) {
            if(expression[1].type() == json::value_t::string) {
                inner_keyword = boost::replace_all_copy(expression[0].get<std::string>(), " ", ""); // remove spaces from string..
                inner_keyword = boost::to_upper_copy(inner_keyword);
                belief_name = expression[1].get<std::string>();
                logger->print(Logger::EveryInfo, "Keyword: "+ inner_keyword);
                logger->print(Logger::EveryInfoPlus, "A: inner_keyword:[" + inner_keyword +"], belief_name:[" + belief_name +"]");
                if(inner_keyword.empty()) {
                    result.generated_errors = true;
                    result.msg = "Expression Numerical: keyword is empty (lv: "+ std::to_string(inner_level) +")!";
                    result.value = -1;
                    return result;
                }

                if(inner_keyword == "READ_BELIEF") {
                    inner_beliefs = beliefset;
                }
                else if(inner_keyword == "READ_CONSTANT") {
                    inner_beliefs = expression_constants;
                }
                else if(inner_keyword == "READ_VARIABLE") {
                    inner_beliefs = expression_variables;
                }
                else {
                    result.generated_errors = true;
                    result.msg = "Expression Numerical: element is an array. First element is not a valid keyword:["+ inner_keyword +"]!";
                    result.value = -1;
                    return result;
                }

                // get the belief specified in the array
                for(int i = 0; i < (int)inner_beliefs.size(); i++) {
                    logger->print(Logger::EveryInfo, "A: " + belief_name +" == "+ inner_beliefs[i]->get_name());
                    if(belief_name == inner_beliefs[i]->get_name())
                    {
                        belief_value = inner_beliefs[i]->get_value();
                        result.value = checkAndConvertVariantToDouble(belief_value, read_has_correct_type);

                        if(read_has_correct_type) {
                            result.generated_errors = false;
                            result.msg = "ok";
                        }
                        else {
                            result.generated_errors = true;
                            result.msg = inner_keyword +":["+ belief_name +"] has invalid type! Must be numeric!";
                            result.value = -1;
                        }

                        return result;
                    }
                }

                result.generated_errors = true;
                result.msg = "Expression Numerical: specified "+ inner_keyword +":["+ belief_name +"] not found, or belief.value is bool!";
                result.value = -1;
                return result;

            }
            else {
                result.generated_errors = true;
                result.msg = "Expression Numerical: second element must be a string (lv: "+ std::to_string(inner_level) +")!";
                result.value = -1;
                return result;
            }
        }
        else {
            result.generated_errors = true;
            result.msg = "Expression Numerical: first element must be a string (lv: "+ std::to_string(inner_level) +")!";
            result.value = -1;
            return result;
        }
    }
    else {
        std::string must_be = "";
        if(inner_level == 0) {
            must_be = "= " + std::to_string(min_expected_params_expression);
        }
        else {
            must_be = ">= "+ std::to_string(min_expected_reads);
        }

        result.generated_errors = true;
        result.msg = "Expression has an invalid number of parameters:["+ std::to_string(expression.size());
        result.msg += "], must be "+ must_be +"!";
        result.value = -1;
        return result;
    }

    result.generated_errors = true;
    result.msg = "Unexpected error";
    result.value = -1;
    return result;
}

AgentSupport::ExpressionBoolResult AgentSupport::effectLogicExpression(const json expression,
        const std::vector<std::shared_ptr<Belief>>& beliefset,
        const std::vector<std::shared_ptr<Belief>>& expression_variables,
        const std::vector<std::shared_ptr<Belief>>& expression_constants,
        int inner_level)
{
    logger->print(Logger::EveryInfo, " ----effectLogicExpression---- ");
    /* Json expression MUST be in format:
     *  [0] "logical operator", # string: AND, OR, NOT
     *                          # string: >, >=, <, <=, ==, !=
     *  [N] args: bool, array: read_belief, read_constant, read_variable, bool_expression, numerical_expression */

    AgentSupport::ExpressionBoolResult result, tmp_result;
    AgentSupport::ExpressionDoubleResult result_double;
    AgentSupport::ExpressionBoolResult result_bool_A, result_bool_B;
    AgentSupport::ExpressionDoubleResult result_double_A, result_double_B;

    std::variant<int, double, bool, std::string> belief_value;
    const int min_expected_params_expression = 2, min_expected_reads = 2, expected_comparison = 3;
    int tmp_inner_level;
    std::string operator_str = "", tmp_no_space_name = "";
    std::string inner_keyword = "", belief_name = "";
    std::string expression_type = "";
    std::vector<std::shared_ptr<Belief>> inner_beliefs;
    bool read_has_correct_type = false;
    bool is_bool_left_side = false; // true if == or != involved and expression left side is boolean; false otherwise
    bool is_bool_right_side = false; // true if == or != involved and expression right side is boolean; false otherwise

    if(expression.size() >= min_expected_params_expression && inner_level == 0)
    {
        // check if the first item is a string...
        if(expression[0].type() == json::value_t::string)
        {
            operator_str = expression[0].get<std::string>();
            tmp_no_space_name = boost::replace_all_copy(operator_str, " ", ""); // remove spaces from string..
            logger->print(Logger::EveryInfo, "Operator: "+ tmp_no_space_name);

            if(operator_str.empty() || tmp_no_space_name.empty()) {
                result.generated_errors = true;
                result.msg = "Expression Logical: keyword is empty!";
                result.value = false;
                return result;
            }

            if(operator_str == "AND" || operator_str == "OR" || operator_str == "NOT")
            {
                // analyze every element in the array
                for(int i = 1; i < (int)expression.size(); i++)
                { // structure expression[i]: number, array: read_belief, read_constant, read_variable, nested_expression
                    if(expression[i].type() == json::value_t::boolean)
                    {
                        if(i == 1)
                        { // expression first element (within simulated brackets)
                            result.generated_errors = false;
                            result.msg = "ok";
                            result.value = expression[i].get<bool>();

                            if(operator_str == "NOT") {
                                if(updateLogicalResult(operator_str, result.value, result.value) == false) {
                                    result.generated_errors = true;
                                    result.msg = "Error while applying NOT to result.";
                                    result.value = false;
                                    return result;
                                }
                            }
                        }
                        else {
                            if(updateLogicalResult(operator_str, result.value, expression[i].get<bool>()) == false)
                            { // updateLogicalResult has generated an error...
                                result.generated_errors = true;
                                result.msg = "Expression Logical: can not apply 'updateLogicalResult', operator:["+ operator_str +"] not valid!";
                                result.value = false;
                                return result;
                            }
                        }
                    }
                    else if(expression[i].type() == json::value_t::array)
                    {   // nested_expression or read_something...
                        if(expression[i][0].type() == json::value_t::string)
                        {
                            inner_keyword = boost::replace_all_copy(expression[i][0].get<std::string>(), " ", ""); // remove spaces from string..
                            inner_keyword = boost::to_upper_copy(inner_keyword);
                            logger->print(Logger::EveryInfo, "Keyword: "+ inner_keyword);

                            if(inner_keyword == "AND" || inner_keyword == "OR" || inner_keyword == "NOT") {
                                tmp_result = effectLogicExpression(expression[i], beliefset,
                                        expression_variables, expression_constants);
                            }
                            else if(inner_keyword == ">" || inner_keyword == ">=" || inner_keyword == "<"
                                    || inner_keyword == "<=" || inner_keyword == "==" || inner_keyword == "!=")
                            {   // it's a boolean expression
                                tmp_result = effectLogicExpression(expression[i], beliefset,
                                        expression_variables, expression_constants);
                            }
                            else if(inner_keyword == "+" || inner_keyword == "*" || inner_keyword == "-"
                                    || inner_keyword == "/" || inner_keyword == "^")
                            {
                                result.generated_errors = true;
                                result.msg = "Expression Logical: keyword:["+ inner_keyword +"] is not valid!";
                                result.value = false;
                                return result;
                            }
                            else { // "READ_BELIEF", "READ_CONSTANT", "READ_VARIABLE"
                                tmp_result = effectLogicExpression(expression[i], beliefset,
                                        expression_variables, expression_constants, inner_level + 1);
                            }

                            if(tmp_result.generated_errors) {
                                return tmp_result;
                            }
                            else {
                                if(i == 1)
                                {
                                    result.generated_errors = false;
                                    result.msg = "ok";
                                    result.value = tmp_result.value;

                                    if(operator_str == "NOT") {
                                        if(updateLogicalResult(operator_str, result.value, result.value) == false) {
                                            result.generated_errors = true;
                                            result.msg = "Error while applying NOT to result.";
                                            result.value = false;
                                            return result;
                                        }
                                        else {
                                            return result;
                                        }
                                    }
                                }
                                else {
                                    logger->print(Logger::EveryInfo, "  i > 1 --> result: " + boolToString(result.value));
                                    logger->print(Logger::EveryInfo, "  i > 1 --> tmp_result: " + boolToString(tmp_result.value));
                                    if(updateLogicalResult(operator_str, result.value, tmp_result.value)) {
                                        result.generated_errors = false;
                                        result.msg = "ok";
                                        logger->print(Logger::EveryInfo, "  i > 1 --> result: " + boolToString(result.value));
                                    }
                                    else {
                                        result.generated_errors = true;
                                        result.msg = "Expression Logical: can't perform operation:["+ inner_keyword +"] on inner array!";
                                        result.value = false;
                                        return result;
                                    }
                                }
                            }
                        }
                        else {
                            result.generated_errors = true;
                            result.msg = "Expression Logical: element in pos:["+ std::to_string(i) +"][0] has invalid type!";
                            result.value = false;
                            return result;
                        }
                    }
                    else {
                        expression_type = expression[i].type_name();
                        result.generated_errors = true;
                        result.msg = "Expression Logical: element in pos:["+ std::to_string(i);
                        result.msg += "] = '"+ expression[i].dump();
                        result.msg += "] has invalid type:["+ expression_type +"]!";
                        result.value = false;
                        return result;
                    }
                }
            }
            else if(operator_str == ">" || operator_str == ">=" || operator_str == "<"
                    || operator_str == "<=" || operator_str == "==" || operator_str == "!=")
            {
                if(expression.size() == expected_comparison)
                {   // structure: ["operator", value 1, value 2 ]. values must be of the same type (bool or numbers)

                    // LEFT SIDE of the comparison.... ------------------------------------
                    result = computeComparisonExpression(expression[1], inner_level,
                            operator_str, "Left", beliefset, expression_variables, expression_constants,
                            result_bool_A, result_double_A, is_bool_left_side);

                    if(result.generated_errors) {
                        return result;
                    }
                    // --------------------------------------------------------------------

                    // RIGHT SIDE of the comparison.... -----------------------------------
                    result = computeComparisonExpression(expression[2], inner_level,
                            operator_str, "Right",beliefset, expression_variables, expression_constants,
                            result_bool_B, result_double_B, is_bool_right_side);

                    if(result.generated_errors) {
                        return result;
                    }
                    // --------------------------------------------------------------------

                    logger->print(Logger::EveryInfoPlus, " - is_bool_left_side:["+ boolToString(is_bool_left_side)
                            +"], is_bool_right_side:["+ boolToString(is_bool_right_side) +"]");

                    if((is_bool_left_side && is_bool_right_side == false)
                            || (is_bool_left_side == false && is_bool_right_side))
                    {   // we can not combine boolean and number in expression based on == or !=
                        result.generated_errors = true;
                        result.msg = "Expression Logical: Left side has not the same type of right side! Left is bool:["+ boolToString(is_bool_left_side);
                        result.msg += "]. Right is bool:["+ boolToString(is_bool_right_side);
                        result.msg += "]! ("+ dump_expression(expression[1], beliefset, expression_constants, expression_variables);
                        result.msg += ") and ("+ dump_expression(expression[2], beliefset, expression_constants, expression_variables) +")";

                        result.value = false;
                        return result;
                    }
                    else if(is_bool_left_side && is_bool_right_side)
                    { // At this point, both side of the bool expression are in a valid format...
                        if(result_bool_A.generated_errors) {
                            result.generated_errors = true;
                            result.msg = "Expression Logical: Error when evaluating left side of operator:["+ operator_str +"]!";
                            result.value = false;
                            return result;
                        }
                        if(result_bool_B.generated_errors) {
                            result.generated_errors = true;
                            result.msg = "Expression Logical: Error when evaluating right side of operator:["+ operator_str +"]!";
                            result.value = false;
                            return result;
                        }

                        result.generated_errors = false;
                        result.msg = "ok";
                        result.value = compareNumericalValues(operator_str, result_bool_A.value, result_bool_B.value);
                    }
                    else {
                        // At this point, both side of the numeric expression are in a valid format...
                        if(result_double_A.generated_errors) {
                            result.generated_errors = true;
                            result.msg = "Expression Logical: Error when evaluating left side of operator:["+ operator_str +"]!";
                            return result;
                        }
                        if(result_double_B.generated_errors) {
                            result.generated_errors = true;
                            result.msg = "Expression Logical: Error when evaluating right side of operator:["+ operator_str +"]!";
                            result.value = false;
                            return result;
                        }

                        result.generated_errors = false;
                        result.msg = "ok";
                        result.value = compareNumericalValues(operator_str, result_double_A.value, result_double_B.value);
                    }
                }
                else {
                    result.generated_errors = true;
                    result.msg = "Expression Logical: Operator:["+ operator_str +"] requires exactly "+ std::to_string(expected_comparison);
                    result.msg += " parameters! Provided:["+ std::to_string(expression.size()) +"]!";
                    result.value = false;
                    return result;
                }
            }
            else {
                result.generated_errors = true;
                result.msg = "Expression Logical: Operator:["+ operator_str +"] is not valid!";
                result.value = false;
            }

            return result;
        }
        else {
            result.generated_errors = true;
            result.msg = "Expression Logical: first element must be a string!";
            result.value = false;
            return result;
        }
    }
    else if(expression.size() >= min_expected_reads && inner_level > 0)
    { // nested_expression or read_something...
        // check if the first item is a string...
        if(expression[0].type() == json::value_t::string) {
            inner_keyword = boost::replace_all_copy(expression[0].get<std::string>(), " ", ""); // remove spaces from string..
            inner_keyword = boost::to_upper_copy(inner_keyword);
            if(expression[1].type() == json::value_t::string) {
                belief_name = expression[1].get<std::string>();
                logger->print(Logger::EveryInfo, "Keyword: "+ inner_keyword);

                logger->print(Logger::EveryInfoPlus, "A: inner_keyword:[" + inner_keyword +"], belief_name:[" + belief_name +"]");
                if(inner_keyword.empty()) {
                    result.generated_errors = true;
                    result.msg = "Expression Logical: keyword is empty (lv: "+ std::to_string(inner_level) +")!";
                    result.value = false;
                    return result;
                }

                if(inner_keyword == "READ_BELIEF") {
                    inner_beliefs = beliefset;
                }
                else if(inner_keyword == "READ_CONSTANT") {
                    inner_beliefs = expression_constants;
                }
                else if(inner_keyword == "READ_VARIABLE") {
                    inner_beliefs = expression_variables;
                }
                else {
                    result.generated_errors = true;
                    result.msg = "Expression Logical: element is an array. First element is not a valid keyword:["+ inner_keyword +"]!";
                    result.value = false;
                    return result;
                }

                // get the belief specified in the array
                for(int i = 0; i < (int)inner_beliefs.size(); i++) {
                    logger->print(Logger::EveryInfoPlus, "A: " + belief_name +" == "+ inner_beliefs[i]->get_name());
                    if(belief_name == inner_beliefs[i]->get_name())
                    {
                        belief_value = inner_beliefs[i]->get_value();
                        result.value = checkAndConvertVariantToBool(belief_value, read_has_correct_type);

                        if(read_has_correct_type) {
                            result.generated_errors = false;
                            result.msg = "ok";
                        }
                        else {
                            result.generated_errors = true;
                            result.msg = inner_keyword +":["+ belief_name +"] has invalid type! Must be boolean!";
                            result.value = false;
                        }

                        return result;
                    }
                }

                result.generated_errors = true;
                result.msg = "Expression Logical: specified "+ inner_keyword +":["+ belief_name +"] not found, or belief.value is bool!";
                result.value = false;
                return result;

            }
            else if(expression[1].type() == json::value_t::array)
            { // i.e.: [ [0]:NOT, [1]:[ NOT, [ READ_BELIEF, is_busy ] ] ]
                tmp_inner_level = 0;

                if(expression[1].size() == 2)
                { // i.e.: [ [0]:NOT, [1]:[ READ_BELIEF, is_busy ]
                    if(expression[1][0].type() == json::value_t::string
                            && expression[1][1].type() == json::value_t::string)
                    {
                        tmp_inner_level = 1;
                    }
                }

                tmp_result = effectLogicExpression(expression[1], beliefset,
                        expression_variables, expression_constants, tmp_inner_level);

                if(tmp_result.generated_errors) {
                    return tmp_result;
                }
                else {
                    if(inner_keyword == "NOT") {
                        updateLogicalResult(inner_keyword, tmp_result.value, tmp_result.value);
                    }
                    return tmp_result;
                }
            }
            else {
                result.generated_errors = true;
                result.msg = "Expression Logical: second element must be a string (lv: "+ std::to_string(inner_level) +")!";
                result.value = false;
                return result;
            }
        }
        else {
            result.generated_errors = true;
            result.msg = "Expression Logical: first element must be a string (lv: "+ std::to_string(inner_level) +")!";
            result.value = false;
            return result;
        }
    }
    else {
        std::string must_be = "";
        if(inner_level == 0) {
            must_be = "= " + std::to_string(min_expected_params_expression);
        }
        else {
            must_be = ">= "+ std::to_string(min_expected_reads);
        }

        result.generated_errors = true;
        result.msg = "Expression has an invalid number of parameters:["+ std::to_string(expression.size());
        result.msg += "], must be "+ must_be +"!";
        result.value = false;
        return result;
    }

    result.generated_errors = true;
    result.msg = "Unexpected error";
    result.value = false;
    return result;
}

double AgentSupport::checkAndConvertVariantToDouble(std::variant<int, double, bool, std::string> value,
        bool& correct_type)
{
    if(auto val = std::get_if<double>(&value))
    { // it's a double
        double& res = *val;
        correct_type = true;
        logger->print(Logger::EveryInfo, "checkAndConvertVariantToDouble: "+ std::to_string(res));
        return res;
    }
    else if(auto val = std::get_if<int>(&value))
    { // it's a double
        int& res = *val;
        correct_type = true;
        logger->print(Logger::EveryInfo, "checkAndConvertVariantToDouble: "+ std::to_string(res));
        return (double) res;
    }
    else {
        correct_type = false;
        return 0;
    }
}

bool AgentSupport::checkAndConvertVariantToBool(std::variant<int, double, bool, std::string> value,
        bool& correct_type)
{
    if(auto val = std::get_if<bool>(&value))
    { // it's a double
        bool& res = *val;
        logger->print(Logger::EveryInfo, "checkAndConvertVariantToBool: "+ boolToString(res));
        correct_type = true;
        return res;
    }
    else {
        correct_type = false;
        return 0;
    }
}

void AgentSupport::convertExpressionDoubleToResult(const AgentSupport::ExpressionDoubleResult result_double,
        AgentSupport::ExpressionResult& result)
{
    result.generated_errors = result_double.generated_errors;
    result.msg = result_double.msg;
    result.value = result_double.value;
}

void AgentSupport::convertExpressionDoubleToBoolResult(const ExpressionDoubleResult result_double,
        ExpressionBoolResult& result_bool) {
    result_bool.generated_errors = result_double.generated_errors;
    result_bool.msg = result_double.msg;
    result_bool.value = result_double.value;
}

void AgentSupport::convertExpressionResultToBool(const ExpressionResult result,
        ExpressionBoolResult& result_bool)
{
    bool succ = false;
    result_bool.value = variantToBool(result.value, succ);

    if(succ == false) {
        result_bool.generated_errors = true;
        result_bool.msg = "Variant can not be converted to bool!";
    }
    else {
        result_bool.generated_errors = result.generated_errors;
        result_bool.msg = result.msg;
    }
}

AgentSupport::ExpressionBoolResult AgentSupport::computeComparisonExpression(const json expression,
        const int inner_level, const std::string operator_str, const std::string equation_side,
        const std::vector<std::shared_ptr<Belief>>& beliefset,
        const std::vector<std::shared_ptr<Belief>>& expression_variables,
        const std::vector<std::shared_ptr<Belief>>& expression_constants,
        AgentSupport::ExpressionBoolResult& result_bool,
        AgentSupport::ExpressionDoubleResult& result_double,
        bool& is_bool_expression)
{
    AgentSupport::ExpressionBoolResult result;

    std::vector<std::shared_ptr<Belief>> inner_beliefs;
    std::variant<int, double, bool, std::string> belief_value;
    std::string keyword, belief_name;
    std::string conversion_type, array_operator;
    bool valid_name = false;

    /* This function is called when dealing with ==, !=, >, >=, <= or <
     * Moreover, it's called once for the {left} side of the equation, and once for the {right} part.
     * i.e. {NOT(((7+3) * 2)} != {["READ_BELIEF", "batteryLevel" ]})
     * ----------------------------------------------------------------------------------------------
     * Each side can contain:
     * a. one number
     * b. one boolean value
     * c. an array
     * If an array is found, its structure must be one of the following ones:
     * 1. a pair (string, string) used for READ data from beliefs, variables or constants     *
     * 2. a mathematical expression
     * 3. a logical expression
     * ----------------------------------------------------------------------------------------------
     * Case a. and b. has size 1.
     * Case 1. has exactly size 2.
     * Case 2. and 3. must have at least size 2 (2 for NOT, 3 for all other cases).
     */

    // Case a: check if the current side of the expression is a single numberic value...
    if(expression.type() == json::value_t::number_float
            || expression.type() == json::value_t::number_integer
            || expression.type() == json::value_t::number_unsigned)
    {
        result_double.generated_errors = false;
        result_double.msg = "ok";
        result_double.value = expression.get<double>();

        // used to tell the caller if the function behaved successfully or not...
        convertExpressionDoubleToBoolResult(result_double, result);
        return result;
    }
    // Case b: check if the current side of the expression is a single boolean value...
    else if(expression.type() == json::value_t::boolean) {
        if(operator_str == "==" || operator_str == "!=")
        { // only operators ==, != can be applied to boolean
            is_bool_expression = true;
            result_bool.generated_errors = false;
            result_bool.msg = "ok";
            result_bool.value = expression.get<bool>();
            return result_bool;
        }
        else { // >, >=, <=, < are not allowed between bool values
            result.generated_errors = false;
            result.msg = equation_side +" side of operator:["+ operator_str +"] must be either a number or an expression. It's bool!";
            result.value = false;
            return result;
        }
    }
    // Case c:  current side is an array
    else if(expression.type() == json::value_t::array)
    {
        if(expression.size() >= 2)
        {
            // in each array the first element must be a string!
            if(expression[0].type() == json::value_t::string)
            {
                keyword = boost::to_upper_copy(expression[0].get<std::string>());
                logger->print(Logger::EveryInfo, " - keyword:["+ keyword +"]");
                if(expression[1].type() == json::value_t::string)
                {   // Case 1:  a pair (string, string) used for READ data from beliefs, variables or constants
                    belief_name = expression[1].get<std::string>();
                    logger->print(Logger::EveryInfo, " - belief_name:["+ belief_name +"]");

                    if(keyword == "READ_BELIEF") {
                        inner_beliefs = beliefset;
                    }
                    else if(keyword == "READ_CONSTANT") {
                        inner_beliefs = expression_constants;
                    }
                    else if(keyword == "READ_VARIABLE") {
                        inner_beliefs = expression_variables;
                    }
                    else {
                        result.generated_errors = true;
                        result.msg = equation_side +" side is an array. First element is not a valid keyword:["+ keyword +"]!";
                        return result;
                    }

                    for(int i = 0; i < (int)inner_beliefs.size(); i++)
                    { // If exists, get the belief specified in the array...
                        logger->print(Logger::EveryInfoPlus, "A: " + belief_name
                                +" == "+ inner_beliefs[i]->get_name());
                        if(belief_name == inner_beliefs[i]->get_name())
                        {
                            valid_name = true;
                            belief_value = inner_beliefs[i]->get_value();
                            conversion_type = get_variant_type_as_string(belief_value);

                            if(conversion_type == "INT" || conversion_type == "DOUBLE") {
                                result_double = effectNumericalExpression(expression, beliefset,
                                        expression_variables, expression_constants, inner_level + 1);

                                if(result_double.generated_errors) {
                                    // used to tell the caller if the function behaved successfully or not...
                                    convertExpressionDoubleToBoolResult(result_double, result);
                                    return result;
                                }
                                else {
                                    // used to tell the caller if the function behaved successfully or not...
                                    // the computed value will be stored in "result_double"
                                    logger->print(Logger::EveryInfo, " - result_double: "+ std::to_string(result_double.value));
                                    result.generated_errors = false;
                                    result.msg = "ok";
                                    result.value = true;
                                    return result;
                                }
                            }
                            else if(conversion_type == "BOOL") {
                                is_bool_expression = true;
                                result_bool = effectLogicExpression(expression, beliefset,
                                        expression_variables, expression_constants, inner_level + 1);
                                return result_bool;
                            }
                            else {
                                result.generated_errors = true;
                                result.msg = equation_side +" side is an array. "+ keyword +" must either be linked to a NUMERICAL or a BOOL value!";
                                return result;
                            }
                            break;
                        }
                    }

                    if(valid_name == false) {
                        result.generated_errors = true;
                        result.msg = equation_side +" side is an array. First element:["+ keyword;
                        result.msg += "] is linked to invalid name:["+ belief_name +"]";
                        return result;
                    }
                    else { // This else should never been called! valid_name == true returns within the for loop!
                        result.generated_errors = true;
                        result.msg = "This else should never been called! valid_name == true returns within the for loop!";
                        return result;
                    }
                }
                else if(keyword == "+" || keyword == "*" || keyword == "-" || keyword == "/" || keyword == "^" )
                { // Case 2: a mathematical expression
                    if(expression.size() >= 3) {
                        result_double = effectNumericalExpression(expression, beliefset,
                                expression_variables, expression_constants);

                        if(result_double.generated_errors) {
                            // used to tell the caller if the function behaved successfully or not...
                            convertExpressionDoubleToBoolResult(result_double, result);
                            return result;
                        }
                        else {
                            // used to tell the caller if the function behaved successfully or not...
                            // the computed value will be stored in "result_double"
                            logger->print(Logger::EveryInfo, " - result_double: "+ std::to_string(result_double.value));
                            result.generated_errors = false;
                            result.msg = "ok";
                            result.value = true;
                            return result;
                        }
                    }
                    else {
                        result.generated_errors = true;
                        result.msg = equation_side +" side is an array. First element:["+ keyword +"] requires at least 3 elements! It's:["+ std::to_string(expression.size()) +"]!";
                        return result;
                    }
                }
                else if(keyword == "AND" || keyword == "OR" || keyword == "NOT")
                {  // Case 3.1: a logical expression
                    if(keyword == "NOT") {
                        if(expression.size() == 2) {
                            is_bool_expression = true;
                            result_bool = effectLogicExpression(expression, beliefset,
                                    expression_variables, expression_constants);
                            return result_bool;
                        }
                        else {
                            result.generated_errors = true;
                            result.msg = equation_side +" side is an array. First element:["+ keyword +"] requires exactly 2 elements! It's:["+ std::to_string(expression.size()) +"]!";
                            return result;
                        }
                    }
                    else {
                        if(expression.size() >= 3) {
                            is_bool_expression = true;
                            result_bool = effectLogicExpression(expression, beliefset,
                                    expression_variables, expression_constants);
                            return result_bool;
                        }
                        else {
                            result.generated_errors = true;
                            result.msg = equation_side +" side is an array. First element:["+ keyword +"] requires at least 3 elements! It's:["+ std::to_string(expression.size()) +"]!";
                            return result;
                        }
                    }
                }
                else if(keyword == ">" || keyword == ">=" || keyword == "<"
                        || keyword == "<=" || keyword == "==" || keyword == "!=")
                {  // Case 3.2: a logical expression
                    if(expression.size() == 3) {
                        is_bool_expression = true;
                        result_bool = effectLogicExpression(expression, beliefset,
                                expression_variables, expression_constants, inner_level + 1);
                        return result_bool;
                    }
                    else {
                        result.generated_errors = true;
                        result.msg = equation_side +" side is an array. First element:["+ keyword +"] requires exactly 3 elements! It's:["+ std::to_string(expression.size()) +"]!";
                        return result;
                    }
                }
                else {
                    result.generated_errors = true;
                    result.msg = equation_side +" side is an array. First element:["+ keyword +"] is not a valid keyword!";
                    return result;
                }
            }
            else {
                result.generated_errors = true;
                result.msg = equation_side +" side is an array. First element must be a string!";
                return result;
            }
        }
        else {
            result.generated_errors = true;
            result.msg = equation_side +" side of operator:["+ operator_str +"] is an array. Size must be >= 2! It's:["+ std::to_string(expression.size()) +"]!";
            result.value = false;
            return result;
        }
    }
    else {
        result.generated_errors = true;
        result.msg = equation_side +" side of operator:["+ operator_str +"] must be a number, a boolean or an expression!";
        result.value = false;
        return result;
    }
}


bool AgentSupport::compareNumericalValues(const std::string op, const double value_A, const double value_B) {
    if(op == ">") {
        return value_A > value_B;
    }
    else if(op == ">=") {
        return value_A >= value_B;
    }
    else if(op == "<") {
        return value_A < value_B;
    }
    else if(op == "<=") {
        return value_A <= value_B;
    }
    else if(op == "==") {
        return fabs(value_A - value_B) < 0.05;
    }
    else if(op == "!=") {
        return value_A != value_B;
    }

    return false;
}

void AgentSupport::convertExpressionBoolToResult(const AgentSupport::ExpressionBoolResult result_bool,
        AgentSupport::ExpressionResult& result)
{
    result.generated_errors = result_bool.generated_errors;
    result.msg = result_bool.msg;
    result.value = result_bool.value;
}

std::string AgentSupport::dump_expression(json expression, const std::vector<std::shared_ptr<Belief>>& beliefset,
        const std::vector<std::shared_ptr<Belief>>& expression_constants,
        const std::vector<std::shared_ptr<Belief>>& expression_variables)
{
    std::string result = "";
    std::string keyword = "", upper_keyword = "";
    bool read_key = false, key_found = false;
    bool define_key = false;
    std::vector<std::shared_ptr<Belief>> inner_beliefs;

    if(expression.size() > 0)
    {
        if(expression.type() == json::value_t::number_float
                || expression.type() == json::value_t::number_integer
                || expression.type() == json::value_t::number_unsigned)
        {
            result += std::to_string(expression.get<double>()) + ", ";
        }
        else if(expression.type() == json::value_t::boolean)
        {
            result += boolToString(expression.get<bool>()) + ", ";
        }
        else if(expression.type() == json::value_t::array)
        {
            for(int i = 0; i < (int)expression.size(); i++)
            {
                if(expression[i].type() == json::value_t::string)
                {
                    keyword = expression[i].get<std::string>();
                    upper_keyword = boost::to_upper_copy(keyword);

                    if(upper_keyword == "READ_BELIEF")
                    {
                        result = keyword + ", ";
                        inner_beliefs = beliefset;
                        read_key = true;
                    }
                    else if(upper_keyword == "READ_CONSTANT")
                    {
                        result = keyword + ", ";
                        inner_beliefs = expression_constants;
                        read_key = true;
                    }
                    else if(upper_keyword == "READ_VARIABLE")
                    {
                        result = keyword + ", ";
                        inner_beliefs = expression_variables;
                        read_key = true;
                    }
                    else if(upper_keyword == "DEFINE_CONSTANT")
                    {
                        result = keyword + ", ";
                        define_key = true;
                    }
                    else if(upper_keyword == "DEFINE_VARIABLE")
                    {
                        result = keyword + ", ";
                        define_key = true;
                    }
                    else {
                        if(define_key == true) {
                            result = keyword + " ";
                        }
                        else if(read_key == true)
                        {
                            for(int i = 0; i < (int)inner_beliefs.size(); i++)
                            { // If exists, get the belief specified in the array...
                                logger->print(Logger::EveryInfoPlus, "A: " + keyword +" == "+ inner_beliefs[i]->get_name());
                                if(keyword == inner_beliefs[i]->get_name())
                                {
                                    result += keyword + ", "+ variantToString(inner_beliefs[i]->get_value()) + " ";
                                    key_found = true;
                                    break;
                                }
                            }

                            if(key_found == false) {
                                result += " ERROR, key:["+ keyword +"] not found!";
                            }
                        }
                        else { // operator
                            result += keyword + ", ";
                        }

                        read_key = false;
                        key_found = false;
                        define_key = false;
                    }
                }
                else {
                    result += "[ " + dump_expression(expression[i], beliefset, expression_constants, expression_variables) + "], ";
                }
            }
        }
    }
    else {
        result += "Empty array! ";
    }

    return result;
}

bool AgentSupport::updateNumericalResult(std::string op, double& result, const double value) {
    if(op == "+") {
        result = result + value;
    }
    else if(op == "-") {
        result = result - value;
    }
    else if(op == "*") {
        if(value == 0 || result == 0) {
            result = 0;
        }
        result = result * value;
    }
    else if(op == "/") {
        if(value == 0) {
            return false;
        }
        result = result / value;
    }
    else if(op == "^") {
        result = pow(result, value);
    }
    else {
        return false;
    }

    return true;
}

bool AgentSupport::updateNumericalResult(std::string op, double& result, const int value) {
    if(op == "+") {
        result = result + value;
    }
    else if(op == "-") {
        result = result - value;
    }
    else if(op == "*") {
        if(value == 0 || result == 0) {
            return false;
        }
        result = result * value;
    }
    else if(op == "/") {
        if(value == 0) {
            return false;
        }
        result = result / value;
    }
    else if(op == "^") {
        result = pow(result, value);
    }
    else {
        return false;
    }

    return true;
}

bool AgentSupport::updateLogicalResult(std::string op, bool& result, const bool value) {
    op = boost::to_upper_copy(op);
    if(op == "AND") {
        result = result && value;
    }
    else if(op == "OR") {
        result = result || value;
    }
    else if(op == "NOT") {
        result = !result;
    }
    else {
        return false;
    }

    return true;
}

bool AgentSupport::updateSingleBeliefDueToEffect(const std::string mode,
        std::variant<int, double, bool, std::string>& current_value,
        std::variant<int, double, bool, std::string> effect_value,
        AgentSupport::ExpressionResult& result_parse)
{
    std::string error_msg = "Error when trying to apply effect! Types do not match!";

    if(auto b = std::get_if<int>(&current_value))
    {
        if(auto s_b = std::get_if<int>(&effect_value)) {
            int& ab = *b;
            int& db = *s_b;

            if(mode == "INCREASE") {
                ab = ab + db;
            }
            else if(mode == "DECREASE") {
                ab = ab - db;
            }
            else { // ASSIGN
                ab = db;
            }
            result_parse.generated_errors = false;
            result_parse.msg = "ok";
            result_parse.value = ab;
            return true;
        }
        else if(auto s_b = std::get_if<double>(&effect_value)) {
            int& ab = *b;
            double& db = *s_b;

            if(mode == "INCREASE") {
                ab = ab + db;
            }
            else if(mode == "DECREASE") {
                ab = ab - db;
            }
            else { // ASSIGN
                ab = db;
            }
            result_parse.generated_errors = false;
            result_parse.msg = "ok";
            result_parse.value = ab;
            return true;
        }
        else {
            result_parse.generated_errors = true;
            result_parse.msg = error_msg + " (INT and invalid type value)";
            logger->print(Logger::EveryInfo, result_parse.msg);
        }
    }
    else if(auto b = std::get_if<double>(&current_value))
    {
        if(auto s_b = std::get_if<double>(&effect_value)) {
            double& ab = *b;
            double& db = *s_b;

            if(mode == "INCREASE") {
                ab = ab + db;
            }
            else if(mode == "DECREASE") {
                ab = ab - db;
            }
            else { // ASSIGN
                ab = db;
            }
            result_parse.generated_errors = false;
            result_parse.msg = "ok";
            result_parse.value = ab;
            return true;
        }
        else if(auto s_b = std::get_if<int>(&effect_value)) {
            double& ab = *b;
            int& db = *s_b;

            if(mode == "INCREASE") {
                ab = ab + db;
            }
            else if(mode == "DECREASE") {
                ab = ab - db;
            }
            else { // ASSIGN
                ab = db;
            }
            result_parse.generated_errors = false;
            result_parse.msg = "ok";
            result_parse.value = ab;
            return true;
        }
        else {
            result_parse.generated_errors = true;
            result_parse.msg = error_msg + " (DOUBLE and invalid type value)";
            logger->print(Logger::EveryInfo, result_parse.msg);
        }
    }
    else if(auto b = std::get_if<bool>(&current_value))
    {
        if(auto s_b = std::get_if<bool>(&effect_value))
        {
            bool& ab = *b;
            bool& db = *s_b;

            // ASSIGN:
            ab = db;

            result_parse.generated_errors = false;
            result_parse.msg = "ok";
            result_parse.value = ab;
            return true;
        }
        else {
            result_parse.generated_errors = true;
            result_parse.msg = error_msg + " (BOOL and invalid type value)";
            logger->print(Logger::EveryInfo, result_parse.msg);
        }
    }
    else if(auto b = std::get_if<std::string>(&current_value))
    {
        if(auto s_b = std::get_if<std::string>(&effect_value)) {
            std::string& ab = *b;
            std::string& db = *s_b;

            // ASSIGN:
            ab = db;

            result_parse.generated_errors = false;
            result_parse.msg = "ok";
            result_parse.value = ab;
            return true;
        }
        else {
            result_parse.generated_errors = true;
            result_parse.msg = error_msg + " (STRING and invalid type value)";
            logger->print(Logger::EveryInfo, result_parse.msg);
        }
    }

    // it reaches this point if the types of the two variant are different
    result_parse.generated_errors = true;
    result_parse.msg = error_msg + " Unexpected error!";
    return false;
}

AgentSupport::ExpressionDoubleResult AgentSupport::formulaExpression(const json formula,
        const std::vector<std::shared_ptr<Belief>>& beliefset)
{
    logger->print(Logger::EveryInfo, " ----formulaExpression---- ");
    AgentSupport::ExpressionDoubleResult result, tmp_result;
    std::variant<int, double, bool, std::string> item_variant;
    const std::vector<std::shared_ptr<Belief>> expression_variables = {};
    const std::vector<std::shared_ptr<Belief>> expression_constants = {};
    std::string item_str;
    int inner_level = 0; // formula starts with an operator (allows to manage all cases, except for formula:[ [ "READ_...", "name" ] ]

    if(formula.type() == json::value_t::array && formula.size() > 0)
    { /*
     * ************************************************************************************************
     * formula is an array (expression)
     * Valid structure(s):
     * - formula:[ ] --> empty formula is set to 0
     * - formula:[ numerical value ]
     * - formula:[ ["READ_VALUE", "name" ] ]
     * - formula:[ [ "operator", values ] ] --> the number of values changes according to the "operator" used
     * ************************************************************************************************
     * NOT Valid structure(s):
     * - formula:[ "operator", values ]
     * - formula:[ bool value ]
     * ************************************************************************************************ */
        if(formula[0].type() == json::value_t::number_float
                || formula[0].type() == json::value_t::number_integer
                || formula[0].type() == json::value_t::number_unsigned)
        {
            result.generated_errors = false;
            result.msg = "ok";
            result.value = formula[0].get<double>();
        }
        else if(formula[0].type() == json::value_t::array) {
            if(formula[0].size() == 2) {
                if(formula[0][0].type() == json::value_t::string && formula[0][1].type() == json::value_t::string) {
                    // to deal with cases like: formula:[ [ "READ_BELIEF", "batteryLevel" ] ]
                    inner_level = 1;
                }
            }

            return effectNumericalExpression(formula[0], beliefset, expression_variables, expression_constants,  inner_level);
        }
        else {
            result.generated_errors = true;
            result.msg = "Numerical formula must always start with a number or an expression array!";
            result.value = -1;
            return result;
        }
    }
    else if(formula.type() == json::value_t::number_float
            || formula.type() == json::value_t::number_integer
            || formula.type() == json::value_t::number_unsigned)
    { // formula is simple value e.g., formula:[ 0.3 ]
        result.generated_errors = false;
        result.msg = "ok";
        result.value = formula.get<double>();
    }
    else
    {
        result.generated_errors = true;
        result.msg = "Numerical formula must be an array with size > 0!";
        result.value = -1;
        return result;
    }

    return result;
}

void AgentSupport::sortSensorVectorBubbleSort(std::vector<std::shared_ptr<Sensor>>& sensors)
{   // bubble-sort
    bool sorted;
    std::shared_ptr<Sensor> tmp;
    if(sensors.size() > 1) {
        do {
            sorted = true;
            for(int i = 0; i < (int)sensors.size() - 1; i++)
                if(sensors[i]->get_time() > sensors[i + 1]->get_time())
                {
                    tmp = sensors[i];
                    sensors[i] = sensors[i+1];
                    sensors[i+1] = tmp;
                    sorted = false;
                }

        } while (sorted == false);
    }
}

void AgentSupport::sortWaitingIntentionsBubbleSort(std::vector<std::shared_ptr<Intention>>& intentions)
{
    bool sorted;
    std::shared_ptr<Intention> tmp;
    if(intentions.size() > 1) {
        do {
            sorted = true;
            for(int i = 0; i < (int)intentions.size() - 1; i++)
                if(intentions[i]->get_scheduled_completion_time() > intentions[i + 1]->get_scheduled_completion_time())
                {
                    tmp = intentions[i];
                    intentions[i] = intentions[i+1];
                    intentions[i+1] = tmp;
                    sorted = false;
                }

        } while (sorted == false);
    }
}

void AgentSupport::removeGoalAndSelectedPlanFromPhiI(std::vector<std::shared_ptr<MaximumCompletionTime>>& ag_selected_plans,
        std::vector<std::shared_ptr<Goal>>& ag_goal_set, const std::shared_ptr<Intention> intention)
{
    for(int i = 0; i < (int)ag_selected_plans.size(); i++) {
        if(ag_selected_plans[i]->getInternalGoalsSelectedPlansSize() > 0) {
            if(ag_selected_plans[i]->getInternalGoalsSelectedPlans()[0].first->get_id() == intention->get_original_planID()) {
                ag_selected_plans.erase(ag_selected_plans.begin() + i);
                break;
            }
        }
    }
    for(int i = 0; i < (int)ag_goal_set.size(); i++) {
        if(ag_goal_set[i]->get_id() == intention->get_goalID()) {
            ag_goal_set.erase(ag_goal_set.begin() + i);
            break;
        }
    }
}

void AgentSupport::removePlanFromCounted(const std::shared_ptr<Plan> plan, std::vector<std::shared_ptr<Plan>>& contained_plans) {
    logger->print(Logger::EveryInfo, " Remove plan:[id="+ std::to_string(plan->get_id()) +"] if present.");

    for(int i = 0; i < (int)contained_plans.size(); i++) {
        logger->print(Logger::EveryInfoPlus, "  - Plan:"+ std::to_string(contained_plans[i]->get_id())
        +" == "+ std::to_string(plan->get_id()));
        if(contained_plans[i]->get_id() == plan->get_id()) {
            contained_plans.erase(contained_plans.begin() + i);
            logger->print(Logger::EveryInfo, " Plan:[id="+ std::to_string(plan->get_id()) +"] has been removed.");

            break;
        }
    }
}

// Getters -------------------------------------------------------------------------------
int AgentSupport::getIntentionIndexGivenPlanId(const int plan_id, const std::vector<std::shared_ptr<Intention>>& intention_set) {
    for(int i = 0; i < (int)intention_set.size(); i++) {
        if(intention_set[i]->get_planID() == plan_id) {
            return i;
        }
    }

    return -1;
}

std::pair<simtime_t, std::string> AgentSupport::getScheduleInFuture() {
    return schedule_in_future;
}

int AgentSupport::getServerIndexInAg_servers(const int server_id, const std::vector<std::shared_ptr<Server>>& servers) {
    for(int i = 0; i < (int)servers.size(); i++) {
        if(servers[i]->getTaskServer() == server_id) {
            return i;
        }
    }
    return -1;
}

double AgentSupport::getTaskDeadline_based_on_nexec(const std::shared_ptr<Task> task,
        bool& isTaskInAlreadyActiveServer, double& server_original_deadline,
        const std::vector<std::shared_ptr<Server>>& servers,
        const std::vector<std::shared_ptr<Task>>& ready,
        const std::vector<std::shared_ptr<Task>>& release)
{
    isTaskInAlreadyActiveServer = false;
    server_original_deadline = -1;

    if(task->getTaskisPeriodic())
    { // PERIODIC
        if(task->getTaskNExecInitial() == -1) {
            return -1; // INFINITE deadline!
        }
        else {
            // in order to correctly allign tasks in IsSchedulabe ----------------------------
            logger->print(Logger::EveryInfo, "getTaskDeadline_based_on_nexec: return ("+ std::to_string(task->getTaskDeadline())
            +" * "+ std::to_string(task->getTaskNExecInitial())
            +") = "+ std::to_string(task->getTaskDeadline() * task->getTaskNExecInitial()));

            return task->getTaskDeadline() * task->getTaskNExecInitial();
            // ------------------------------------------------------------------------------
        }
    }
    else if(task->getTaskisPeriodic() == false)
    { // APERIODIC
        for(int i = 0; i < (int)servers.size(); i++)
        {
            if(servers[i]->getTaskServer() == task->getTaskServer())
            {
                server_original_deadline = servers[i]->get_curr_ddl();

                // check if server is already "active" -> meaning that it's either in "ready" or "release" queue
                double server_ddl = checkIfServerIsActiveAndGetDDL(task->getTaskServer(), ready, release);
                if(server_ddl != -2)
                { // server already active
                    isTaskInAlreadyActiveServer = true;
                    logger->print(Logger::EveryInfo, "SERVER:["+ std::to_string(task->getTaskServer())
                    +"] ALREADY active. Now:["+ std::to_string(sim_current_time().dbl())
                    +"], curr_ddl:["+ std::to_string(server_ddl));
                    return server_ddl;
                }
                else
                { // the server is not active already
                    logger->print(Logger::EveryInfo, "SERVER:["+ std::to_string(task->getTaskServer())
                    +"] NOT active. Now:["+ std::to_string(sim_current_time().dbl())
                    +"], curr_ddl:["+ std::to_string(servers[i]->get_curr_ddl())
                    +"] -> start: now, end: "+ std::to_string(sim_current_time().dbl() + servers[i]->get_curr_ddl()));
                    return servers[i]->get_curr_ddl();
                }
            }
        }
    }

    return -1;
}

std::string AgentSupport::get_variant_type_as_string(std::variant<int, double, bool, std::string> variant) {
    std::variant<int, double, bool, std::string> tmp = variant;

    if(std::get_if<int>(&tmp))
    { // it's a int
        return "INT";
    }
    else if(std::get_if<double>(&tmp))
    { // it's a double
        return "DOUBLE";
    }
    else if(std::get_if<bool>(&tmp))
    { // it's a bool
        return "BOOL";
    }
    else if(std::get_if<std::string>(&tmp))
    {
        return "STRING";
    }

    return "undefined";
}

void AgentSupport::setLogger_level(int level) {
    logger->set_level(level);
}

void AgentSupport::setScheduleInFuture(std::string event, simtime_t time) {
    if(time > schedule_in_future.first) {
        schedule_in_future.first = time;
        schedule_in_future.second = event;
    }
}

void AgentSupport::setSimulationLastExecution(std::string event, simtime_t time) {
    simulation_last_execution.first = time;
    simulation_last_execution.second = event;
}
// ---------------------------------------------------------------------------------------

// Initialize() Support functions --------------------------------------------------------
void AgentSupport::initialize_delete_files(std::string folder, std::string user, int sched_type) {
    delete_report_sched_type_folder(folder, user, convertSchedType_to_string(sched_type));
}

// Finish() support functions ------------------------------------------------------------
void AgentSupport::finishSaveToFile(std::string folder, std::string user, std::string sched_type) {
    save_ddl_checks_json(folder, user, sched_type);
    save_ddl_json(folder, user, sched_type);
    save_lateness_json(folder, user, sched_type);
    save_resp_per_task_json(folder, user, sched_type);
    save_response_json(folder, user, sched_type); // response per time
    save_simulation_timeline(folder, user, sched_type);
    save_debug_isschedulable(folder, user, sched_type);
    save_util_json(folder, user, sched_type);
}
// ---------------------------------------------------------------------------------------

/************** PRINT FUNCTIONS *****************************************************************/
void AgentSupport::printActions(const std::vector<std::shared_ptr<Action>> actions) {
    logger->print(Logger::EssentialInfo, "\nPrint Intention's Actions...");

    std::shared_ptr<Task> t; // tmp task
    std::shared_ptr<Goal> g; // tmp goal

    for(int i = 0; i < (int)actions.size(); i++)
    {
        if(actions[i]->get_type() == "task")
        {
            t = std::dynamic_pointer_cast<Task>(actions[i]);
            logger->print(Logger::EssentialInfo, " - Action ["+ std::to_string(t->getTaskId())
            +"], plan:["+ std::to_string(t->getTaskPlanId())
            +"], original_plan:["+ std::to_string(t->getTaskOriginalPlanId())
            +"], Type:[Task], arrivalTime:["+ std::to_string(t->getTaskArrivalTime())
            +"], N_exec left: " + std::to_string(t->getTaskNExec())
            +" of " + std::to_string(t->getTaskNExecInitial())
            +", compTime: " + std::to_string(t->getTaskCompTime())
            +", RES_compTime: " + std::to_string(t->getTaskCompTimeRes())
            +", DDL: " + std::to_string(t->getTaskDeadline())
            +", SERVER:["+ std::to_string(t->getTaskServer())
            +"], Execution_type:["+ t->getTaskExecutionType_as_string()
            +"] isBeenActivated:["+ boolToString(t->getTaskisBeenActivated()) +"]");
        }
        else if(actions[i]->get_type() == "goal") {
            g = std::dynamic_pointer_cast<Goal>(actions[i]);
            logger->print(Logger::EssentialInfo, " - Action ["+ std::to_string(g->get_id())
            +"], Type:[Goal] is '"+ g->get_goal_name()
            +"', Arrival Time:["+ std::to_string(g->get_arrival_time())
            +"], ExecType:["+ g->getGoalExecutionType_as_string()
            +"], isBeenActivated:["+ std::to_string(g->getGoalisBeenActivated()) +"]");
        }
    }
    logger->print(Logger::EssentialInfo, "-----------------------------");
}

void AgentSupport::printApplicablePlans(const std::vector<std::vector<std::shared_ptr<Plan>>>& applicable_plans, const std::vector<std::shared_ptr<Goal>>& ag_goal_set) {
    for(int i = 0; i < (int)applicable_plans.size(); i++) {
        printPlanset(applicable_plans[i], ag_goal_set);
    }
}

void AgentSupport::printBeliefset(const std::vector<std::shared_ptr<Belief>>& beliefset, std::string list_desc)
{
    logger->print(Logger::EssentialInfo, list_desc);

    for(std::shared_ptr<Belief> belief : beliefset) {
        logger->print(Logger::EssentialInfo, " - Belief: (" + belief->get_name() + ", " + variantToString(belief->get_value()) +")");
    }
}

void AgentSupport::printCompletedIntentions(const std::vector<std::shared_ptr<Intention>>& intentionset, const std::vector<std::shared_ptr<Goal>>& goalset) {
    logger->print(Logger::EssentialInfo, "\nList of Completed Intentions[tot = "+ std::to_string(intentionset.size()) +"]: ");
    std::string description = "";

    for(std::shared_ptr<Intention> intention : intentionset) {
        description = " Intention index:["+ std::to_string(intention->get_id());
        description += "], from goal:["+ std::to_string(intention->get_goalID());
        description += "], goal:("+ intention->get_goal_name();
        description += "), plan:["+ std::to_string(intention->get_planID());
        description += "], original_plan:["+ std::to_string(intention->get_original_planID());
        description += "], num. of actions: "+ std::to_string(intention->get_Actions().size());
        description += ", startTime: "+ std::to_string(intention->get_startTime());
        description += ", firstActivation: "+ std::to_string(intention->get_firstActivation());
        description += ", lastExecution: "+ std::to_string(intention->get_lastExecution());
        description += ", batch_startTime: "+ std::to_string(intention->get_batch_startTime());
        description += ", current completion time: "+ std::to_string(intention->get_current_completion_time());
        description += ", set completion time: "+ std::to_string(intention->get_scheduled_completion_time());

        logger->print(Logger::Default, description);
    }
}

void AgentSupport::printDesireset(const std::vector<std::shared_ptr<Desire>>& desireset)
{
    logger->print(Logger::EssentialInfo, "\nList of Desires: ");

    for(std::shared_ptr<Desire> desire : desireset) {
        logger->print(Logger::EssentialInfo, "Desire [id=" + std::to_string(desire->get_id())
        +"] is linked to Skill:["+ desire->get_goal_name() +"]:");

        logger->print(Logger::EssentialInfo, " - Preconditions: "+ desire->get_preconditions().dump());
        logger->print(Logger::EssentialInfo, " - Cont_Conditions: "+ desire->get_cont_conditions().dump());

        logger->print(Logger::EssentialInfo, " - Priority: " + std::to_string(desire->get_priority()));
        logger->print(Logger::EssentialInfo, " - Priority expression: "+ desire->get_priority_formula().dump());
        logger->print(Logger::EssentialInfo, " - Deadline: " + std::to_string(desire->get_DDL()));
        logger->print(Logger::EssentialInfo, "-----------------------------");
    }
}

void AgentSupport::printEffects(const std::vector<std::shared_ptr<Effect>>& effects) {
    logger->print(Logger::EssentialInfo, "\nList of Effects: ");

    for(std::shared_ptr<Effect> effect : effects) {
        logger->print(Logger::EssentialInfo, " - "+ effect->get_expression().dump());
    }
}

void AgentSupport::printFeasiblePlans(const std::vector<std::vector<std::shared_ptr<MaximumCompletionTime>>>& feasible_plans, std::shared_ptr<Metric> ag_metric) {
    logger->print(Logger::Default, " List of Feasible Plans[tot="+ std::to_string(feasible_plans.size()) +"]:");

    for(int i = 0; i < (int)feasible_plans.size(); i++)
    {
        printSelectedPlans(feasible_plans[i]);

        for(int p = 0; p < (int)feasible_plans[i].size(); p++)
        {
            if(ag_metric != nullptr)
            {
                if(feasible_plans[i][p]->getMCTValue() == 0 && feasible_plans[i].size() > 1) {
                    ag_metric->addGeneratedError(sim_current_time(), "printFeasiblePlans", "Error: MCT is 0 and there are no feasible plans for goal:["+ std::to_string(i) +"]");
                }
            }
        }
    }
}

void AgentSupport::printGoalset(const std::vector<std::shared_ptr<Goal>>& goalset) {
    logger->print(Logger::EssentialInfo, "List of Goals[tot = "+ std::to_string(goalset.size()) +"]: ");

    std::shared_ptr<Belief> belief;

    for(std::shared_ptr<Goal> goal : goalset) {
        logger->print(Logger::EssentialInfo, "Goal:[id="+ std::to_string(goal->get_id())
        +"], name:["+ goal->get_goal_name()
        +"], Priority:["+ std::to_string(goal->get_priority())
        +"], status:["+ goal->getGoalStatus_as_string()
        +"], Deadline:["+ std::to_string(goal->get_DDL()) +"]");
    }
    logger->print(Logger::EssentialInfo, "----------------------------------");
}

void AgentSupport::printIntentions(const std::vector<std::shared_ptr<Intention>>& intentionset, const std::vector<std::shared_ptr<Goal>>& goalset,
        const bool printSubPlans) {
    logger->print(Logger::EssentialInfo, "\nList of Intentions[tot = "+ std::to_string(intentionset.size()) +"]: ");

    std::shared_ptr<Task> t; // tmp task
    std::shared_ptr<Goal> g; // tmp goal
    std::vector<std::shared_ptr<Action>> actions;
    std::string description = "";

    for(std::shared_ptr<Intention> intention : intentionset) {
        description = " Intention index:["+ std::to_string(intention->get_id());
        description += "], from goal:["+ std::to_string(intention->get_goalID());
        description += "], goal:("+ intention->get_goal_name();
        description += "), plan:["+ std::to_string(intention->get_planID());
        description += "], original_plan:["+ std::to_string(intention->get_original_planID());
        description += "], num. of actions: "+ std::to_string(intention->get_Actions().size());
        description += ", startTime: "+ std::to_string(intention->get_startTime());
        description += ", firstActivation: "+ std::to_string(intention->get_firstActivation());
        description += ", lastExecution: "+ std::to_string(intention->get_lastExecution());
        description += ", batch_startTime: "+ std::to_string(intention->get_batch_startTime());
        description += ", current completion time: "+ std::to_string(intention->get_current_completion_time());
        description += ", set completion time: "+ std::to_string(intention->get_scheduled_completion_time());

        if(intention->get_waiting_for_completion()) {
            description += ", waiting for completion: "+ boolToString(intention->get_waiting_for_completion()) +" <-- ";
        }
        else {
            description += ", waiting for completion: "+ boolToString(intention->get_waiting_for_completion());
        }
        logger->print(Logger::EssentialInfo, description);

        if(printSubPlans)
        {
            if(intention->getMainPlan() != nullptr) {
                std::vector<std::pair<std::shared_ptr<Plan>, int>> subplans = intention->getMainPlan()->getInternalGoalsSelectedPlans();
                logger->print(Logger::EssentialInfo, " Internal goals plans: ");
                for(int i = 0; i < (int)subplans.size(); i++) {
                    logger->print(Logger::EssentialInfo, " - Subplan:["+ std::to_string(subplans[i].first->get_id())
                    +"], belief:("+ subplans[i].first->get_goal_name()
                    +"), body.size:["+ std::to_string(subplans[i].first->get_body().size())
                    +"], nest level:["+ std::to_string(subplans[i].second) +"]");
                }
            }
        }

        actions = intention->get_Actions();
        for(int i = 0; i < (int)actions.size(); i++)
        {
            if(actions[i]->get_type() == "task") {
                t = std::dynamic_pointer_cast<Task>(actions[i]);
                logger->print(Logger::EssentialInfo, " - Task:["+ std::to_string(t->getTaskId())
                +"], plan:["+ std::to_string(t->getTaskPlanId())
                +"], original_plan:["+ std::to_string(t->getTaskOriginalPlanId())
                +"], N_exec left: " + std::to_string(t->getTaskNExec())
                +" of " + std::to_string(t->getTaskNExecInitial())
                +", compTime: " + std::to_string(t->getTaskCompTime())
                +", DDL: " + std::to_string(t->getTaskDeadline())
                +", SERVER:["+ std::to_string(t->getTaskServer())
                +"], arrivalTimeInitial: " + std::to_string(t->getTaskArrivalTimeInitial())
                +", arrivalTime: " + std::to_string(t->getTaskArrivalTime())
                +", Execution_type:["+ t->getTaskExecutionType_as_string()
                +"] isBeenActivated:["+ boolToString(t->getTaskisBeenActivated()) +"]");
            }
            else if(actions[i]->get_type() == "goal") {
                g = std::dynamic_pointer_cast<Goal>(actions[i]);
                logger->print(Logger::EssentialInfo, " - Goal:[id="+ std::to_string(g->get_id())
                +"], name:["+ g->get_goal_name()
                +"], plan:["+ std::to_string(g->get_plan_id())
                +"], original_plan:["+ std::to_string(g->get_original_plan_id())
                +"], belief:("+ g->get_goal_name()
                +"), arrivalTime:["+ std::to_string(g->get_arrival_time())
                +"], absolute arrivalTime:["+ std::to_string(g->get_absolute_arrival_time())
                +"], DDL:["+ std::to_string(g->get_DDL())
                +"], ExecType:["+ g->getGoalExecutionType_as_string()
                +"], forceReasoning:["+ boolToString(g->getGoalforceReasoningCycle())
                +"], isBeenActivated:["+ boolToString(g->getGoalisBeenActivated()) +"]");
            }
        }
    }

    logger->print(Logger::EssentialInfo, "-----------------------------");
}

void AgentSupport::printIntentionsBodyDetailed(const std::vector<std::shared_ptr<Intention>> ag_intention_set)
{
    std::vector<std::shared_ptr<Action>> actions;
    std::shared_ptr<Task> t; // tmp task
    std::shared_ptr<Goal> g; // tmp goal
    int index_plan = 0;

    for(int y = 0; y < (int)ag_intention_set.size(); y++) {
        logger->print(Logger::Default, " Intention index:["+ std::to_string(ag_intention_set[y]->get_id())
        +"], from goal:["+ std::to_string(ag_intention_set[y]->get_goalID())
        +"], goal:("+ ag_intention_set[y]->get_goal_name()
        +"), plan:["+ std::to_string(ag_intention_set[y]->get_planID())
        +"], original_plan:["+ std::to_string(ag_intention_set[y]->get_original_planID())
        +"], num. of actions: "+ std::to_string(ag_intention_set[y]->get_Actions().size())
        +", completion time: "+ std::to_string(ag_intention_set[y]->get_scheduled_completion_time()));

        printIntentionsBodyInternalGoals(ag_intention_set[y]->getMainPlan(), index_plan);
        index_plan = 0;
    }
}

void AgentSupport::printIntentionsBodyInternalGoals(const std::shared_ptr<MaximumCompletionTime> plan, int& index_plan)
{
    std::vector<std::shared_ptr<Action>> body = plan->getInternalGoalsSelectedPlans()[index_plan].first->get_body();
    std::shared_ptr<Task> t; // tmp task
    std::shared_ptr<Goal> g; // tmp goal
    std::string nesting_spaces = "";

    // DEBUG printing purposes ----------------------------
    for(int i = 0; i < index_plan; i++) {
        nesting_spaces += "    ";
    }
    // -------------------------------------------

    for(int i = 0; i < (int)body.size(); i++)
    {
        if(body[i]->get_type() == "task")
        {
            t = std::dynamic_pointer_cast<Task>(body[i]);
            logger->print(Logger::EssentialInfo, nesting_spaces +" - Action ["+ std::to_string(t->getTaskId())
            +"], plan:["+ std::to_string(t->getTaskPlanId())
            +"], original_plan:["+ std::to_string(t->getTaskOriginalPlanId())
            +"], Type:[Task], arrivalTime:["+ std::to_string(t->getTaskArrivalTime())
            +"], N_exec left: " + std::to_string(t->getTaskNExec())
            +" of " + std::to_string(t->getTaskNExecInitial())
            +", compTime: " + std::to_string(t->getTaskCompTime())
            +", RES_compTime: " + std::to_string(t->getTaskCompTimeRes())
            +", DDL: " + std::to_string(t->getTaskDeadline())
            +", SERVER:["+ std::to_string(t->getTaskServer())
            +"], Execution_type:["+ t->getTaskExecutionType_as_string()
            +"] isBeenActivated:["+ boolToString(t->getTaskisBeenActivated()) +"]");
        }
        else if(body[i]->get_type() == "goal") {
            g = std::dynamic_pointer_cast<Goal>(body[i]);
            logger->print(Logger::EssentialInfo, nesting_spaces +" - Action ["+ std::to_string(g->get_id())
            +"], Type:[Goal] is '"+ g->get_goal_name()
            +"', Arrival Time:["+ std::to_string(g->get_arrival_time())
            +"], ExecType:["+ g->getGoalExecutionType_as_string()
            +"], isBeenActivated:["+ std::to_string(g->getGoalisBeenActivated()) +"]");

            index_plan += 1;
            printIntentionsBodyInternalGoals(plan, index_plan);
        }
    }
}

void AgentSupport::printInvalidIntentions(const std::vector<std::shared_ptr<Intention>>& intentionset, const std::vector<std::shared_ptr<Goal>>& goalset)
{
    logger->print(Logger::EssentialInfo, "\nList of INVALID Intentions[tot = "+ std::to_string(intentionset.size()) +"]: ");

    std::shared_ptr<Task> t;
    std::shared_ptr<Goal> g;
    std::vector<std::shared_ptr<Action>> actions;
    std::string description = "";

    for(std::shared_ptr<Intention> intention : intentionset) {
        description = " Intention index:["+ std::to_string(intention->get_id());
        description += "], from goal:["+ std::to_string(intention->get_goalID());
        description += "], goal:("+ intention->get_goal_name();
        description += "), plan:["+ std::to_string(intention->get_planID());
        description += "], original_plan:["+ std::to_string(intention->get_original_planID());
        description += "], num. of actions: "+ std::to_string(intention->get_Actions().size());
        description += ", startTime: "+ std::to_string(intention->get_startTime());
        description += ", firstActivation: "+ std::to_string(intention->get_firstActivation());
        description += ", lastExecution: "+ std::to_string(intention->get_lastExecution());
        description += ", batch_startTime: "+ std::to_string(intention->get_batch_startTime());
        description += ", current completion time: "+ std::to_string(intention->get_current_completion_time());
        description += ", set completion time: "+ std::to_string(intention->get_scheduled_completion_time());

        if(intention->get_waiting_for_completion()) {
            description += ", waiting for completion: "+ boolToString(intention->get_waiting_for_completion()) +" <-- ";
        }
        else {
            description += ", waiting for completion: "+ boolToString(intention->get_waiting_for_completion());
        }
        logger->print(Logger::EssentialInfo, description);

        actions = intention->get_Actions();
        for(int i = 0; i < (int)actions.size(); i++)
        {
            if(actions[i]->get_type() == "task") {
                t = std::dynamic_pointer_cast<Task>(actions[i]);
                logger->print(Logger::EssentialInfo, " - Task:["+ std::to_string(t->getTaskId())
                +"], plan:["+ std::to_string(t->getTaskPlanId())
                +"], original_plan:["+ std::to_string(t->getTaskOriginalPlanId())
                +"], N_exec left: " + std::to_string(t->getTaskNExec())
                +" of " + std::to_string(t->getTaskNExecInitial())
                +", compTime: " + std::to_string(t->getTaskCompTime())
                +", DDL: " + std::to_string(t->getTaskDeadline())
                +", SERVER:["+ std::to_string(t->getTaskServer())
                +"], arrivalTimeInitial: " + std::to_string(t->getTaskArrivalTimeInitial())
                +", arrivalTime: " + std::to_string(t->getTaskArrivalTime())
                +", Execution_type:["+ t->getTaskExecutionType_as_string()
                +"] isBeenActivated:["+ boolToString(t->getTaskisBeenActivated()) +"]");
            }
            else if(actions[i]->get_type() == "goal") {
                g = std::dynamic_pointer_cast<Goal>(actions[i]);
                logger->print(Logger::EssentialInfo, " - Goal:[id="+ std::to_string(g->get_id())
                +"], name:["+ g->get_goal_name()
                +"], plan:["+ std::to_string(g->get_plan_id())
                +"], original_plan:["+ std::to_string(g->get_original_plan_id())
                +"], belief:("+ g->get_goal_name()
                +"), arrivalTime:["+ std::to_string(g->get_arrival_time())
                +"], absolute arrivalTime:["+ std::to_string(g->get_absolute_arrival_time())
                +"], DDL:["+ std::to_string(g->get_DDL())
                +"], ExecType:["+ g->getGoalExecutionType_as_string()
                +"], forceReasoning:["+ boolToString(g->getGoalforceReasoningCycle())
                +"], isBeenActivated:["+ boolToString(g->getGoalisBeenActivated())
                +"], selected plan id:["+ std::to_string(g->get_selected_plan_id()) +"]");
            }
        }
    }

    logger->print(Logger::EssentialInfo, "-----------------------------");
}

void AgentSupport::printWaitingCompletionIntentions(const std::vector<std::shared_ptr<Intention>>& intentionset, const std::vector<std::shared_ptr<Goal>>& goalset, const bool printSubPlans) {
    std::vector<std::shared_ptr<Intention>> waiting_completion;

    for(std::shared_ptr<Intention> intention : intentionset) {
        if(intention->get_waiting_for_completion()) {
            waiting_completion.push_back(std::make_shared<Intention>(*intention));
        }
    }

    sortWaitingIntentionsBubbleSort(waiting_completion);// sort by increasing completion time

    logger->print(Logger::EssentialInfo, "\nList of Waiting for Completion[tot = "+ std::to_string(waiting_completion.size()) +"]: ");

    printIntentions(waiting_completion, goalset, printSubPlans);
}

void AgentSupport::printInternalGoalsSelectedPlans(const std::vector<std::pair<std::shared_ptr<Plan>, int>>& plans) {
    logger->print(Logger::EssentialInfo, " ----printInternalGoalsSelectedPlans---- ");
    for(int i = 0; i < (int)plans.size(); i++) {
        if(plans[i].first != nullptr) {
            logger->print(Logger::EssentialInfo, " Annidation:["+ std::to_string(plans[i].second)
            +"], plan_id:["+ std::to_string(plans[i].first->get_id())
            +"], belief:("+ plans[i].first->get_goal_name() +")");
        }
    }
}

void AgentSupport::printPlanset(const std::vector<std::shared_ptr<Plan>>& planset, const std::vector<std::shared_ptr<Goal>>& ag_goal_set) {
    logger->print(Logger::EssentialInfo, "\nList of Plans: ");

    double end = 0;
    int index_goal = 0;
    std::string warning_msg = "";

    if(planset.size() > 0) {
        for(std::shared_ptr<Plan> plan : planset) {
            if(plan != nullptr) {
                logger->print(Logger::EssentialInfo, " - Plan [id="+ std::to_string(plan->get_id())
                +"] is linked to Skill:["+ plan->get_goal_name()
                +"]:\n -- Belief: ("+ plan->get_goal_name()+ ")");
                logger->print(Logger::EssentialInfo, " -- Pref value: "+ std::to_string(plan->get_pref()));

                logger->print(Logger::EveryInfo, " -- Preconditions: "+ plan->get_preconditions().dump());
                logger->print(Logger::EveryInfo, " -- Context conditions: "+ plan->get_cont_conditions().dump());
                logger->print(Logger::EveryInfo, " -- Post conditions: "+ plan->get_post_conditions().dump());

                logger->print(Logger::EssentialInfo, " -- Body:");
                for(std::shared_ptr<Action> action : plan->get_body()) {

                    if(action->get_type() == "goal") {
                        std::shared_ptr<Goal> g = std::dynamic_pointer_cast<Goal>(action);
                        logger->print(Logger::EssentialInfo, " --- Plan.body(Type: Goal) is linked to Skill:["+ g->get_goal_name() +"]");
                    }
                    else if(action->get_type() == "task") {
                        std::shared_ptr<Task> t = std::dynamic_pointer_cast<Task>(action);
                        logger->print(Logger::EssentialInfo, " --- Plan.body(Type: Task)");

                        if(t->getTaskDeadline() > end) {
                            end = t->getTaskDeadline();
                        }
                    }
                    else {
                        warning_msg = " [WARNING] Plan:["+ std::to_string(plan->get_id()) +"] contains invalid action(s)!";
                        logger->print(Logger::Warning, warning_msg);
                    }
                }
            }
            else {
                logger->print(Logger::EssentialInfo, " There are no plans for this specific Goal:["
                        +std::to_string(ag_goal_set[index_goal]->get_id()) +"]");
                index_goal += 1;
            }
            logger->print(Logger::EssentialInfo, "--------------------------------------------");
        }
    }
    else {
        logger->print(Logger::EssentialInfo, " List is empty...");
    }
}

void AgentSupport::printPlanset(const std::vector<std::shared_ptr<MaximumCompletionTime>>& planset, const std::vector<std::shared_ptr<Goal>>& ag_goal_set) {
    logger->print(Logger::EveryInfo, "\nList of Plans[tot="+ std::to_string(planset.size()) +"]: ");

    std::shared_ptr<Plan> plan;
    std::string warning_msg = "";
    double end = 0;
    int index_goal = 0;

    if(planset.size() > 0) {
        for(std::shared_ptr<MaximumCompletionTime> ps : planset) {
            plan = ps->getInternalGoalsSelectedPlans()[0].first;

            if(plan != nullptr) {
                logger->print(Logger::EssentialInfo, " - Plan [id="+ std::to_string(plan->get_id())
                +"] is linked to Skill:["+ plan->get_goal_name()
                +"]:\n -- Belief: ("+ plan->get_goal_name() +")");
                logger->print(Logger::EssentialInfo, " -- Pref value: "+ std::to_string(plan->get_pref()));

                logger->print(Logger::EveryInfo, " -- Preconditions: "+ plan->get_preconditions().dump());
                logger->print(Logger::EveryInfo, " -- Context conditions: "+ plan->get_cont_conditions().dump());
                logger->print(Logger::EveryInfo, " -- Post conditions: "+ plan->get_post_conditions().dump());

                logger->print(Logger::EveryInfo, " -- Body:");
                for(std::shared_ptr<Action> action : plan->get_body()) {
                    if(action->get_type() == "goal") {
                        std::shared_ptr<Goal> g = std::dynamic_pointer_cast<Goal>(action);
                        logger->print(Logger::EveryInfo, " --- Plan.body(Type: Goal) is linked to Skill:["+ g->get_goal_name()
                                +"], belief: ("+ g->get_goal_name() +")");
                    }
                    else if(action->get_type() == "task") {
                        std::shared_ptr<Task> t = std::dynamic_pointer_cast<Task>(action);
                        logger->print(Logger::EveryInfo, " --- Plan.body(Type: Task)");

                        if(t->getTaskDeadline() > end) {
                            end = t->getTaskDeadline();
                        }
                    }
                    else {
                        warning_msg = " [WARNING] Plan:["+ std::to_string(plan->get_id()) +"] contains invalid action(s)!";
                        logger->print(Logger::Warning, warning_msg);
                    }
                }
            }
            else {
                logger->print(Logger::EssentialInfo, " There are no plans for this specific Goal:["
                        +std::to_string(ag_goal_set[index_goal]->get_id()) +"]");
                index_goal += 1;
            }
            logger->print(Logger::EssentialInfo, "--------------------------------------------");
        }
    }
    else {
        logger->print(Logger::EssentialInfo, " List is empty...");
    }
}

void AgentSupport::printSelectedPlans(const std::vector<std::shared_ptr<MaximumCompletionTime>>& selected_plans) {
    logger->print(Logger::Default, " List of Selected Plans[tot="+ std::to_string(selected_plans.size()) +"]:");
    for(int p = 0; p < (int)selected_plans.size(); p++)
    {
        if(selected_plans[p] != nullptr)
        {
            logger->print(Logger::Default, "SelectedPlan id: "+ std::to_string(selected_plans[p]->getInternalGoalsSelectedPlans()[0].first->get_id())
            +", goal:["+ selected_plans[p]->getInternalGoalsSelectedPlans()[0].first->get_goal_name()
            +"], pref:["+ std::to_string(selected_plans[p]->getInternalGoalsSelectedPlans()[0].first->get_pref())
            +"], actions:["+ std::to_string(selected_plans[p]->getInternalGoalsSelectedPlans()[0].first->get_body().size())
            +"], DDL:["+ std::to_string(selected_plans[p]->getMCTValue())
            +"], num_of_internal_goals:["+ std::to_string(selected_plans[p]->getInternalGoalsSelectedPlansSize()) +"]: ");

            if(selected_plans[p]->getInternalGoalsSelectedPlansSize() > 1)
            {
                for(int i = 1; i < selected_plans[p]->getInternalGoalsSelectedPlansSize(); i++)
                {
                    logger->print(Logger::Default, " - sub_plan:[id="+ std::to_string(selected_plans[p]->getInternalGoalsSelectedPlans()[i].first->get_id())
                    +"], goal:["+ selected_plans[p]->getInternalGoalsSelectedPlans()[i].first->get_goal_name()
                    +"], pref:["+ std::to_string(selected_plans[p]->getInternalGoalsSelectedPlans()[i].first->get_pref())
                    +"], actions:["+ std::to_string(selected_plans[p]->getInternalGoalsSelectedPlans()[i].first->get_body().size())
                    +"], nested level: "+ std::to_string(selected_plans[p]->getInternalGoalsSelectedPlans()[i].second));
                }
            }
        }
    }
}

void AgentSupport::printSelectedPlan(const std::shared_ptr<MaximumCompletionTime> selected_plan) {
    logger->print(Logger::EssentialInfo, " ----printSelectedPlan---- ");
    if(selected_plan != nullptr) {
        logger->print(Logger::EssentialInfo, "SelectedPlan id: "+ std::to_string(selected_plan->getInternalGoalsSelectedPlans()[0].first->get_id())
        +", goal:["+ selected_plan->getInternalGoalsSelectedPlans()[0].first->get_goal_name()
        +"], plan:["+ std::to_string(selected_plan->getInternalGoalsSelectedPlans()[0].first->get_id())
        +"], actions:["+ std::to_string(selected_plan->getInternalGoalsSelectedPlans()[0].first->get_body().size())
        +"], num_of_internal_goals:["+ std::to_string(selected_plan->getInternalGoalsSelectedPlansSize()) +"]: ");

        if(selected_plan->getInternalGoalsSelectedPlansSize() > 1) {
            for(int i = 1; i < selected_plan->getInternalGoalsSelectedPlansSize(); i++) {
                logger->print(Logger::EssentialInfo, " - sub_plan: "+ std::to_string(selected_plan->getInternalGoalsSelectedPlans()[i].first->get_id())
                +", goal:["+ selected_plan->getInternalGoalsSelectedPlans()[i].first->get_goal_name()
                +"], plan:["+ std::to_string(selected_plan->getInternalGoalsSelectedPlans()[i].first->get_id())
                +"], actions:["+ std::to_string(selected_plan->getInternalGoalsSelectedPlans()[i].first->get_body().size())
                +"], annidation: "+ std::to_string(selected_plan->getInternalGoalsSelectedPlans()[i].second));
            }
        }
    }
}

void AgentSupport::printSensors(const std::vector<std::shared_ptr<Sensor>>& sensors)
{
    logger->print(Logger::Default, "List of Sensors[tot = "+ std::to_string(sensors.size()) +"]: ");

    for(int i = 0; i < (int)sensors.size(); i++) {
        logger->print(Logger::Default, " - Sensor:["+ std::to_string(sensors[i]->get_id())
        +"], belief:["+ sensors[i]->get_belief_name()
        +"], value:["+ variantToString(sensors[i]->get_value())
        +"], mode:["+ sensors[i]->get_mode_as_string()
        +"], already scheduled:["+ boolToString(sensors[i]->get_scheduled())
        +"], activation t: "+ std::to_string(sensors[i]->get_time()));
    }
}

void AgentSupport::printServers(const std::vector<std::shared_ptr<Server>>& servers) {
    logger->print(Logger::Default, "List of Servers[tot = "+ std::to_string(servers.size()) +"]: ");

    for(std::shared_ptr<Server> server : servers) {
        logger->print(Logger::Default, " - Server:["+ std::to_string(server->get_server_id()) +"]");
        logger->print(Logger::Default, " - Period:[" + std::to_string(server->get_curr_ddl()) +"], default:[" + std::to_string(server->get_period()) +"]");
        logger->print(Logger::Default, " - Budget:[" + std::to_string(server->get_curr_budget()) +"], default:[" + std::to_string(server->get_budget()) +"]");
        logger->print(Logger::Default, "-------------------------------");
    }
}

void AgentSupport::printSkillset(const std::vector<std::shared_ptr<Skill>>& skillset) {
    logger->print(Logger::EssentialInfo, "List of Skills[tot = "+ std::to_string(skillset.size()) +"]: ");

    for(std::shared_ptr<Skill> skill : skillset) {
        logger->print(Logger::EssentialInfo, "Skill:["+ skill->get_goal_name() +"]");
    }
    logger->print(Logger::EssentialInfo, "----------------------------------");
}

void AgentSupport::printTaskset(const std::vector<std::shared_ptr<Task>>& taskset) {
    logger->print(Logger::EssentialInfo, "\nPrint Actions...");
    std::shared_ptr<Task> t;

    for(int i = 0; i < (int)taskset.size(); i++) {
        t = taskset[i];
        logger->print(Logger::EssentialInfo, " - Action ["+ std::to_string(t->getTaskId())
        +"], plan:["+ std::to_string(t->getTaskPlanId())
        +"], original_plan:["+ std::to_string(t->getTaskOriginalPlanId())
        +"], Type:[Task], N_exec left: " + std::to_string(t->getTaskNExec())
        +", of " + std::to_string(t->getTaskNExecInitial())
        +", compTime: " + std::to_string(t->getTaskCompTime())
        +", RES_compTime: " + std::to_string(t->getTaskCompTimeRes())
        +", DDL: " + std::to_string(t->getTaskDeadline())
        +", Execution_type:["+ t->getTaskExecutionType_as_string()
        +"] isBeenActivated:["+ boolToString(t->getTaskisBeenActivated()) +"]");
    }
    logger->print(Logger::EssentialInfo, "-----------------------------");
}

void AgentSupport::printVectorOfString(const std::vector<std::string>& strings) {
    logger->print(Logger::Default, " Print vector of string[tot="+ std::to_string(strings.size()) +"]:");
    for(int i = 0; i < (int)strings.size(); i++) {
        logger->print(Logger::Default, strings[i]);
    }
}

void AgentSupport::printVectorOfPairString(const std::vector<std::pair<std::string, std::string>>& strings)  {
    logger->print(Logger::Default, " Print vector of pair of string[tot="+ std::to_string(strings.size()) +"]:");
    for(int i = 0; i < (int)strings.size(); i++) {
        logger->print(Logger::Default, strings[i].first +" --- "+ strings[i].second);
    }
}
/*********************************************************************************************/

void AgentSupport::update_activated_goal_list(const std::vector<std::shared_ptr<Goal>>& ag_goal_set,
        const std::vector<std::shared_ptr<Goal>>& reasoning_cycle_already_active_goals, std::vector<std::string>& reasoning_cycle_new_activated_goals)
{
    bool new_goal = true;
    bool already_present = false;

    for(int i = 0; i < (int)ag_goal_set.size(); i++)
    {
        new_goal = true;
        already_present = false;

        for(int j = 0; j < (int)reasoning_cycle_already_active_goals.size(); j++)
        {
            logger->print(Logger::EveryInfoPlus, "update_activated_goal_list: "+ ag_goal_set[i]->get_goal_name() +" == "+ reasoning_cycle_already_active_goals[j]->get_goal_name());
            if(ag_goal_set[i]->get_goal_name() == reasoning_cycle_already_active_goals[j]->get_goal_name())
            {
                new_goal = false;
                break;
            }
        }

        if(new_goal)
        {
            // loop added to manage cases where multiple reasoning cycles are scheduled at the same time (e.g. when sensors are scheduled at the same t)
            for(int k = 0; k < (int)reasoning_cycle_new_activated_goals.size(); k++)
            {
                logger->print(Logger::EveryInfoPlus, "update_activated_goal_list 2: "+ ag_goal_set[i]->get_goal_name() +" == "+ reasoning_cycle_new_activated_goals[k]);
                if(reasoning_cycle_new_activated_goals[k] == ag_goal_set[i]->get_goal_name())
                {
                    already_present = true;
                    break;
                }
            }

            if(already_present == false) {
                reasoning_cycle_new_activated_goals.push_back(ag_goal_set[i]->get_goal_name());
            }
        }
    }
}

void AgentSupport::update_internal_goal_selectedPlans(const std::vector<std::shared_ptr<MaximumCompletionTime>>& ag_selected_plans,
        std::vector<std::shared_ptr<Intention>>& ag_intention_set)
{
    logger->print(Logger::EveryInfo, " --- update_internal_goal_selectedPlans --- ");

    for(int i = 0; i < (int)ag_selected_plans.size(); i++) {
        for(int j = 0; j < (int)ag_intention_set.size(); j++) {
            logger->print(Logger::EveryInfo, std::to_string(ag_intention_set[j]->get_original_planID())
            +" == "+ std::to_string(ag_selected_plans[i]->getInternalGoalsSelectedPlans()[0].first->get_id()));

            if(ag_intention_set[j]->get_original_planID() == ag_selected_plans[i]->getInternalGoalsSelectedPlans()[0].first->get_id())
            {
                logger->print(Logger::EveryInfo, "Intention:["+ std::to_string(ag_intention_set[j]->get_id())
                +"], plan id:["+ std::to_string(ag_intention_set[j]->get_planID())
                +"], original plan id:["+ std::to_string(ag_intention_set[j]->get_original_planID())
                +"], name:["+ ag_intention_set[j]->get_goal_name()
                +"], change internal goal selected plan using 'ag_selected_plans:["+ std::to_string(i)
                +"], plan id:["+ std::to_string(ag_selected_plans[i]->getInternalGoalsSelectedPlans()[0].first->get_id())
                +"], name:["+ ag_selected_plans[i]->getInternalGoalsSelectedPlans()[0].first->get_goal_name() +"]'");
                logger->print(Logger::EveryInfo, "i: "+ std::to_string(i) +", j: "+ std::to_string(j));

                ag_intention_set[j]->update_internal_goal_selectedPlans(ag_selected_plans[i]);
            }
        }
        logger->print(Logger::EveryInfo, " --------------------- ");
    }
}

std::variant<int, double, bool, std::string> AgentSupport::updateBeliefValueBasedOnSensorRead(std::variant<int, double, bool, std::string> ag_belief_value,
        std::variant<int, double, bool, std::string> effect_value,
        const Sensor::Mode mode, bool& updatedSuccessfully)
{
    /* step 1: check that the agent_belief's value and the sensor's value have the same type.
     *           We don't want to decrease a bool...
     * step 2: update the value base on the sensor **Mode** */
    std::string error_msg = " Sensor can't update the belief, the types don't match! ";
    updatedSuccessfully = true;

    if(auto b = std::get_if<int>(&ag_belief_value))
    {
        if(auto s_b = std::get_if<int>(&effect_value)) {
            int& ab = *b;
            int& db = *s_b;

            if(mode == Sensor::INCREASE) {
                return (ab + db);
            }
            else if(mode == Sensor::DECREASE) {
                return (ab - db);
            }
            else {
                return db;
            }
        }
        else if(auto s_b = std::get_if<double>(&effect_value)) {
            int& ab = *b;
            double& db = *s_b;

            if(mode == Sensor::INCREASE) {
                return (ab + db);
            }
            else if(mode == Sensor::DECREASE) {
                return (ab - db);
            }
            else {
                return db;
            }
        }
        else {
            logger->print(Logger::EveryInfo, error_msg);
        }
    }
    else if(auto b = std::get_if<double>(&ag_belief_value))
    {
        if(auto s_b = std::get_if<double>(&effect_value)) {
            double& ab = *b;
            double& db = *s_b;

            if(mode == Sensor::INCREASE) {
                return (ab + db);
            }
            else if(mode == Sensor::DECREASE) {
                return (ab - db);
            }
            else {
                return db;
            }
        }
        else if(auto s_b = std::get_if<int>(&effect_value)) {
            double& ab = *b;
            int& db = *s_b;

            if(mode == Sensor::INCREASE) {
                return (ab + db);
            }
            else if(mode == Sensor::DECREASE) {
                return (ab - db);
            }
            else {
                return db;
            }
        }
        else {
            logger->print(Logger::EveryInfo, error_msg);
        }
    }
    else if(std::get_if<bool>(&ag_belief_value))
    {
        if(auto s_b = std::get_if<bool>(&effect_value)) {
            bool& db = *s_b;

            return db; // ONLY the CHANGE mode is available for BOOL type
        }
        else {
            logger->print(Logger::EveryInfo, error_msg);
        }
    }
    else if(std::get_if<std::string>(&ag_belief_value))
    {
        if(auto s_b = std::get_if<std::string>(&effect_value)) {
            std::string& db = *s_b;

            return db; // ONLY the CHANGE mode is available for STRING type
        }
        else {
            logger->print(Logger::EveryInfo, error_msg);
        }
    }

    // it reaches this point if the types of the two variant are different
    updatedSuccessfully = false;
    return nullptr;
}

int AgentSupport::updateBeliefsetDueToEffect(std::vector<std::shared_ptr<Belief>>& beliefset,
        std::shared_ptr<Intention>& intention,
        const std::vector<std::shared_ptr<Effect>>& effects)
{
    logger->print(Logger::EssentialInfo, " ----updateBeliefsetDueToEffect---- ");
    AgentSupport::ExpressionResult evaluated_expression;
    std::vector<std::shared_ptr<Belief>> expression_constants = intention->get_expression_constants();
    std::vector<std::shared_ptr<Belief>> expression_variables = intention->get_expression_variables();
    std::shared_ptr<Belief> belief;
    std::string warning_msg = "";
    json expression;
    int cnt_updated_beliefs = 0;

    if(logger->get_level() >= Logger::EveryInfo)
    {
        printBeliefset(expression_constants, " List of Constants [tot = "+ std::to_string(expression_constants.size()) +"]:");
        printBeliefset(expression_variables, " List of Variables [tot = "+ std::to_string(expression_variables.size()) +"]:");

        logger->print(Logger::Default, " ------------------- ");
    }

    for(int i = 0; i < (int)effects.size(); i++) {
        expression = effects[i]->get_expression();

        int log_level = logger->get_level(); // Debug
        // /* xx */logger->set_level(Logger::EveryInfoPlus);

        // Get the belief the effect i-th is going to update. If effect = DEFINE, belief will be nullptr
        belief = nullptr;
        if(expression.size() >= 3) {
            if(expression[1].type() == json::value_t::string) {
                for(int y = 0; y < (int)beliefset.size(); y++) {
                    if(beliefset[y]->get_name() == expression[1].get<std::string>()) {
                        belief = std::make_shared<Belief>(*beliefset[y]); // make a copy of the belief
                    }
                }
            }
        }

        evaluated_expression = parseExpression(expression, beliefset,
                expression_variables, expression_constants);

        if(logger->get_level() >= Logger::EveryInfo) {
            logger->print(Logger::Default, "evaluated_expression -> EFFECT:["+ std::to_string(i + 1) +"]");
            logger->print(Logger::Default, " - expression: "+ expression.dump());
            logger->print(Logger::Default, " - EVALUATION: \n - errors: "+ boolToString(evaluated_expression.generated_errors)
                    +"\n - msg: " + evaluated_expression.msg
                    +"\n - value: " + variantToString(evaluated_expression.value));
            logger->print(Logger::EveryInfoPlus, " ************************************** ");
        }

        if(evaluated_expression.generated_errors == true) {
            warning_msg = "[WARNING] Effect:["+ std::to_string(i + 1);
            warning_msg += "] has generated an error! Cause:\""+ evaluated_expression.msg +"\"";
            logger->print(Logger::Warning, warning_msg);
        }
        else {
            if(expression[0].get<std::string>() == "DEFINE_VARIABLE" || expression[0].get<std::string>() == "DEFINE_CONSTANT")
            {
                logger->print(Logger::EssentialInfo, " - Effect:["+ std::to_string(i + 1)
                +" of "+ std::to_string(effects.size())
                +"] -> Performed:["+ expression[0].get<std::string>()
                +"] with name:["+ variantToString(expression[1].get<std::string>())
                +"] and value:["+ variantToString(evaluated_expression.value) +"]");
            }
            else {
                if(compareBeliefValues(belief->get_value(), evaluated_expression.value, Operator::EQUAL) == false) {
                    // if the value of the belief does not change, we do not count it as an actual update.
                    // this approach allows to avoid repeating reasoning cycles if the beliefset has the same identical state of before effects_at_...
                    cnt_updated_beliefs += 1;
                }

                // notify user about the update
                logger->print(Logger::EssentialInfo, " - Effect:["+ std::to_string(i + 1)
                +" of "+ std::to_string(effects.size())
                +"] -> Belief:["+ expression[1].get<std::string>()
                +"] updated from:["+ variantToString(belief->get_value())
                +"] to:["+ variantToString(evaluated_expression.value) +"]");
            }
        }

        // DEBUG ********************************************************
        if(logger->get_level() >= Logger::EveryInfo) {
            printBeliefset(beliefset);
            logger->print(Logger::EveryInfoPlus, " --- DEBUG --- ");
        }
        // ********************************************************************

        logger->set_level(log_level);
    }

    // store valid constants and variables in intention object....
    intention->set_expression_constants(expression_constants);
    intention->set_expression_variables(expression_variables);

    return cnt_updated_beliefs;
}

void AgentSupport::updatePeriodicTasksInsideIntention(std::shared_ptr<Task> task,
        std::vector<std::shared_ptr<Intention>>& intentionset)
{
    std::vector<std::shared_ptr<Action>> actions;
    std::shared_ptr<Task> tmp_t;

    for(int i = 0; i < (int)intentionset.size(); i++)
    {
        actions = intentionset[i]->get_Actions();
        for(int a = 0; a < (int)actions.size(); a++)
        {
            if(actions[a]->get_type() == "task")
            {
                tmp_t = std::dynamic_pointer_cast<Task>(actions[a]);

                logger->print(Logger::EveryInfo,"updatePeriodicTasksInsideIntention: "+ std::to_string(tmp_t->getTaskId()) +" == "+ std::to_string(task->getTaskId())
                +" && "+ std::to_string(tmp_t->getTaskPlanId()) +" == "+ std::to_string(task->getTaskPlanId())
                +" && "+ std::to_string(tmp_t->getTaskOriginalPlanId()) +" == "+ std::to_string(task->getTaskOriginalPlanId())
                +" && "+ std::to_string(tmp_t->getTaskisBeenActivated()) +" == 1");

                if(tmp_t->getTaskId() == task->getTaskId()
                        && tmp_t->getTaskPlanId() == task->getTaskPlanId()
                        && tmp_t->getTaskOriginalPlanId() == task->getTaskOriginalPlanId()
                        && tmp_t->getTaskisBeenActivated() == true
                        && tmp_t->get_server_id() == -1)
                {
                    logger->print(Logger::EveryInfo,"updatePeriodicTasksInsideIntention N_exec: from '"+ std::to_string(tmp_t->getTaskNExec())
                    +"' to '"+ std::to_string(task->getTaskNExec()) +"'");

                    tmp_t->setTaskNExec(task->getTaskNExec());
                }
            }
        }
    }
}

void AgentSupport::updateSelectedPlans(std::vector<std::shared_ptr<MaximumCompletionTime>>& selectedPlans) {
    /* In order to update scenarios like:
     * SelectedPlan id: 0, goal:[ready_to_takeoff], num_of_internal_goals:[3]:
     *  - sub_plan: 3, goal:[destination_set], annidation: 1 <---- this subplan must be 2, not 3
     *  - sub_plan: 7, goal:[boarding_luggage], annidation: 1
     * SelectedPlan id: 2, goal:[destination_set], num_of_internal_goals:[1]:
     * SelectedPlan id: 7, goal:[boarding_luggage], num_of_internal_goals:[1]: */
    logger->print(Logger::EveryInfo, " \n updateSelectedPlans... ");
    std::vector<std::pair<std::shared_ptr<Plan>, int>> tmp_plans;
    std::shared_ptr<Plan> tmp_plan;

    for(int i = 0; i < (int)selectedPlans.size(); i++) {
        tmp_plan = selectedPlans[i]->getInternalGoalsSelectedPlans()[0].first;

        // for each selectedPlans[i], check if this plan is a sub-plan of the other selectedPlans...
        for(int j = 0; j < (int)selectedPlans.size(); j++) {
            if(i != j) {
                tmp_plans = selectedPlans[j]->getInternalGoalsSelectedPlans();
                for(int k = 0; k < (int)tmp_plans.size(); k++) {
                    logger->print(Logger::EveryInfoPlus, tmp_plan->get_goal_name()
                            +" == "+ tmp_plans[k].first->get_goal_name() + "?");

                    if(tmp_plan->get_goal_name() == tmp_plans[k].first->get_goal_name()) {
                        logger->print(Logger::EveryInfoPlus, std::to_string(tmp_plan->get_id())
                        +" == "+ std::to_string(tmp_plans[k].first->get_id()) + "?");

                        if(tmp_plan->get_id() != tmp_plans[k].first->get_id()) {
                            // same goal, different selectedPlan...
                            // update the internal select plan...
                            selectedPlans[j]->getInternalGoalsSelectedPlans()[k].first = tmp_plan;
                        }
                    }
                }
            }
        }
    }
}

// Convert variant variable to string in order to be printable
std::string AgentSupport::variantToString(std::variant<int, double, bool, std::string> variant)
{
    std::variant<int, double, bool, std::string> tmp;
    tmp = variant;

    if(auto val = std::get_if<int>(&tmp))
    { // it's a int
        int& s = *val;
        return std::to_string(s);
    }
    else if(auto val = std::get_if<double>(&tmp))
    { // it's a double
        double& s = *val;
        return std::to_string(s);
    }
    else if(auto val = std::get_if<bool>(&tmp))
    { // it's a bool
        bool& s = *val;
        return (s) ? "true" : "false";
    }
    else if(auto val = std::get_if<std::string>(&tmp))
    {
        std::string& s = *val;
        return s;
    }

    return ""; // Type not supported (should not reach this point)
}

double AgentSupport::variantToDouble(std::variant<int, double, bool, std::string> variant, bool& succ) {
    if(auto val = std::get_if<double>(&variant))
    { // it's double
        double& s = *val;
        succ = true;
        return s;
    }
    else if(auto val = std::get_if<int>(&variant))
    { // it's double
        int& s = *val;
        succ = true;
        return (double) s;
    }

    succ = false;
    return -1;
}

bool AgentSupport::variantToBool(std::variant<int, double, bool, std::string> variant, bool& succ) {
    if(auto val = std::get_if<bool>(&variant))
    { // it's double
        bool& s = *val;
        succ = true;
        return s;
    }

    succ = false;
    return false;
}

/* *******************************************************************
 * **************    Private Functions    ****************************
 * *******************************************************************/
simtime_t AgentSupport::sim_current_time() {
    return simTime().trunc(sim_time_unit);
}

