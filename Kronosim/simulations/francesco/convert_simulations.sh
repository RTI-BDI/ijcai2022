#!/bin/bash
# Run this file using the command: 'bash random_events_generator.sh'
# Move from this folder, to the one containing the simulator: '../../kronosim/'
# use : ' to start multiple lines comment. End it with '
# >>> IMPORTANT: DO NOT use variable PATH, or the command "batch ..." will give error: bash: command not found

start=$(date +%s.%N)

FOLDER_PATH="simulations_scenarios/"
DESTINATION_FOLDER_PATH="/converted_simulations_scenarios/"
CONVERT_FILE=true

cd ../../kronosim/

#  TESTS BATCH 0 --------------------------
echo " ****** START Simulation: corner-cases ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"corner-cases"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: corner-cases ****** "
echo " ****** START Simulation: _test-batch ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-batch"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: _test-batch ****** "
echo " ****** START Simulation: test-CalcDDL-consecutive ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-CalcDDL-consecutive"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: test-CalcDDL-consecutive ****** "
echo " ****** START Simulation: _Test-goal-operator ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_Test-goal-operator"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: _Test-goal-operator ****** "
echo " ****** START Simulation: test-isschedulable.4 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-isschedulable.4"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: test-isschedulable.4 ****** "
echo " ****** START Simulation: _test-multi-loop5 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-multi-loop5"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: _test-multi-loop5 ****** "
echo " ****** START Simulation: test-multiplans.1 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-multiplans.1"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: test-multiplans.1 ****** "
echo " ****** START Simulation: test-multiplans.1.2 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-multiplans.1.2"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: test-multiplans.1.2 ****** "
echo " ****** START Simulation: test-multiplans.1.3 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-multiplans.1.3"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: test-multiplans.1.3 ****** "
echo " ****** START Simulation: _test_post_effects ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test_post_effects"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: _test_post_effects ****** "
echo " ****** START Simulation: _Test-multiple-goal ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_Test-multiple-goal"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: _Test-multiple-goal ****** "
echo " ****** START Simulation: _test-multiple-plans ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-multiple-plans"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: _test-multiple-plans ****** "
echo " ****** START Simulation: test-overlapping-plans-1 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-overlapping-plans-1"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: test-overlapping-plans-1 ****** "
echo " ****** START Simulation: test-overlapping-plans-2 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-overlapping-plans-2"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: test-overlapping-plans-2 ****** "

#  TESTS BATCH 1 --------------------------
echo " ****** START Simulation: test-overlapping-plans-3 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-overlapping-plans-3"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: test-overlapping-plans-3 ****** "
echo " ****** START Simulation: test-overlapping-plans-4 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-overlapping-plans-4"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: test-overlapping-plans-4 ****** "
echo " ****** START Simulation: test-parallelo.1 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-parallelo.1"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: test-parallelo.1 ****** "
echo " ****** START Simulation: test-phiI_only_period1.3 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-phiI_only_period1.3"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: test-phiI_only_period1.3 ****** "
echo " ****** START Simulation: test-phiI_only_period1.3.1 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-phiI_only_period1.3.1"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: test-phiI_only_period1.3.1 ****** "
echo " ****** START Simulation: test-phiI_only_period1.3_multiplan ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-phiI_only_period1.3_multiplan"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: test-phiI_only_period1.3_multiplan ****** "
echo " ****** START Simulation: test-phiI_only_period1.3_sensors ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-phiI_only_period1.3_sensors"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: test-phiI_only_period1.3_sensors ****** "
echo " ****** START Simulation: _test-phiI_only_period-INF.8 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-phiI_only_period-INF.8"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: _test-phiI_only_period-INF.8 ****** "
echo " ****** START Simulation: _test-phiI_only_period-INF.8.1 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-phiI_only_period-INF.8.1"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: _test-phiI_only_period-INF.8.1 ****** "
echo " ****** START Simulation: _test-phiI_only_period-INF.8.1_cornercaseEDF ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-phiI_only_period-INF.8.1_cornercaseEDF"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: _test-phiI_only_period-INF.8.1_cornercaseEDF ****** "
echo " ****** START Simulation:_test-shared-goal ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-shared-goal"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: _test-shared-goal ****** "
echo " ****** START Simulation:_test-shared-goal_DDL ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-shared-goal_DDL"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: _test-shared-goal_DDL ****** "


#  TESTS BATCH 2 --------------------------
echo " ****** START Simulation: _test-shared-goal-different-result ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-shared-goal-different-result"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: _test-shared-goal-different-result ****** "
echo " ****** START Simulation: _test-shared-goal-different-result2 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-shared-goal-different-result2"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: _test-shared-goal-different-result2 ****** "
echo " ****** START Simulation: _test-shared-goal-different-result2-operator ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-shared-goal-different-result2-operator"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: _test-shared-goal-different-result2-operator ****** "
echo " ****** START Simulation: _test-shared-goal-different-result2-operator_DDL ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-shared-goal-different-result2-operator_DDL"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: _test-shared-goal-different-result2-operator_DDL ****** "
echo " ****** START Simulation: _test-shared-goal-different-result-operator ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-shared-goal-different-result-operator"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: _test-shared-goal-different-result-operator ****** "
echo " ****** START Simulation: _test-shared-goal-loop2 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-shared-goal-loop2"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: _test-shared-goal-loop2 ****** "
echo " ****** START Simulation: _test-shared-goal-loop3 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-shared-goal-loop3"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: _test-shared-goal-loop3 ****** "
echo " ****** START Simulation: _test-shared-goal-loop4 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-shared-goal-loop4"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: _test-shared-goal-loop4 ****** "
echo " ****** START Simulation: _test-shared-goal_originale ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-shared-goal_originale"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: _test-shared-goal_originale ****** "
echo " ****** START Simulation: test-trigger-goals.2.1 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-trigger-goals.2.1"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: test-trigger-goals.2.1 ****** "
echo " ****** START Simulation: _thesis_EDF ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_thesis_EDF"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: _thesis_EDF ****** "
echo " ****** START Simulation: _thesis_EDF_DDL ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_thesis_EDF_DDL"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: _thesis_EDF_DDL ****** "
echo " ****** START Simulation: _test_expressions ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test_expressions"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: _test_expressions ****** "

# Compute TIME needed to execute the entire file: ------------------------ #
end=$(date +%s.%N)

dt=$(echo "$end - $start" | bc)
dd=$(echo "$dt/86400" | bc)
dt2=$(echo "$dt-86400*$dd" | bc)
dh=$(echo "$dt2/3600" | bc)
dt3=$(echo "$dt2-3600*$dh" | bc)
dm=$(echo "$dt3/60" | bc)
ds=$(echo "$dt3-60*$dm" | bc)

# printf "Start time: $start, End time: $end\n"
LC_NUMERIC=C printf "Total runtime: %d:%02d:%02d:%02.4f\n" $dd $dh $dm $ds
# ------------------------------------------------------------------------ #