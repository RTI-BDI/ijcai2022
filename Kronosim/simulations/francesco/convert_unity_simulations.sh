#!/bin/bash
# Run this file using the command: 'bash random_events_generator.sh'
# Move from this folder, to the one containing the simulator: '../../kronosim/'
# use : ' to start multiple lines comment. End it with '
# >>> IMPORTANT: DO NOT use variable PATH, or the command "batch ..." will give error: bash: command not found

start=$(date +%s.%N)

FOLDER_PATH="unity_simulations/"
DESTINATION_FOLDER_PATH="/converted_unity_simulations/"
CONVERT_FILE=true

cd ../../kronosim/

#  TESTS BATCH 0 --------------------------
echo " ****** START Simulation: _agent_stadium_manager ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"_agent_stadium_manager"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: _agent_stadium_manager ****** "
echo " ****** START Simulation: test_1 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test_1"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: test_1 ****** "
echo " ****** START Simulation: test_conditions ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test_conditions"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: test_conditions ****** "
echo " ****** START Simulation: test_intentions_with_same_internal_goals ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test_intentions_with_same_internal_goals"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: test_intentions_with_same_internal_goals ****** "
echo " ****** START Simulation: test_internal_goal_arrival_time ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test_internal_goal_arrival_time"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: test_internal_goal_arrival_time ****** "
echo " ****** START Simulation: test_movement_0 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test_movement_0"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: test_movement_0 ****** "
echo " ****** START Simulation: test_movement_1 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test_movement_1"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: test_movement_1 ****** "
echo " ****** START Simulation: test_movement_1_FCFS ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test_movement_1_FCFS"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: test_movement_1_FCFS ****** "
echo " ****** START Simulation: test_movement_ddl ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test_movement_ddl"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: test_movement_ddl ****** "
echo " ****** START Simulation: test_PDDL ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test_PDDL"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: test_PDDL ****** "
echo " ****** START Simulation: test_PDDL_arrivaltime ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test_PDDL_arrivaltime"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: test_PDDL_arrivaltime ****** "
echo " ****** START Simulation: test_PDDL_post_conditions ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test_PDDL_post_conditions"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: test_PDDL_post_conditions ****** "
echo " ****** START Simulation: test_PDDL_U=1 ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"test_PDDL_U=1"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE
echo " ****** ENDED Simulation: test_PDDL_U=1 ****** "
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