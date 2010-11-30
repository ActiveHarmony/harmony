
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
