#ifndef nonlinz_h
#define nonlinz_h

class NonLinImpRep;

// A solve for non linear equation of complex numbers.
// Matrix should be squared.
class NonLinImp final {
  public:
    ~NonLinImp();
    // Prepare the matrix before solving it.
    void compute(double omega, double deltafac, int maxiter);
    double transfer_amp(int curloc, int vloc);  // v_node[arg] is the node
    double transfer_phase(int curloc, int vloc);
    double input_amp(int curloc);
    double input_phase(int curloc);
    double ratio_amp(int clmploc, int vloc);
    int solve(int curloc);

  private:
    NonLinImpRep* rep_{};
};

#endif
