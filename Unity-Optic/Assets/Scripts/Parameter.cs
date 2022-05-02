using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Newtonsoft.Json.Linq;

public class Parameter
{
    //Parameter's name
    public string name;
    //Parameter's type
    public string type;

    public Parameter(string name, string type)
    {
        this.name = name;
        this.type = type;
    }

    public Parameter(Parameter other)
    {
        this.name = other.name;
        this.type = other.type;
    }

    //Utility function determining if two parameters are equal (considering the case where the type is not specified, like in actions)
    public bool Equals(Parameter other)
    {
        if (this.type == null || other.type == null)
            return this.name == other.name;
        else
            return this.name == other.name && this.type == other.type;
    }

    //Transalte from JSON to Object
    public static List<Parameter> Evaluate(JToken token)
    {
        List<Parameter> parameters = new List<Parameter>();
        if (token.First.ToString() == "array")
        {
            foreach (JToken j in token)
            {
                if(j != token.First)
                {
                    if (j.Type == JTokenType.Array)
                    {
                        parameters.Add(new Parameter(j.First.Next.ToString(), j.First.Next.Next.ToString()));
                    }
                    else
                    {
                        parameters.Add(new Parameter(j.ToString(), null));
                    }
                }
            }
        }

        return parameters;

        
    }

    //Translate from Object to PDDL
    public string ToPDDL(bool questionMark)
    {
        if (questionMark)
        {
            if (this.type != null)
            {
                return "?" + this.name + " - " + this.type;

            }
            else
            {
                return "?" + this.name;
            }
        }
        else
        {
            if (this.type != null)
            {
                return this.name + " - " + this.type;

            }
            else
            {
                return this.name;
            }
        }
    }
}
