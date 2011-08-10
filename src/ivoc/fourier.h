#ifndef fourier_h
#define fourier_h

void nrn_convlv(double* data, unsigned long n, double* respns, unsigned long m,
	    int isign, double* ans);
void nrn_correl(double* data1, double* data2, unsigned long n, double* ans);
void nrn_spctrm(double* data, double* psd, int setsize, int numsegpairs);

void nrngsl_realft(double* data, unsigned long n, int isign);
void nrn_gsl2nrc(double* gsl, double* nrc, unsigned long n);
void nrn_nrc2gsl(double* nrc, double* gsl, unsigned long n);
#endif
