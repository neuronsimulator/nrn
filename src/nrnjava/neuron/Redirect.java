package neuron;

import java.io.*;

/** Mac and mswin need stdout and stderr redirected to the console.
*/

public class Redirect {
	static private PrintStream stdout;
	static private PrintStream stderr;

	static {
		try {
			stdout = new PrintStream(new ConsoleStream(1));
			stderr = new PrintStream(new ConsoleStream(2));
			System.setOut(stdout);
			System.setErr(stderr);
		}catch (Exception e) {
			System.out.println(e);
			System.exit(1);
		}
	}
	static native void consoleWrite(int fd, int b);
}

// for now just put it in files til things work right

class ConsoleStream extends OutputStream {
	private int fd;

	public ConsoleStream(int i) {
		fd= i;		
	}
	public void write(int b) {
		Redirect.consoleWrite(fd, b);
	}
}


/*
class ConsoleStream extends OutputStream {
	private int fd;

	public ConsoleStream(int i) {
		fd= i;		
	}

	public void close() {
		try { if (!closed) {
			stream.close();
			closed = true;
		}}catch (Exception e) {
		}
	}
	public void flush() {
		try { if (!closed) {
			stream.flush();
		}}catch (Exception e) {
		}
	}
	public void write(int b) throws IOException {
		try {if (!closed) {
			stream.write(b);
		}}catch (Exception e) {
		}
	}
		
	private static FileOutputStream stream;
	private static boolean closed;
	static {
		try {
			stream = new FileOutputStream("Redirect.out");
			closed = false;
		}catch(Exception e) {
			System.exit(1);
		}
	}
}
*/
