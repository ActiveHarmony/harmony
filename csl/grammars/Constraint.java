package org.umd.periCSL;

import java.util.HashSet;


public class Constraint
{
    //name of this constraint
    private String name;

    // parameters/regions referenced in the body of the constraint make up
    // the arguments for the constraint.
    private HashSet<References> arguments;

    // Should we need the search space name ...
    private String problem_name;

    public Constraint(String name)
    {
        this.name=name;
    }

    public void setArguments(HashSet<References> args)
    {
        this.arguments = args;
    }

    public void setProblemName(String pname)
    {
        this.problem_name=pname;
    }

    public String getName()
    {
        return name;
    }

    public String getProblemName()
    {
        return problem_name;
    }

    public HashSet<References> getArguments()
    {
        return arguments;
    }
}
