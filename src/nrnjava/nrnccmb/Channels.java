package nrnccmb;

import java.util.*;
import java.awt.*;
import catacomb.channel.*;
import catacomb.modeler.*;
import catacomb.core.*;
import neuron.*;

public class Channels implements DataListener {

	CcmbList chlist;
	Hashtable table;

	public Channels() {
		catacomb.modeler.checkInit();
		table = new Hashtable();
		String[] h = {"pore, level=6", "nRNPore, level=3"};
		chlist = new CcmbList("KSChannel");
		chlist.applyHints(h);
		chlist.addDataListener(this);
	}
		
	public int hocSessionPriority() {
		return 2000;
	}

	public char[] hocSessionSave(String sfname) {
		StringBuffer s;
		s = new StringBuffer(sfname); s.append(".ccm");
		save(s.toString());
		s = new StringBuffer("{ocbox_.map() ocbox_.load(\"");
		s.append(sfname).append(".ccm\")}\n");
//System.out.println("hocSessionSave" + s);
		return s.toString().toCharArray();
	}

	public void load(String fname) {
		Sys.setObject(chlist);
		Sys.importFile(fname);
	}

	public void save(String fname) {
		Sys.saveComponent(chlist, fname);
	}

	public void map() {
		if (Sys.interactor == null) {
			Sys.interactor = new catacomb.gui.GuiInteractor();
		}
		try {
			Sys.editObject(chlist);
			Sys.setMainProgram(false);
			Object o = Sys.getEditor(chlist);
			Frame f = (Frame)((catacomb.gui.ObjectViewer)o).getFrame();
			Neuron.windowListen(f, this);
		}catch (Exception e) {
			System.out.println(e);
			e.printStackTrace();
		}
	}

	public void sync_rates() {
		Enumeration e;
		for (e = table.elements(); e.hasMoreElements();) {
			((NrnKSChan)e.nextElement()).sync_rates(null);
		}
	}

	public String getName() { return "NrnCcmbChList"; }

	public void dataChange(DataEvent d) {
		if (d.type == d.STRUCTURE && d.srcVob == chlist) {
			int i, j;
			
			Enumeration e;
			for (e = table.elements(); e.hasMoreElements();) {
				((NrnKSChan)e.nextElement()).mark(0);
			}
			for (i=0; i < chlist.size(); ++i) {
				KSChannel c = (KSChannel)chlist.at(i);
				if (table.containsKey(c)) {
					((NrnKSChan)table.get(c)).mark(1);
				}else{
					table.put(c, new NrnKSChan(c));
//System.out.println("Adding " + c.getName());
				}
			}
			for (e = table.elements(); e.hasMoreElements();) {
				NrnKSChan nc = (NrnKSChan)e.nextElement();
				if (nc.mark() == 0) {
					table.remove(nc.ch_);
//System.out.println("Removing " + nc.ch_.getName());
				}else{
					nc.checkName();
				}
			}
		}
//		System.out.println(d);
//		for (int i = 0; i < chlist.size(); ++i) {
//			System.out.println("" + i+ " " + chlist.at(i));
//		}
	}
}
