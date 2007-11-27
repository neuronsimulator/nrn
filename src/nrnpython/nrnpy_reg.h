extern "C" {
extern myPyMODINIT_FUNC nrnpy_hoc();
extern myPyMODINIT_FUNC nrnpy_nrn();

static myPyMODINIT_FUNC (*nrnpy_reg_[])() = {
	nrnpy_hoc,
	nrnpy_nrn,
	0
};

}
