using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Newtonsoft.Json.Linq;

public class Node
{
    //Node's types 
    public enum NodeType
    {
        Operator,
        Unary,
        LeafBelief,
        LeafValue
    }

    public NodeType type;
    public Belief belief;
    public string value;

    //Initialization of a leaf-belief
    public Node(NodeType type, Belief belief)
    {
        this.type = type;
        this.belief = belief;
        this.value = null;
    }

    //Initialization of an operator (binary or unary) or a leaf-value
    public Node(NodeType type, string value)
    {
        this.type = type;
        this.belief = null;
        this.value = value;
    }

    public Node(Node other)
    {
        this.type = other.type;
        if(other.type == NodeType.LeafBelief)
        {
            this.value = null;
            this.belief = new Belief(other.belief);
        } else
        {
            this.belief = null;
            this.value = other.value;
        }
    }

    //Translate from JSON to Object
    public static Node Evaluate(JToken token)
    {
        if (token.Type == JTokenType.Array)
        {
            switch (token.First.ToString())
            {
                case ("at_start"):
                    return new Node(NodeType.Unary, "at start");
                    break;
                case ("at_end"):
                    return new Node(NodeType.Unary, "at end");
                    break;
                case (">"):
                    return new Node(NodeType.Operator, ">");
                    break;
                case ("<"):
                    return new Node(NodeType.Operator, "<");
                    break;
                case ("true"):
                    return new Node(NodeType.Unary, "true");
                    break;
                case ("false"):
                    return new Node(NodeType.Unary, "false");
                    break;
                case ("increase"):
                    return new Node(NodeType.Operator, "increase");
                    break;
                case ("decrease"):
                    return new Node(NodeType.Operator, "decrease");
                    break;
                case ("function"):
                    return new Node(NodeType.LeafBelief, Belief.Evaluate(token));
                    break;
                case ("predicate"):
                    return new Node(NodeType.LeafBelief, Belief.Evaluate(token));
                    break;
                case ("constant"):
                    return new Node(NodeType.LeafBelief, Belief.Evaluate(token));
                    break;
                case ("equal"):
                    return new Node(NodeType.Operator, "=");
                    break;
                case ("-"):
                    return new Node(NodeType.Operator, "-");
                    break;
                default:
                    return null;
                    break;
            }
        } else {
            return new Node(NodeType.LeafValue, token.ToString());
        }
    }

    //Translate from Object to PDDL
    public string ToPDDL(bool questionMark)
    {
        if(this.type != NodeType.LeafBelief)
        {
            switch (this.value)
            {
                case ("true"):
                    return "";
                    break;
                case ("false"):
                    return "not";
                    break;
                default:
                    return this.value;
                    break;
            }
        } else
        {
            return this.belief.ToPDDL(questionMark);
        }
    }

}
