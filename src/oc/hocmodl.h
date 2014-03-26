extern void hoc_model(), hoc_initmodel(), hoc_terminal(), hoc_prconst();
extern int *hoc_pindepindex;

static VoidFunc function[] = {
"model", hoc_model,
"initmodel", hoc_initmodel,
"terminal", hoc_terminal,
0,0
};

static struct {		/* Integer Scalars */
	char 	*name;
	int	*pint;
} scint[] = {
0, 0
};

static struct {		/* Vector integers */
	char 	*name;
	int	*pint;
	int	index1;
} vint[] = {
0,0,0
};

static DoubScal scdoub[] = {
	0,0
};

static DoubVec vdoub[] = {
	0,0,0
};

static struct {		/* Arrays */
	char 	*name;
	double	*pdoub;
	int	index1;
	int 	index2;
} ardoub[] = {
0, 0, 0, 0
};

static struct {		/* triple dimensioned arrays */
	char 	*name;
	double	*pdoub;
	int	index1;
	int 	index2;
	int	index3;
} thredim[] = {
0, 0, 0, 0, 0
};
