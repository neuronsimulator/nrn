package neuron;

import java.net.*;
import java.io.File;

public class NrnClassLoader extends URLClassLoader {

	private NrnClassLoader() {
		super(new URL[0], ClassLoader.getSystemClassLoader());
//		System.out.println("new NrnClassLoader instance " + this);
	}

	public static void add(String s) throws java.net.MalformedURLException {
		if (s.length() == 0) { return; }
//		System.out.println("NrnClassLoader.add " + s);
		ncl.addURL(new URL(s));		
	}

	public static Class load(String s) throws Exception {
		Class c = ncl.loadClass(s);
//		System.out.println(c + " loaded by NrnClassLoader");
		return c;
	}

	public static void path() {
		URL[] urls = ncl.getURLs();
		for (int i=0; i < urls.length; ++i) {
			System.out.println(urls[i]);
		}
	}		

	public static void add_jars(String path) throws Exception {
		File dir = new File(path);
		File[] jars = dir.listFiles();
		for (int i=0; i < jars.length; ++i) {
			String n = jars[i].getName();
if (n.substring(n.length() - 4).equalsIgnoreCase(".jar")) {
			ncl.addURL(jars[i].toURL());
}
		}
	}

	static private NrnClassLoader ncl;

	static {
	    try {
		ncl = new NrnClassLoader();
		String cp = System.getProperty("java.class.path");
		String sep = System.getProperty("path.separator");
		cp = cp.substring(0, cp.indexOf("nrnclsld.jar"));
		int i = cp.lastIndexOf("sep");
		if (i >= 0) {
			cp = cp.substring(i);
		}
		System.out.println("path to jars " + cp);
		add_jars(cp);
//		ncl.add("file:/home/hines/nrn5/src/nrnjava/neuron.jar");
//		System.out.println("NrnClassLoader ready");
	    }catch (Exception e) {
		System.out.println(e);
	    }
	}
}
