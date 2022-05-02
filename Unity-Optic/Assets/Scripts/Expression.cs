using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Newtonsoft.Json.Linq;

public class Expression
{
    //Representing the node of the expression 'tree'
    public Node node;
    //Left branch
    public Expression exp_1;
    //Right branch
    public Expression exp_2;

    //Initialization of operator 
    public Expression(Node node, Expression exp_1, Expression exp_2)
    {
        this.node = node;
        this.exp_1 = exp_1;
        this.exp_2 = exp_2;
    }

    //Initialization of unary operator
    public Expression(Node node, Expression exp_1)
    {
        this.node = node;
        this.exp_1 = exp_1;
        this.exp_2 = null;
    }

    //Initialization of a leaf (value or belief)
    public Expression(Node node)
    {
        this.node = node;
        this.exp_1 = null;
        this.exp_2 = null;
    }

    public Expression(Expression other)
    {
        this.node = new Node(other.node);
        switch (other.node.type)
        {
            case Node.NodeType.LeafBelief:
                this.exp_1 = null;
                this.exp_2 = null;
                break;
            case Node.NodeType.LeafValue:
                this.exp_1 = null;
                this.exp_2 = null;
                break;
            case Node.NodeType.Unary:
                this.exp_1 = new Expression(other.exp_1);
                this.exp_2 = null;
                break;
            case Node.NodeType.Operator:
                this.exp_1 = new Expression(other.exp_1);
                this.exp_2 = new Expression(other.exp_2);
                break;
        }
    }

    //Transalte from JSON to Object
    public static Expression Evaluate(JToken token)
    {
        Node node = Node.Evaluate(token);
        switch (node.type)
        {
            case (Node.NodeType.Operator):
                return new Expression(node, Expression.Evaluate(token.First.Next), Expression.Evaluate(token.First.Next.Next));
                break;
            case (Node.NodeType.Unary):
                return new Expression(node, Expression.Evaluate(token.First.Next));
                break;
            case (Node.NodeType.LeafValue):
                return new Expression(node);
                break;
            case (Node.NodeType.LeafBelief):
                return new Expression(node);
                break;
            default:
                return null;
                break;
        }
    }

    //Translate from Object to PDDL
    public string ToPDDL(bool questionMark)
    {
        if (this.node.type == Node.NodeType.Operator)
        {
            return "(" + this.node.ToPDDL(questionMark) + " " + this.exp_1.ToPDDL(questionMark) + " " + this.exp_2.ToPDDL(questionMark) + ")";
        } else if (this.node.type == Node.NodeType.Unary && this.node.value != "true") {
            return "(" + this.node.ToPDDL(questionMark) + " " + this.exp_1.ToPDDL(questionMark) + ")";
        } else if (this.node.value == "true"){
            return this.exp_1.ToPDDL(questionMark);
        } else {
            return this.node.ToPDDL(questionMark);
        }
    }

    //Transalte from Object to Kronosim Condition
    public string ToKronosimExpCondition()
    {
        string result = "";

        switch (this.node.type)
        {
            case (Node.NodeType.Operator):
                string op = "";
                switch (this.node.value)
                {
                    case "=":
                        op = "==";
                        break;
                    case "increase":
                        op = "+";
                        break;
                    case "decrease":
                        op = "-";
                        break;
                    default:
                        op = this.node.value;
                        break;
                }
                result = result + " [ \"" + op + "\", " + this.exp_1.ToKronosimExpCondition() + ", " + this.exp_2.ToKronosimExpCondition() + " ] "; 
                break;
            case (Node.NodeType.Unary):
                switch (this.node.value) {
                    case "at start":
                        result = result + this.exp_1.ToKronosimExpCondition();
                        break;
                    case "at end":
                        result = result + this.exp_1.ToKronosimExpCondition();
                        break;
                    case "true":
                        result = result + " [ \"==\", " + this.exp_1.ToKronosimExpCondition() + ", true ] "; 
                        break;
                    case "false":
                        result = result + " [ \"==\", " + this.exp_1.ToKronosimExpCondition() + ", false ] ";
                        break;
                    default:
                        break;
                } 
                break;
            case (Node.NodeType.LeafBelief):
                result = result + " [ \"READ_BELIEF\", \"" + this.node.belief.GetGroundedName() + "\" ] ";
                break;
            case (Node.NodeType.LeafValue):
                result = result + this.node.value;
                break;
            default:
                break;
        }

        return result;
    }

    //Translate from Object to Kronosim Condition and substitute any matching Belief into a Variable
    public string ToKronosimExpCondition(string toSubstitute)
    {
        string result = "";

        switch (this.node.type)
        {
            case (Node.NodeType.Operator):
                string op = "";
                switch (this.node.value)
                {
                    case "=":
                        op = "==";
                        break;
                    case "increase":
                        op = "+";
                        break;
                    case "decrease":
                        op = "-";
                        break;
                    default:
                        op = this.node.value;
                        break;
                }
                result = result + " [ \"" + op + "\", " + this.exp_1.ToKronosimExpCondition(toSubstitute) + ", " + this.exp_2.ToKronosimExpCondition(toSubstitute) + " ] ";
                break;
            case (Node.NodeType.Unary):
                switch (this.node.value)
                {
                    case "at start":
                        result = result + this.exp_1.ToKronosimExpCondition(toSubstitute);
                        break;
                    case "at end":
                        result = result + this.exp_1.ToKronosimExpCondition(toSubstitute);
                        break;
                    case "true":
                        result = result + " [ \"==\", " + this.exp_1.ToKronosimExpCondition(toSubstitute) + ", true ] ";
                        break;
                    case "false":
                        result = result + " [ \"==\", " + this.exp_1.ToKronosimExpCondition(toSubstitute) + ", false ] ";
                        break;
                    default:
                        break;
                }
                break;
            case (Node.NodeType.LeafBelief):
                if(this.node.belief.GetGroundedName() == toSubstitute)
                {
                    result = result + " [ \"READ_VARIABLE\", \"" + this.node.belief.GetGroundedName() + "_old\" ] ";
                } else
                {
                    result = result + " [ \"READ_BELIEF\", \"" + this.node.belief.GetGroundedName() + "\" ] ";
                }

                break;
            case (Node.NodeType.LeafValue):
                result = result + this.node.value;
                break;
            default:
                break;
        }

        return result;
    }

    //Translate from Object to Kronosim Expression
    public string ToKronosimExpEffect()
    {
        string result = "";
        switch (this.node.type)
        {
            case Node.NodeType.Operator:
                //Mapping
                switch (this.node.value)
                {
                    case "=":
                        result = result + "[ \"ASSIGN\", ";
                        break;
                    case "-":
                        result = result + "[ \"-\", ";
                        break;
                    case "increase":
                        result = result + "[ \"INCREASE\", ";
                        break;
                    case "decrease":
                        result = result + "[ \"DECREASE\", ";
                        break;
                    default:
                        result = result + "[ \"ERROR\", ";
                        break;
                }
                result = result + this.exp_1.ToKronosimExpEffect() + ", " + this.exp_2.ToKronosimExpEffect() + " ]";
                break;
            case Node.NodeType.Unary:
                switch (this.node.value)
                {
                    case "true":
                        result = result + "[ \"ASSIGN\", " + this.exp_1.ToKronosimExpEffect() + ", true ]";
                        break;
                    case "false":
                        result = result + "[ \"ASSIGN\", " + this.exp_1.ToKronosimExpEffect() + ", false ]";
                        break;
                    default:
                        result = result + "[ \"ASSIGN\", " + this.exp_1.ToKronosimExpEffect() + ", error ]";
                        break;
                }
                break;
            case Node.NodeType.LeafValue:
                result = result + this.node.value;
                break;
            case Node.NodeType.LeafBelief:
                result = result + "\"" + this.node.belief.GetGroundedName() + "\"";
                break;
            default:
                break;
        }

        return result;
    }

    //Translate from Object to Kronosim Initialization (like conditions, but simpler)
    public string ToKronosimExpInitialization()
    {
        string jsonStr = "";

        if (this.exp_1.node.belief.type != Belief.BeliefType.Predicate)
        {
            jsonStr = jsonStr + " [ \"==\", [ \"READ_BELIEF\", \"" + this.exp_1.node.belief.GetGroundedName() + "\" ], ";
            jsonStr = jsonStr + this.exp_2.node.value + " ]";

        }
        else
        {
            jsonStr = jsonStr + " [ \"==\", [ \"READ_BELIEF\", \"" + this.exp_1.node.belief.GetGroundedName() + "\" ], ";
            jsonStr = jsonStr + this.node.value + " ]";
        }

        return jsonStr;
    }


}
