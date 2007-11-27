
extern	int eval(), add(), sub(), mul(), hoc_div(), hoc_cyclic(), negate(), power();
extern	int assign(), bltin(), varpush(), constpush(), print(), varread();
extern	int nopop(), prexpr(), prstr(), assstr(), pushzero();
extern	int gt(), lt(), eq(), ge(), le(), ne(), hoc_and(), hoc_or(), hoc_not();
extern	int ifcode(), forcode(), shortfor(), call(), arg(), argassign();
extern int hoc_argrefasgn(), hoc_argref(), hoc_iterator(), hoc_iterator_stmt();
extern	int funcret(), procret(), Break(), Continue(), Stop();
extern	int debug(), edit(), hoc_evalpointer();
extern	int hoc_newline(), hoc_delete_symbol(), hoc_stringarg(), hoc_push_string();
extern	int hoc_parallel_begin(), hoc_parallel_end(), hoc_argrefarg();

/* OOP */
extern int hoc_objectvar(), hoc_object_component(), hoc_object_eval();
extern int hoc_object_asgn(), hoc_objvardecl(), hoc_cmp_otype(), hoc_newobj();
extern int hoc_asgn_obj_to_str(), hoc_known_type(), hoc_push_string();
extern int hoc_objectarg(), hoc_ob_pointer(), hoc_constobject();
extern int hoc_push_current_object(), hoc_newobj_arg();
extern int hoc_autoobject(), hocobjret(), hoc_newobj_ret();
/* END OOP */

/* NEWCABLE */
extern	int connectsection(), add_section(), range_const(), range_interpolate();
extern	int clear_sectionlist(), install_sectionlist();
extern  int rangevareval(), sec_access(), mech_access();
extern	int for_segment(), for_segment1();
extern	int sec_access_temp(), sec_access_push(), sec_access_pop();
extern	int rangepoint(), forall_section(), hoc_ifsec();
extern	int rangevarevalpointer();
extern	int connectpointer(), connect_point_process_pointer(), nrn_cppp();
extern  int ob_sec_access(), sec_access_object();
extern	int forall_sectionlist(), connect_obsec_syntax();
extern	int hoc_ifseclist(), mech_uninsert();
extern	int simpleconnectsection(), range_interpolate_single();
/* END NEWCABLE*/
