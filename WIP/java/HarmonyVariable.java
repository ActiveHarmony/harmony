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

class HarmonyVariable {

	//final int VAR_INT=0;
	//final int VAR_STR=1;


	public int INTvalue;
	public String STRvalue;
	//public String app;
	public String bundle;
	public int type;

	public HarmonyVariable (String bundleName) {


		//this.INTvalue=value;
		//this.type=VAR_INT;
		this.STRvalue=new String("");
		//this.app=new String("");
		this.bundle= new String(bundleName);
	}

}
