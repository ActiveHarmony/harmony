
package org.umd.periCSL;


/*
  Extends References class.
*/
public class RegionReference extends References
{

    // what region does this reference belong to?
    private String region_name;
    public  RegionReference(String var_name, String region_name)
    {
        // name of the parameter
        this.name=var_name;
        this.region_name = region_name;
    }
    public String getName()
    {
        return name;
    }
    public String getRegionName()
    {
        return region_name;
    }

    public String toString()
    {
        return name+"_"+region_name;
    }

    public boolean equals(Object o)
    {
        if (o instanceof RegionReference)
            return (((RegionReference)o).toString().equals(this.toString()));
        else
            return false;
    }

     public int hashCode(){
         return name.hashCode()+region_name.hashCode();
	}

}
