using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System;
using System.Globalization;

public class Plan
{
    public List<KeyValuePair<float, Action>> steps;

    public Plan(List<KeyValuePair<float, Action>> steps)
    {
        this.steps = new List<KeyValuePair<float, Action>>();
        foreach (KeyValuePair<float, Action> entry in steps)
        {
            this.steps.Add(new KeyValuePair<float, Action>(entry.Key, new Action(entry.Value)));
        }
    }

    //Parse the output of optic into the Plan object
    public static Plan FromPlainToObject(string[] plain, List<Action> groundedActions)
    {
        Plan result = null;

		int index = 0;
		for (int counter = 0; counter < plain.Length; counter++)
		{
			if (plain[counter].StartsWith("; Time"))
			{
				index = counter;
			}
		}

        List<KeyValuePair<float, Action>> tempSteps = new List<KeyValuePair<float, Action>>();
        for(int i=index+1; i<plain.Length; i++)
        {
            string arrivalTime = string.Empty;
            string action = string.Empty;
            string tempParam = string.Empty;
            List<string> param = new List<string>();
            string duration = string.Empty;
            int considering = 0;

            foreach (char c in plain[i])
            {
                if(c == ':')
                {
                    considering = 4;
                }
                else if(c == '(')
                {
                    considering = 1;
                } else if(c == ')')
                {
                    considering = 4;
                    param.Add(tempParam);
                    tempParam = string.Empty;
                } else if(Char.IsWhiteSpace(c) && considering == 1)
                {
                    considering = 2;
                } else if(Char.IsWhiteSpace(c) && considering == 2)
                {
                    param.Add(tempParam);
                    tempParam = string.Empty;
                } else if(c == '.' && considering == 0)
                {
                    arrivalTime += ',';
                } else if(c == '[')
                {
                    considering = 3;
                } else if(c == '.' && considering == 3)
                {
                    duration += ',';
                } else if(c == ']')
                {
                    considering = 4;
                }
                else
                {
                    switch (considering)
                    {
                        //considering arrivalTime
                        case 0:
                            arrivalTime += c;
                            break;
                        //considering action
                        case 1:
                            action += c;
                            break;
                        //considering params
                        case 2:
                            tempParam += c;
                            break;
                        //considering duration
                        case 3:
                            duration += c;
                            break;
                        //skipping
                        case 4:
                            break;
                    }
                }
            }

            foreach (Action a in groundedActions)
            {
                bool areEqual = true;
                if (action != a.name)
                {
                    areEqual = false;
                }
                else
                {
                    for (int j = 0; j < a.parameters.Count; j++)
                    {
                        if (j < param.Count && param[j] != a.parameters[j].name)
                        {
                            areEqual = false;
                        }
                    }
                }

                if (areEqual)
                {
                    Expression newDuration = new Expression(new Node(Node.NodeType.LeafValue, duration));
                    tempSteps.Add(new KeyValuePair<float, Action>(float.Parse(CommaToDotDuration(arrivalTime), CultureInfo.InvariantCulture), new Action(a, newDuration)));
                }
            }
        }

		Debug.Log("Temp Steps count " + tempSteps.Count); 

        result = new Plan(tempSteps);


        return result;
    }

    public string ToSubGoals()
    {
        string result = "";

        foreach (KeyValuePair<float, Action> entry in this.steps)
        {
            result = result + ", { ";

            //Goal name
            result = result + "\"goal_name\": \"" + entry.Value.GetGroundedName() + "\", ";

            //Body
            result = result + "\"body\": [ { \"action_type\": \"TASK\", \"execution\": \"SEQUENTIAL\", ";
            result = result + "\"action\": { \"computationTime\": " + entry.Value.computation_cost + ", ";
            result = result + "\"n_exec\": " + (int)((float.Parse(CommaToDotDuration(entry), CultureInfo.InvariantCulture)) / entry.Value.period) + ", ";
            result = result + "\"period\": " + entry.Value.period + ", \"relativeDeadline\": " + entry.Value.period + ", ";
            result = result + "\"server\": -1";
            result = result + " } } ],";

            //Cont conditions
            result = result + "\"cont_conditions\": [ ], ";

            //Effects at begin
            result = result + "\"effects_at_begin\": [ ";
            foreach (Expression e in entry.Value.effects)
            {
                if(e.node.value == "at start")
                {
                    result = result + " " + e.exp_1.ToKronosimExpEffect() + ",";
                }
                
            }
            foreach (Expression e in entry.Value.postConditions)
            {
                result = result + " [ \"DEFINE_VARIABLE\", \"" + e.exp_1.exp_1.node.belief.GetGroundedName() + "_old\", [ \"READ_BELIEF\", \"" + e.exp_1.exp_1.node.belief.GetGroundedName() + "\" ] ], ";
            }
            //remove last comma
            result = result.Remove(result.Length - 1);
            result = result + " ], ";

            //Effects at end
            result = result + "\"effects_at_end\": [ ";
            foreach (Expression e in entry.Value.effects)
            {
                if (e.node.value == "at end")
                {
                    result = result + " " + e.exp_1.ToKronosimExpEffect() + ",";
                }
            }
            //remove last comma
            result = result.Remove(result.Length - 1);
            result = result + " ], ";

            //Postconditions
            result = result + "\"post_conditions\": [ [ \"AND\", ";

            int counter = 0;
            foreach (Expression e in entry.Value.postConditions)
            {
                result = result + "[ \"==\", [ \"READ_BELIEF\", \"" + e.exp_1.exp_1.node.belief.GetGroundedName() + "\" ], " + e.ToKronosimExpCondition(e.exp_1.exp_1.node.belief.GetGroundedName()) + " ]";

                if (counter != entry.Value.conditions.Count - 1)
                {
                    result = result + ", ";
                }
                counter++;
            }

            result = result + " ] ], ";

            //Preconditions
            result = result + "\"preconditions\": [ [ \"AND\", ";

            counter = 0;
            foreach (Expression e in entry.Value.conditions)
            {
                result = result + e.ToKronosimExpCondition();

                if (counter != entry.Value.conditions.Count - 1)
                {
                    result = result + ", ";
                }
                counter++;
            }

            result = result + " ] ], ";

			//Preference
			result = result + "\"preference\" : { \"computedDynamically\" : true, \"formula\" : [ 0.8 ], \"reference_table\" : [ ] } ";

			result = result + "} ";
        }



        return result;
    }

    //Change comma to dot in arrival time
    public static string CommaToDotArrivalTime(KeyValuePair<float, Action> entry)
    {
        string result = string.Empty;
        foreach (Char c in entry.Key.ToString())
        {
            if(c == ',')
            {
                result += '.';
            } else
            {
                result += c;
            }
        }
        return result;
    }

    //Change comma to dot in duration
    public static string CommaToDotDuration(KeyValuePair<float, Action> entry)
    {
        string result = string.Empty;
        foreach (Char c in entry.Value.duration.node.value)
        {
            if(c == ',')
            {
                result += '.';
            } else
            {
                result += c;
            }
        }

        return result;
    }
    
    //Change comma to dot in duration from String
    public static string CommaToDotDuration(string duration){
    
    	string result = string.Empty;
    	foreach (Char c in duration){
    		if(c == ',')
            {
                result += '.';
            } else
            {
                result += c;
            }
    	}
    	
    	return result;
    }
}
