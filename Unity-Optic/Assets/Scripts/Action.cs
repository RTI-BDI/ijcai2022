using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Newtonsoft.Json.Linq;

public class Action
{
    //Action's name
    public string name;
    //List of parameters (not bound to expressions' parameters)
    public List<Parameter> parameters;
    //Action's duration
    public Expression duration;
    //List of expression representing the preconditions (AND-connected)
    public List<Expression> conditions;
    //List of expression representing the effects (AND-connected)
    public List<Expression> effects;
    //List of post conditions ('public' effects) (AND-connected)
    public List<Expression> postConditions;

    //Kronosim parameters
    public int period;
    public int computation_cost;

    public Action(string name, List<Parameter> parameters, Expression duration, List<Expression> conditions, List<Expression> effects, List<Expression> postConditions, int period, int computation_cost)
    {
        this.name = name;
        this.parameters = parameters;
        this.duration = new Expression(duration);
        this.conditions = conditions;
        this.effects = effects;
        this.postConditions = postConditions;
        this.period = period;
        this.computation_cost = computation_cost;
    }

    public Action(Action other)
    {
        this.name = other.name;
        this.parameters = new List<Parameter>();
        foreach (Parameter p in other.parameters)
        {
            this.parameters.Add(new Parameter(p));
        }
        this.duration = new Expression(other.duration);
        this.conditions = new List<Expression>();
        foreach (Expression e in other.conditions)
        {
            this.conditions.Add(new Expression(e));
        }
        this.effects = new List<Expression>();
        foreach (Expression e in other.effects)
        {
            this.effects.Add(new Expression(e));
        }
        this.postConditions = new List<Expression>();
        foreach (Expression e in other.postConditions)
        {
            this.postConditions.Add(new Expression(e));
        }
        this.period = other.period;
        this.computation_cost = other.computation_cost;
    }

    public Action(Action other, Expression newDuration)
    {
        this.name = other.name;
        this.parameters = new List<Parameter>();
        foreach (Parameter p in other.parameters)
        {
            this.parameters.Add(new Parameter(p));
        }
        this.duration = new Expression(newDuration);
        this.conditions = new List<Expression>();
        foreach (Expression e in other.conditions)
        {
            this.conditions.Add(new Expression(e));
        }
        this.effects = new List<Expression>();
        foreach (Expression e in other.effects)
        {
            this.effects.Add(new Expression(e));
        }
        this.postConditions = new List<Expression>();
        foreach (Expression e in other.postConditions)
        {
            this.postConditions.Add(new Expression(e));
        }
        this.period = other.period;
        this.computation_cost = other.computation_cost;
    }

    //Translate from JSON to Object
    public static Action Evaluate(JToken token)
    {
        if(token.First.ToString() == "action")
        {
            //2nd element in token
            string name = token.First.Next.ToString();

            //3rd element in token
            List<Parameter> parameters = new List<Parameter>();
            if(token.First.Next.Next.First.ToString() == "defineParameters")
            {
                parameters = Parameter.Evaluate(token.First.Next.Next.First.Next);
            }

            //4th element in token
            Expression duration = Expression.Evaluate(token.First.Next.Next.Next.First.Next);

            //5th element in token
            List<Expression> conditions = new List<Expression>();
            foreach  (JToken j in token.First.Next.Next.Next.Next)
            {
                if(j != token.First.Next.Next.Next.Next.First)
                    conditions.Add(Expression.Evaluate(j));
            }

            //6th element in token
            List<Expression> effects = new List<Expression>();
            foreach (JToken j in token.First.Next.Next.Next.Next.Next)
            {
                if (j != token.First.Next.Next.Next.Next.Next.First)
                    effects.Add(Expression.Evaluate(j));
            }

            //7th element in token
            List<Expression> postConditions = new List<Expression>();
            foreach (JToken j in token.First.Next.Next.Next.Next.Next.Next)
            {
                if (j != token.First.Next.Next.Next.Next.Next.Next.First)
                {
                    postConditions.Add(Expression.Evaluate(j));
                }
            }

            //8th element in token
            int period = 0;
            if(token.First.Next.Next.Next.Next.Next.Next.Next.First.ToString() == "kronosim_period")
            {
                period = int.Parse(token.First.Next.Next.Next.Next.Next.Next.Next.First.Next.ToString());
            }

            //9th element in token
            int computation_cost = 0;
            if (token.First.Next.Next.Next.Next.Next.Next.Next.Next.First.ToString() == "kronosim_computation_cost")
            {
                computation_cost = int.Parse(token.First.Next.Next.Next.Next.Next.Next.Next.Next.First.Next.ToString());
            }

            return new Action(name, parameters, duration, conditions, effects, postConditions, period, computation_cost);

        } else {
            return null;
        }
    }

    //Translate from Object to PDDL
    public string ToPDDL(bool questionMark)
    {
        string pddl = "";

        pddl = pddl + "(:durative-action " + this.name + "\n";

        //Parameters
        pddl = pddl + ":parameters (";
        foreach(Parameter p in this.parameters)
        {
            pddl = pddl + p.ToPDDL(questionMark) + " ";
        }
        pddl = pddl + ")\n";

        //Duration
        pddl = pddl + ":duration (= ?duration " + this.duration.ToPDDL(questionMark) + ")\n";

        //Conditions
        pddl = pddl + ":condition (and\n";
        foreach (Expression e in this.conditions)
        {
            pddl = pddl + e.ToPDDL(questionMark) + "\n";
        }
        pddl = pddl + ")\n";

        //Effects
        pddl = pddl + ":effect (and\n";
        foreach (Expression e in this.effects)
        {
            pddl = pddl + e.ToPDDL(questionMark) + "\n";
        }
        foreach (Expression e in this.postConditions)
        {
            pddl = pddl + e.ToPDDL(questionMark) + "\n";
        }
        pddl = pddl + ")\n";

        pddl = pddl + ")";

        return pddl;
    }

    //Translate from Object to Desire (json)
    public string ToDesire()
    {
        string result = "";
        result = result + "{ \"priority\" : { \"computedDynamically\" : true, \"formula\" : [ 0.8 ], \"reference_table\" : [] },";
        result = result + "\"deadline\" : 1000.0,";
        result = result + "\"preconditions\" : [ [ \"AND\", ";

        int counter = 0;
        foreach (Expression e in this.conditions)
        {
            result = result + e.ToKronosimExpCondition();

            if (counter != this.conditions.Count - 1)
            {
                result = result + ", ";
            }
            counter++;
        }

        result = result + " ] ],";
        result = result + " \"goal_name\" : \"" + this.GetGroundedName() + "\"";
        result = result + "} ";

        return result;
    }

    //Get the grounded name
    public string GetGroundedName()
    {
        string result = "";
        result = result + this.name;
        foreach (Parameter p in this.parameters)
        {
            result = result + "_" + p.name;
        }
        return result;
    }
}
