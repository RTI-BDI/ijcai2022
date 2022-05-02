using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Newtonsoft.Json.Linq;

public class Belief
{
    //Belief types
    public enum BeliefType
    {
        Predicate,
        Function,
        Constant
    }

    //Belief's type
    public BeliefType type;
    //Belief's name
    public string name;
    //Belief's parameters
    public List<Parameter> param;

    public Belief(BeliefType type, string name, List<Parameter> param)
    {
        this.type = type;
        this.name = name;
        this.param = param;
    }

    public Belief(Belief other)
    {
        this.type = other.type;
        this.name = other.name;
        this.param = new List<Parameter>();
        if(other.param != null)
            foreach (Parameter p in other.param)
            {
                this.param.Add(new Parameter(p));
            }
    }

    //Utility function to determine if two beliefs are the same and applied to the same parameters
    public bool Equals(Belief other)
    {
        bool result = true;
        result = result && this.name == other.name && this.type.Equals(other.type);
        if(this.param.Count == other.param.Count)
        {
            for (int i = 0; i < this.param.Count; i++)
            {
                result = result && this.param[i].Equals(other.param[i]);
            }
        } else
        {
            return false;
        }

        return result;
    }

    //Translate from JSON to Object
    public static Belief Evaluate(JToken token)
    {
        switch (token.First.ToString())
        {
            case ("predicate"):
                return new Belief(BeliefType.Predicate, token.First.Next.ToString(), Parameter.Evaluate(token.First.Next.Next));
                break;
            case ("function"):
                return new Belief(BeliefType.Function, token.First.Next.ToString(), Parameter.Evaluate(token.First.Next.Next));
                break;
            case ("constant"):
                return new Belief(BeliefType.Constant, token.First.Next.ToString(), null);
                break;
            default:
                return null;
                break;
        }
    }

    //Translate from Object to PDDL
    public string ToPDDL(bool questionMark)
    {
        string result = "(" + this.name;
        if(this.type != BeliefType.Constant)
            foreach (Parameter p in this.param)
            {
                result = result + " " + p.ToPDDL(questionMark);
            }
        result = result + ")";
        return result;
    }

    //Get the grounded name
    public string GetGroundedName()
    {
        string result = this.name;
        if(this.type != BeliefType.Constant)
            foreach (Parameter p in this.param)
            {
                result = result + "_" + p.name;
            }
        return result;
    }
}
