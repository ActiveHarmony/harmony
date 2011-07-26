package org.umd.periCSL;
import java.util.HashSet;
import java.util.ArrayList;
import java.util.Iterator;

/*
  On any given search space, there can be at most one specification that
  ties up all the constraints with and/or operators.

  Specification objects keeps track of all the constraints referenced in
  its body, along with a collection of all the arguments to those
  constraints.
 */


public class Specification
{

    // Constraints referenced in the body of the specification
    ArrayList<Constraint> constraints = new ArrayList<Constraint>();

    // Arguments: need this if we want to turn specification into a
    // function in the target language
    HashSet<References> arguments = new HashSet<References>();

    // Should we need the search space name ...
    String problem_name ="";

    public Specification(ArrayList<Constraint> cons)
    {
        this.constraints = cons;
    }

    public HashSet<References> getArguments()
    {
        // iterate through all the given constraint objects and create the
        // set
        Iterator iter_constraints = constraints.iterator();
        while(iter_constraints.hasNext())
        {
            Constraint temp_constraint = (Constraint)iter_constraints.next();
            Iterator iter_arguments = (temp_constraint.getArguments()).iterator();
            while(iter_arguments.hasNext())
            {
                arguments.add((References)iter_arguments.next());
            }
        }
        return arguments;
    }

    public void setProblemName(String pname) 
    {
        this.problem_name=pname;
    }

    public String getProblemName()
    {
        return problem_name;
    }

    public ArrayList<Constraint> getConstraints()
    {
        return constraints;
    }


}
