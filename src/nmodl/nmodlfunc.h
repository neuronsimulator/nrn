struct Rlist;
struct Reaction;

int in_solvefor(Symbol*);
int cvode_cnexp_success(Item* q1, Item* q2);

void cvode_parse(Symbol* s, List* e);
void Unit_push(const char*);
void unit_pop();
void unit_div();
void install_units(char* s1, char* s2);
void modl_units();
void consistency();
void printlist(List*);
void vectorize_substitute(Item* q, const char* str);
void vectorize_do_substitute();
void solv_diffeq(Item* qsol,
                 Symbol* fun,
                 Symbol* method,
                 int numeqn,
                 int listnum,
                 int steadystate,
                 int btype);
void add_deriv_imp_list(char*);
void deriv_used(Symbol* s, Item* q1, Item* q2);
void massagederiv(Item* q1, Item* q2, Item* q3, Item* q4);
void copyitems(Item* q1, Item* q2, Item* qdest);
void disc_var_seen(Item* q1, Item* q2, Item* q3, int array);
void massagediscblk(Item* q1, Item* q2, Item* q3, Item* q4);
void init_disc_vars();
void init();
void inblock(char*);
void unGets(char*);
void diag(const char*, const char*);
void enquextern(Symbol*);
void include_file(Item*);
void reactname(Item* q1, Item* lastok, Item* q2);
void leftreact();
void massagereaction(Item* qREACTION, Item* qREACT1, Item* qlpar, Item* qcomma, Item* qrpar);
void flux(Item* qREACTION, Item* qdir, Item* qlast);
void massagekinetic(Item* q1, Item* q2, Item* q3, Item* q4);
void fixrlst(struct Rlist*);
void kinetic_intmethod(Symbol* fun, const char* meth);
void genfluxterm(struct Reaction* r, int type, int n);
void kinetic_implicit(Symbol* fun, const char* dt, const char* mname);
void massageconserve(Item* q1, Item* q3, Item* q5);
void check_block(int standard, int actual, const char* mes);
void massagecompart(Item* qexp, Item* qb1, Item* qb2, Symbol* indx);
void massageldifus(Item* qexp, Item* qb1, Item* qb2, Symbol* indx);
void kin_vect1(Item* q1, Item* q2, Item* q4);
void kin_vect2();
void kin_vect3(Item* q1, Item* q2, Item* q4);
void prn(Item* q1, Item* q2);
void cvode_kinetic(Item* qsol, Symbol* fun, int numeqn, int listnum);
void freelist(List**);
void remove(Item*);
void deltokens(Item*, Item*);
void move(Item* q1, Item* q2, Item* q3);
void movelist(Item* q1, Item* q2, List* s);
void replacstr(Item* q, const char* s);
void c_out();
void printitem(Item*);
void debugprintitem(Item*);
void c_out_vectorize();
void vectorize_substitute(Item* q, const char* str);
void vectorize_do_substitute();
void nrninit();
void parout();
void warn_ignore(Symbol*);
void ldifusreg();
void decode_ustr(Symbol* sym, double* pg1, double* pg2, char* s);
void units_reg();
void nrn_list(Item*, Item*);
void bablk(int ba, int type, Item* q1, Item* q2);
void nrn_use(Item* q1, Item* q2, Item* q3, Item* q4);
void nrn_var_assigned(Symbol*);
void slist_data(Symbol* s, int indx, int findx);
void out_nt_ml_frag(List*);
void cvode_emit_interface();
void cvode_proced_emit();
void cvode_interface(Symbol* fun, int num, int neq);
void cvode_valid();
void cvode_rw_cur(char (&b)[NRN_BUFSIZE]);
void net_receive(Item* qarg, Item* qp1, Item* qp2, Item* qstmt, Item* qend);
void net_init(Item* qinit, Item* qp2);
void fornetcon(Item* keyword, Item* par1, Item* args, Item* par2, Item* stmt, Item* qend);
void chk_thread_safe();
void chk_global_state();
void check_useion_variables();
void explicit_decl(Item* q);
void parm_array_install(Symbol* n, const char* num, char* units, char* limits, int index);
void parminstall(Symbol* n, const char* num, const char* units, const char* limits);
void indepinstall(Symbol* n, const char* from, const char* to, const char* with, const char* units);
void depinstall(int type,
                Symbol* n,
                int index,
                const char* from,
                const char* to,
                const char* units,
                Item* qs,
                int makeconst,
                const char* abstol);
void statdefault(Symbol* n, int index, const char* units, Item* qs, int makeconst);
void vectorize_scan_for_func(Item* q1, Item* q2);
void defarg(Item* q1, Item* q2);
void lag_stmt(Item* q1, int blocktype);
void add_reset_args(Item*);
void add_nrnthread_arg(Item*);
void check_tables();
void table_massage(List* tablist, Item* qtype, Item* qname, List* arglist);
void hocfunchack(Symbol* n, Item* qpar1, Item* qpar2, int hack);
void hocfunc(Symbol* n, Item* qpar1, Item* qpar2);
void vectorize_use_func(Item* qname, Item* qpar1, Item* qexpr, Item* qpar2, int blocktype);
void function_table(Symbol* s, Item* qpar1, Item* qpar2, Item* qb1, Item* qb2);
void watchstmt(Item* par1, Item* dir, Item* par2, Item* flag, int blocktype);
void threadsafe(const char*);
void nrnmutex(int, Item*);
void solv_nonlin(Item* qsol, Symbol* fun, Symbol* method, int numeqn, int listnum);
void solv_lineq(Item* qsol, Symbol* fun, Symbol* method, int numeqn, int listnum);
void eqnqueue(Item*);
void massagenonlin(Item* q1, Item* q2, Item* q3, Item* q4);
void init_linblk(Item*);
void init_lineq(Item*);
void lin_state_term(Item* q1, Item* q2);
void linterm(Item* q1, Item* q2, int pstate, int sign);
void massage_linblk(Item* q1, Item* q2, Item* q3, Item* q4);
void solvequeue(Item* q1, Item* q2, int blocktype);
void solvhandler();
void save_dt(Item*);
void symbol_init();
void pushlocal();
void poplocal();

void conductance_hint(int blocktype, Item* q1, Item* q2);
void possible_local_current(int blocktype, List* symlist);
Symbol* breakpoint_current(Symbol* s);
void netrec_asgn(Item* varname, Item* equal, Item* expr, Item* lastok);
void netrec_discon();
char* items_as_string(Item* begin, Item* last); /* does not include last */
int slist_search(int listnum, Symbol* s);
void nrnunit_str(char (&buf)[NRN_BUFSIZE], const char* name, const char* unit1, const char* unit2);

// help know if setdata required to call FUNCTION or PROCEDURE
void check_range_in_func(Symbol*);
void set_inside_func(Symbol*);
void func_needs_setdata();
void hocfunc_setdata_item(Symbol*, Item*);
