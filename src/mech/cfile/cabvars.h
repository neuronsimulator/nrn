extern  void capac_reg_(void), _passive_reg_(void),
#if EXTRACELLULAR
	extracell_reg_(void),
#endif
	_stim_reg_(void),
	_hh_reg_(void),
	_expsyn_reg_(void);

static void (*mechanism[])(void) = { /* type will start at 3 */
	capac_reg_,
	_passive_reg_,
#if EXTRACELLULAR
	/* extracellular requires special handling and must be type 5 */
	extracell_reg_,
#endif
	_stim_reg_,
	_hh_reg_,
	_expsyn_reg_,
	0
};

static char *morph_mech[] = { /* this is type 2 */
	"0", "morphology", "diam", 0,0,0,
};

#if 0
extern void cab_alloc(void);
extern void morph_alloc(void);
#endif

#if 0
 first two memb_func
	NULL_CUR, NULL_ALLOC, NULL_STATE, NULL_INITIALIZE, (Pfri)0, 0,	/*Unused*/
	NULL_CUR, cab_alloc, NULL_STATE, NULL_INITIALIZE, (Pfri)0, 0,	/*CABLESECTION*/
#endif
