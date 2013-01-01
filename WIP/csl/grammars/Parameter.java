package org.umd.periCSL;

import java.util.Vector;


/*
  Parameter
*/

public class Parameter {

    // name of the parameter
    private String name;

    // type
	private int type;

    // name of the problem that this parameter is associated with
    private String problem_name;

    // Region set
	private RegionSet rs = null;

    // Default Value
	private DefaultVal def = null;

    // Domain of the parameter
    private CSLDomain domain = null;

    public Parameter()
    {
    }

	public Parameter(String name, int type)
	{
		this.name=name;
		this.type=type;

	}

    public void setProblemName(String problem_name)
    {
        this.problem_name = problem_name;
    }

	public void setRegionSet(RegionSet rs)
	{
		this.rs=rs;
	}

    public void setName(String name)
    {
        this.name=name;
    }

    public void setType(int type)
    {
        this.type=type;
    }

    public void setDomain(CSLDomain domain)
    {
        this.domain = domain;
    }

    public void setDefault(DefaultVal def)
    {
        this.def = def;
    }

    public String getName()
    {
        return name;
    }

    public CSLDomain getDomain()
    {
        return domain;
    }


	public int getType()
	{
		return type;
	}

    public DefaultVal getDefault()
	{
		return def;
	}

	public RegionSet getRegionSet()
	{
		return rs;
	}

    public Vector getCodeRegions()
    {
        Vector return_vector = new Vector();
        if(rs != null)
            return_vector = rs.getCodeRegions();

        return return_vector;
    }

    public String getProblemName()
    {
        return problem_name;
    }

}
