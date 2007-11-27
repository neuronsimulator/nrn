package nrnccmb;

import java.awt.*;
import catacomb.channel.*;
import catacomb.core.*;
import catacomb.standard.*;
import neuron.*;

public class NrnKSChan implements DataListener{

	public KSChannel ch_;

	public NrnKSChan(KSChannel chan) {
		ch_ = chan;
		mark_ = 1;
		name_ = ch_.getName();
		structure_ = new HocVector(100);
		f_ = new HocVector(10);
		rate_ = new HocVector(5);
		ho_ = new HocObject("KSChan");
		ho_.dMethod("setname", new Object[]{name_});
		ccmb2nrn();
		ch_.addDataListener(this);
	}

	public void mark(int i) {
		mark_ = i;
	}
	public int mark() { return mark_; }
	public void checkName() {
		if (name_ != ch_.getName()) {
//System.out.println("Change name from " + name_ + " to " + ch_.getName());
			name_ = ch_.getName();
			ho_.dMethod("setname", new Object[]{name_});
			ccmb2nrn();
		}
	}

	public String getName() { return name_; }

	public void dataChange(DataEvent d) {
//		System.out.println("NrnKSChan dataChange for " + name_ + ": "+d);
		if (d.getSource() instanceof CcmbLink) {
			data = ch_.getData();
			ratevalues(data);
		}else{
			System.out.println(d);
			ccmb2nrn();			
		}
	}
	
	public void ccmb2nrn() {
		data = ch_.getData();
//		System.out.println("ccmb2nrn " + data);
//		catacomb.core.U.dumpArrays(data);
		structure(data);
		snames(data);
		ligand_names(data);
		ratevalues(data);
	}

	private void structure(Object[] data) {
		int len, ntrans, nvtrans, nstate, nligand, i, j, k, ia[];
		k = 0;
		len = 0;
		// determine size
		nstate = 0;
		ntrans = 0;
		nligand = 0;
		nvtrans = 0;
		ngate_ = ((int[])data[1])[0];
		int indx[] = new int[ngate_];
		for (i = 0; i < ngate_; ++i) {
			indx[i] = nstate;
			Object gc[] = (Object[])data[2+i];
			ia = (int[])gc[0];
			nstate += ia[0];
			ntrans += ia[1];
			if (gc[5] != null) {
				nligand += ((String[])gc[5]).length;
//System.out.println("for gate " + i + " nligand=" + nligand);
			}
			for (j=0; j < ia[1]; ++j) {
				int ta[] = ((int[][])gc[1])[j];
				if (ta[0] != 3) { ++nvtrans; }
			}
		}
		double st[] = new double[3*ngate_ + 4*ntrans + nstate + 6];
		st[k++] = ((int[])data[1])[1]; // conductance model
		st[k++] = ngate_;
		st[k++] = nstate;
		st[k++] = ntrans;
		st[k++] = nligand;
		st[k++] = nvtrans;
		nstate = 0;
		// index for complex, number of states in complex,
		// power, then, for each state in complex,, its fractional
		// conductance
		for (i = 0; i < ngate_; ++i) {
			st[k++] = indx[i];
			Object gc[] = (Object[])data[2+i];
			ia = (int[])gc[0];
			st[k++] = ia[0];
			st[k++] = ia[2];
			double f[] = (double[])gc[3];
			for (j=0; j < ia[0]; ++j) {
				st[k++] = f[j];
			}
		}
		// for each transition, the src, target, type, and ligand index
		// but only vtrans first
		nligand = 0;
		for (i = 0; i < ngate_; ++i) {
			Object gc[] = (Object[])data[2+i];
			ia = (int[])gc[0];
			for (j=0; j < ia[1]; ++j) {
				int ta[] = ((int[][])gc[1])[j];
				if (ta[0] == 3) { continue; }
				st[k++] = indx[i] + ta[1];
				st[k++] = indx[i] + ta[2];
				st[k++] = ta[0];
				st[k++] = nligand + ta[4];
			}
			if (gc[5] != null) {
				nligand += ((String[])gc[5]).length;
			}
		}
		// now the ligand ones
		nligand = 0;
		for (i = 0; i < ngate_; ++i) {
			Object gc[] = (Object[])data[2+i];
			ia = (int[])gc[0];
			for (j=0; j < ia[1]; ++j) {
				int ta[] = ((int[][])gc[1])[j];
				if (ta[0] != 3) { continue; }
				st[k++] = indx[i] + ta[1];
				st[k++] = indx[i] + ta[2];
				st[k++] = ta[0];
				st[k++] = nligand + ta[4];
			}
			if (gc[5] != null) {
				nligand += ((String[])gc[5]).length;
			}
		}
//System.out.println("structure_ ");
//for (i=0; i < st.length; ++i) {
//	System.out.println(i + " " +st[i]);
//}
		structure_.toHoc(st);
		ho_.dMethod("setstructure", new Object[]{structure_});
		ho_.dMethod("setion", new Object[]{
			((String[])data[0])[1],
			new Double(((int[])data[1])[2])
		});
	}
				
	private void snames(Object[] data) {
		int i, j, ntot, n;
		ntot = 0;
		for (i=0; i < ngate_; ++i) {
			Object gc[] = (Object[])data[2+i];
			n = ((int[])gc[0])[0];
			String s[] = (String[])gc[4];
			for (j=0; j < n; ++j) {
				ho_.dMethod("setsname", new Object[]{
					new Double(ntot), s[j]});
				++ntot;
			}
		}
	}

	private void ligand_names(Object[] data) {
		int i, j, ntot, n;
		ntot = 0;
		for (i=0; i < ngate_; ++i) {
			Object gc[] = (Object[])data[2+i];
			if (gc[5] != null) {
				String s[] = (String[])gc[5];
				for (j=0; j < s.length; ++j) {
					ho_.dMethod("setligand", new Object[]{
						new Double(ntot), s[j]});
					++ntot;
				}
			}
		}
	}

	private void ratevalues(Object[] data) {
		int i, j, ntot, n;
		ntot = 0;
		for (i=0; i < ngate_; ++i) {
			Object gc[] = (Object[])data[2+i];
			n = ((int[])gc[0])[1];
			double rat[][] = (double[][])gc[2];
			for (j=0; j < n; ++j) {
				rate_.toHoc(rat[j]);
				ho_.dMethod("setrate", new Object[]{
					new Double(ntot), rate_});
				++ntot;
			}
		}
	}

	public void sync_rates(HocObject h) {
		if (h != null && h != ho_) { return; }
		int i, j, ntot, n;
		ntot = 0;
		for (i=0; i < ngate_; ++i) {
			Object gc[] = (Object[])data[2+i];
			n = ((int[])gc[0])[1];
			double rat[][] = (double[][])gc[2];
			for (j=0; j < n; ++j) {
				ho_.dMethod("getrate", new Object[]{
					new Double(ntot), rate_});
				rat[j] = rate_.fromHoc();
				++ntot;
//System.out.println("sync_rates ");
//for (int k=0; k < rat[j].length; ++k) {
//	System.out.println(k + " " +rat[j][k]);
//}
			}
		}
		ch_.setData(data);
	}

	private Object[] data; 
	private int ngate_;
	private int mark_;
	private String name_;
	private HocObject ho_;
	private HocVector structure_;
	private HocVector f_;
	private HocVector rate_;
}
