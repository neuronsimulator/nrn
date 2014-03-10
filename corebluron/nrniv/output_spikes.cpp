#include "corebluron/nrnconf.h"
#include "corebluron/nrniv/nrniv_decl.h"
#include "corebluron/nrniv/output_spikes.h"
#include "corebluron/nrnmpi/nrnmpi.h"

int spikevec_buffer_size;
int spikevec_size;
double* spikevec_time;
int* spikevec_gid;

void mk_spikevec_buffer(int sz) {
  spikevec_buffer_size = sz;
  spikevec_size = 0;
  spikevec_time = new double[sz];
  spikevec_gid = new int[sz];
}

void output_spikes() {
  char fname[100];
  sprintf(fname, "out%d.dat", nrnmpi_myid);
  FILE* f = fopen(fname, "w");
  for (int i=0; i < spikevec_size; ++i) {
    fprintf(f, "%g\t %d\n", spikevec_time[i], spikevec_gid[i]);
  }
  fclose(f);
}


void validation(std::vector<std::pair<double,int> >& res)
{
   for (int i=0; i < spikevec_size; ++i)
       res.push_back(std::make_pair(spikevec_time[i], spikevec_gid[i]));
}
