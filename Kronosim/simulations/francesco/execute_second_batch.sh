#!/bin/bash
# Run this file using the command: 'bash random_events_generator.sh'
# Move from this folder, to the one containing the simulator: '../../kronosim/'
# use : ' to start multiple lines comment. End it with '
# >>> IMPORTANT: DO NOT use variable PATH, or the command "batch ..." will give error: bash: command not found
start=$(date +%s.%N)

SCHED_TYPE=0
CONVERT_FILE=false
LOGGER_LEVEL=1 # Print Errors only 
FOLDER_PATH="simulations_scenarios/"
DESTINATION_FOLDER_PATH="/generated_simulations/"

cd ../../kronosim/
echo " ****** START Simulation: _test-shared-goal-different-result ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-shared-goal-different-result"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: _test-shared-goal-different-result ****** "
echo " ****** START Simulation: _test-shared-goal-different-result2 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-shared-goal-different-result2"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: _test-shared-goal-different-result2 ****** "
echo " ****** START Simulation: _test-shared-goal-different-result2-operator ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-shared-goal-different-result2-operator"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: _test-shared-goal-different-result2-operator ****** "
echo " ****** START Simulation: _test-shared-goal-different-result2-operator_DDL ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-shared-goal-different-result2-operator_DDL"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: _test-shared-goal-different-result2-operator_DDL ****** "
echo " ****** START Simulation: _test-shared-goal-different-result-operator ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-shared-goal-different-result-operator"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: _test-shared-goal-different-result-operator ****** "
echo " ****** START Simulation: _test-shared-goal-loop2 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-shared-goal-loop2"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: _test-shared-goal-loop2 ****** "
echo " ****** START Simulation: _test-shared-goal-loop3 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-shared-goal-loop3"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: _test-shared-goal-loop3 ****** "
echo " ****** START Simulation: _test-shared-goal-loop4 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-shared-goal-loop4"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: _test-shared-goal-loop4 ****** "
echo " ****** START Simulation: _test-shared-goal_originale ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-shared-goal_originale"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: _test-shared-goal_originale ****** "
echo " ****** START Simulation: test-trigger-goals.2.1 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-trigger-goals.2.1"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: test-trigger-goals.2.1 ****** "
echo " ****** START Simulation: _thesis_EDF ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_thesis_EDF"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: _thesis_EDF ****** "
echo " ****** START Simulation: _thesis_EDF_DDL ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_thesis_EDF_DDL"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: _thesis_EDF_DDL ****** "
echo " ****** START Simulation: _test_expressions ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test_expressions"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: _test_expressions ****** "
echo " ****** START Simulation: _test_queue ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test_queue"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: _test_queue ****** "

# ------------------------------------------------------------------------ #
# ...Random_events generated. Execute them...
# ------------------------------------------------------------------------ #

cd ../../simulator/simulations/francesco/generated_simulations/ &&
cd _test-shared-goal-different-result && bash fast_execution_cmd.sh && cd .. &&
cd _test-shared-goal-different-result2 && bash fast_execution_cmd.sh && cd .. &&
cd _test-shared-goal-different-result2-operator && bash fast_execution_cmd.sh && cd .. &&
cd _test-shared-goal-different-result2-operator_DDL && bash fast_execution_cmd.sh && cd .. &&
cd _test-shared-goal-different-result-operator && bash fast_execution_cmd.sh && cd .. &&
cd _test-shared-goal-loop2 && bash fast_execution_cmd.sh && cd .. &&
cd _test-shared-goal-loop3 && bash fast_execution_cmd.sh && cd .. &&
cd _test-shared-goal-loop4 && bash fast_execution_cmd.sh && cd .. &&
cd _test-shared-goal_originale && bash fast_execution_cmd.sh && cd .. &&
cd test-trigger-goals.2.1 && bash fast_execution_cmd.sh && cd ..  &&
cd _thesis_EDF && bash fast_execution_cmd.sh && cd .. &&
cd _thesis_EDF_DDL && bash fast_execution_cmd.sh && cd .. &&
cd _test_expressions && bash fast_execution_cmd.sh && cd .. &&
cd _test_queue && bash fast_execution_cmd.sh && cd .. 


# Compute TIME needed to execute the entire file: ------------------------ #
end=$(date +%s.%N)
interval=$(echo "$end - $start" | bc)

dt=$(echo "$end - $start" | bc)
dd=$(echo "$dt/86400" | bc)
dt2=$(echo "$dt-86400*$dd" | bc)
dh=$(echo "$dt2/3600" | bc)
dt3=$(echo "$dt2-3600*$dh" | bc)
dm=$(echo "$dt3/60" | bc)
ds=$(echo "$dt3-60*$dm" | bc)
LC_NUMERIC=C printf "Total runtime: %d:%02d:%02d:%02.4f\n" $dd $dh $dm $ds
# ------------------------------------------------------------------------ #