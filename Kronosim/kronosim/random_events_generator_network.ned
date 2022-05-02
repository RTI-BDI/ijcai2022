//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//
simple EventsGenerator
{	
	@display("i=block/user"); 
	// Configurable Parameters ------------------------------------------------
	int sched_type = default(1); 	//scheduler type to use (0 = FCFS, 1 = RR, 2 = EDF)
	int logger_level = default(5); 	// 0 = PrintNothing, Error, Debug, Default, Warning, 5 = EssentialInfo, EveryInfo, EveryInfoPlus 
	string user = default("francesco");
	
	string fast_outputFileName = default("fast_execution_cmd.sh"); // name of the file that will be executed to simulate all the created simulations
	
	// Parameters used to define the command that will execute the single simulation seed_n
	string cmd_command = default("./kronosim -u Cmdenv -c General omnetpp.ini --ag_network.ag[*].path="); 
	string cmd_sched_type = default(" --ag_network.ag[*].sched_type=");
	string cmd_log_level = default(" --ag_network.ag[*].logger_level=1"); // logger_level = 1 in order to PRINT ERRORS
	
	// Parameters used to define the command that will analyze the simulation results
	string cmd_analyze_simulation = default("./kronosim -u Cmdenv -c analyze_simulations omnetpp.ini --analyze_simulations_network.simulation.simulation_name=");
	string cmd_analyze_simulation_log_level = default(" --analyze_simulations_network.simulation.logger_level=1");
	string cmd_analyze_simulation_sched_type = default(" --analyze_simulations_network.simulation.sched_type=");
	string cmd_analyze_simulation_folder_path = default(" --analyze_simulations_network.simulation.folder_path=");
	
	bool convertFile = default(false); // when changed, check "writeinputs.cc, function: write_planset_json, if(goal != nullptr) { Note }
	
	string folder_path = default("simulations_scenarios/"); 		// folder containing the original simulation 'simulation_name'
	string destination_folder = default("/generated_simulations/"); // destination folder for the #num_of_simulations generated new versions
	string simulation_name = default("_Test-multiple-goal"); 			// original simulation (created offline by the designer)
    
    //  If seed is set to 1, the generator is reinitialized to its initial value and produces the same values as before any call to rand or srand.
    // seed: https://www.cplusplus.com/reference/cstdlib/srand/
    // Note: for some reasoning, 0 and 1 generate the same identical results. Start from 1 or greater.
    int initial_seed = default(1); 
    int num_of_simulations = default(200);	 // Number of different simulation generated
	int num_of_sensors = default(5);		 // Number of sensors for each simulation
	
	int change_bool_items_prob = default(10);	 // Used to decide whether or not the bool value will be inverted. Process: extract a number between 0 and change_bool_items_prob (not included). If number is less than 3 (arbitrarily choose), then change the value
	int max_simulation_time = default(50);		 // time [0, maximum) is the interval of time where sensors can be activated
	int min_numeric_value = default(0); 		 // min value that can be assigned to numeric parameters
	int max_numeric_value = default(101);		 // max (not included) value that can be assigned to numeric parameters
	int delta_deadline_percentage = default(50); // used with desires. Defines the percentage of the deadline original value that can be used to update the deadline of the same desire in generated simulations
	int min_server_budget = default(1); 	// min accepted budget for a server
	int max_server_budget = default(10); 	// max accepted budget for a server
	int max_server_period = default(10); 	// period is computed as: rand_budget * (rand_value between 1 and max_server_period)
	
	// false: just copy the ones defined by the desginer. true: update budget and period randomly, using max_server_budget and max_server_period
	bool computeServersDinamically = default(false); 
}

network random_events_generator_network
{       
    parameters:        
        @display("bgb=331,135");
    types:
        channel Channel extends ned.DelayChannel { }
    submodules:
        simulation: EventsGenerator;
}