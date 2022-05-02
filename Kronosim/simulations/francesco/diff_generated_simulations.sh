#!/bin/bash
# Run this file using the command: 'bash random_events_generator.sh'
# Move from this folder, to the one containing the simulator: '../../kronosim/'
# use : ' to start multiple lines comment. End it with '
# >>> IMPORTANT: DO NOT use variable PATH, or the command "batch ..." will give error: bash: command not found

FOLDER_PATH="./generated_simulations"
DIFF_FOLDER_PATH="/home/stornace/Desktop/_OMNETPP/FCFS_generated_simulations"
FILE="FCFS_simulation_analysis.json"


echo " ****** DIFF Simulation: _test_expressions ****** "
diff "$FOLDER_PATH/_test_expressions/$FILE" "$DIFF_FOLDER_PATH/_test_expressions/$FILE" 

echo " ****** DIFF Simulation: _test_post_effects ****** "
diff "$FOLDER_PATH/_test_post_effects/$FILE" "$DIFF_FOLDER_PATH/_test_post_effects/$FILE" 

echo " ****** DIFF Simulation: _test_queue ****** "
diff "$FOLDER_PATH/_test_queue/$FILE" "$DIFF_FOLDER_PATH/_test_queue/$FILE" 

echo " ****** DIFF Simulation: _test-batch ****** "
diff "$FOLDER_PATH/_test-batch/$FILE" "$DIFF_FOLDER_PATH/_test-batch/$FILE" 

echo " ****** DIFF Simulation: _Test-goal-operator ****** "
diff "$FOLDER_PATH/_Test-goal-operator/$FILE" "$DIFF_FOLDER_PATH/_Test-goal-operator/$FILE" 

echo " ****** DIFF Simulation: _test-multi-loop5 ****** "
diff "$FOLDER_PATH/_test-multi-loop5/$FILE" "$DIFF_FOLDER_PATH/_test-multi-loop5/$FILE" 

echo " ****** DIFF Simulation: _Test-multiple-goal ****** "
diff "$FOLDER_PATH/_Test-multiple-goal/$FILE" "$DIFF_FOLDER_PATH/_Test-multiple-goal/$FILE" 

echo " ****** DIFF Simulation: _test-multiple-plans ****** "
diff "$FOLDER_PATH/_test-multiple-plans/$FILE" "$DIFF_FOLDER_PATH/_test-multiple-plans/$FILE" 

echo " ****** DIFF Simulation: _test-phiI_only_period-INF.8 ****** "
diff "$FOLDER_PATH/_test-phiI_only_period-INF.8/$FILE" "$DIFF_FOLDER_PATH/_test-phiI_only_period-INF.8/$FILE" 

echo " ****** DIFF Simulation: _test-phiI_only_period-INF.8.1 ****** "
diff "$FOLDER_PATH/_test-phiI_only_period-INF.8.1/$FILE" "$DIFF_FOLDER_PATH/_test-phiI_only_period-INF.8.1/$FILE" 

echo " ****** DIFF Simulation: _test-phiI_only_period-INF.8.1_cornercaseEDF ****** "
diff "$FOLDER_PATH/_test-phiI_only_period-INF.8.1_cornercaseEDF/$FILE" "$DIFF_FOLDER_PATH/_test-phiI_only_period-INF.8.1_cornercaseEDF/$FILE" 

echo " ****** DIFF Simulation: _test-shared-goal ****** "
diff "$FOLDER_PATH/_test-shared-goal/$FILE" "$DIFF_FOLDER_PATH/_test-shared-goal/$FILE" 

echo " ****** DIFF Simulation: _test-shared-goal_DDL ****** "
diff "$FOLDER_PATH/_test-shared-goal_DDL/$FILE" "$DIFF_FOLDER_PATH/_test-shared-goal_DDL/$FILE" 

echo " ****** DIFF Simulation: _test-shared-goal_originale ****** "
diff "$FOLDER_PATH/_test-shared-goal_originale/$FILE" "$DIFF_FOLDER_PATH/_test-shared-goal_originale/$FILE" 

echo " ****** DIFF Simulation: _test-shared-goal-different-result ****** "
diff "$FOLDER_PATH/_test-shared-goal-different-result/$FILE" "$DIFF_FOLDER_PATH/_test-shared-goal-different-result/$FILE" 

echo " ****** DIFF Simulation: _test-shared-goal-different-result-operator ****** "
diff "$FOLDER_PATH/_test-shared-goal-different-result-operator/$FILE" "$DIFF_FOLDER_PATH/_test-shared-goal-different-result-operator/$FILE" 

echo " ****** DIFF Simulation: _test-shared-goal-different-result2 ****** "
diff "$FOLDER_PATH/_test-shared-goal-different-result2/$FILE" "$DIFF_FOLDER_PATH/_test-shared-goal-different-result2/$FILE" 

echo " ****** DIFF Simulation: _test-shared-goal-different-result2-operator ****** "
diff "$FOLDER_PATH/_test-shared-goal-different-result2-operator/$FILE" "$DIFF_FOLDER_PATH/_test-shared-goal-different-result2-operator/$FILE" 

echo " ****** DIFF Simulation: _test-shared-goal-different-result2-operator_DDL ****** "
diff "$FOLDER_PATH/_test-shared-goal-different-result2-operator_DDL/$FILE" "$DIFF_FOLDER_PATH/_test-shared-goal-different-result2-operator_DDL/$FILE" 

echo " ****** DIFF Simulation: _test-shared-goal-loop2 ****** "
diff "$FOLDER_PATH/_test-shared-goal-loop2/$FILE" "$DIFF_FOLDER_PATH/_test-shared-goal-loop2/$FILE" 

echo " ****** DIFF Simulation: _test-shared-goal-loop3 ****** "
diff "$FOLDER_PATH/_test-shared-goal-loop3/$FILE" "$DIFF_FOLDER_PATH/_test-shared-goal-loop3/$FILE" 

echo " ****** DIFF Simulation: _test-shared-goal-loop4 ****** "
diff "$FOLDER_PATH/_test-shared-goal-loop4/$FILE" "$DIFF_FOLDER_PATH/_test-shared-goal-loop4/$FILE" 

echo " ****** DIFF Simulation: _thesis_EDF ****** "
diff "$FOLDER_PATH/_thesis_EDF/$FILE" "$DIFF_FOLDER_PATH/_thesis_EDF/$FILE" 

echo " ****** DIFF Simulation: _thesis_EDF_DDL ****** "
diff "$FOLDER_PATH/_thesis_EDF_DDL/$FILE" "$DIFF_FOLDER_PATH/_thesis_EDF_DDL/$FILE" 

echo " ****** DIFF Simulation: corner-cases ****** "
diff "$FOLDER_PATH/corner-cases/$FILE" "$DIFF_FOLDER_PATH/corner-cases/$FILE" 

echo " ****** DIFF Simulation: test-CalcDDL-consecutive ****** "
diff "$FOLDER_PATH/test-CalcDDL-consecutive/$FILE" "$DIFF_FOLDER_PATH/test-CalcDDL-consecutive/$FILE" 

echo " ****** DIFF Simulation: test-isschedulable.4 ****** "
diff "$FOLDER_PATH/test-isschedulable.4/$FILE" "$DIFF_FOLDER_PATH/test-isschedulable.4/$FILE" 

echo " ****** DIFF Simulation: test-multiplans.1 ****** "
diff "$FOLDER_PATH/test-multiplans.1/$FILE" "$DIFF_FOLDER_PATH/test-multiplans.1/$FILE" 

echo " ****** DIFF Simulation: test-multiplans.1.2 ****** "
diff "$FOLDER_PATH/test-multiplans.1.2/$FILE" "$DIFF_FOLDER_PATH/test-multiplans.1.2/$FILE" 

echo " ****** DIFF Simulation: test-multiplans.1.3 ****** "
diff "$FOLDER_PATH/test-multiplans.1.3/$FILE" "$DIFF_FOLDER_PATH/test-multiplans.1.3/$FILE" 

echo " ****** DIFF Simulation: test-overlapping-plans-1 ****** "
diff "$FOLDER_PATH/test-overlapping-plans-1/$FILE" "$DIFF_FOLDER_PATH/test-overlapping-plans-1/$FILE" 

echo " ****** DIFF Simulation: test-overlapping-plans-2 ****** "
diff "$FOLDER_PATH/test-overlapping-plans-2/$FILE" "$DIFF_FOLDER_PATH/test-overlapping-plans-2/$FILE" 

echo " ****** DIFF Simulation: test-overlapping-plans-3 ****** "
diff "$FOLDER_PATH/test-overlapping-plans-3/$FILE" "$DIFF_FOLDER_PATH/test-overlapping-plans-3/$FILE" 

echo " ****** DIFF Simulation: test-overlapping-plans-4 ****** "
diff "$FOLDER_PATH/test-overlapping-plans-4/$FILE" "$DIFF_FOLDER_PATH/test-overlapping-plans-4/$FILE" 

echo " ****** DIFF Simulation: test-parallelo.1 ****** "
diff "$FOLDER_PATH/test-parallelo.1/$FILE" "$DIFF_FOLDER_PATH/test-parallelo.1/$FILE" 

echo " ****** DIFF Simulation: test-phiI_only_period1.3 ****** "
diff "$FOLDER_PATH/test-phiI_only_period1.3/$FILE" "$DIFF_FOLDER_PATH/test-phiI_only_period1.3/$FILE" 

echo " ****** DIFF Simulation: test-phiI_only_period1.3_multiplan ****** "
diff "$FOLDER_PATH/test-phiI_only_period1.3_multiplan/$FILE" "$DIFF_FOLDER_PATH/test-phiI_only_period1.3_multiplan/$FILE" 

echo " ****** DIFF Simulation: test-phiI_only_period1.3_sensors ****** "
diff "$FOLDER_PATH/test-phiI_only_period1.3_sensors/$FILE" "$DIFF_FOLDER_PATH/test-phiI_only_period1.3_sensors/$FILE" 

echo " ****** DIFF Simulation: test-phiI_only_period1.3.1 ****** "
diff "$FOLDER_PATH/test-phiI_only_period1.3.1/$FILE" "$DIFF_FOLDER_PATH/test-phiI_only_period1.3.1/$FILE" 

echo " ****** DIFF Simulation: test-trigger-goals.2.1 ****** "
diff "$FOLDER_PATH/test-trigger-goals.2.1/$FILE" "$DIFF_FOLDER_PATH/test-trigger-goals.2.1/$FILE" 