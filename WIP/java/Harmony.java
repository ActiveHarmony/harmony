/*
 * Copyright 2003-2015 Jeffrey K. Hollingsworth
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
import java.util.*;


class Harmony {

	final int VAR_INT=0;
	final int VAR_STR=1;

    public native void Startup(int use_signals);
    public native void ApplicationSetup(String description);
    public native int AddVariableINT(String appName, String bundleName);
    public native String AddVariableSTR(String appName, String bundleName);
    public native void SetVariableINT(String appName, String bundleName, int currentvalue);
    public native void SetVariableSTR(String appName, String bundleName, String currentvalue);
    //public native void SetAll();
    public native int RequestVariableINT(String appName, String bundleName);
    public native String RequestVariableSTR(String appName, String bundleName);
    //public native void RequestAll();
    public native void PerformanceUpdate(int value);
    public native void End();

	private Vector v;
	public String app;
    

    static {
        System.loadLibrary("harmonyapi");
    }

	
    public Harmony(String appName) {

	app=new String(appName);
	v= new Vector();

    }

    public void AddVariable(HarmonyVariable hv, int type) {

	int i;
	HarmonyVariable vhv;

        for(i=0;i<v.size();i++) {

                vhv=(HarmonyVariable)v.elementAt(i);

                if (vhv.bundle.equals(hv.bundle)) 

			return;  // already there
	}

	hv.type=type;

	if(type==VAR_INT) 
		hv.INTvalue=AddVariableINT(app, hv.bundle);
	else // type==VAR_STR
		hv.STRvalue=AddVariableSTR(app, hv.bundle);

	v.addElement(hv);

	return; 

    }

    public void SetVariable(HarmonyVariable hv) {

/*
	int i;
	HarmonyVariable vhv=null;

        for(i=0;i<v.size();i++) {

                vhv=(HarmonyVariable)v.elementAt(i);

                if (vhv.bundle.equals(hv.bundle))

                        break;  // already there
        }

	if(i==v.size())
		return; //not found
*/

        if(hv.type==VAR_INT)

		SetVariableINT(app, hv.bundle, hv.INTvalue);

	else // VAR_STR

		SetVariableSTR(app, hv.bundle, hv.STRvalue);

	return;
    }

    public void SetAll() {

	int i;
	HarmonyVariable vhv;

	for(i=0;i<v.size();i++) {

		vhv=(HarmonyVariable)v.elementAt(i);
		SetVariable(vhv);

	}

	return;
    }


    public void RequestVariable(HarmonyVariable hv) {

/*
        int i;
        HarmonyVariable vhv=null;

        for(i=0;i<v.size();i++) {

                vhv=(HarmonyVariable)v.elementAt(i);

                if (vhv.bundle.equals(hv.bundle))

                        break;  // already there
        }

        if(i==v.size())
                return; //not found
*/
        if(hv.type==VAR_INT)

                hv.INTvalue=RequestVariableINT(app, hv.bundle);

        else // VAR_STR

		hv.STRvalue=RequestVariableSTR(app,hv.bundle);

	return;
}

    public void RequestAll() {

        int i;
        HarmonyVariable vhv;

        for(i=0;i<v.size();i++) {

                vhv=(HarmonyVariable)v.elementAt(i);
                RequestVariable(vhv);

        }

        return;
    }



    public void displayVector() {

	int j;
	HarmonyVariable hv;

	for(j=0;j<v.size();j++) {
	
		hv=(HarmonyVariable)v.elementAt(j);

		System.out.println(">>elementAt "+j+"="+hv.INTvalue);
	}
    }


/*
  
    public void Application_Setup(String description) {

	APIApplicationSetup(description);
	return;

    }

*/

/*
    public static void main(String[] args) {
        HelloWorld HC=new HelloWorld();
	int i=HC.displayHelloWorld();
	System.out.println("i="+i);
    }
*/

}

