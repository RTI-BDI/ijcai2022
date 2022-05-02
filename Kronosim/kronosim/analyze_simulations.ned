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
simple AnalyzeSimulations
{	
	@display("i=block/user"); 
	// Configurable Parameters ------------------------------------------------
	int logger_level = default(5); // 0 = PrintNothing, Error, Debug, Default, Warning, 5 = EssentialInfo, EveryInfo, EveryInfoPlus
	string user = default("francesco");

	string folder_path = default("generated_simulations");
	string simulation_name = default("test-multiplans.1.3"); // simulation that will be analyzed
		
	string inputFileName = default("metrics.json");	// for each simulation_seed_n, use the data contained in "simulation_seed_n/reports/inputFileName"
	string outputFileName = default("simulation_analysis.json"); // used as: "{scheduling_algorithm}_simulation_analysis.json -> i.e. EDF_sim... or FCFS_sim... or RR_sim...
    string sched_type = default("EDF"); // EDF, RR, FCFS
    
    int initial_seed = default(1); // must be the same value defined in "random_events_generator_network.ned"
    int num_of_simulations = default(200); // must be the same value defined in "random_events_generator_network.ned"
	bool continuousMode = default(true); // false: when a simulation_seed_n is not found, stop execution. true: ignore not found seeds
}

network analyze_simulations_network
{       
    parameters:        
        @display("bgb=331,135");
    types:
        channel Channel extends ned.DelayChannel { }
    submodules:
        simulation: AnalyzeSimulations;
}