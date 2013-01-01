package org.umd.periCSL;


// Parameter Reference
public class ParamReference extends References
{
    public  ParamReference(String name)
    {
        // name of the parameter
        this.name=name;
    }

    public String getName()
    {
        return name;
    }

    public String toString()
    {
        return name;
    }

    public boolean equals(Object o)
    {
        if (o instanceof ParamReference)
            return (((ParamReference)o).toString().equals(this.toString()));
        else
            return false;
    }

    public int hashCode(){
		return name.hashCode();
	}
}
