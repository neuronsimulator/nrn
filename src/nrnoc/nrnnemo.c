#include <../../nrnconf.h>

#include "section.h"

#define OBSOLETE 1

#if !OBSOLETE
#include "membfunc.h"
#include "hocassrt.h"

/* basic loop taken from topology() in solve.c */
static FILE *fin, *fmark, *fdat;
static int inode;
static dashes(), file_func(), dat_head();
#define MAXMARKS	32
static long *marksec;
static printline();

extern int section_count;
extern Section** secorder;
#endif

void nemo2neuron(void)
{
	hoc_retpushx(1.0);
}

void neuron2nemo(void)
{
#if OBSOLETE
	hoc_execerror("neuron2nemo:", "implementation is obsolete");
#else
	short i, isec, imark;
	extern int tree_changed;
	char name[50];

	if (tree_changed) {
		setup_topology();
	}
	Sprintf(name, "in/%s", gargstr(1));
	if ((fin = fopen(name, "w")) == (FILE *)0) {
		hoc_execerror("Can't write to ", name);
	}
	Sprintf(name, "mark/%s", gargstr(1));
	if ((fmark = fopen(name, "w")) == (FILE *)0) {
		hoc_execerror("Can't write to ", name);
	}
	Sprintf(name, "dat/%s", gargstr(1));
	if ((fdat = fopen(name, "w")) == (FILE *)0) {
		hoc_execerror("Can't write to ", name);
	}
	dat_head();
	
	/* set up the mark for every section */
	imark = 1;
	marksec = (long *)ecalloc(section_count, sizeof(long));
	for (i=1; i < section_count; i++) {
		isec = i;
		if (marksec[isec]) {
			marksec[i] = marksec[isec];
		}else{
			if (imark < MAXMARKS) {
				marksec[i] = 1L << (imark++);
				file_func(secorder[isec]);
			}
		}
	}
	Fprintf(fin, "/* created by NEURON */\n");
	inode = 0;
	for (i=0; i<rootnodecount; i++) {
		printline(v_node[i]->child, 0, 0., (double)(i*100));
		dashes(v_node[i]->child, 0., (double)(i*100), 0., 1);
	}
	free((char *)marksec);
	fclose(fin);
	fclose(fmark);
#endif
	hoc_retpushx(1.0);
}

#if !OBSOLETE
static dashes(Section* sec, double x, double y, double theta, int leftend)
{
	Section* ch;
	int i, nrall, irall;
	double cos(), sin(), xx, yy, ttheta, dx;
	
	dx = section_length(sec)/((double)sec->nnode - 1);
	nrall = (int) sec->prop->dparam[4].val;
   for (irall=0; irall < nrall; irall++) {
	xx = x; yy=y;
	ttheta = theta + (double)(nrall - 2*irall - 1)/(double)nrall;
	for (i=0; i<sec->nnode - 1; i++) {
		xx += dx*cos(ttheta);
		yy += dx*sin(ttheta);
		printline(sec, i, xx, yy);
	}
	for (i=sec->nnode - 1; i>=0; i--) {
		if ((ch = sec->pnode[i]->child) != (Section*)0) {
			if (i == sec->nnode - 1) {
				dashes(ch, xx, yy, ttheta, 0);
			}else{
				dashes(ch, xx, yy, ttheta+.5, 0);
			}
		}
		if (i < sec->nnode - 1) {
			xx -= dx*cos(ttheta);
			yy -= dx*sin(ttheta);
		}
	}
	if (leftend) {
		ttheta = 3.14159 - .5;
	}
	if ((ch = sec->sibling) != (Section*)0) {
		dashes(ch, x, y, ttheta+.5, 0);
	}
   }
}

static double diamval(Node* nd)
{
	Prop *p;

	for (p = nd->prop; p; p = p->next) {
		if (p->type == MORPHOLOGY) {
			break;
		}
	}
	assert(p);
	return p->param[0];
}

static void printline(Section* sec, int i, double x, double y)
{
	char type;
	int nb;
	Section* csec;
	Node *nd;
	double d;
	
	++inode;
	assert(sec->nnode > 1);
	type = 'C';
	nd = sec->pnode[i];
	d = diamval(nd);
	nb = 0;
	for (csec = nd->child; csec; csec = csec->sibling) {
		nb += (int)csec->prop->dparam[4].val;
	}
	if (i == sec->nnode - 2) {
		++i;
		nd = sec->pnode[i];
		for (csec = nd->child; csec; csec = csec->sibling) {
			nb += (int)csec->prop->dparam[4].val;
		}
	}		
	if (i < sec->nnode - 2) {
		nb++;
	}
	if (nb>2) { /*many branches*/
		type = 'B';
		x -= .001;
		while (nb>2) {
			fwrite((char *)&marksec[sec->order], sizeof(long), 1, fmark);
			Fprintf(fin, "%d\t%c\t%g\t%g\t0\t%g\n",
				inode++, type, (x += .001), y, d);
			--nb;
		}
	} else if ( nb == 2) { /*one branch*/
		type = 'B';
	} else if (nb == 0) {
		type = 'T';
	}
	fwrite((char *)&marksec[sec->order], sizeof(long), 1, fmark);
	Fprintf(fin, "%d\t%c\t%g\t%g\t0\t%g\n", inode, type, x, y, d);
}

/* modified from nemo.c */
static void file_func(Section* sec)
{
	int active;
	Node *nd;
	Prop *p;
	Symbol *s, *hoc_lookup();
	static int hhtype = 0;
	
	if (!hhtype) {
		s = hoc_lookup("HH");
		hhtype = s->subtype;
	}
	nd = sec->pnode[0];
	
	active = 0;
	for (p = nd->prop; p; p = p->next) {
		if (p->type == hhtype) {
			active = 1;
		}
	}
	
	assert(sec->prop->dparam[0].sym);
	fprintf(fdat,"%s\n",sec->prop->dparam[0].sym->name);
	fprintf(fdat,"^area %g\n",1.);
	fprintf(fdat,"^Rm %g\n",1.);
	fprintf(fdat,"^active %d\n",active);
	fprintf(fdat,"^synapse %d\n",0);
	fprintf(fdat,"^Erev %g\n",0.);
	fprintf(fdat,"^gback %g\n",0.);
	fprintf(fdat,"^gsyn %g\n",0.);
	fprintf(fdat,"^tstart %g\n",0.);
	fprintf(fdat,"^twidth %g\n",0.);
}

static void dat_head(void)
{
	double Ra = 35.4;
	
	fprintf(fdat,"freq\n");
	fprintf(fdat,"%d %g %g\n",4, 1., 1000.);

	fprintf(fdat,"tran\n");
	fprintf(fdat,"%g %g\n", .1, 10.);

	fprintf(fdat,"elec\n");
	fprintf(fdat,"%g %g %g\n",1000., 1.e-6, Ra);

	fprintf(fdat,"syn\n");
	fprintf(fdat,"%g %g\n",30., -15.);

	fprintf(fdat,"hh\n");
	fprintf(fdat,"%g %g %g %g %g %g\n",1.,1.,1.,1.,1.,1.);

	fprintf(fdat,"batt\n");
	fprintf(fdat,"%g %g\n",115., -12.);

	fprintf(fdat,"%10d\n",9999);
}
	
#endif
