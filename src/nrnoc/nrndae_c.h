#ifndef nrndae_c_h
#define nrndae_c_h

#if defined(__cplusplus)
extern "C" {
#endif

extern void nrndae_alloc(void);
extern int nrndae_extra_eqn_count(void);
extern void nrndae_init(void);
extern void nrndae_rhs(void); /* relative to c*dy/dt = -g*y + b */
extern void nrndae_lhs(void);
extern void nrndae_dkmap(double**, double**);
extern void nrndae_dkres(double*, double*, double*);
extern void nrndae_dkpsol(double);
extern void nrndae_update(void);
extern void nrn_matrix_node_free(void);
extern int nrndae_list_is_empty(void);

extern int nrn_use_daspk_;

#if defined(__cplusplus)
}
#endif

#endif
