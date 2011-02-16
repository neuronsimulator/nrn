#ifndef fourier_h
#define fourier_h

void nrn_convlv(double* data, unsigned long n, double* respns, unsigned long m,
	    int isign, double* ans);
void nrn_correl(double* data1, double* data2, unsigned long n, double* ans);
void nrn_spctrm(double* data, double* p, int m, int k);

void nrngsl_realft(double* data, unsigned long n, int isign);
#endif
