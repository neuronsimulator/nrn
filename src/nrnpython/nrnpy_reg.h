extern PyObject* nrnpy_hoc();
extern PyObject* nrnpy_nrn();

static PyObject* (*nrnpy_reg_[])() = {nrnpy_hoc, nrnpy_nrn, 0};
