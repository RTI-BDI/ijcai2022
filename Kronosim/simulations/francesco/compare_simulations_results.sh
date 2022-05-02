#!/bin/bash
# Run this file using the command: 'bash random_events_generator.sh'
# Move from this folder, to the one containing the simulator: '../../kronosim/'
# use : ' to start multiple lines comment. End it with '
# >>> IMPORTANT: DO NOT use variable PATH, or the command "batch ..." will give error: bash: command not found

FOLDER_PATH="generated_simulations/"
LOGGER_LEVEL=2
SINGLE_SIMULATION=false

cd ../../kronosim/

#  TESTS generated_simulations/ --------------------------
echo " ****** START Simulation: corner-cases ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"corner-cases"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: corner-cases ****** "
echo " ****** START Simulation: _test-batch ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"_test-batch"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: _test-batch ****** "
echo " ****** START Simulation: test-CalcDDL-consecutive ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"test-CalcDDL-consecutive"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: test-CalcDDL-consecutive ****** "
echo " ****** START Simulation: _Test-goal-operator ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"_Test-goal-operator"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: _Test-goal-operator ****** "
echo " ****** START Simulation: test-isschedulable.4 ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"test-isschedulable.4"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: test-isschedulable.4 ****** "
echo " ****** START Simulation: _test-multi-loop5 ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"_test-multi-loop5"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: _test-multi-loop5 ****** "
echo " ****** START Simulation: test-multiplans.1 ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"test-multiplans.1"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: test-multiplans.1 ****** "
echo " ****** START Simulation: test-multiplans.1.2 ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"test-multiplans.1.2"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: test-multiplans.1.2 ****** "
echo " ****** START Simulation: test-multiplans.1.3 ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"test-multiplans.1.3"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: test-multiplans.1.3 ****** "
echo " ****** START Simulation: _Test-multiple-goal ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"_Test-multiple-goal"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: _Test-multiple-goal ****** "
echo " ****** START Simulation: _test-multiple-plans ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"_test-multiple-plans"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: _test-multiple-plans ****** "
echo " ****** START Simulation: test-overlapping-plans-1 ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"test-overlapping-plans-1"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: test-overlapping-plans-1 ****** "
echo " ****** START Simulation: test-overlapping-plans-2 ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"test-overlapping-plans-2"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: test-overlapping-plans-2 ****** "

#  TESTS BATCH 1 --------------------------
echo " ****** START Simulation: test-overlapping-plans-3 ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"test-overlapping-plans-3"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: test-overlapping-plans-3 ****** "
echo " ****** START Simulation: test-overlapping-plans-4 ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"test-overlapping-plans-4"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: test-overlapping-plans-4 ****** "
echo " ****** START Simulation: test-parallelo.1 ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"test-parallelo.1"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: test-parallelo.1 ****** "
echo " ****** START Simulation: test-phiI_only_period1.3 ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"test-phiI_only_period1.3"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: test-phiI_only_period1.3 ****** "
echo " ****** START Simulation: test-phiI_only_period1.3.1 ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"test-phiI_only_period1.3.1"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: test-phiI_only_period1.3.1 ****** "
echo " ****** START Simulation: test-phiI_only_period1.3_multiplan ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"test-phiI_only_period1.3_multiplan"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: test-phiI_only_period1.3_multiplan ****** "
echo " ****** START Simulation: test-phiI_only_period1.3_sensors ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"test-phiI_only_period1.3_sensors"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: test-phiI_only_period1.3_sensors ****** "
echo " ****** START Simulation: _test_post_effects ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"_test_post_effects"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: _test_post_effects ****** "
echo " ****** START Simulation: _test-phiI_only_period-INF.8 ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"_test-phiI_only_period-INF.8"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: _test-phiI_only_period-INF.8 ****** "
echo " ****** START Simulation: _test-phiI_only_period-INF.8.1 ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"_test-phiI_only_period-INF.8.1"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: _test-phiI_only_period-INF.8.1 ****** "
echo " ****** START Simulation: _test-phiI_only_period-INF.8.1_cornercaseEDF ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"_test-phiI_only_period-INF.8.1_cornercaseEDF"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: _test-phiI_only_period-INF.8.1_cornercaseEDF ****** "
echo " ****** START Simulation:_test-shared-goal ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"_test-shared-goal"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: _test-shared-goal ****** "
echo " ****** START Simulation:_test-shared-goal_DDL ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"_test-shared-goal_DDL"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: _test-shared-goal_DDL ****** "


#  TESTS BATCH 2 --------------------------
echo " ****** START Simulation: _test-shared-goal-different-result ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"_test-shared-goal-different-result"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: _test-shared-goal-different-result ****** "
echo " ****** START Simulation: _test-shared-goal-different-result2 ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"_test-shared-goal-different-result2"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: _test-shared-goal-different-result2 ****** "
echo " ****** START Simulation: _test-shared-goal-different-result2-operator ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"_test-shared-goal-different-result2-operator"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: _test-shared-goal-different-result2-operator ****** "
echo " ****** START Simulation: _test-shared-goal-different-result2-operator_DDL ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"_test-shared-goal-different-result2-operator_DDL"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: _test-shared-goal-different-result2-operator_DDL ****** "
echo " ****** START Simulation: _test-shared-goal-different-result-operator ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"_test-shared-goal-different-result-operator"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: _test-shared-goal-different-result-operator ****** "
echo " ****** START Simulation: _test-shared-goal-loop2 ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"_test-shared-goal-loop2"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: _test-shared-goal-loop2 ****** "
echo " ****** START Simulation: _test-shared-goal-loop3 ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"_test-shared-goal-loop3"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: _test-shared-goal-loop3 ****** "
echo " ****** START Simulation: _test-shared-goal-loop4 ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"_test-shared-goal-loop4"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: _test-shared-goal-loop4 ****** "
echo " ****** START Simulation: _test-shared-goal_originale ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"_test-shared-goal_originale"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: _test-shared-goal_originale ****** "
echo " ****** START Simulation: test-trigger-goals.2.1 ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"test-trigger-goals.2.1"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: test-trigger-goals.2.1 ****** "
echo " ****** START Simulation: _thesis_EDF ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"_thesis_EDF"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: _thesis_EDF ****** "
echo " ****** START Simulation: _thesis_EDF_DDL ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"_thesis_EDF_DDL"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: _thesis_EDF_DDL ****** "
echo " ****** START Simulation: _test_expressions ****** "
./kronosim -u Cmdenv -c compare_simulations_results omnetpp.ini --compare_simulations_results_network.simulation.folder_path='"'$FOLDER_PATH'"' --compare_simulations_results_network.simulation.simulation_name='"_test_expressions"' --compare_simulations_results_network.simulation.logger_level=$LOGGER_LEVEL --compare_simulations_results_network.simulation.single_simulation=$SINGLE_SIMULATION
echo " ****** ENDED Simulation: _test_expressions ****** "