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
