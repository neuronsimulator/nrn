/** Neuron / Java interface
 * @author Fred Howell
 * @date 1 March 2001
 */

package neuron;

import java.util.Vector;
import java.util.Hashtable;
import java.lang.reflect.*;
import java.awt.Frame;
import java.awt.Point;
import java.net.*;

/** Info about a hoc-callable method ------
 */
class MethodInfo {
    String signature;
    Method m; 
    char returnType;
    MethodInfo(Method m) {
	this.m = m;
	signature = "";
	Class type = m.getReturnType();
	if (type.equals(double.class)) {
		returnType = 'd';
	}else if (type.equals(int.class)) {
		returnType = 'i';
	}else if (type.equals(boolean.class)) {
		returnType = 'b';
	}else if (type.equals(void.class)) {
		returnType = 'v';
	}else if (type.equals(char[].class)) {
		returnType = 's';
	}else if (type.getName().indexOf('.') != -1) {
		// don't care what the type is at this point
		returnType = 'o';
	}else{
		returnType = '\0';
	}
    }

    Method getMethod() { return m; }

}


/** Info about a java-accessible hoc class
 */
class ClassInfo {
    Class c;
    int id; // the index of this ClassInfo in Neuron.classList
    boolean pureStatic;
    //constructorSignatures
    Vector cons = new Vector();	// constructors that have args.
    Vector conSig = new Vector(); // String java type signatures that hoc understands.
    Vector hocSig = new Vector(); // String hoc type signatures, only dso.
    Vector methods = new Vector(); // references to MethodInfo    
    ClassInfo( Class c, int id ) {
	this.c = c;
	this.id = id;
	Constructor[] constructors = c.getConstructors();
	int len = constructors.length;
	pureStatic = (len == 0);
	for (int i = 0; i < len; ++i) {
		Class[] pt = constructors[i].getParameterTypes();
		if (pt.length == 0) {
			continue; //don't put in list
		}
		String sig = hocSignature(pt);
		if (sig != null) { //put in list
			cons.addElement(constructors[i]);
			conSig.addElement(sig);
			String hs = "";
			for (int j = 0; j < sig.length(); ++j) {
				char a = sig.charAt(j);
				switch(a) {
				case 'i':
				case 'b':
					hs = hs + 'd';
					break;
				default:
					hs = hs + a;
				}
			}
			hocSig.addElement(hs);
		}
	}
//	System.out.println(c.getName() + ": pureStatic is " + pureStatic);
    }
    void addMethodInfo( MethodInfo mi ) {
	methods.addElement( mi );
    }
    MethodInfo getMethodInfo( int i ) {
	return (MethodInfo) methods.elementAt(i);
    }
    int getConstructor(String sig) {
	int len = cons.size();
	for (int i=0; i < len; ++i) {
//System.out.println("getConstructor comparing " + sig + " " + (String)hocSig.get(i));
		if (sig.equals((String)hocSig.elementAt(i))) {
			return i;
		}
	}
	return -1;
    }
    static String hocSignature(Class[] pt) {
	String sig = "";
	for (int k = 0; k < pt.length; k ++) {
		Class s = pt[k];
		if (s.equals(double.class)) {
			sig += "d";
		}else if (s.equals(int.class)) {
			sig += "i";
		}else if (s.equals(boolean.class)) {
			sig += "b";
		}else if (s.equals(char[].class)) {
			sig += "s";
		}else if (s.equals(String.class)) {
			sig += "S";
		}else if (s.getName().indexOf('.') != -1) {
			sig += "o";
		}else{
			sig = null;
			break;
		}
	}
	return sig;
    }
}

/** Static class for interfacing Neuron to Java

The general case for arguments involves matching the actual signature
of a hoc call to a method with one of the (overloaded) method signatures.
This happens especially with constructors. We want to be
fast in the case of non-overloaded methods. Here is the sequence.
1)A method call is made in hoc to the proper c function returning a
double, strdef, or object.
2)The c function calls a java function with the index of the class (it
is of course a java class) and index of the method. The java function called
depends on whether the return type is supposed to be a double, strdef (char[]),
or object. (the latter case requires a good deal of extra effort).
3)Java gets all the args by calling a pair of jni functions. The first
returns the type (double, strdef, or object type) For the object type, the
index of the java class (if the hoc object contained a java object) is
returned. A negative index implies double, strdef, HocObject,
JavaObject, HocVector, etc.
HocObject means that Hoc understands it but it is opaque to Java. JavaObject
is a hoc object that contains a Java object handle of a type unregistered
to hoc.
The second jni function called is the one that returns a double,strdef,
or object. Based on the type return of the first jni call
the return value is cast to the proper object (e.g. Double, String, etc.)
4) If the method is not overloaded it is invoked. If overloaded the
signature types are compared and the right method is invoked.
(note: at this time, for overloaded methods, hoc also creates a unique
symbol of the form name%dargs where %d is the number of args and args
is a string containing diso)
5) If the return type desired is double or char[] then we return a double
or String to be converted to strdef. Object return types specifically
call a jni function that tells the c side how to interpret the hobject.
i.e. if it is a registered java object then we
know it's class id and can construct the proper hoc portion
of the object. If it is a HocObject then the return long value
can be cast to the hoc object pointer. If it is an unregistered java type
then a JavaObject can be constructed and the jobject saved in it.
 */
public class Neuron {   

//	static { System.out.println("loading Neuron");}

	static native String getHocStringArg( int i );

	static native double getHocDoubleArg( int i, int type );
	static native Object getHocObjectArg( int i, IllegalArgumentException e );

	// If we know the id we can find the class. If we know the class
	// we can find the id
	static Hashtable classInfo = new Hashtable();
	static Vector classList = new Vector();
    
	// hoc objects must be enclosed in a HocObject type or
	// a type that extends it. Here are the java types in an
	// order known by nrnjava.cpp:nj_encapsulate.
	static Vector hoClasses = new Vector();
	static private URLClassLoader ncl;
	static {
		try {
			hoClasses.addElement(NrnClassLoader.load("neuron.HocObject"));
			hoClasses.addElement(NrnClassLoader.load("neuron.HocVector"));
		}catch (Exception e) {
			System.out.println(e);
			e.printStackTrace();
		}
	}

	// A positive number or 0 is the class id.
	// Negative numbers refer to double, strdef, etc.
	// not currently used.
	static native int getHocArgType(int i);

	// for actual constructor signature call. Reasonably fast but
	// not very general since hoc knows the java types
	static native String getHocArgSig();

	static native void hocObjectUnref(long hocObjectCast);

	/** Create a hoc version of a class
	 */
	static native void java2nrnClass( 
	    String classname, int classindex, String methods);

	/** Execute a hoc statement
	returns 1 if no hoc error, 0 otherwise
	*/
	public static native boolean execute(String statement);

	/** For use by HocVariable
	*/
	static native long getVarPointer();
	static native double getVal(long hocVarPointerCast);
	static native void setVal(long hocVarPointerCast, double x);

	// for passing back the return type from callMethod
	static MethodInfo lastMi;

	/** Make a hoc class for a given Java class
		Check if the java class is already loaded
		If not, load it.
		Register it with hoc.
	*/
static public int makeHocClass(String className, String hocName, String path) {
//	System.out.println("Neuron: makeHocClass called "+className+"  "+hocName + " |"+path+"|");
	try {
		Class c = null;
		try {
			NrnClassLoader.add(path);
			c = NrnClassLoader.load(className);
		} catch	(Throwable e) {
			System.out.println(e);
			e.printStackTrace();
			return 0;
		}
		// Space separated list of method names
		// in the format "type name signature type name signature ..."
		// where type is d, s, or o and signature is a string of
		// d,i,s,o
		String methods="";

		ClassInfo ci = new ClassInfo(c, classList.size());
//		System.out.println("Neuron: makeHocClass getting methods for "+className);

		Method[] theMethods = c.getMethods();
		for (int i = 0; i < theMethods.length; i++) {
			String methodString = theMethods[i].getName();
		
//			System.out.println("Method :"+ methodString);

			// May be overloaded. Then some methodStrings are
			// repeated and hoc will note that fact and
			// only create one symbol for them and then
			// one mangled symbol for each that can be used
			// with no lookup overhead.
			MethodInfo mi = new MethodInfo( theMethods[i] );
			char rt = mi.returnType;
			String htype = null;
			switch (rt) {
			case 'd':
			case 'i':
			case 'b':
			case 'v':
				htype = "d";
				break;
			case 's':
				htype = "s";
				break;
			case 'o':
				htype = "o";
				break;
			default:
				break; // don't use
			}
			
			if (htype != null) {
				// however, only use if argtypes are ok.
				Class[] pt = theMethods[i].getParameterTypes();
				mi.signature = ClassInfo.hocSignature(pt);
				if (mi.signature == null) {
					htype = null;
				}
			}

			if (htype != null) {
				ci.addMethodInfo( mi );
				if (methods.length() > 0) {
					methods = methods + " ";
				}
				methods = methods + htype + " "
					+ methodString + " " + mi.signature;
			}
		}
		java2nrnClass( hocName, ci.id, methods);
		classInfo.put( c, ci );
		classList.addElement(ci);
	} catch (Throwable e) {
		System.out.println("Exception in makeHocClass "+e);
		e.printStackTrace();
		return 0;
	}
	return 1;
}
 
	static int identity(Object o1, Object o2) {
		return (o1 == o2) ? 1 : 0;
	}

	/* the real java class name for a java object */
	static String javaClassName(Object jo) {
		return jo.getClass().toString();
	}

/* Return a generic Java object given a class name
     */
    static Object constructNoArg(int cid) {
	try {
	    ClassInfo ci = (ClassInfo)classList.elementAt(cid);
	    if (ci.pureStatic) {
		return null;
	    }
	    Class c = ci.c;
//	    System.out.println(" Got class: "+c);
	    Object o = c.newInstance();
//	    System.out.println(" Got object: "+o);
	    return o;
	} catch (Exception e) {
	    System.out.println("Exception in constructNoArg "+e);
	    return new Integer(-1);
	}
    }

    /** Return args from hoc in an object array */
    static Object[] fillArgs(String signature) throws IllegalArgumentException{
//System.out.println("Trying to get args with Signature "+signature);
	int narg = signature.length();
	Object[] ret = new Object[narg];
	double d;
	for (int i=0; i<narg; i++) {
	    switch (signature.charAt(i)) {
		case 'd':
			d = Neuron.getHocDoubleArg(i+1, 0);
			if (d == -1e98) {throw badarg;}
			ret[i] = new Double( d );
			break;
		case 'i':
			d = Neuron.getHocDoubleArg(i+1, 1);
			if (d == -1e98) {throw badarg;}
			ret[i] = new Integer((int) d );
			break;
		case 'b':
			d = Neuron.getHocDoubleArg(i+1, 1);
			if (d == -1e98) {throw badarg;}
			ret[i] = new Boolean(d != 0);
			break;
		case 'S':
			ret[i] = Neuron.getHocStringArg(i+1);
			if (ret[i] == null) {throw badarg;}
			break;
		case 'o':
			ret[i] = Neuron.getHocObjectArg(i+1, badarg);
			if (ret[i] == badarg) {throw badarg;}
			break;
		default:
			ret[i] = null;
			System.out.println("fillArgs: arg "+i+" type "+signature.charAt(i)+
				   " not handled yet");
			throw badarg;
	    }
	}
	return ret;
    }
	private static IllegalArgumentException badarg;
	static {badarg = new IllegalArgumentException();}

    static Object constructWithArg(int cid) {
	try {
	    ClassInfo ci = (ClassInfo)classList.elementAt(cid);
	    Class c = ci.c;
	    Constructor con = null;
	    String sig; // signature;
	    int icon;
	    // if only one constructor be fast.
	    if (ci.cons.size() <= 1) {
		icon = 0;
	    }else{
		// get the (first!) constructor having the proper signature
		sig = getHocArgSig();
		icon = ci.getConstructor(sig);
	    }
	    con = (Constructor)ci.cons.elementAt(icon);
	    sig = (String)ci.conSig.elementAt(icon);

	    // get whatever args there are
	    int narg = sig.length();
	    Object[] args = fillArgs(sig);

//	    System.out.println(" Got class: "+c);
	    Object o = con.newInstance(args);
//	    System.out.println(" Got object: "+o);
	    return o;
	} catch (Exception e) {
	    System.out.println("Exception in constructWithArg "+e);
//		e.printStackTrace();
	    return new Integer(-1);
	}
    }
    /** Invoke a Java method returning Object
     */
    static Object callMethod( Object o, int classID, int methodID, int overloaded) {
	try {
	    ClassInfo ci  = (ClassInfo)classList.elementAt( classID );
	    if (ci==null) {
		System.out.println("Error: ClassInfo NULL in callMethod");
		return null;
	    }
	    // need to figure out which of the overloaded methods to use.
	    if (overloaded != 0) {
		// actual value of overload is overloaded-1 due to trick
		// used in src/oc/hoc_oop.c for s_varn.
System.out.println("don't know which of " + (overloaded-1) + " overloaded methods to use");
	    }
	    MethodInfo mi = ci.getMethodInfo( methodID );
	    if (mi==null) {
		System.out.println("Error: MethodInfo NULL in callMethod");
		return null;
	    }
	    Object[] args = fillArgs(mi.signature);
	    Object ret =  mi.getMethod().invoke(o, args);
	    // pass the return type back via
	    lastMi = mi;
	    return ret; 
	} catch (Exception e) {
	    System.out.println("Exception in callMethod "+e);
		e.printStackTrace();
		lastMi = null;
	    return new Integer(-1);
	}	    
    }

    /** Invoke a Java method returning double
     */
    static double invokeDoubleMethod( Object o, int classID, int methodID, int overloaded) {
	Object ret =  callMethod( o, classID, methodID, overloaded);
	if (lastMi == null) {
		return -1e98;
	}
	char rt = lastMi.returnType;
	switch (rt) {
	case 'd':
		return ((Double)ret).doubleValue();
	case 'b':
		return ((Boolean)ret).booleanValue() ? 1.0 : 0.0;
	case 'i':
		return ((Integer)ret).doubleValue();
	case 'v':
		return 1.0;
	}
	return -1e98;
    }    
    
    /** Invoke a Java method returning char[]
	We want this to end up being assignable to a strdef in hoc.
     */
    static String invokeCharsMethod( Object o, int classID, int methodID, int overloaded) {
	String s = null;
try {
	Object ob = callMethod( o, classID, methodID, overloaded);
	if (ob == null) {
		s = null;
	}else{
		s = new String((char[])ob);
	}
	if (lastMi == null) {
		return null;
	}
}catch (Exception e) {
	System.out.println("invokeCharsMethod exception " + e);
	s = null;
}
	return s;
    }

    /** Invoke a Java method returning any other kind of Object.
	We want this to end up being assignable to an objref.
	There are several cases.

	A HocObject is a java reference to a NEURON interpreter Object.
	Such objects make sense in Hoc but not in Java

	A JavaObject in Hoc is opaque to Hoc because the type was
	never registered.
	Such objects make sense in Java but not in Hoc, In Java these
	are the objects themselves but in Hoc they are encapsulated in
	a class called JavaObject.

	An object in java whose class has been registered with hoc makes
	sense in both Java and Hoc. (In hoc it is an object whose template
	has the subtype	JAVAOBJECT.
     */
    static Object invokeObjectMethod( Object o, int classID, int methodID, int overloaded)
	throws Exception
    {
	Object ro = callMethod( o, classID, methodID, overloaded);
	if (lastMi == null) {
		throw new Exception();
	}
	return ro;
    }
	static int getObjectType(Object o) {
		try {
//System.out.println("getObjectType " + o);
		Class c = o.getClass();
		ClassInfo ci = (ClassInfo)classInfo.get(c);
		if (ci == null) {
			// all encapsulated hoc objects extend HocObject
			return -((HocObject)o).id - 2;
		}
//System.out.println("getObjectType of class " + c + "id=" + ci.id);
		return ci.id;
		}catch (Exception e){
			// it's an unregistered java object
//			System.out.println("getObjectType " + ((o != null) ? o.toString() : "null") + " not registered to hoc");
		}
//System.out.println("getObjectType returning -1");
		return -1;
	}

    static Object encapsulateHocObject(long ho, int type) {
	HocObject o = null;
	try {
		Class c = (Class)hoClasses.elementAt(type);
		o = (HocObject)c.newInstance();
		o.hocObjectCast = ho;
		o.id = type;
//System.out.println("encapsulateHocObject " + ho + " " + type);
	}catch (Exception e) {
		System.out.println("encapsulateHocObject failed " + e);
		e.printStackTrace();
	}
	return o;
    }

	static public double dMethod(String name, Object[] args) {
		HocObject.pushArgs(args);
		return hDoubleMethod(name, args.length);
	}

static native long vectorNew(int size);
static native int vectorSize(long v);
static native void vectorSet(long v, int i, double x);
static native double vectorGet(long v, int i);
static native void vectorToHoc(long v, double[] p, int size);
static native double[] vectorFromHoc(long v);
static native long cppPointer(long v);

static native String hocObjectName(long v);
static native long getNewHObject(String name, int narg);
static native double hDoubleMethod(long v, long methodID, int narg);
static native double hDoubleMethod(String name, int narg);
static native String hCharsMethod(long v, long methodID, int narg);
static native Object hObjectMethod(long v, long methodID, int narg);
static native void pushArgD(double x);
static native void pushArgS(String s);
static native void pushArgO(Object o, int type); // from getObjectType
static native long methodID(long v, String method);

public static        double dGet(String name) {return new HocVariable(name).getVal();}
public static native String sGet(String name);
public static native Object oGet(String name);
public static        void dSet(String name, double x) {new HocVariable(name).setVal(x);}
public static        void sSet(String name, String s) {execute(name+"=\""+s+"\"");} 
public static        void oSet(String name, Object o) {hSetObjectField(name, o, getObjectType(o));}

static native String hGetStringField(long v, String name);
static native Object hGetObjectField(long v, String name);
static native void hSetObjectField(long v, String name, Object o, int type);
static native void hSetObjectField(String name, Object o, int type);

// see the hypotheses below
static Vector frames = new Vector(); //list of Frame objects
static Vector njwins = new Vector(); //list of JavaWindow casts of type Long

/** Register a java frame with the print and file window manager.
The print and file window manager will observe
the Close, iconify, resize, move, etc. events.
*/
    static public void windowListen(Frame w, Object o) {
	if (frames.contains(w)) {
		return;
	}
	long jwc = pwmListen(w.getTitle(), o, getObjectType(o));
//System.out.println("windowListen jwc=" + jwc + " " + w );
	PWMListener wl = new PWMListener(w, jwc);
	w.addWindowListener(wl);
	w.addComponentListener(wl);
	frames.addElement(w);
	njwins.addElement(new Long(jwc));
    }

/**
From http://java.sun.com/products/jdk/faq/jni-j2sdk-faq.html#windowid
14. How can I get the native window pointer of an AWT Window? 
At this time there is no documented or reliable way to do this. We are
working on providing this in a future release.

So how can the pwm change the location or show/hide of a window?
They run in different threads. The env for pwmEvent is different from
neuron_env and it is a
# HotSpot Virtual Machine Error, Internal Error
# Problematic Thread: prio=6 tid=0x284588 nid=0xb runnable
when neuron calls setwin (map) from a pwmEvent.
Futhermore, in the normal case when setwin is called from the
main thread, the Frame is a null object. That is in the native interface
wincast = (long)w; from pwmListen and then calling setwin after casting it
back with w = (hobject)wincast; then setwin says w is null.

Some experimentation shows that in the main thread case, keeping a
static ref of the Frame, then it is valid when setwin is called.
So my guess is that if we keep a list of Frame and long (latter
returned by pwmListen), then we can look up the Frame. Furthermore,
on the c++ side we have to keep the env on the pwmListen call so
the right env is used on the callback to setwin. Which, in turn, I suppose,
means a complete lookup of how to callback setwin... We'll see by
experiment what happens if we only allow a callback when the env
is nrnjava_env.
*/

static native long pwmListen(String title, Object o, int type);
static native void pwmEvent(long jwc, int type, int l, int t, int w, int h);

    static int setwin(long jwc, int type, int left, int top) { // or width,height
	try{
//	System.out.println("Neuron.setwin " + jwc + " "+ type +" " + left + " " + top);
	int cnt = njwins.size();
	for (int i = 0; i < cnt; ++i) {
		long j = ((Long)njwins.elementAt(i)).longValue();
//System.out.println("setwin comparing i=" + i + " " + j + " " + jwc);
		if (j == jwc) {
			Frame f = (Frame)frames.elementAt(i);
//			System.out.println(f);
//			System.out.println(f.getTitle());
			switch(type) {
			case 1: // map
//System.out.println("Show");
				f.show();
				break;
			case 2: // hide
//System.out.println("hide");
				f.hide();
				break;
			case 3: // move
//System.out.println("move " + left + " " + top);
				f.setLocation(new Point(left, top));
				break;
			case 4: // resize
//System.out.println("resize " + left + " " + top);
				f.setSize(left, top);
				break;
			}
		}
	}
	}catch(Exception e) {
		System.out.println(e);
		e.printStackTrace();
	}
	return 0;
    }
}
