/* /local/src/master/nrn/src/nrnoc/cabvars.h,v 1.5 1999/02/05 18:09:50 hines Exp */
#pragma once
#define XMECH 0


static struct { /* USERPROPERTY */
    const char* name;
    short type;
    short index;
} usrprop[] = {{"nseg", 0, 0}, {"L", 1, 2}, {"rallbranch", 1, 4}, {"Ra", 1, 7}, {nullptr, 0, 0}};

extern "C" {
void capac_reg_(), passive0_reg_(), _passive_reg_(),
#if EXTRACELLULAR
    extracell_reg_(),
#endif
    _stim_reg_(), _syn_reg_(), _expsyn_reg_(), _exp2syn_reg_(), _svclmp_reg_(), _vclmp_reg_(),
    _oclmp_reg_(), _apcount_reg_(), _netstim_reg_(), _intfire1_reg_(), _intfire2_reg_(),
    _intfire4_reg_(), _ppmark_reg_(), _pattern_reg_(),
#if XMECH
    _xmech_reg_(),
#endif
    _feature_reg_(), _hh_reg_();

static Pfrv mechanism[] = {/* type will start at 3 */
                           capac_reg_,
                           _passive_reg_,
#if EXTRACELLULAR
                           /* extracellular requires special handling and must be type 5 */
                           extracell_reg_,
#endif
                           passive0_reg_, /* twice as fast as model description? */
                           _stim_reg_,
                           _syn_reg_,
                           _expsyn_reg_,
                           _exp2syn_reg_,
                           _svclmp_reg_,
                           _vclmp_reg_,
                           _oclmp_reg_,
                           _apcount_reg_,
                           _hh_reg_,
                           _feature_reg_,
                           _netstim_reg_,
                           _intfire1_reg_,
                           _intfire2_reg_,
                           _intfire4_reg_,
                           _ppmark_reg_,
                           _pattern_reg_,
#if XMECH
                           _xmech_reg_,
#endif
                           0};
}  // extern "C"

static const char* morph_mech[] = {
    /* this is type 2 */
    "0",
    "morphology",
    "diam",
    0,
    0,
    0,
};

extern void cab_alloc(Prop*);
extern void morph_alloc(Prop*);

#if 0
 first two memb_func
	NULL_CUR, NULL_ALLOC, NULL_STATE, NULL_INITIALIZE, (Pfri)0, 0,	/*Unused*/
	NULL_CUR, cab_alloc, NULL_STATE, NULL_INITIALIZE, (Pfri)0, 0,	/*CABLESECTION*/
#endif


extern std::vector<Memb_func> memb_func;
