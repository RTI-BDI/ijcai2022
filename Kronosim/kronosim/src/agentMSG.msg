cplusplus {{
   #include <vector>
   #include "../headers/ag_need.h"
   
   typedef std::vector<int> IntVector;
   typedef Need *NeedPtr;
}};

class noncobject IntVector;
class noncobject NeedPtr;

message agentMSG
{    
    int source = 0;
    int	destination = 0;
    int	hopCount = 0;
    
    int	informative = 0;
   
    // scheduler related
    //TODO: semplificare come per i need
    int ag_id = 0;
    int ag_task_id = 0;
    int ag_task_demander = 0;
    int ag_task_server = 0;
    double ag_task_t_finito = 0;
    double ag_task_release_time = 0;
    double ag_task_ddl = 0;
    double replenish = 0;
    // needs and services
    NeedPtr need;
	IntVector ag_needed_tasks;
	IntVector agents_per_need;
	IntVector public_tasks;
     
    // negotiation related
    int content = 0;
    int neg_step = 0;
    int round = 0;
    double bid = 0;
    double utilization = -1;
    bool processed = false;          
}