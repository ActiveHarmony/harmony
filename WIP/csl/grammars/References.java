
package org.umd.periCSL;

public abstract class References {
	protected String name=null;
	private int type = -1;

    public abstract String getName();
    public abstract boolean equals(Object o);
    public abstract int hashCode();
}
