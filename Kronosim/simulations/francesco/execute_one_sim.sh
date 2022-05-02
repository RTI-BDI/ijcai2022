#!/bin/bash
# Run this file using the command: 'bash random_events_generator.sh'
# Move from this folder, to the one containing the simulator: '../../kronosim/'
# use : ' to start multiple lines comment. End it with '
# >>> IMPORTANT: DO NOT use variable PATH, or the command "batch ..." will give error: bash: command not found

SCHED_TYPE=0
CONVERT_FILE=false
LOGGER_LEVEL=1 # Print Errors only 
FOLDER_PATH="simulations_scenarios/"
DESTINATION_FOLDER_PATH="/generated_simulations/"
SIM_NAME="_Test-multiple-goal"  # test-overlapping-plans-1"

cd ../../kronosim/ 
echo " ****** START Simulation: ****** "
./kronosim -u Cmdenv -c random_events_generator omnetpp.ini --random_events_generator_network.simulation.folder_path='"'$FOLDER_PATH'"'  --random_events_generator_network.simulation.destination_folder='"'$DESTINATION_FOLDER_PATH'"'  --random_events_generator_network.simulation.simulation_name='"'$SIM_NAME'"' --random_events_generator_network.simulation.convertFile=$CONVERT_FILE --random_events_generator_network.simulation.sched_type=$SCHED_TYPE --random_events_generator_network.simulation.logger_level=$LOGGER_LEVEL
echo " ****** ENDED Simulation: ****** "

cd ../../simulator/simulations/francesco/generated_simulations/ &&
cd _Test-multiple-goal && bash fast_execution_cmd.sh
