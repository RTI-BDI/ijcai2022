INFO:List of Intentions[tot = 3]:
INFO: Intention index:[0], from goal:[4], goal:(skill_isLightOn), plan:[5], original_plan:[5], num. of actions: 3, startTime: 0.000000, firstActivation: 0.000000, lastExecution: 0.000000, batch_startTime: 0.000000, completion time: -1.000000, waiting for completion: false
INFO: - Task:[1], plan:[5], original_plan:[5], N_exec left: 0 of 1, compTime: 5.000000, DDL: 30.000000, arrivalTimeInitial: 0.000000, arrivalTime: 0.000000, Execution_type:[PARALLEL] isBeenActivated:[true]
INFO: - Task:[2], plan:[5], original_plan:[5], N_exec left: 0 of 1, compTime: 6.000000, DDL: 30.000000, arrivalTimeInitial: 0.000000, arrivalTime: 0.000000, Execution_type:[PARALLEL] isBeenActivated:[true]
INFO: Intention index:[1], from goal:[2], goal:(skill_isOvenOn), plan:[3], original_plan:[3], num. of actions: 4, startTime: 0.000000, firstActivation: 0.000000, lastExecution: 0.000000, batch_startTime: 0.000000, completion time: -1.000000, waiting for completion: false
INFO: - Task:[0], plan:[3], original_plan:[3], N_exec left: 0 of 1, compTime: 7.000000, DDL: 35.000000, arrivalTimeInitial: 0.000000, arrivalTime: 0.000000, Execution_type:[SEQUENTIAL] isBeenActivated:[true]
INFO: - Task:[1], plan:[3], original_plan:[3], N_exec left: 1 of 1, compTime: 4.000000, DDL: 30.000000, arrivalTimeInitial: 0.000000, arrivalTime: 0.000000, Execution_type:[SEQUENTIAL] isBeenActivated:[false]
INFO: - Task:[2], plan:[3], original_plan:[3], N_exec left: 1 of 1, compTime: 6.000000, DDL: 35.000000, arrivalTimeInitial: 0.000000, arrivalTime: 0.000000, Execution_type:[SEQUENTIAL] isBeenActivated:[false]
INFO: - Task:[3], plan:[3], original_plan:[3], N_exec left: 1 of 1, compTime: 6.000000, DDL: 30.000000, arrivalTimeInitial: 0.000000, arrivalTime: 0.000000, Execution_type:[PARALLEL] isBeenActivated:[false]
INFO: Intention index:[2], from goal:[5], goal:(skill_isRoomAtRightTemperature), plan:[0], original_plan:[0], num. of actions: 1, startTime: 2.000000, firstActivation: -1.000000, lastExecution: 2.000000, batch_startTime: 2.000000, completion time: -1.000000, waiting for completion: false
INFO: - Task:[0], plan:[0], original_plan:[0], N_exec left: 10 of 10, compTime: 8.000000, DDL: 10.000000, arrivalTimeInitial: 0.000000, arrivalTime: 0.000000, Execution_type:[SEQUENTIAL] isBeenActivated:[false]
INFO:-----------------------------


50 completed Task:[0], plan:[0], original_plan:[0]

INFO:Release queue[tot = 1]:
INFO:- Task:[0], plan:[0], original_plan:[0], deadline:[10.000000], arrivalTime:[52.000000], releaseTime:[-1.000000], N_exec left:[5 of 10], tskCres:[8.000000], tskStatus:[ACTIVE], tskExecType:[SEQUENTIAL], isBeenAct:[false]
INFO:-----------------------------
INFO:Ready queue[tot = 2]:
INFO:- SERVER[0]: curr_budget:[5.000000], curr_ddl:[60.000000], queue.size:[2], deadline:[60.000000], startTime:[0.000000], arrivalTime:[21.000000], tskCres:[0.000000], tskStatus:[READY], tskExecType:[SEQUENTIAL], isBeenAct:[true], task:[1], plan:[5], original_plan:[5]
INFO: -- task:[1], plan:[5], original_plan:[5], deadline:[30.000000], arrivalTime:[0.000000], releaseTime:[0.000000], N_exec left:[0 of 1], tskCres:[4.000000], tskStatus:[READY], tskExecType:[PARALLEL], isBeenAct:[true]
INFO: -- task:[2], plan:[5], original_plan:[5], deadline:[30.000000], arrivalTime:[0.000000], releaseTime:[0.000000], N_exec left:[0 of 1], tskCres:[6.000000], tskStatus:[READY], tskExecType:[PARALLEL], isBeenAct:[true]
INFO:- SERVER[1]: curr_budget:[5.000000], curr_ddl:[70.000000], queue.size:[1], deadline:[70.000000], startTime:[0.000000], arrivalTime:[34.000000], tskCres:[0.000000], tskStatus:[READY], tskExecType:[SEQUENTIAL], isBeenAct:[true], task:[0], plan:[3], original_plan:[3]
INFO: -- task:[0], plan:[3], original_plan:[3], deadline:[35.000000], arrivalTime:[0.000000], releaseTime:[0.000000], N_exec left:[0 of 1], tskCres:[2.000000], tskStatus:[READY], tskExecType:[SEQUENTIAL], isBeenAct:[true]
INFO:-----------------------------

50 - 52 executing, add Task:[0], plan:[0], original_plan:[0] NO PREEMPTION

INFO:Release queue[tot = 1]:
INFO:- Task:[0], plan:[0], original_plan:[0], deadline:[10.000000], arrivalTime:[62.000000], releaseTime:[-1.000000], N_exec left:[4 of 10], tskCres:[8.000000], tskStatus:[IDLE], tskExecType:[SEQUENTIAL], isBeenAct:[false]
INFO:-----------------------------
INFO:Ready queue[tot = 3]:
INFO:- SERVER[0]: curr_budget:[5.000000], curr_ddl:[60.000000], queue.size:[2], deadline:[60.000000], startTime:[0.000000], arrivalTime:[21.000000], tskCres:[0.000000], tskStatus:[EXEC], tskExecType:[SEQUENTIAL], isBeenAct:[true], task:[1], plan:[5], original_plan:[5]
INFO: -- task:[1], plan:[5], original_plan:[5], deadline:[30.000000], arrivalTime:[0.000000], releaseTime:[0.000000], N_exec left:[0 of 1], tskCres:[4.000000], tskStatus:[EXEC], tskExecType:[PARALLEL], isBeenAct:[true]
INFO: -- task:[2], plan:[5], original_plan:[5], deadline:[30.000000], arrivalTime:[0.000000], releaseTime:[0.000000], N_exec left:[0 of 1], tskCres:[6.000000], tskStatus:[READY], tskExecType:[PARALLEL], isBeenAct:[true]
INFO:- Task:[0], plan:[0], original_plan:[0], deadline:[62.000000], arrivalTime:[52.000000], releaseTime:[52.000000], N_exec left:[4 of 10], tskCres:[8.000000], tskStatus:[READY], tskExecType:[SEQUENTIAL], isBeenAct:[false]
INFO:- SERVER[1]: curr_budget:[5.000000], curr_ddl:[70.000000], queue.size:[1], deadline:[70.000000], startTime:[0.000000], arrivalTime:[34.000000], tskCres:[0.000000], tskStatus:[READY], tskExecType:[SEQUENTIAL], isBeenAct:[true], task:[0], plan:[3], original_plan:[3]
INFO: -- task:[0], plan:[3], original_plan:[3], deadline:[35.000000], arrivalTime:[0.000000], releaseTime:[0.000000], N_exec left:[0 of 1], tskCres:[2.000000], tskStatus:[READY], tskExecType:[SEQUENTIAL], isBeenAct:[true]
INFO:-----------------------------


t: 54 ************************************************************************
INFO:Release queue[tot = 1]:
INFO:- Task:[0], plan:[0], original_plan:[0], deadline:[10.000000], arrivalTime:[62.000000], releaseTime:[-1.000000], N_exec left:[4 of 10], tskCres:[8.000000], tskStatus:[ACTIVE], tskExecType:[SEQUENTIAL], isBeenAct:[false]
INFO:-----------------------------
INFO:Ready queue[tot = 3]:
INFO:- SERVER[0]: curr_budget:[1.000000], curr_ddl:[60.000000], queue.size:[1], deadline:[60.000000], startTime:[0.000000], arrivalTime:[54.000000], tskCres:[0.000000], tskStatus:[EXEC], tskExecType:[SEQUENTIAL], isBeenAct:[true], task:[2], plan:[5], original_plan:[5]
INFO: -- task:[2], plan:[5], original_plan:[5], deadline:[30.000000], arrivalTime:[0.000000], releaseTime:[0.000000], N_exec left:[0 of 1], tskCres:[6.000000], tskStatus:[EXEC], tskExecType:[PARALLEL], isBeenAct:[true]
INFO:- Task:[0], plan:[0], original_plan:[0], deadline:[62.000000], arrivalTime:[52.000000], releaseTime:[52.000000], N_exec left:[4 of 10], tskCres:[8.000000], tskStatus:[READY], tskExecType:[SEQUENTIAL], isBeenAct:[false]
INFO:- SERVER[1]: curr_budget:[5.000000], curr_ddl:[70.000000], queue.size:[1], deadline:[70.000000], startTime:[0.000000], arrivalTime:[34.000000], tskCres:[0.000000], tskStatus:[READY], tskExecType:[SEQUENTIAL], isBeenAct:[true], task:[0], plan:[3], original_plan:[3]
INFO: -- task:[0], plan:[3], original_plan:[3], deadline:[35.000000], arrivalTime:[0.000000], releaseTime:[0.000000], N_exec left:[0 of 1], tskCres:[2.000000], tskStatus:[READY], tskExecType:[SEQUENTIAL], isBeenAct:[true]
INFO:-----------------------------

t: 54, SERVER:[0] COMPLETED tskId:[1], plan:[5], original_id:[5], tskRt: 0.000000, budget: 5.000000, deadline: 60.000000, status:[EXEC]
budget res = 1
il server viene ri-aggiunto
server ddl = 60
60 < ddl task head in ready queue = 62
server[0] viene prioritizzato, 
ma porterà head "Task:[0], plan:[0], original_plan:[0]" al MISS
PERCHE: T0,0,0 è periodica, attivata a t: 52 con compTime = 8 e ddl = 62

Da 52 a 54 server[0] ha priorità. 
da 54 a 62 ci sono giusti 8,
ma server viene aggiunto di nuovo in coda e ha ancora ddl minore di 62
da 54 a 55 eseguo server[0] finche expired 
a 55 Task:[0], plan:[0], original_plan:[0] ha finalmente priorità
da 55 a (55+8 = 63) eseguo 0,0,0
DDL MISS = 1 (63 completion - 62 ddl)
************************************************************************


Stessa cosa per DDL MISS at t: 73 per tskId:[0], plan:[0], original_id:[0].
Da 63 a 65 viene eseguito SERVER:[1] COMPLETED tskId:[0], plan:[3], original_id:[3]
Ma a t: 62 era stato aggiunto in coda: Task:[0], plan:[0], original_plan:[0], con DDL = 72

72 - 65 = 7 time slot -> 0,0,0 ne necessità di 8
Da 65 a 73 esegue 0,0,0 e a 73 dà DDL MISS = 1
