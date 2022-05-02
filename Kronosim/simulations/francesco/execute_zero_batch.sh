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
echo " ****** START Simulation: corner-cases ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"corner-cases"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: corner-cases ****** "
echo " ****** START Simulation: _test-batch ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-batch"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: _test-batch ****** "
echo " ****** START Simulation: test-CalcDDL-consecutive ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-CalcDDL-consecutive"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: test-CalcDDL-consecutive ****** "
echo " ****** START Simulation: _Test-goal-operator ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_Test-goal-operator"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: _Test-goal-operator ****** "
echo " ****** START Simulation: test-isschedulable.4 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-isschedulable.4"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: test-isschedulable.4 ****** "
echo " ****** START Simulation: _test-multi-loop5 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-multi-loop5"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: _test-multi-loop5 ****** "
echo " ****** START Simulation: test-multiplans.1 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-multiplans.1"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: test-multiplans.1 ****** "
echo " ****** START Simulation: test-multiplans.1.2 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-multiplans.1.2"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: test-multiplans.1.2 ****** "
echo " ****** START Simulation: test-multiplans.1.3 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-multiplans.1.3"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: test-multiplans.1.3 ****** "
echo " ****** START Simulation: _Test-multiple-goal ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_Test-multiple-goal"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: _Test-multiple-goal ****** "
echo " ****** START Simulation: _test-multiple-plans ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_test-multiple-plans"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: _test-multiple-plans ****** "
echo " ****** START Simulation: test-overlapping-plans-1 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-overlapping-plans-1"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: test-overlapping-plans-1 ****** "
echo " ****** START Simulation: test-overlapping-plans-2 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test-overlapping-plans-2"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: test-overlapping-plans-2 ****** "


# ------------------------------------------------------------------------ #
# ...Random_events generated. Execute them...
# ------------------------------------------------------------------------ #

cd ../../simulator/simulations/francesco/generated_simulations/ &&
cd corner-cases && bash fast_execution_cmd.sh && cd .. &&
cd _test-batch && bash fast_execution_cmd.sh && cd .. &&
cd test-CalcDDL-consecutive && bash fast_execution_cmd.sh && cd .. &&
cd _Test-goal-operator && bash fast_execution_cmd.sh && cd .. &&
cd test-isschedulable.4 && bash fast_execution_cmd.sh && cd .. &&
cd _test-multi-loop5 && bash fast_execution_cmd.sh && cd .. &&
cd test-multiplans.1 && bash fast_execution_cmd.sh && cd .. &&
cd test-multiplans.1.2 && bash fast_execution_cmd.sh && cd .. &&
cd test-multiplans.1.3 && bash fast_execution_cmd.sh && cd .. &&
cd _Test-multiple-goal && bash fast_execution_cmd.sh && cd .. &&
cd _test-multiple-plans && bash fast_execution_cmd.sh && cd .. &&
cd test-overlapping-plans-1 && bash fast_execution_cmd.sh && cd .. &&
cd test-overlapping-plans-2 && bash fast_execution_cmd.sh && cd ..

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