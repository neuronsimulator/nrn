#ifndef nrn_ansi_h
#define nrn_ansi_h

#if defined(__cplusplus)
extern "C" {
#endif

extern void recalc_diam(void);
extern void nrn_pt3dclear(Section*);
extern void nrn_length_change(Section*, double);
extern void stor_pt3d(Section*, double x, double y, double z, double d);

#if defined(__cplusplus)
}
#endif

#endif
