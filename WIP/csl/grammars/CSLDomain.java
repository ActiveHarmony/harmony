
package org.umd.periCSL;

import java.util.Vector;


// Extend this class if we want to add more data-types/domain to our
// language.
public abstract class CSLDomain {
	public Vector v = null;
	int type = -1;

	public abstract String toString();
	public abstract Vector getValues();
	public abstract void fillValues(String min, String max, String step);
    public abstract void fillPowerValues(String min, String max, String base);
	public abstract int getType();

}
