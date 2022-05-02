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
echo " ****** START Simulation: test-overlapping-plans-3 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-overlapping-plans-3"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: test-overlapping-plans-3 ****** "
echo " ****** START Simulation: test-overlapping-plans-4 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-overlapping-plans-4"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: test-overlapping-plans-4 ****** "
echo " ****** START Simulation: test-parallelo.1 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-parallelo.1"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: test-parallelo.1 ****** "
echo " ****** START Simulation: test-phiI_only_period1.3 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-phiI_only_period1.3"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: test-phiI_only_period1.3 ****** "
echo " ****** START Simulation: test-phiI_only_period1.3.1 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-phiI_only_period1.3.1"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: test-phiI_only_period1.3.1 ****** "
echo " ****** START Simulation: test-phiI_only_period1.3_multiplan ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-phiI_only_period1.3_multiplan"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: test-phiI_only_period1.3_multiplan ****** "
echo " ****** START Simulation: test-phiI_only_period1.3_sensors ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-phiI_only_period1.3_sensors"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: test-phiI_only_period1.3_sensors ****** "
echo " ****** START Simulation:_test_post_effects ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test_post_effects"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: _test_post_effects ****** "
echo " ****** START Simulation: _test-phiI_only_period-INF.8 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-phiI_only_period-INF.8"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: _test-phiI_only_period-INF.8 ****** "
echo " ****** START Simulation: _test-phiI_only_period-INF.8.1 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-phiI_only_period-INF.8.1"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: _test-phiI_only_period-INF.8.1 ****** "
echo " ****** START Simulation: _test-phiI_only_period-INF.8.1_cornercaseEDF ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-phiI_only_period-INF.8.1_cornercaseEDF"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: _test-phiI_only_period-INF.8.1_cornercaseEDF ****** "
echo " ****** START Simulation:_test-shared-goal ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-shared-goal"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: _test-shared-goal ****** "
echo " ****** START Simulation:_test-shared-goal_DDL ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-shared-goal_DDL"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: _test-shared-goal_DDL ****** "


# ------------------------------------------------------------------------ #
# ...Random_events generated. Execute them...
# ------------------------------------------------------------------------ #

cd ../../simulator/simulations/francesco/generated_simulations/ &&
cd test-overlapping-plans-3 && bash fast_execution_cmd.sh && cd .. &&
cd test-overlapping-plans-4 && bash fast_execution_cmd.sh && cd .. &&
cd test-parallelo.1 && bash fast_execution_cmd.sh && cd .. &&
cd test-phiI_only_period1.3 && bash fast_execution_cmd.sh && cd .. &&
cd test-phiI_only_period1.3.1 && bash fast_execution_cmd.sh && cd .. &&
cd test-phiI_only_period1.3_multiplan && bash fast_execution_cmd.sh && cd .. &&
cd test-phiI_only_period1.3_sensors && bash fast_execution_cmd.sh && cd .. &&
cd _test_post_effects && bash fast_execution_cmd.sh && cd .. &&
cd _test-phiI_only_period-INF.8 && bash fast_execution_cmd.sh && cd .. &&
cd _test-phiI_only_period-INF.8.1 && bash fast_execution_cmd.sh && cd .. &&
cd _test-phiI_only_period-INF.8.1_cornercaseEDF && bash fast_execution_cmd.sh && cd .. &&
cd _test-shared-goal && bash fast_execution_cmd.sh && cd .. &&
cd _test-shared-goal_DDL && bash fast_execution_cmd.sh && cd ..



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