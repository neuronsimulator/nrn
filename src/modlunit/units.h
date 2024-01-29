#define NDIM 10
struct unit {
    double factor;
#if - 1 == '\377'
    char dim[NDIM];
#else
    signed char dim[NDIM];
#endif
    int isnum;
};

extern const char* Unit_str(unit*);
extern int unit_diff();
extern int unit_cmp_exact();
