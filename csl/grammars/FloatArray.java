package org.umd.periCSL;

import java.util.Vector;
// Extends CSLDomain class to represent and float array defined in the
// language
public class FloatArray extends CSLDomain
{

	public FloatArray()
	{
		v = new Vector<Float>();
		type=Types.INT;
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

	public float getValueAt(int index)
	{
		return (Float)v.get(index);
	}

	public void pushValue(float value)
	{
		v.add(new Float(value));
	}

	public void fillValues(float min, float max, float step)
	{
		for(float i=min; i <= max; i+=step)
		{
			v.add(new Float(i));
		}
	}

    // acts like a built-in range function -- if the target language does
    // not have range construct, simply grab the values from the internal
    // representation
    public void fillValues(String min, String max, String step)
	{
        float min_num = Float.parseFloat(min);
        float max_num = Float.parseFloat(max);
        float step_num = Float.parseFloat(step);
		for(float i=min_num; i <= max_num; i+=step_num)
		{
			v.add(new Float(i));
		}
	}

    // power range
    public void fillPowerValues(String min, String max, String base)
	{
        float min_num = Float.parseFloat(min);
        float max_num = Float.parseFloat(max);
        float base_num = Float.parseFloat(base);;
		for(float i=min_num; i <= max_num; i++) 
        {
			v.add(new Float(Math.pow(base_num,i)));
		}
	}

}
