#### Notes for Code Generation

###### when vectorize is true and what is relation with threadsafe?

nocpout.c@99 has following fragment:

```
#if VECTORIZE
int vectorize = 1;
/*
the idea is to put all variables into a vector of vectors so there
there is no static data. Every function has an implicit argument, underbar ix
which tells which set of data _p[ix][...] to use. There are going to have
to be limits on the kinds of scopmath functions called since many of them
need static data. We can have special versions of the most useful of these.
ie sparse.c.
Above is obsolete in detail , underbar ix is no longer used.
When vectorize = 1 then we believe the code is thread safe and most functions
pass around _p, _ppvar, _thread. When vectorize is 0 then we are definitely
not thread safe and _p and _ppvar are static.
*/
```

So by default code is compiled with `-DVECTORIZE`. Then solvehandler in solve.c turn off vectorize in various cases:

```
void solvhandler() {

  case DERF:
    if (method == SYM0) {
      method = lookup("adrunge");
    }

    if (btype == BREAKPOINT && !steadystate) {
      /* derivatives recalculated after while loop */
      if (strcmp(method - > name, "cnexp") != 0 && strcmp(method - > name, "derivimplicit") != 0) {
        fprintf(stderr, "Notice: %s is not thread safe. Complain to Hines\n", method - > name);
        vectorize = 0;
      }

    case KINF:
      if (method == SYM0) {
        method = lookup("_advance");
      }
      if (btype == BREAKPOINT && (method - > subtype & DERF)) {#
        if VECTORIZE
        fprintf(stderr, "Notice: KINETIC with is thread safe only with METHOD sparse. Complain to Hines\n");
        vectorize = 0;#
        endif

      case NLINF:
        #if VECTORIZE
        fprintf(stderr, "Notice: NONLINEAR is not thread safe.\n");
        vectorize = 0;#
        endif
        if (method == SYM0) {
          method = lookup("newton");
        }

      case LINF:
        #if VECTORIZE
        fprintf(stderr, "Notice: LINEAR is not thread safe.\n");
        vectorize = 0;#
        endif
        if (method == SYM0) {
          method = lookup("simeq");
        }

      case DISCF:
        #if VECTORIZE
        fprintf(stderr, "Notice: DISCRETE is not thread safe.\n");
        vectorize = 0;#
        endif
        if (btype == BREAKPOINT) whileloop(qsol, (long) DISCRETE, 0);
        Sprintf(buf, "0; %s += d%s; %s();\n",
          indepsym - > name, indepsym - > name, fun - > name);
        replacstr(qsol, buf);
        break;

        #
        if 1 /* really wanted? */
      case PROCED:
        if (btype == BREAKPOINT) {
          whileloop(qsol, (long) DERF, 0);#
          if CVODE
          if (cvodemethod_ == 1) { /*after_cvode*/
            cvode_interface(fun, listnum, 0);
          }
          if (cvodemethod_ == 2) { /*cvode_t*/
            cvode_interface(fun, listnum, 0);
            insertstr(qsol, "if (!cvode_active_)");
            cvode_nrn_cur_solve_ = fun;
            linsertstr(procfunc, "extern int cvode_active_;\n");
          }
          if (cvodemethod_ == 3) { /*cvode_t_v*/
            cvode_interface(fun, listnum, 0);
            insertstr(qsol, "if (!cvode_active_)");
            cvode_nrn_current_solve_ = fun;
            linsertstr(procfunc, "extern int cvode_active_;\n");
          }#
          endif
        }
        Sprintf(buf, " %s();\n", fun - > name);
        replacstr(qsol, buf);#
        if VECTORIZE
        Sprintf(buf, "{ %s(_threadargs_); }\n",
          fun - > name);
        vectorize_substitute(qsol, buf);#
        endif
        break;#
        endif
      case PARF:
        #if VECTORIZE
        fprintf(stderr, "Notice: PARTIAL is not thread safe.\n");
        vectorize = 0;#
        endif
        if (btype == BREAKPOINT) whileloop(qsol, (long) DERF, 0);
        solv_partial(qsol, fun);
        break;
      default:
        diag("Illegal or unimplemented SOLVE type: ", fun - > name);
        break;
      }
      #if CVODE
      if (btype == BREAKPOINT) {
        cvode_valid();
      }
      #endif
      /* add the error check */
      Insertstr(qsol, "error =");
      move(errstmt - > next, errstmt - > prev, qsol - > next);
      #if VECTORIZE
      if (errstmt - > next == errstmt - > prev) {
        vectorize_substitute(qsol - > next, "");
        vectorize_substitute(qsol - > prev, "");
      } else {
        fprintf(stderr, "Notice: SOLVE with ERROR is not thread safe.\n");
        vectorize = 0;
      }
      #endif

    }
```

So various blocks are not thread safe. Also, nocpcout.c has:

```
	if (!mechname) {
		sprintf(suffix,"_%s", modbase);
		mechname = modbase;
	} else if (strcmp(mechname, "nothing") == 0) {
		vectorize = 0;
		suffix[0] = '\0';
		mechname = modbase;
	}else{
		sprintf(suffix, "_%s", mechname);
	}

	.....

	if (artificial_cell && vectorize && (thread_data_index || toplocal_)) {
		fprintf(stderr, "Notice: ARTIFICIAL_CELL models that would require thread specific data are not thread safe.\n");
		vectorize = 0;
	}
```

parsact.c has :

```
void vectorize_use_func(qname, qpar1, qexpr, qpar2, blocktype)
Item * qname, * qpar1, * qexpr, * qpar2;
int blocktype; {
  Item * q;
  if (SYM(qname) - > subtype & EXTDEF) {
    if (strcmp(SYM(qname) - > name, "nrn_pointing") == 0) {
      Insertstr(qpar1 - > next, "&");
    } else if (strcmp(SYM(qname) - > name, "state_discontinuity") == 0) {#
      if CVODE
      fprintf(stderr, "Notice: Use of state_discontinuity is not thread safe");
      vectorize = 0;
      if (blocktype == NETRECEIVE) {
        Fprintf(stderr, "Notice: Use of state_discontinuity in a NET_RECEIVE block is unnecessary and prevents use of this mechanism in a multithreaded simulation.\n");
      }
```
`vectorize_use_func` function is called from parser for every function call. So `state_discontinuity` call is not thread safe.

parse1.y has following in function call grammer block:

```
funccall: NAME '('
		 if (SYM($1)->subtype & EXTDEF5) {
			if (!assert_threadsafe) {
fprintf(stderr, "Notice: %s is not thread safe\n", SYM($1)->name);
				vectorize = 0;
			}
	....
	| all VERBATIM
		/* read everything and move as is to end of procfunc */
		{inblock(SYM($2)->name); replacstr($2, "\n/*parse1.y:163: VERBATIM*/\n");
		if (!assert_threadsafe && !saw_verbatim_) {
 		 fprintf(stderr, "Notice: VERBATIM blocks are not thread safe\n");
		 saw_verbatim_ = 1;
		 vectorize = 0;
		}
```
`EXTDEF5` are functions/methods defined in token_mapping. These methods are also not thread safe. Verbatim bocks are also not thrad-safe unless mod file is marked thread safe explicitly.




If following variables are encountered and if mod file is not marked THREADSAFE then vectorize becomes false. This is how it is done:

```
	case EXTERNAL:
	  #if VECTORIZE
	  threadsafe("Use of EXTERNAL is not thread safe.");#
	  endif
	  for (q = q1 - > next; q != q2 - > next; q = q - > next) {
	    SYM(q) - > nrntype |= NRNEXTRN | NRNNOTP;
	  }
	  plist = (List * * ) 0;
	  break;
	case POINTER:
	  threadsafe("Use of POINTER is not thread safe.");
	  plist = & nrnpointers;
	  for (q = q1 - > next; q != q2 - > next; q = q - > next) {
	    SYM(q) - > nrntype |= NRNNOTP | NRNPOINTER;
	  }
	  break;
	case BBCOREPOINTER:
	  threadsafe("Use of BBCOREPOINTER is not thread safe.");
	  plist = & nrnpointers;
	  for (q = q1 - > next; q != q2 - > next; q = q - > next) {
	    SYM(q) - > nrntype |= NRNNOTP | NRNBBCOREPOINTER;
	  }
	  use_bbcorepointer = 1;
	  break;
	  }
```
and threadsafe function call looks like below:

```
void threadsafe(char* s) {
	if (!assert_threadsafe) {
		fprintf(stderr, "Notice: %s\n", s);
		vectorize = 0;
	}
}
```

When THREADSAFE keyword is encountered in mod file, parser calls following function:


```
void threadsafe_seen(Item* q1, Item* q2) {
	Item* q;
	assert_threadsafe = 1;
	if (q2) {
		for (q = q1->next; q != q2->next; q = q->next) {
			SYM(q)->assigned_to_ = 2;
		}
	}
}
```

Some helper functions like:

```
void chk_thread_safe() {
	Symbol* s;
	int i;
	Item* q;
 	SYMLISTITER { /* globals are now global with respect to C as well as hoc */
		s = SYM(q);
		if (s->nrntype & (NRNGLOBAL) && s->assigned_to_ == 1) {
	sprintf(buf, "Assignment to the GLOBAL variable, \"%s\", is not thread safe", s->name);
			threadsafe(buf);
		}
	}
}
```



###### when _thread_mem init and cleanup callbacks generated?

nocpout.c@625 has following conditions:

```
	if (vectorize && toplocal_) {
	  int cnt;
	  cnt = 0;
	  ITERATE(q, toplocal_) {
	    if (SYM(q) - > assigned_to_ != 2) {
	      if (SYM(q) - > subtype & ARRAY) {
	        cnt += SYM(q) - > araydim;
	      } else {
	        ++cnt;
	      }
	    }
	  }
	  sprintf(buf, "  _thread[%d]._pval = (double*)ecalloc(%d, sizeof(double));\n", thread_data_index, cnt);
	  lappendstr(thread_mem_init_list, buf);
	  sprintf(buf, "  free((void*)(_thread[%d]._pval));\n", thread_data_index);
	  lappendstr(thread_cleanup_list, buf);
	  cnt = 0;
	  ITERATE(q, toplocal_) {
	      if (SYM(q) - > assigned_to_ != 2) {
	        if (SYM(q) - > subtype & ARRAY) {
	          sprintf(buf, "#define %s (_thread[%d]._pval + %d)\n", SYM(q) - > name, thread_data_index, cnt);
	          cnt += SYM(q) - > araydim;
	        } else {
	          sprintf(buf, "#define %s _thread[%d]._pval[%d]\n", SYM(q) - > name, thread_data_index, cnt);
	          ++cnt;
	        }
	      } else { /* stay file static */
	        if (SYM(q) - > subtype & ARRAY) {
	          sprintf(buf, "static double %s[%d];\n", SYM(q) - > name, SYM(q) - > araydim);
	        } else {
	          sprintf(buf, "static double %s;\n", SYM(q) - > name);
	        }
	      }
	      lappendstr(defs_list, buf);
	    }
	    ++thread_data_index;
	}
```

toplocal_ list non-empty when there are local variables in the top level global scopes. In that case we initialize `thread_mem_init_list` and `thread_cleanup_list.` Also in below context:

```
	/* per thread global data */
	gind = 0;
	if (vectorize) SYMLISTITER {
		s = SYM(q);
		if (s->nrntype & (NRNGLOBAL) && s->assigned_to_ == 1) {
			if (s->subtype & ARRAY) {
				gind += s->araydim;
			}else{
				++gind;
			}
		}
	}
	/* double scalars declared internally */
	Lappendstr(defs_list, "/* declare global and static user variables : nocpout.c 675 */\n");
    /* @todo: not being used by any model? */
	if (gind) {
		sprintf(buf, "static int _thread1data_inuse = 0;\nstatic double _thread1data[%d];\n#define _gth %d\n", gind, thread_data_index);
		Lappendstr(defs_list, buf);
		sprintf(buf, " if (_thread1data_inuse) {_thread[_gth]._pval = (double*)ecalloc(%d, sizeof(double));\n }else{\n _thread[_gth]._pval = _thread1data; _thread1data_inuse = 1;\n }\n", gind);
		lappendstr(thread_mem_init_list, buf);
		lappendstr(thread_cleanup_list, " if (_thread[_gth]._pval == _thread1data) {\n   _thread1data_inuse = 0;\n  }else{\n   free((void*)_thread[_gth]._pval);\n  }\n");
		++thread_data_index;
	}
```

We loop over all global variables and see if any of the global variable is used as `assigned` i.e. shoul dbe written once. In that case those global variables get promited to thread variable. But note that this happens only in THREADSAFE marked mod file.


##### Confusions


###### Can ion variable appear in read as well as write list?

```
USEION ttx READ ttxo, ttxi WRITE ittx, ttxo VALENCE 1
```

mod2c/neuron only consider ttxo in read list but not in write list. But note that `_ion_dittxdv` does appear and style variable. Check if above condition is semantically correct.


####### v as static when model is not thread safe

Sometime I see `v` being declared as static variable:

```
 static double delta_t = 0.01;
 static double h0 = 0;
 static double m0 = 0;
 static double v = 0;
```

 and that is used in kernels:

```
 static double _nrn_current(double _v){double _current=0.;v=_v;{ {
   gNaTs2_t = gNaTs2_tbar * m * m * m * h ;
   ina = gNaTs2_t * ( v - ena ) + asecond [ 1 ] ;
```

Why this works? If model is marked threadsafe then v is passed as parameter to function calls where v is derived from:

 ```
  _v = _vec_v[_nd_idx];
 ```
file:/Users/kumbhar/workarena/repos/bbp/mod2c_debug/src/mod2c_core/init.c
But in case of non-thread-safe models, v can be just static. This is done in nocpout.c@1994 where we see:

```
#if VECTORIZE
    if (vectorize) {
	s = ifnew_install("v");
	s->nrntype = NRNNOTP; /* this is a lie, it goes in at end specially */
	/* no it is not a lie. Use an optimization where voltage passed via arguments */
    }else
#endif
    {
	s = ifnew_install("v");
	s->nrntype |= NRNSTATIC | NRNNOTP;
    }
	s = ifnew_install("t");
	s->nrntype |= NRNEXTRN | NRNNOTP;
	s = ifnew_install("dt");
	s->nrntype |= NRNEXTRN | NRNNOTP;
    /*@todo: why this vectorize_substitute? */
    //printer->print_pragma_t_dt();
```

So, declare v as static if model is not thread-safe.



#### Order of variables in p array?

This is driving me nuts! Initially I thought the order is dected by NEURON block order. But certainly it's not the case!
For example:

- if we have RANGE variable declared in the NEURON block doesn't appear anywhere then it's not appear in translated c file!

It looks like NEURON block is just declaration and doesn't decide the order. The variable must appear in definition blocks like parameter or assigned etc. For example, in above case if is mentioned in PARAMETER then only it appeared in the C file.

```
NEURON {
   SUFFIX kca
   RANGE gk, gbar, m_inf, tau_m,ik, PPP, QQQ
   USEION ca READ cai
   USEION k READ ek WRITE ik
   GLOBAL beta, cac
}

PARAMETER {
  PPP
}

```

`PPP` appeared first in p array. QQQ is not appeared anywhere then it's not used at all. Oh boy! if variable is mentioned as RANGE and used in one of the block, then it's not defined at all. It must be "defined" in parameter or assigned block.

Consider below example:


```
    NEURON {
            SUFFIX kca
            USEION ca READ cai
    }

    PARAMETER {
            ek      = -80   (mV)
            cai
    }
```

In this case ek comes first and then cai. This order is preserved across blocks i.e. :

```
    NEURON {
            SUFFIX kca
            USEION ca READ cai
    }

	ASSIGNED {
				cai

	}

    PARAMETER {
            ek      = -80   (mV)
    }

```

Now cai becomes firsr variable. So definitions are important for order.

Also, suppose STATE block appears before PARAMETER and ASSIGNED block. In this case, Dstate variables must come before other variables "in symorder" block. Here symorder means remaining variables that we print after range+state type.

CONCLUSION: we can keep global order by excluding NEURON block definitions.

Oh Wait: More caveats! For thread variables (global variables get converted to thread specific in threadsafe model), they don't appear in the order of their definition. This is because when we split out we iterate over symbol table in alphabetical order (?):

```
 	SYMLISTITER { /* globals are now global with respect to C as well as hoc */
 	  s = SYM(q);
 	  if (s - > nrntype & (NRNGLOBAL)) {
 	    if (vectorize && s - > assigned_to_ == 1) {
 	      if (s - > subtype & ARRAY) {
 	        sprintf(buf, "#define %s%s (_thread1data + %d)\n\
#define %s (_thread[_gth]._pval + %d)\n",
 	          s - > name, suffix, gind, s - > name, gind);
 	      } else {
 	        sprintf(buf, "#define %s%s _thread1data[%d]\n\
#define %s _thread[_gth]._pval[%d]\n",
 	          s - > name, suffix, gind, s - > name, gind);
 	      }
 	      q1 = Lappendstr(defs_list, buf);
 	      q1 - > itemtype = VERBATIM;
 	      if (s - > subtype & ARRAY) {
 	        gind += s - > araydim;
 	      } else {
 	        ++gind;
 	      }
```

Due to this thread variable declaration is not in the order of definitions. Here is mod file example:

```
NEURON {
    THREADSAFE
    SUFFIX NaTs2_t
    GLOBAL hfirst, hlast, gsecond, gthird
}

PARAMETER   {
    hlast
    hfirst
    glast
    gsecond
    gfirtst
    gthird
}

LOCAL abc, abd

BREAKPOINT  {
    hlast = 1
    hfirst = 11
    gfirtst= 11
    glast = 1
    gthird = 11
    abc = 1
    abd = 2
}
```

In this case, when global variables are encountered, they get inserted into symbol list with symbol[0] i.e. first character is used to select a bucket. Then subsequent symbols with same symbol[0] get prepended and hence the order becomes:

```
gfirst, glast, gthird, gsecond   # note: symbol not found get prepended to the bucket
hfirst   # this is separate bucket

```

and hence generated order is:


```
 #define _zabc _thread[0]._pval[0]
 #define _zabd _thread[0]._pval[1]

 /* declare global and static user variables */
 static int _thread1data_inuse = 0;
static double _thread1data[5];
#define _gth 1
#define gfirtst_NaTs2_t _thread1data[0]
#define gfirtst _thread[_gth]._pval[0]
#define glast_NaTs2_t _thread1data[1]
#define glast _thread[_gth]._pval[1]
#define gthird_NaTs2_t _thread1data[2]
#define gthird _thread[_gth]._pval[2]
#define gsecond gsecond_NaTs2_t
 double gsecond = 0;
 #pragma acc declare copyin (gsecond)
#define hlast_NaTs2_t _thread1data[3]
#define hlast _thread[_gth]._pval[3]
#define hfirst_NaTs2_t _thread1data[4]
#define hfirst _thread[_gth]._pval[4]
```

Note that top local thread variables are in order because they are simply appended from parser.


####### ppvar semantics

PPVAR semantics are registred by following function:

```
void ppvar_semantics(int i, const char* name) {
	Item* q;
    nmp_add_ppvar_semantics(printer, name, i);
	if (!ppvar_semantics_) { ppvar_semantics_ = newlist(); }
	q = Lappendstr(ppvar_semantics_, name);
	q->itemtype = (short)i;
}
```

and this is called by following functions:

```
void parout() {
	....funciton that emil all header part....

	if (net_send_seen_) {
		tqitem_index = ppvar_cnt;
		ppvar_semantics(ppvar_cnt, "netsend");
		ppvar_cnt++;
	}
	if (watch_seen_) {
		watch_index = ppvar_cnt;
		for (i=0; i < watch_seen_ ; ++i) {
			ppvar_semantics(i+ppvar_cnt, "watch");
		}
		ppvar_cnt += watch_seen_;
		sprintf(buf, "\n#define _watch_array _ppvar + %d", watch_index);
		Lappendstr(defs_list, buf);
		Lappendstr(defs_list, "\n");
	}
	if (for_netcons_) {
		sprintf(buf, "\n#define _fnc_index %d\n", ppvar_cnt);
		Lappendstr(defs_list, buf);
		ppvar_semantics(ppvar_cnt, "fornetcon");
		ppvar_cnt += 1;
	}
	if (cvode_emit) {
		cvode_ieq_index = ppvar_cnt;
		ppvar_semantics(ppvar_cnt, "cvodeieq");
		ppvar_cnt++;
	}

	if (diamdec) {
		ppvar_semantics(ioncount + pointercount, "diam");
	}
	if (areadec) {
		ppvar_semantics(ioncount + pointercount + diamdec, "area");
	}

}

/* note: _nt_data is only defined in nrn_init, nrn_cur and nrn_state where ions are used in the current mod files */
static int iondef(p_pointercount) int * p_pointercount; {
  int ioncount, it, need_style;
  Item * q, * q1, * q2;
  Symbol * sion;
  char ionname[100];
  char pionname[100];

  lappendstr(defs_list, "/*ion definitions in nocpout.c:2077 in iondef() */\n");

  ioncount = 0;
  if (point_process) {
    ioncount = 2;
    q = lappendstr(defs_list, "#define _nd_area  _nt_data[_ppvar[0*_STRIDE]]\n");
    //printer->print_pragma_ion_var("_nd_area", 0, ION_REGULAR);
    nmp_add_ion_variable(printer, "_nd_area", VAR_DOUBLE, 0, ION_REGULAR);
    q - > itemtype = VERBATIM;
    ppvar_semantics(0, "area");
    ppvar_semantics(1, "pntproc");
  }
  ITERATE(q, useion) {
    int dcurdef = 0;
    if (!uip) {
      uip = newlist();
      lappendstr(uip, "static void _update_ion_pointer(Datum* _ppvar) {\n");
    }
    need_style = 0;
    sion = SYM(q);
    sprintf(ionname, "%s_ion", sion - > name);
    nmp_add_ion_name(printer, sion - > name);
    q = q - > next;
    ITERATE(q1, LST(q)) {
      SYM(q1) - > nrntype |= NRNIONFLAG;
      Sprintf(buf, "#define _ion_%s		_nt_data[_ppvar[%d*_STRIDE]]\n",
        SYM(q1) - > name, ioncount);
      q2 = lappendstr(defs_list, buf);
      sprintf(pionname, "_ion_%s", SYM(q1) - > name);
      //printer->print_pragma_ion_var(pionname, ioncount, ION_REGULAR);
      nmp_add_ion_variable(printer, pionname, VAR_DOUBLE, ioncount, ION_REGULAR);
      q2 - > itemtype = VERBATIM;
      sprintf(buf, "  nrn_update_ion_pointer(_%s_sym, _ppvar, %d, %d);\n",
        sion - > name, ioncount, iontype(SYM(q1) - > name, sion - > name));#
      if 0 /*BBCORE*/
      lappendstr(uip, buf);#
      endif /*BBCORE*/
      SYM(q1) - > ioncount_ = ioncount;
      ppvar_semantics(ioncount, ionname);
      ioncount++;
    }
    q = q - > next;
    ITERATE(q1, LST(q)) {
      if (SYM(q1) - > nrntype & NRNIONFLAG) {
        SYM(q1) - > nrntype &= ~NRNIONFLAG;
      } else {
        Sprintf(buf, "#define _ion_%s	_nt_data[_ppvar[%d*_STRIDE]]\n",
          SYM(q1) - > name, ioncount);
        q2 = lappendstr(defs_list, buf);
        sprintf(pionname, "_ion_%s", SYM(q1) - > name);
        //printer->print_pragma_ion_var(pionname, ioncount, ION_REGULAR);
        nmp_add_ion_variable(printer, pionname, VAR_DOUBLE, ioncount, ION_REGULAR);
        q2 - > itemtype = VERBATIM;
        sprintf(buf, "  nrn_update_ion_pointer(_%s_sym, _ppvar, %d, %d);\n",
          sion - > name, ioncount, iontype(SYM(q1) - > name, sion - > name));#
        if 0 /*BBCORE*/
        lappendstr(uip, buf);#
        endif /*BBCORE*/
        SYM(q1) - > ioncount_ = ioncount;
        ppvar_semantics(ioncount, ionname);
        ioncount++;
      }
      it = iontype(SYM(q1) - > name, sion - > name);
      if (it == IONCUR) {
        dcurdef = 1;
        Sprintf(buf, "#define _ion_di%sdv\t_nt_data[_ppvar[%d*_STRIDE]]\n", sion - > name, ioncount);
        q2 = lappendstr(defs_list, buf);
        sprintf(pionname, "_ion_di%sdv", sion - > name);
        //printer->print_pragma_ion_var(pionname, ioncount, ION_REGULAR);
        nmp_add_ion_variable(printer, pionname, VAR_DOUBLE, ioncount, ION_REGULAR);
        q2 - > itemtype = VERBATIM;
        sprintf(buf, "  nrn_update_ion_pointer(_%s_sym, _ppvar, %d, 4);\n",
          sion - > name, ioncount);#
        if 0 /*BBCORE*/
        lappendstr(uip, buf);#
        endif /*BBCORE*/
        ppvar_semantics(ioncount, ionname);
        ioncount++;
      }
      if (it == IONIN || it == IONOUT) { /* would have wrote_ion_conc */
        need_style = 1;
      }
    }
    if (need_style) {
      Sprintf(buf, "#define _style_%s\t_ppvar[%d]\n", sion - > name, ioncount);
      q2 = lappendstr(defs_list, buf);
      sprintf(pionname, "_style_%s", sion - > name);
      //printer->print_pragma_ion_var(pionname, ioncount, ION_NEED_STYLE);
      nmp_add_ion_variable(printer, pionname, VAR_DOUBLE, ioncount, ION_NEED_STYLE);
      q2 - > itemtype = VERBATIM;
      sprintf(buf, "#%s", ionname);
      ppvar_semantics(ioncount, buf);
      ioncount++;
    }
    q = q - > next;
    if (!dcurdef && ldifuslist) {
      Sprintf(buf, "#define _ion_di%sdv\t_nt_data[_ppvar[%d*_STRIDE]]\n", sion - > name, ioncount);
      q2 = lappendstr(defs_list, buf);
      sprintf(pionname, "_ion_di%sdv", sion - > name);
      //printer->print_pragma_ion_var(pionname, ioncount, ION_REGULAR);
      nmp_add_ion_variable(printer, pionname, VAR_DOUBLE, ioncount, ION_REGULAR);
      q2 - > itemtype = VERBATIM;
      sprintf(buf, "  nrn_update_ion_pointer(_%s_sym, _ppvar, %d, 4);\n",
        sion - > name, ioncount);#
      if 0 /*BBCORE*/
      lappendstr(uip, buf);#
      endif /*BBCORE*/
      ppvar_semantics(ioncount, ionname);
      ioncount++;
    }
  } * p_pointercount = 0;
  ITERATE(q, nrnpointers) {
    sion = SYM(q);
    if (sion - > nrntype & NRNPOINTER) {
      Sprintf(buf, "#define %s	_nt_data[_ppvar[%d*_STRIDE]]\n",
        sion - > name, ioncount + * p_pointercount);
      ppvar_semantics(ioncount + * p_pointercount, "pointer");
      sprintf(pionname, "%s", sion - > name);
      //printer->print_pragma_ion_var(pionname, ioncount + *p_pointercount, ION_REGULAR);
      nmp_add_ion_variable(printer, pionname, VAR_DOUBLE, ioncount + * p_pointercount, ION_REGULAR);
    }
    /*@todo: forgot or doesnt matter ? */
    if (sion - > nrntype & NRNBBCOREPOINTER) {
      Sprintf(buf, "#define _p_%s	_nt->_vdata[_ppvar[%d*_STRIDE]]\n",
        sion - > name, ioncount + * p_pointercount);
      ppvar_semantics(ioncount + * p_pointercount, "bbcorepointer");
      sprintf(pionname, "_p_%s", sion - > name);
      //printer->print_pragma_ion_var(pionname, ioncount + *p_pointercount, ION_BBCORE_PTR);
      nmp_add_ion_variable(printer, pionname, VAR_DOUBLE, ioncount + * p_pointercount, ION_BBCORE_PTR);
    }
    sion - > used = ioncount + * p_pointercount;
    q2 = lappendstr(defs_list, buf);
    q2 - > itemtype = VERBATIM;
    ( * p_pointercount) ++;
  }

  if (diamdec) { /* must be last */
    Sprintf(buf, "#define diam	*_ppvar[%d]._pval\n", ioncount + * p_pointercount);
    //printer->print_error("diamdec ion declaration not possible!");

    q2 = lappendstr(defs_list, buf);
    q2 - > itemtype = VERBATIM;
  } /* notice that ioncount is not incremented */
  if (areadec) {
    /* must be last, if we add any more the administrative
			procedures must be redone */
    Sprintf(buf, "#define area	_nt->_data[_ppvar[%d*_STRIDE]]\n", ioncount + * p_pointercount + diamdec);
    q2 = lappendstr(defs_list, buf);
    //printer->print_pragma_ion_var("area", ioncount+ *   p_pointercount + diamdec, ION_REGULAR);
    nmp_add_ion_variable(printer, "area", VAR_DOUBLE, ioncount + * p_pointercount + diamdec, ION_REGULAR);
    q2 - > itemtype = VERBATIM;
  } /* notice that ioncount is not incremented */
  if (uip) {
    lappendstr(uip, "}\n");
  }
  return ioncount;
}
```



####### USEION statement ordering

If we take following (fake) example:

```
    USEION na READ ena, ina WRITE ina, nai
```


ina appears on read as well as write list. Is that possible? Neuron generate code as:

```
#define _ion_ena        _nt_data[_ppvar[0*_STRIDE]]
#define _ion_ina        _nt_data[_ppvar[1*_STRIDE]]
#define _ion_nai    _nt_data[_ppvar[2*_STRIDE]]
#define _ion_dinadv _nt_data[_ppvar[3*_STRIDE]]
```

So ina doesnt appear twice. After looking at the mod file, the ASSUMPTION is particular variable appears either in read list
or write list but not in both. If it appears in both list, then currently the order in symbol table is used and which case ina will appear later in write list. Hence the wrong indexes!


######### When write_conc gets written and different ionic type macros are defined?

Here q->next->next is write_ion list where we search for intra/extra conc.

```
	/* Models that write concentration need their INITIAL blocks called
	   before those that read the concentration or reversal potential. */
	i = 0;
	ITERATE(q, useion) {
		ITERATE(q1, LST(q->next->next)) {
			int type;
			type = iontype(SYM(q1)->name, SYM(q)->name);
			if (type == IONIN || type == IONOUT) {
				i += 1;
			}
		}
		q = q->next->next->next;
	}

    nmp_set_writes_conc(printer, i);

	if (i) {
		Lappendstr(defs_list, "\tnrn_writes_conc(_mechtype, 0);\n");
	}
```

Note that IONIN or IONOUT is determined by:

```
static int iontype(s1, s2)	/* returns index of variable in ion mechanism */
	char *s1, *s2;
{
	Sprintf(buf, "i%s", s2);
	if (strcmp(buf, s1) == 0) {
		return IONCUR;
	}
	Sprintf(buf, "e%s", s2);
	if (strcmp(buf, s1) == 0) {
		return IONEREV;
	}
	Sprintf(buf, "%si", s2);
	if (strcmp(buf, s1) == 0) {
		return IONIN;
	}
	Sprintf(buf, "%so", s2);
	if (strcmp(buf, s1) == 0) {
		return IONOUT;
	}
	Sprintf(buf, "%s is not a valid ionic variable for %s", s1, s2);
	diag(buf, (char *)0);
	return -1;
}
```

The loop that processes the use ion statements is following:

```
void nrn_use(q1, q2, q3, q4)
	Item *q1, *q2, *q3, *q4;
{
	int used, i;
	Item *q, *qr, *qw;
	List *readlist, *writelist;
	Symbol *ion;

	ion = SYM(q1);
	/* is it already used */
	used = ion_declared(SYM(q1));
	if (used) { /* READ gets promoted to WRITE */
		diag("mergeing of neuron models not supported yet", (char *)0);
	}else{ /* create all the ionic variables */
		Lappendsym(useion, ion);
		readlist = newlist();
		writelist = newlist();
		qr = lappendsym(useion, SYM0);
		qw = lappendsym(useion, SYM0);
		if (q4) {
			lappendstr(useion, STR(q4));
		}else{
			lappendstr(useion, "-10000.");
		}
		LST(qr) = readlist;
		LST(qw) = writelist;
		if (q2) { Item *qt = q2->next;
			move(q1->next->next, q2, readlist);
			if (q3) {
				move(qt->next, q3, writelist);
			}
		}else if (q3) {
			move(q1->next->next, q3, writelist);
		}
		ITERATE(q, readlist) {
			i = iontype(SYM(q)->name, ion->name);
			if (i == IONCUR) {
				SYM(q)->nrntype |= NRNCURIN;
			}else{
				SYM(q)->nrntype |= NRNPRANGEIN;
				if (i == IONIN || i == IONOUT) {
					SYM(q)->nrntype |= IONCONC;
				}
			}
		}
		ITERATE(q, writelist) {
			i = iontype(SYM(q)->name, ion->name);
			if (i == IONCUR) {
				if (!currents) {
					currents = newlist();
				}
				Lappendsym(currents, SYM(q));
				SYM(q)->nrntype |= NRNCUROUT;
			}else{
				SYM(q)->nrntype |= NRNPRANGEOUT;
				if (i == IONIN || i == IONOUT) {
					SYM(q)->nrntype |= IONCONC;
				}
			}
		}
	}
}
```


######### Does derivX_advance (X is 1, 2) can appear multiple times?

main() in modl.c calls  solvhandler() which in turn calls solvhandler() in solve.c. There we loop over all solve blocks:

```
	ITERATE(lq, solvq) { /* remember it goes by 3's */
		steadystate=0;
		btype = lq->itemtype;
		if (btype < 0) {
			btype = lq->itemtype = -btype;

		case DERF:
			if (method == SYM0) {
				method = lookup("adrunge");
			}
			if (btype == BREAKPOINT && !steadystate) {
				/* derivatives recalculated after while loop */
				if (strcmp(method->name, "cnexp") != 0 && strcmp(method->name, "derivimplicit") != 0 && strcmp(method->name, "euler") != 0) {
					fprintf(stderr, "Notice: %s is not thread safe. Complain to Hines\n", method->name);
					vectorize = 0;
					Sprintf(buf, " %s();\n", fun->name);
					Insertstr(follow, buf);
				}
				/* envelope calls go after the while loop */
				sens_nonlin_out(follow, fun);
				#if CVODE
				cvode_interface(fun, listnum, numeqn);
				#endif
			}
			if (btype == BREAKPOINT) whileloop(qsol, (long)DERF, steadystate);
			solv_diffeq(qsol, fun,  method, numeqn, listnum, steadystate, btype);
```

And solv_diffeq() is the one who print out derivimplicit related code:

```
if (deriv_imp_list) {	/* make sure deriv block translation matches method */
	Item *q; int found=0;
	ITERATE(q, deriv_imp_list) {
		if (strcmp(STR(q), fun->name) == 0) {
			found = 1;
		}
	}
	if ((strcmp(method->name, Derivimplicit) == 0) ^ (found == 1)) {
	diag("To use the derivimplicit method the SOLVE statement must\
 		precede the DERIVATIVE block\n",
		" and all SOLVEs using that block must use the derivimplicit method\n");
	}
	derivimplic_listnum = listnum;
	Sprintf(buf, "static int _deriv%d_advance = 1;\n", listnum);
	q = linsertstr(procfunc, buf);
	Sprintf(buf, "\n#define _deriv%d_advance _thread[%d]._i\n\
	#define _dith%d %d\n#define _newtonspace%d _thread[%d]._pvoid\nextern void* nrn_cons_newtonspace(int, int);\n\
", listnum, thread_data_index, listnum, thread_data_index+1, listnum, thread_data_index+2);
```

So if there are multiple solve blocks appear in the MOD file then only this can appear twice.


######### where  nrn_newton_thread(_newtonspac.. get printed?

mixed_eqns is where the callbacks get printed:

```
Item *mixed_eqns(q2, q3, q4)	/* name, '{', '}' */
	Item *q2, *q3, *q4;
{
	int counts;
	Item *qret;
	Item* q;

	if (!eqnq) {
		return ITEM0; /* no nonlinear algebraic equations */
	}
	/* makes use of old massagenonlin split into the guts and
	the header stuff */
	numlist++;
	counts = nonlin_common(q4, 0);
	q = insertstr(q3, "{\n");
	sprintf(buf, "{ double* _savstate%d = (double*)_thread[_dith%d]._pval;\n\
 double* _dlist%d = (double*)(_thread[_dith%d]._pval) + (%d*_cntml_padded);\n",
numlist-1, numlist-1,
numlist, numlist-1, counts);
	vectorize_substitute(q, buf);

	Sprintf(buf, "error = newton(%d,_slist%d, _p, _newton_%s%s, _dlist%d);\n",
		counts, numlist, SYM(q2)->name, suffix, numlist);
	qret = insertstr(q3, buf);
	Sprintf(buf,
	  "#pragma acc routine(nrn_newton_thread) seq\n"
#if 0
	  "_reset = nrn_newton_thread(_newtonspace%d, %d,_slist%d, _newton_%s%s, _dlist%d,  _threadargs_);\n"
#else
	  "_reset = nrn_newton_thread(_newtonspace%d, %d,_slist%d, _derivimplicit_%s%s, _dlist%d,  _threadargs_);\n"
#endif
	  , numlist-1, counts, numlist, SYM(q2)->name, suffix, numlist);
	vectorize_substitute(qret, buf);
	Insertstr(q3, "/*if(_reset) {abort_run(_reset);}*/ }\n");
	Sprintf(buf,
	  "extern int _newton_%s%s(_threadargsproto_);\n"
	  , SYM(q2)->name, suffix);
	Linsertstr(procfunc, buf);

	Sprintf(buf,
	  "\n"
	  "/* _derivimplicit_ %s %s */\n"
	  "#ifndef INSIDE_NMODL\n"
	  "#define INSIDE_NMODL\n"
	  "#endif\n"
	  "#include \"_kinderiv.h\"\n"
	  , SYM(q2)->name, suffix);
	Linsertstr(procfunc, buf);

	Sprintf(buf, "\n  return _reset;\n}\n\nint _newton_%s%s (_threadargsproto_) {  int _reset=0;\n", SYM(q2)->name, suffix);
	Insertstr(q3, buf);
	q = insertstr(q3, "{ int _counte = -1;\n");
	sprintf(buf, "{ double* _savstate%d = (double*)_thread[_dith%d]._pval;\n\
 double* _dlist%d = (double*)(_thread[_dith%d]._pval) + (%d*_cntml_padded);\n int _counte = -1;\n",
numlist-1, numlist-1,
numlist, numlist-1, counts);
	vectorize_substitute(q, buf);

	Insertstr(q4, "\n  }");
	return qret;
}
```


##### Thread promoted variables may not appear in the thread in NOCMODL and code will be different !

See below example in old ProbAMPANMDA_EMS.mod:

```

PARAMETER {
	mggate
}

BREAKPOINT {

        SOLVE state
        mggate = 1 / (1 + exp(0.062 (/mV) * -(v)) * (mg / 3.57 (mM))) :mggate kinetics - Jahr & Stevens 1990
        g_AMPA = gmax*(B_AMPA-A_AMPA) :compute time varying conductance as the difference of state variables B_AMPA and A_AMPA
        g_NMDA = gmax*(B_NMDA-A_NMDA) * mggate :compute time varying conductance as the difference of state variables B_NMDA and A_NMDA and mggate kinetics
        g = g_AMPA + g_NMDA
        i_AMPA = g_AMPA*(v-e) :compute the AMPA driving force based on the time varying conductance, membrane potential, and AMPA reversal
        i_NMDA = g_NMDA*(v-e) :compute the NMDA driving force based on the time varying conductance, membrane potential, and NMDA reversal
        i = i_AMPA + i_NMDA
}

```

As mggate is parameter and being written, NEURON will promote this to thread variable. But, nocmodl localizer pass will convert the mggate to local variable and then mggate in global symbol table is not written and hence wont be promoted to Thread variable.

It seems this is ok because we should use optimized mod file for NEURON as well. And in that case code generated by NEURON will be compatible with CoreNEURON.