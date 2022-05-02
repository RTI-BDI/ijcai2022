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
simple CompareSimulations
{	
	@display("i=block/user"); 
	// Configurable Parameters ------------------------------------------------
	int logger_level = default(5); // 0 = PrintNothing, Error, Debug, Default, Warning, 5 = EssentialInfo, EveryInfo, EveryInfoPlus
	string user = default("francesco");
	string folder_path = default("generated_simulations");
	string analysis_file = default("simulation_analysis.json"); 	// used as: "{scheduling_algorithm}_simulation_analysis.json -> i.e. EDF_sim... or FCFS_sim... or RR_sim...
	string simulation_analysis_result = default("analysis_result.json"); // name of the file that will be created
	bool print_latex = default(false); 			// used to format results in latex longtable structure
	bool print_short_format = default(true); 	// print data in short-form and compare with journal hardcoded data
	bool single_simulation = default(false);	// true: compare EDF, RR, FCFS of simulation 'simulation_name'. false: use the list defined in CompareSimulations.h > simulations vector
	
	// Parameter used ONLY if SINGLE_SIMULATION is set to TRUE
	string simulation_name = default("test-trigger-goals.2.1");
}

network compare_simulations_results_network
{       
    parameters:        
        @display("bgb=331,135");
    types:
        channel Channel extends ned.DelayChannel { }
    submodules:
        simulation: CompareSimulations;
}