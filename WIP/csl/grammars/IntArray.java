package org.umd.periCSL;

import java.util.Vector;

// Extends CSLDomain class to represent and int array defined in the
// language
public class IntArray extends CSLDomain
{

	public IntArray()
	{
		v = new Vector<Integer>();
		type=Types.INTARRAY;
	}

	public String toString()
	{
		return v.toString();
	}

	public Vector getValues()
	{
		return v;
	}

	public int getType()
	{
		return type;
	}

	public int getValueAt(int index)
	{
		return (Integer)v.get(index);
	}

	public void pushValue(int value)
	{
		v.add(new Integer(value));
	}

	public void fillValues(int min, int max, int step)
	{
		for(int i=min; i <= max; i+=step)
		{
			v.add(new Integer(i));
		}
	}

    // acts like a built-in range function -- if the target language does
    // not have range construct, simply grab the values from the internal
    // representation
    public void fillValues(String min, String max, String step)
	{
        int min_num = Integer.parseInt(min);
        int max_num = Integer.parseInt(max);
        int step_num = Integer.parseInt(step);
		for(int i=min_num; i <= max_num; i+=step_num)
		{
			v.add(new Integer(i));
		}
	}

    // power range built-in
    public void fillPowerValues(String min, String max, String base)
	{
        int min_num = Integer.parseInt(min);
        int max_num = Integer.parseInt(max);
        int base_num = Integer.parseInt(base);
		for(int i=min_num; i <= max_num; i++)
		{
			v.add(new Integer((int)Math.pow(base_num,i)));
		}
	}


}
