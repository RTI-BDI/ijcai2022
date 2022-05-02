import json
import os
import os.path

SCHED_TYPE = "FCFS"

def listdirs(folder):
    return [d for d in os.listdir(folder) if os.path.isdir(os.path.join(folder, d))]


def main():
    # read the list of names in specific folder:
    simulations = os.listdir('./generated_simulations')
    print(simulations)

    for batch in simulations:
        seeds = listdirs('./generated_simulations/'+batch)
        for seed in seeds:
            path = './generated_simulations/'+batch+"/"+seed+"/reports/"+SCHED_TYPE+"/metrics.json"
            if os.path.exists(path):
                with open(path) as metrics_file:
                    data = json.load(metrics_file)
                    # print(seed)
                    if(len(data) > 6):
                        if(data[6]['Num_of_generated_errors'] > 0):
                            #print ("Simulation: "+ seed 
                            #+",\t Num_of_generated_errors: "+ str(data[6]['Num_of_generated_errors']))
                            for error in data[6]['Generated_errors']:
                                print ("Simulation: "+ seed +",\t "+ error['Cause'])

main()