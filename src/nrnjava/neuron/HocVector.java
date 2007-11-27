package neuron;

/** Java reference to a NEURON Vector object.
*/
public class HocVector extends HocObject {
	public HocVector(int size) {
		super();
		super.id = 1; // the index in Neuron.hoClasses
		super.hocObjectCast = Neuron.vectorNew(size);
	}
	// used only for passing a Vector arg from hoc to java
	HocVector() {
		//System.out.println("HocVector construct");
	}
	
	public int size() {
		return Neuron.vectorSize(hocObjectCast);
	}

	public void set(int i, double x) {
		Neuron.vectorSet(hocObjectCast, i, x);
	}

	public double get(int i) {
		return Neuron.vectorGet(hocObjectCast, i);
	}

	public double[] fromHoc() { // too bad about the copying
		return Neuron.vectorFromHoc(hocObjectCast);
	}

	public void toHoc(double[] v) { // too bad about the copying
		Neuron.vectorToHoc(hocObjectCast, v, v.length);
	}
}
