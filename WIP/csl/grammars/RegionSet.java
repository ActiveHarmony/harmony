package org.umd.periCSL;

import java.util.Vector;
import java.util.List;


public class RegionSet {

    // names of code regions associated with this region set
	private Vector code_regions = null;

    // name of the region set
	private String name;

	public RegionSet(String name)
	{
		code_regions=new Vector();
        this.name = name;
	}

    // push one code region at a time
	public void push(String code_region)
	{
		code_regions.add(code_region);
	}

    // iterate through the list and add all the code regions
    public void pushList(List ls)
    {
        System.out.println(ls.toString());
        for(int i=0; i < ls.size(); i++)
        {
            code_regions.add(ls.get(i));
        }
    }

    public String getName()
    {
        return name;
    }

	public Vector getCodeRegions() {
		return code_regions;
	}
}
