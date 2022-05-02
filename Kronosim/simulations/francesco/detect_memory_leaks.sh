#!/bin/bash
# Run this file using the command: 'bash random_events_generator.sh'
# Move from this folder, to the one containing the simulator: '../../kronosim/'
# use : ' to start multiple lines comment. End it with '
# >>> IMPORTANT: DO NOT use variable PATH, or the command "batch ..." will give error: bash: command not found

SCHED_TYPE=2
LOGGER_LEVEL=5 # Print Errors only 
FOLDER_PATH="/generated_simulations/test-multiplans.1.3"
SIM_NAME="/test-multiplans.1.3_seed_196"
FULL_PATH=FOLDER_PATH + SIM_NAME

cd ../../kronosim/

 valgrind --tool=memcheck --leak-check=yes ./kronosim -u Cmdenv -c General omnetpp.ini --ag_network.ag[*].path='"'$FULL_PATH'"' --ag_network.ag[*].sched_type=$SCHED_TYPE --ag_network.ag[*].logger_level=$LOGGER_LEVEL


 :' 
 valgrind --leak-check=full --show-leak-kinds=all ./kronosim -u Cmdenv -c General omnetpp.ini --ag_network.ag[*].path='"/unity_simulations/__test_EDF_miss"' --ag_network.ag[*].sched_type=2 --ag_network.ag[*].logger_level=1
 '