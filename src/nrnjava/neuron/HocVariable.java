package neuron;

/** Java reference to a NEURON hoc variable
	HocVariables are for fast evaluation and assignment of
	any variable in hoc.
	At this time there is no notification when a value has changed.
*/
public class HocVariable {
	public HocVariable(String varname) {
		if (Neuron.execute("{hoc_pointer_(&" + varname + ")}")) {
			hocVarPointerCast = Neuron.getVarPointer();
			//System.out.println("HocVariable: " + varname + " " + hocVarPointerCast);
			varname_ = varname;
		}else{
			varname_ = "Invalid_" + varname;
			System.out.println("HocVariable: Invalid variable name: " + varname);
			hocVarPointerCast = 0;
		}
	}
	public boolean valid() { return hocVarPointerCast != 0; }
	public String name() { return varname_; }
	public double getVal() {
		return valid() ? Neuron.getVal(hocVarPointerCast) : -1e98;
	}
	public double setVal(double x) {
		if (valid()) {
			Neuron.setVal(hocVarPointerCast, x);
			return x;
		}else{
			return -1e98;
		}
	}
	String varname_;
	long hocVarPointerCast;
}


