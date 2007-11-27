package neuron;

/** Java reference to a NEURON hoc object.
	HocObjects are for receiving hoc Object arguments
	and passing them back as a return value.
	As such, it is intended for use in container classes.
	A HocObject is only created when it comes into a method
	as an argument. At that time the hoc Object* reference
	count is incremented and the Object* is cast to
	a long and stored in hocObjectCast.
	When the HocObject is garbage collected, the Object*
	reference count is decremented.
*/
public class HocObject {
	HocObject() {
		hocObjectCast = 0;
		id = 0;
		//System.out.println("HocObject construct");
	}
	long hocObjectCast;
	int id;
	/** Java can create a HocObject of a particular type
	*/
	protected void finalize() throws Throwable {
		try {
			if (hocObjectCast != 0) {
				Neuron.hocObjectUnref(hocObjectCast);
				//System.out.println("HocObject finalize " + hocObjectCast);
			}
		} finally {
			super.finalize();
		}
	}
	static void pushArgs(Object[] args) {
		for (int i=0; i < args.length; ++i) {
			pushArg(args[i]);
		}
	}
	static void pushArg(Object arg) {
		if (arg instanceof Double) {
			Neuron.pushArgD(((Double)arg).doubleValue());
		}else if (arg instanceof String) {
			Neuron.pushArgS((String)arg);
		}else if (arg instanceof Object) {
			Neuron.pushArgO(arg, Neuron.getObjectType(arg));
		}else{
			Neuron.pushArgO(null, 0);
		}
	}
	public long cppPointer() {
		// had better be a CPLUSPLUS type object.
		return Neuron.cppPointer(hocObjectCast);
	}
	long methodID(String name) {
		return Neuron.methodID(hocObjectCast, name);
	}
	public String toString() {
		return Neuron.hocObjectName(hocObjectCast);
	}
	public HocObject(String hocClassName) {
		id = 0;
		hocObjectCast = Neuron.getNewHObject(hocClassName, 0);
	}
	public HocObject(String hocClassName, Object[] args) {
		id = 0;
		pushArgs(args);
		hocObjectCast = Neuron.getNewHObject(hocClassName, args.length);
	}
	public double dMethod(String name, Object[] args) {
		pushArgs(args);
		return Neuron.hDoubleMethod(hocObjectCast, methodID(name), args.length);
	}
	public String sMethod(String name, Object[] args) {
		pushArgs(args);
		return Neuron.hCharsMethod(hocObjectCast, methodID(name), args.length);
	}
	public Object oMethod(String name, Object[] args) {
		pushArgs(args);
		return Neuron.hObjectMethod(hocObjectCast, methodID(name), args.length);
	}
// limited to scalars
	public double dGet(String name) {
		return new HocVariable(this.toString()+"."+name).getVal();
	}
	public String sGet(String name) {
		return Neuron.hGetStringField(hocObjectCast, name);
	}
	public Object oGet(String name) {
		return Neuron.hGetObjectField(hocObjectCast, name);
	}
	public void Set(String name, double x) {
		new HocVariable(this.toString()+"."+name).setVal(x);
	}
	public void Set(String name, String s) {
		Neuron.execute(this.toString()+"."+name+"=\""+s+"\"");
	}
	public void Set(String name, Object o) {
		if (o instanceof String) {
			Neuron.execute(this.toString()+"."+name+"=\""+o+"\"");
		}else{
			Neuron.hSetObjectField(hocObjectCast, name, o, Neuron.getObjectType(o));
		}
	}
}


