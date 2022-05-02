#!/bin/bash
# Run this file using the command: 'bash random_events_generator.sh'
# Move from this folder, to the one containing the simulator: '../../kronosim/'
# use : ' to start multiple lines comment. End it with '
# >>> IMPORTANT: DO NOT use variable PATH, or the command "batch ..." will give error: bash: command not found

SCHED_TYPE=2
CONVERT_FILE=false
FOLDER_PATH="unity_simulations/"
DESTINATION_FOLDER_PATH="/generated_unity_simulations/"
SIM_NAME="__test_client_server"

cd ../../kronosim/ 
echo " ****** START Simulation: ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"' --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"' --random_events_generator_network.simulation.simulation_name='"'$SIM_NAME'"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE
echo " ****** ENDED Simulation: ****** "

cd ../../simulator/simulations/francesco/generated_unity_simulations/ &&
cd __test_client_server && bash fast_execution_cmd.sh
