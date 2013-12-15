int main_ring(int argc, char** argv, char** env, std::string & path){
  std::string name("bbcore_mech.dat");
  name = path+name;
  nrnmpi_init(1, &argc, &argv);
  mk_mech(name.c_str());
  mk_netcvode();
  nrn_setup(1, path.c_str());
  t = 0;
  dt = 0.025;
  double mindelay = BBS_netpar_mindelay(10.0);
  printf("mindelay = %g\n", mindelay);
  mk_spikevec_buffer(10000);
  nrn_finitialize(1, -65.0);
  BBS_netpar_solve(100.);

  return 0;
}

void modl_reg() {
	// not right place, but plays role of nrnivmodl constructed
	// mod_func.c.
}
