/*
 * Copyright 2003-2011 Jeffrey K. Hollingsworth
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
import java.util.*;



public class Process_hosts
{

    public static void main(String[] args) throws Exception
    {
        BufferedReader stream = new BufferedReader(new FileReader(new File(args[0])));

        String line = stream.readLine();
	
        while(line != null ) {
	  StringTokenizer strtok = new StringTokenizer(line," ");
	  String machine_name=strtok.nextToken();	
	  int num_instances=Integer.parseInt(strtok.nextToken());
	  String path = strtok.nextToken();
	  for(int i=1; i<= num_instances; i++){
	    System.out.println(machine_name + " " + machine_name + "_" + i + " " + path);
	  }
	  
	  line = stream.readLine();
	  
        }

        //System.out.println("Unique number of elements: " + data.size());



    }
}
