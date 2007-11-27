#ifndef fourier_h
#define fourier_h

void four1(double data[], unsigned long nn, int isign);
void twofft(double data1[], double data2[], double fft1[], double fft2[],
	    unsigned long n);
void realft(double data[], unsigned long n, int isign);
void convlv(double data[], unsigned long n, double respns[], unsigned long m,
	    int isign, double ans[]);
void correl(double data1[], double data2[], unsigned long n, double ans[]);
void spctrm(double data[], double p[], int m, int k);

#endif
