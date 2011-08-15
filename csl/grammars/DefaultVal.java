package org.umd.periCSL;

public class DefaultVal {


    /*
      This class does not have much in it in this pre-release, however, as
      we add more datatypes to our Constraint Language, this will expand.
    */
	private int type;

	private String val;

	public DefaultVal(int type, String val) {
		this.type=type;
		this.val=val;
	}

	public String getValue()
	{
		return val;
	}

    public String getVal() 
    {
        return val;
    }

    public String toString() 
    {
        return val;
    }


}
