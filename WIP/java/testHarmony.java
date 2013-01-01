/*
 * Copyright 2003-2013 Jeffrey K. Hollingsworth
 *
 * This file is part of Active Harmony.
 *
 * Active Harmony is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Active Harmony is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Active Harmony.  If not, see <http://www.gnu.org/licenses/>.
 */
import java.io.*;


class testHarmony {
	
	final static int VAR_INT=0;
	final static int VAR_STR=1;


    public static void main(String[] args) throws Exception{

  	final int MAX=20;
  	int y[]=new int [MAX];
  	int i;
  	int loop=100;

  	for (i=0;i<MAX;i++) {
        	y[i]=(i-9)*(i-9)*(i-9)-75*(i-9)+24 ;

        	//System.out.println("y["+i+"]="+y[i]);
  	}

	Harmony H=new Harmony("SimplexT");
	H.Startup(0);

	FileReader fr= new FileReader("simplext.tcl");
	BufferedReader br = new BufferedReader(fr);

	String inputLine;
	String tclscript = new String("");

        while ((inputLine = br.readLine()) != null)
            	tclscript=tclscript+inputLine;

	//System.out.println("["+tclscript+"]");

	br.close();
	fr.close();

	H.ApplicationSetup(tclscript);

	//int x;

	//x=H.testAddVariableINT("SimplexT","x");

	HarmonyVariable x=new HarmonyVariable("x");

	H.AddVariable(x,VAR_INT);

	//H.displayVector();

	//System.out.println("x="+x.INTvalue);


	for (i=0;i<loop;i++) {
  		H.PerformanceUpdate(y[x.INTvalue]);
  		H.RequestAll();
		//System.out.println(i+":[x="+x.INTvalue+"]");
	}



	H.End();



    }


}

