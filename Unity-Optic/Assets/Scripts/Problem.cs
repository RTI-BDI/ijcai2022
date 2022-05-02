using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Newtonsoft.Json.Linq;

public class Problem
{
    //Problem's name
    public string name;
    //Domain used
    public string domain;
    //List of istances of objects
    public List<Parameter> objects;
    //List of expressions representing initializations (AND-connected)
    public List<Expression> initializations;
    //List of expression representing goals (AND-connected)
    public List<Expression> goals;

    public Problem(string name, string domain, List<Parameter> objects, List<Expression> initializations, List<Expression> goals)
    {
        this.name = name;
        this.domain = domain;
        this.objects = objects;
        this.initializations = initializations;
        this.goals = goals;
    }

    //Translate from JSON to Object
    public static Problem Evaluate(JToken token)
    {
        if(token.First.ToString() == "defineProblem")
        {
            //2nd element
            string name = token.First.Next.ToString();

            //3rd element
            string domain = token.First.Next.Next.ToString();

            //4th element
            List<Parameter> objects = new List<Parameter>();
            if(token.First.Next.Next.Next.First.ToString() == "defineObjects")
            {
                objects = Parameter.Evaluate(token.First.Next.Next.Next.First.Next);
            }

            //5th elements
            List<Expression> initializations = new List<Expression>();
            foreach (JToken j in token.First.Next.Next.Next.Next)
            {
                if (j != token.First.Next.Next.Next.Next.First)
                {
                    initializations.Add(Expression.Evaluate(j));
                }
            }

            //6th elements
            List<Expression> goals = new List<Expression>();
            foreach (JToken j in token.First.Next.Next.Next.Next.Next)
            {
                if (j != token.First.Next.Next.Next.Next.Next.First)
                {
                    goals.Add(Expression.Evaluate(j));
                }
            }

            return new Problem(name, domain, objects, initializations, goals);

        } else {
            return null;
        }
    }

    //Translate from Object to PDDL
    public string ToPDDL()
    {
        string pddl = "";

        pddl = pddl + "(define \n";

        pddl = pddl + "(problem " + this.name + ")\n";
        pddl = pddl + "(:domain " + this.domain + ")\n";

        pddl = pddl + "(:objects \n";
        foreach (Parameter p in this.objects)
        {
            pddl = pddl + p.ToPDDL(false) + "\n";
        }
        pddl = pddl + ")\n";

        pddl = pddl + "(:init \n";
        foreach (Expression e in this.initializations)
        {
            pddl = pddl + e.ToPDDL(false) + "\n";
        }
        pddl = pddl + ")\n";

        pddl = pddl + "(:goal \n";
        pddl = pddl + "(and \n";
        foreach (Expression e in this.goals)
        {
            pddl = pddl + e.ToPDDL(false) + "\n";
        }
        pddl = pddl + ")\n";
        pddl = pddl + ")\n";

        pddl = pddl + ")";

        return pddl;
    }

}
