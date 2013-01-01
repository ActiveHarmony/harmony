package org.umd.periCSL;

// Types
public class Types {
	public static final int INT = 1;
	public static final int FLOAT = 2;
	public static final int BOOLEAN = 3;
	public static final int INTARRAY = 4;
	public static final int STRING=5;
    public static final int MIXED=6;

    public static String getType(int type) 
    {
        if(type==INT)
        {
            return "int";
        }
        if(type==FLOAT)
        {
            return "float";
        }

        if(type==BOOLEAN)
        {
            return "boolean";
        }

        if(type==STRING)
        {
            return "string";
        }

         if(type==MIXED)
        {
            return "mixed";
        }

         return "UNKNOWN";
    }

}
