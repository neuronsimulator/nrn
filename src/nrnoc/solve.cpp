#include <../../nrnconf.h>
/* /local/src/master/nrn/src/nrnoc/solve.cpp,v 1.15 1999/07/12 14:34:13 hines Exp */

/* solve.cpp 15-Dec-88 */

/* The data structures in section.h were developed primarily for the needs of
     solving and setting up the matrix equations reasonably efficiently in
     space and time. I am hypothesizing that they will also be convenient
     for accessing parameters.
     Important properties of the structures used here are:
	Each section must have at least one node. There may be 0 sections.
	The order for back substitution is given by section[i]->order.
	The order of last node to first node is used in triangularization.
	First node to last is used in back substitution.
*/
/* An equation is associated with each node. d and rhs are the diagonal and
   right hand side respectively.  a is the effect of this node on the parent
   node's equation.  b is the effect of the parent node on this node's
   equation.
   d is assumed to be non-zero.
*/

/* We have seen that it is best to have nodes generally denote the
   centers of segments because most properties are most easily defined
   at those points. The old problem then arises again of 2nd order correct
   demands that a point be exactly at any branches. For this reason we
   always allocate one extra node at the end with 0 length.  Sections
   that connect at x=1 connect to this node.  Sections that connect
   from 0<x<1 connect to nodes 0 to nnode-2 directly to the point
   (no parent resistance, only a half segment resistance). It was an error
   to try to connect a section to position 0.  Now we are going to allow
   that by the following artifice.
   
   Section 0 is always special. Its nodes act as roots to the independent
   trees.  They do not connect to each other. It has no properties.
*/

#if EXTRACELLULAR
/* Two users (Padjen and Shrager) require that the extracellular potential
be simulated.  To do this we introduce a new node structure into the
section called extnode.  This points to a vector of extnodes containing
the extracellular potential, the Node vector
is interpreted as the internal potential. Thus the membrane potential is
node[i].v - extnode[i].v[0]
*/
/*
With vectorization, node.v is the membrane potential and node.rhs
after solving is the internal potential. Thus internal potential is
node.v + extnode.v[0]
*/

#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<math.h>
#include	<nrnmpiuse.h>
#include	"section.h"
#include	"membdef.h"
#include	"membfunc.h"
#include 	"spmatrix.h"



extern int	tree_changed;
static void node_free();
static void triang(NrnThread*), bksub(NrnThread*);

#if PARANEURON
void (*nrnmpi_splitcell_compute_)();
#endif

void (*nrn_multisplit_solve_)();
extern void (*nrnpy_o2loc_p_)(Object*, Section**, double*);
extern void (*nrnpy_o2loc2_p_)(Object*, Section**, double*);

/* used for vectorization and distance calculations */
int section_count;
Section** secorder;

/* nrn_solve() solves the matrix equations represented in "section"
   rhs is replaced by the solution (delta v's).
   d is destroyed.
*/
/* Section *sec_alloc(nsec) allocates a vector of nsec sections and returns a
   pointer to the first one in the list. No nodes are allocated.
   The usage is normally "section = sec_alloc(nsection)"
   After allocation one must allocate nodes for each section with
   node_alloc(sec, n).
   After all this connect sections together using section indices.
   Finally order the sections with section_order(section, nsec).
   When the section vector is no longer needed (and before creating another
   one) free the space with sec_free(section, nsection);
*/

#if DEBUGSOLVE
double debugsolve(void)	/* returns solution error */
{
	short inode;
	int i;
	Section *sec, *psec, *ch;
	Node *nd, *pnd, **ndP;
	double err, sum;

	/* save parts of matrix that will be destroyed */
assert(0)
	/* need to save the rootnodes too */
	ForAllSections(sec)
		assert(sec->pnode && sec->nnode);
		for (inode = sec->nnode-1; inode >= 0; inode--) {
			nd = sec->pnode[inode];
			nd->savd = NODED(nd);
			nd->savrhs = NODERHS(nd);
		}
	}

	triang(nrn_threads);
	bksub(nrn_threads);

	err = 0.;
/* need to check the rootnodes too */
	ForAllSections(sec)
		for (inode = sec->nnode-1; inode >= 0; inode--) {
			ndP= sec->pnode + inode;
			nd = sec->pnode[inode];
			/* a single internal current equation */
			sum = nd->savd * NODERHS(nd);
			if (inode > 0) {
				sum += NODEB(nd) * NODERHS(nd - 1);
			}else{
				pnd = sec->parentnode;
				sum += NODEB(nd) * NODERHS(pnd);
			}
			if (inode < sec->nnode-1) {
				sum += NODEA(ndP[1]) * NODERHS(ndP[1]);
			}
			for (ch = nd->child; ch; ch = ch->sibling){
				psec = ch;
				pnd = psec->pnode[0];
				assert(pnd && psec->nnode);
				sum += NODEA(pnd) * NODERHS(pnd);
			}
			sum -= nd->savrhs;
			err += fabs(sum);
		}
	}

	return err;
}	
#endif /*DEBUGSOLVE*/


double node_dist(Section* sec, Node* node)
{
	int inode;
	double ratio;
	
	if (!sec || sec->parentnode == node) {
		return 0.;
	}else if ((inode = node->sec_node_index_) == sec->nnode-1) {
		ratio = 1.;
	}else{
		ratio = ((double)inode+.5)/((double)sec->nnode - 1.);
	}
	return  section_length(sec)*ratio;
}

double topol_distance(Section* sec1, Node* node1, Section* sec2, Node* node2,
  Section** prootsec, Node** prootnode)
{	/* returns the distance between the two nodes
		Ie the sum of the appropriate length portions
		of those sections connecting these two
		nodes.
	*/
	double d, x1, x2;

	d = 0;
	if (tree_changed) {
		setup_topology();
	}
	/* keep moving toward a common node */
	while (sec1 != sec2) {
		if (!sec1) {
			d += node_dist(sec2, node2);
			node2 = sec2->parentnode;
			sec2 = sec2->parentsec;
		}else if (!sec2){
			d += node_dist(sec1, node1);
			node1 = sec1->parentnode;
			sec1 = sec1->parentsec;
		}else if (sec1->order > sec2->order) {
			d += node_dist(sec1, node1);
			node1 = sec1->parentnode;
			sec1 = sec1->parentsec;
		}else{
			d += node_dist(sec2, node2);
			node2 = sec2->parentnode;
			sec2 = sec2->parentsec;
		}
	}
	if (!sec1) {
		if (node1 != node2) {
			sec1 = 0;
			d = 1e20;
			node1 = (Node*)0;
		}
	} else if (node1 != node2) {
		x1 = node_dist(sec1, node1);
		x2 = node_dist(sec2, node2);
		if (x1 < x2) {
			d += x2 - x1;
		}else{
			node1 = node2;
			d += x1 - x2;
		}
	}
	*prootsec = sec1;
	*prootnode = node1;
	return d;
}

static Section *origin_sec;

void distance(void) {
	double d, d_origin; //, chkarg();
	int mode;
	Node* node;//, *node_exact();
	Section *sec;	
	static Node* origin_node;
	Node* my_origin_node;
	Section* my_origin_sec;
	
	if (tree_changed) {
		setup_topology();
	}
			
	if (ifarg(2)) {
		nrn_seg_or_x_arg2(2, &sec, &d);
		if (hoc_is_double_arg(1)) {
			mode = (int) chkarg(1, 0., 1.);
		}else{
			mode = 2;
			Object* o = *hoc_objgetarg(1);
			my_origin_sec = (Section*)0;
			if (nrnpy_o2loc2_p_) {
				(*nrnpy_o2loc2_p_)(o, &my_origin_sec, &d_origin);
			}
			if (!my_origin_sec) {
				hoc_execerror("Distance origin not valid.","If first argument is an Object, it needs to be a Python Segment object, a rxd.node object, or something with a segment property.");
			}
			my_origin_node = node_exact(my_origin_sec, d_origin);
		}		
	}else if (ifarg(1)) {
		nrn_seg_or_x_arg2(1, &sec, &d);
		mode = 1;
	}else{
		sec = chk_access();
		d = 0.;
		mode = 0;
	}
	node = node_exact(sec, d);
	if (mode == 0) {
		origin_node = node;
		origin_sec = sec;
	}else{
		if (mode != 2 && (!origin_sec || !origin_sec->prop)) {
			hoc_execerror("Distance origin not valid.","Need to initialize origin with distance()");
		}
		if (mode == 1) {
			my_origin_sec = origin_sec;
			my_origin_node = origin_node;
		}
		d = topol_distance(my_origin_sec, my_origin_node, sec, node,
			&sec, &node );
	}
	hoc_retpushx(d);
}
	
static void dashes(Section* sec, int offset, int first);

void nrnhoc_topology(void) /* print the topology of the branched cable */
{
	hoc_Item* q;

	v_setup_vectors();
	Printf("\n");
	ITERATE(q, section_list) {
		Section* sec = (Section*)VOIDITM(q);
		if (sec->parentsec == (Section*)0) {
			Printf("|");
			dashes(sec, 0, '-');
		}
	}
	Printf("\n");
	hoc_retpushx(1.);
}

static void dashes(Section* sec, int offset, int first)
{
	int i, scnt;
	Section* ch;
	char direc[30];
//	extern double nrn_section_orientation();
	
	i = (int)nrn_section_orientation(sec);
	sprintf(direc, "(%d-%d)", i, 1-i);
	for (i=0; i<offset; i++) Printf(" ");
	Printf("%c", first);
	for (i=2; i<sec->nnode; i++) Printf("-");
	if (sec->prop->dparam[4].val == 1) {
		Printf("|       %s%s\n", secname(sec), direc);
	}else{
		Printf("|       %s%s with %g rall branches\n",
			secname(sec), direc, sec->prop->dparam[4].val);
	}
	/* navigate the sibling list backwards */
	/* note that the sibling list is organized monotonically by
	  increasing distance from parent */
	for (scnt=0, ch=sec->child; ch; ++scnt, ch = ch->sibling) {
		hoc_pushobj((Object**)ch);
	}
	while(scnt--) {		
//		Object** hoc_objpop();
		ch = (Section*)hoc_objpop();
		i = node_index_exact(sec, nrn_connection_position(ch));
		Printf(" ");
		dashes(ch, i+offset+1, 0140); /* the ` char*/
	}
}

/* solve the matrix equation */
void nrn_solve(NrnThread* _nt) {
#if 0
	printf("\nnrn_solve enter %lx\n", (long)_nt);
	nrn_print_matrix(_nt);
#endif
#if PARANEURON
	if (nrn_multisplit_solve_) {
		nrn_thread_error("nrn_multisplit_solve");
		(*nrn_multisplit_solve_)();
		return;
	}
#endif

#if DEBUGSOLVE
    {
	double err;
	nrn_thread_error("debugsolve");
	err = debugsolve();
	if (err > 1.e-10) {
		Fprintf(stderr, "solve error = %g\n", err);
	}
    }
#else
	if (use_sparse13) {
		int e;
		nrn_thread_error("solve use_sparse13");
		e = spFactor(_nt->_sp13mat);
		if (e != spOKAY) {
			switch (e) {
			case spZERO_DIAG:
				hoc_execerror("spFactor error:", "Zero Diagonal");
			case spNO_MEMORY:
				hoc_execerror("spFactor error:", "No Memory");
			case spSINGULAR:
				hoc_execerror("spFactor error:", "Singular");
			}
		}
		spSolve(_nt->_sp13mat, _nt->_actual_rhs, _nt->_actual_rhs);
	}else{
		triang(_nt);
#if PARANEURON
		if (nrnmpi_splitcell_compute_) {
			nrn_thread_error("nrnmpi_splitcell_compute");
			(*nrnmpi_splitcell_compute_)();
		}
#endif
		bksub(_nt);
	}
#endif
#if 0
	printf("\nnrn_solve leave %lx\n", (long)_nt);
	nrn_print_matrix(_nt);
#endif
}

#if VECTORIZE && _CRAY
extern Node*** v_node_depth_lists;
extern Node*** v_parent_depth_lists; /* parents must be unique in each list */
extern int* v_node_depth_count;
extern int v_node_depth; /* so depth may be more than twice what you'd expect */
#endif

/* triangularization of the matrix equations */
void triang(NrnThread* _nt)
{
	register Node *nd, *pnd;
	double p;
	int i, i2, i3;
	i2 = _nt->ncell;
	i3 = _nt->end;
#if CACHEVEC
    if (use_cachevec) {
	for (i = i3 - 1; i >= i2; --i) {
		p = VEC_A(i) / VEC_D(i);
		VEC_D(_nt->_v_parent_index[i]) -= p * VEC_B(i);
		VEC_RHS(_nt->_v_parent_index[i]) -= p * VEC_RHS(i);
	}
    }else
#endif /* CACHEVEC */
    {
	for (i = i3 - 1; i >= i2; --i) {
		nd = _nt->_v_node[i];
		pnd = _nt->_v_parent[i];
		p = NODEA(nd) / NODED(nd);
		NODED(pnd) -= p * NODEB(nd);
		NODERHS(pnd) -= p * NODERHS(nd);
	}
    }
}

/* back substitution to finish solving the matrix equations */
void bksub(NrnThread* _nt)
{
	register Node *nd, *cnd;
	int i, i1, i2, i3;
	i1 = 0;
	i2 = i1 + _nt->ncell;
	i3 = _nt->end;
#if CACHEVEC
    if (use_cachevec) {
	for (i = i1; i < i2; ++i) {
		VEC_RHS(i) /= VEC_D(i);
	}
	for (i = i2; i < i3; ++i) {
		VEC_RHS(i) -= VEC_B(i) * VEC_RHS(_nt->_v_parent_index[i]);
		VEC_RHS(i) /= VEC_D(i);
	}	
    }else
#endif /* CACHEVEC */
    {
	for (i = i1; i < i2; ++i) {
		NODERHS(_nt->_v_node[i]) /= NODED(_nt->_v_node[i]);
	}
	for (i = i2; i < i3; ++i) {
		cnd = _nt->_v_node[i];
		nd = _nt->_v_parent[i];
		NODERHS(cnd) -= NODEB(cnd) * NODERHS(nd);
		NODERHS(cnd) /= NODED(cnd);
	}	
    }
}

void nrn_clear_mark(void) {
	hoc_Item* qsec;
	ForAllSections(sec)
		sec->volatile_mark = 0;
	}
}
short nrn_increment_mark(Section* sec) { return sec->volatile_mark++;}
short nrn_value_mark(Section* sec) { return sec->volatile_mark;}

/* allocate space for sections (but no nodes) */
/* returns pointer to Section */
Section* sec_alloc(void)
{
	Section  *sec;

	/* changed from emalloc to allocation from a SectionPool in order
	   to allow safe checking of whether a void* is a possible Section*
	   without the possibility of invalid memory read errors.
	   Note that freeing sections must be done
	   with nrn_section_free(Section*)
	*/
	sec = nrn_section_alloc();
	sec->refcount = 0;
	sec->nnode = 0;
	sec->parentsec = sec->sibling = sec->child = (Section*)0;
	sec->parentnode = (Node*)0;
	sec->pnode = (Node **)0;
#if DIAMLIST
	sec->npt3d = 0;
	sec->pt3d_bsize = 0;
	sec->pt3d = (Pt3d *)0;
	sec->logical_connection = (Pt3d*)0;
#endif
	sec->prop = (Prop *)0;
	sec->recalc_area_ = 0;

	return sec;
}

/* free a node vector for one section */
static void node_free(Section* sec)
{
    Node **pnd;

    pnd = sec->pnode;
    if (!pnd) {
        sec->nnode = 0;
    }
    if (sec->nnode == 0) {
        return;
    }
    node_destruct(sec->pnode, sec->nnode);
    sec->pnode = (Node **)0;
    sec->nnode = 0;
}

static void section_unlink(Section* sec);
/* free everything about sections */
void sec_free(hoc_Item* secitem)
{
	Section *sec;
	
	if (!secitem) {
		return;
	}
	sec = hocSEC(secitem);
	assert(sec);
/*printf("sec_free %s\n", secname(sec));*/
	section_unlink(sec);
	{Object* ob = sec->prop->dparam[6].obj;
		if (ob && ob->secelm_ == secitem) { /* it is the last */
			hoc_Item* q = secitem->prev;
			if (q->itemtype && hocSEC(q)->prop && hocSEC(q)->prop->dparam[6].obj == ob) {
				ob->secelm_ = q;
			}else{
				ob->secelm_ = (hoc_Item*)0;
			}
		}
	}
	hoc_l_delete(secitem);
	prop_free(&(sec->prop));
	node_free(sec);
	if (!sec->parentsec && sec->parentnode) {
		nrn_node_destruct1(sec->parentnode);
	}
#if DIAMLIST
	if (sec->pt3d) {
		free((char *)sec->pt3d);
		sec->pt3d = (Pt3d*)0;
		sec->npt3d = 0;
		sec->pt3d_bsize = 0;
	}
	if (sec->logical_connection) {
		free(sec->logical_connection);
		sec->logical_connection = (Pt3d*)0;
	}
#endif
	section_unref(sec);
}


/* can't actually release the space till the refcount goes to 0 */
void section_unref(Section* sec)
{
/*printf("section_unref %lx %d\n", (long)sec, sec->refcount-1);*/
	if (--sec->refcount <= 0) {
#if 0
printf("section_unref: freed\n");
#endif
		assert (!sec->parentsec);
		nrn_section_free(sec);
	}
}
void section_ref(Section* sec)
{
/*printf("section_ref %lx %d\n", (long)sec,sec->refcount+1);*/
	++sec->refcount;
}

void nrn_sec_ref(Section** psec, Section* sec)
{
	Section* s = *psec;
	if (sec) {
		section_ref(sec);
	}
	*psec = sec;
	if (s) {
		section_unref(s);
	}
}

static void section_unlink(Section* sec) /* other sections no longer reference this one */
{
	/* only sections that are explicitly connected to this are disconnected */
	Section* child;
	tree_changed = 1;
	/* disconnect the sections connected to this at the parent end */
	for (child = sec->child; child; child = child->sibling) {
		nrn_disconnect(child);
	}
	nrn_disconnect(sec);
}

Node** node_construct(int n)
{
	Node* nd, **pnode;
	int i;
	
	pnode = (Node**)ecalloc((unsigned)n, sizeof(Node*));
	for (i = n - 1; i >= 0; i--) {
		nd = (Node *)ecalloc(1, sizeof(Node));
#if CACHEVEC
		nd->_v = &nd->_v_temp;
		nd->_area = 100.;
		nd->_rinv = 0.;
#endif
		nd->sec_node_index_ = i;
		pnode[i] = nd;
		nd->prop = (Prop *)0;
		NODEV(nd) = DEF_vrest;
#if EXTRACELLULAR
		nd->extnode = (Extnode*)0;
#endif
#if EXTRAEQN
		nd->eqnblock = (Eqnblock *)0;
#endif
	}
	return pnode;
}

Node* nrn_node_construct1(void) {
	Node* nd;
	Node** ndp;
	ndp = node_construct(1);
	nd = ndp[0];
	free(ndp);
	return nd;
}

void nrn_node_destruct1(Node* nd) {
	if (!nd) { return; }
	prop_free(&(nd->prop));
#if CACHEVEC
	notify_freed_val_array(&NODEV(nd), 1);
	notify_freed_val_array(&NODEAREA(nd), 2);
#else
	notify_freed_val_array(&NODEV(nd), 2);
#endif
#if EXTRACELLULAR
	if (nd->extnode) {
		notify_freed_val_array(nd->extnode->v, nlayer);
	}
#endif
#if EXTRAEQN
{
	Eqnblock *e, *e1;
	for (e = nd->eqnblock; e; e = e1) {
		e1 = e->eqnblock_next;
		free((char *)e);
	}
}
#endif
#if EXTRACELLULAR
	if (nd->extnode) {
		free((char *)nd->extnode);
	}
#endif
	free(nd);
}

void node_destruct(Node** pnode, int n)
{
	int i;
	Node* nd;

	for (i = n - 1; i >= 0; i--) {
		if (pnode[i]) {
			nrn_node_destruct1(pnode[i]);
		}
	}
	free((char *)pnode);
}

#if KEEP_NSEG_PARM

extern int keep_nseg_parm_;

static Node* node_clone(Node* nd1) {
	Node* nd2;
	Prop* p1, *p2;
    extern Prop* prop_alloc(Prop**, int, Node*);//, *prop_alloc();
	int i, imax;
	nd2 = (Node *)ecalloc(1, sizeof(Node));
#if CACHEVEC
	nd2->_v = &nd2->_v_temp;
#endif
	NODEV(nd2) = NODEV(nd1);
	for (p1 = nd1->prop; p1; p1 = p1->next) {
		if (!memb_func[p1->type].is_point) {
			p2 = prop_alloc(&(nd2->prop), p1->type, nd2);
			if (p2->ob) {
				Symbol* s, *ps;
				double* px, *py;
				int j, jmax;
				s = memb_func[p1->type].sym;
				jmax = s->s_varn;
				for (j=0; j < jmax; ++j) {
					ps = s->u.ppsym[j];
					px = p2->ob->u.dataspace[ps->u.rng.index].pval;
					py = p1->ob->u.dataspace[ps->u.rng.index].pval;
					imax = hoc_total_array_data(ps, 0);
					for (i=0; i < imax; ++i) {
						px[i] = py[i];
					}
				}
			}else{
				for (i=0; i < p1->param_size; ++i) {
					p2->param[i] = p1->param[i];
				}
			}
		}
	}
	/* in case the user defined an explicit ion_style, make sure
	   the new node has the same style for all ions. */
	for (p1 = nd1->prop; p1; p1 = p1->next) {
		if (nrn_is_ion(p1->type)) {
			p2 = nd2->prop;
			while (p2 && p2->type != p1->type) { p2 = p2->next; }
			assert(p2 && p1->type == p2->type);
			p2->dparam[0].i = p1->dparam[0].i;
		}
	}

	return nd2;
}

static Node* node_interp(Node* nd1, Node* nd2, double frac)
{
	Node* nd;
	if (frac > .5) {
		nd = node_clone(nd2);
	}else{
		nd = node_clone(nd1);
	}
	return nd;
}

static void node_realloc(Section* sec, short nseg)
{
	Node** pn1, **pn2;
	int n1, n2, i1, i2, i;
	double x;
	pn1 = sec->pnode;
	n1 = sec->nnode;
	pn2 = (Node**)ecalloc((unsigned)nseg, sizeof(Node*));
	n2 = nseg;
	sec->pnode = pn2;
	sec->nnode = n2;

	n1--; n2--;		/* number of non-zero area segments */
	pn2[n2] = pn1[n1];	/* 0 area node at end of section */
	pn1[n1] = (Node*)0;

	/* sprinkle nodes from pn1 to pn2 */
	if (n1 < n2) {
		/* the ones that are reused */
		for (i1 = 0; i1 < n1; ++i1) {
			x = (i1+.5)/(double)n1;
			i2 = (int)(n2*x); /* because we want to round to n2*x-.5 */
			pn2[i2] = pn1[i1];
		}
		/* the ones that are cloned */
		for (i2 = 0; i2 < n2; ++i2) if (pn2[i2] == NULL) {
			x = (i2 + 0.5)/(double)n2;
			i1 = (int)(n1*x);
			pn2[i2] = node_clone(pn1[i1]);
		}
		for (i1 = 0; i1 < n1; ++i1) {
			pn1[i1] = (Node*)0;
		}
	}else{
		for (i2 = 0; i2 < n2; ++i2) {
			x = (i2+.5)/(double)n2;
			i1 = (int)(n1*x);
			pn2[i2] = pn1[i1];
			pn1[i1] = (Node*)0;
		}	
		/* do not lose any point processes */
		i1 = 0;
		for (i2=0; i2 < n2; ++i2) {
			double x1, x2;
			x2 = (i2+1.)/(double)n2; /* far end of new segment */
			for (; i1 < n1; ++i1) {
				x1 = (i1+.5)/(double)n1;
				if (x1 > x2) {
					break;
				}
				if (pn1[i1] == (Node*)0) {
					continue;
				}
#if 0
printf("moving point processes from pn1[%d] to pn2[%d]\n", i1, i2);
printf("i.e. x1=%g in the range %g to %g\n", x1, x2-1./n2, x2);
#endif
				nrn_relocate_old_points(sec, pn1[i1], sec, pn2[i2]);
			}
		}
		/* Some of the pn1 were not used */
	}
	node_destruct(pn1, n1 + 1);
	for (i2=0; i2 < nseg; ++i2) {
		pn2[i2]->sec_node_index_ = i2;
	}
#if EXTRACELLULAR
	if (sec->pnode[sec->nnode-1]->extnode) {
		extcell_2d_alloc(sec);
	}
#endif
}

#endif

/* allocate node vectors for a section list */
void node_alloc(Section* sec, short nseg)
{
	int i;
#if KEEP_NSEG_PARM
	if (keep_nseg_parm_ && (nseg > 0) && sec->pnode) {
		node_realloc(sec, nseg);
	}else
#endif
	{
		node_free(sec);
		if (nseg == 0) {
			return;
		}
		sec->pnode = node_construct(nseg);
		sec->nnode = nseg;
	}
	for (i=0; i < nseg; ++i) {
		sec->pnode[i]->sec = sec;
	}
}

void section_order(void) /* create a section order consistent */
		/* with connection info */
{
	int order, isec;
	Section* ch;
	Section *sec;
	hoc_Item* qsec;

	/* count the sections */
	section_count = 0;
	/*SUPPRESS 765*/
	ForAllSections(sec)
		sec->order = -1;
		++section_count;
	}
	
	if (secorder) {
		free((char *)secorder);
		secorder = (Section**)0;
	}
	if (section_count) {
		secorder = (Section**)emalloc(section_count*sizeof(Section*));
	}
	order = 0;
	ForAllSections(sec) /* all the roots first */
		if (!sec->parentsec) {
			secorder[order] = sec;
			sec->order = order;
			++order;
		}
	}

	for (isec=0; isec<section_count; isec++) {
		if (isec >= order) { /* there is a loop */
			ForAllSections(sec)
				Section* psec, *s = sec;
				for (psec = sec->parentsec; psec; s = psec, psec = psec->parentsec) {
					if (!psec || s->order >= 0) {
						break;
					}else if (psec == sec) {
fprintf(stderr, "A loop exists consisting of:\n %s", secname(sec));
for (s = sec->parentsec; s != sec; s = s->parentsec) {
	fprintf(stderr, " %s", secname(s));
}
fprintf(stderr, " %s\nUse <section> disconnect() to break the loop\n ", secname(sec));
hoc_execerror("A loop exists involving section", secname(sec));
					}
				}
			}
		}
		sec = secorder[isec];
		for (ch = sec->child; ch; ch = ch->sibling) {
			secorder[order] = ch;
			ch->order = order;
			++order;
		}
	}
	assert(order == section_count);
}

