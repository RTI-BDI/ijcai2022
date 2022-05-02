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
simple agent
{	
        @display("i=block/user"); 
        
        int sched_type; // = default(2); //scheduler type to use (0 = FCFS, 1 = RR, 2 = EDF)
        int server_type = default(0); //server type to use for messages (0 = CBS)
        int neg_type = default(0); // 0 = CNET 
        
        double neg_timeout = default(0);
        double quantum = default(4); // only used in Round Robin        
        bool MCTIdentity = default(false); // true: optimistic; false: pessimistic
        
        bool msg_server_mode = false; // needed in 'compute_MsgUtil_sever()'
        bool phiI_prioritize_internal_goals = default(false); // only used in Round Robin
        
        // Setup in omnetpp.ini ------
        string path = default("/default_simulation");
        int logger_level; // 0 = PrintNothing, Error, Debug, Default, Warning, 5 = EssentialInfo, EveryInfo, EveryInfoPlus
        // ---------------------------
    gates:
        inout gate[];
}

network ag_network
{
    parameters:
        int n = default(1);
        int k = 0;
        string user = default("francesco");
        
        @display("bgb=331,135");
    types:
        channel Channel extends ned.DelayChannel { 
            // delay = 100s;
        }
    submodules:
        ag[n]: agent;
}

