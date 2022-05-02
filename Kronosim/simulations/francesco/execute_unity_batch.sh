#!/bin/bash
# Run this file using the command: 'bash random_events_generator.sh'
# Move from this folder, to the one containing the simulator: '../../kronosim/'
# use : ' to start multiple lines comment. End it with '
# >>> IMPORTANT: DO NOT use variable PATH, or the command "batch ..." will give error: bash: command not found
start=$(date +%s.%N)

SCHED_TYPE=0
CONVERT_FILE=false
FOLDER_PATH="unity_simulations/"
DESTINATION_FOLDER_PATH="/generated_unity_simulations/"

cd ../../kronosim/
echo " ****** START Simulation: test_1 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test_1"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE
echo " ****** ENDED Simulation: test_1 ****** "
echo " ****** START Simulation: test_conditions ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test_conditions"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE
echo " ****** ENDED Simulation: test_conditions ****** "
echo " ****** START Simulation: test_intentions_with_same_internal_goals ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test_intentions_with_same_internal_goals"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE
echo " ****** ENDED Simulation: test_intentions_with_same_internal_goals ****** "


# ------------------------------------------------------------------------ #
# ...Random_events generated. Execute them...
# ------------------------------------------------------------------------ #

cd ../../simulator/simulations/francesco/generated_unity_simulations/ &&
cd test_1 && bash fast_execution_cmd.sh && cd .. &&
cd test_conditions && bash fast_execution_cmd.sh && cd .. &&
cd test_intentions_with_same_internal_goals && bash fast_execution_cmd.sh && cd .. 

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