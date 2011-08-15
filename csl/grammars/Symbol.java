
package org.umd.periCSL;

public class Symbol {
	
	// constants
	public static final int INT = 1;
	public static final int FLOAT = 2;
	public static final int BOOL = 3;
	
	// attributes
	public String name;
	public int type;
	public Object value;
	
	public Symbol(String name_, int type_, Object value_) {
		this.name=name_;
		this.type=type_;
		this.value=value_;
	}
	
}
