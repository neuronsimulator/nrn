#ifndef nonlinz_h
#define nonlinz_h

class NonLinImpRep;

class NonLinImp {
public:
	NonLinImp();
	virtual ~NonLinImp();
	void compute(double omega, double deltafac);
	double transfer_amp(int, int); // v_node[arg] is the node
	double input_amp(int);
	double transfer_phase(int, int);
	double input_phase(int);
	double ratio_amp(int clmploc, int vloc);
	
private:
	void solve(int curloc);
	NonLinImpRep* rep_;	
};
	
#endif
