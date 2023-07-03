#ifndef nonlinz_h
#define nonlinz_h

class NonLinImpRep;

class NonLinImp {
  public:
    NonLinImp();
    virtual ~NonLinImp();
    void compute(double omega, double deltafac, int maxiter);
    double transfer_amp(int curloc, int vloc);  // v_node[arg] is the node
    double transfer_phase(int curloc, int vloc);
    double input_amp(int curloc);
    double input_phase(int curloc);
    double ratio_amp(int clmploc, int vloc);
    int solve(int curloc);

  private:
    NonLinImpRep* rep_;
};

#endif
