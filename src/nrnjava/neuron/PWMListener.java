package neuron;

//import java.awt.event.*;
import java.awt.event.WindowAdapter;
import java.awt.event.ComponentListener;
import java.awt.event.WindowEvent;
import java.awt.event.ComponentEvent;
import java.awt.Frame;
import java.awt.Rectangle;

class PWMListener extends WindowAdapter implements ComponentListener {
	PWMListener(Frame w, long hocJavaWindowCast) {
//		System.out.println("PWMListener for " + w.getTitle());
		jwc = hocJavaWindowCast;
	}
	public void windowClosing(WindowEvent e) {
//		System.out.println(e);
		Neuron.pwmEvent(jwc, 1, 0, 0, 0, 0);
	}
	public void windowIconified(WindowEvent e) {
//		System.out.println(e);
		Frame f = (Frame)e.getWindow();
		f.setState(Frame.NORMAL);
		Neuron.setwin(jwc, 2, 0, 0);
		Neuron.pwmEvent(jwc, 3, 0, 0, 0, 0);
	}
	public void componentHidden(ComponentEvent e) {
		doit(3, e);
	}
	public void componentMoved(ComponentEvent e) {
		doit(4, e);
	}
	public void componentResized(ComponentEvent e) {
		doit(5, e);
	}
	public void componentShown(ComponentEvent e) {
		doit(6, e);
	}

	private long jwc; // cast of the pwm pointer to the corresponding JavaWindow rep.

	private void doit(int type, ComponentEvent e) {
//		System.out.println(e);
		Rectangle r = e.getComponent().getBounds();
		if (!e.getComponent().isShowing()) {
			type = 3;
		}
		Neuron.pwmEvent(jwc, type, r.x, r.y, r.width, r.height);
	}
}
