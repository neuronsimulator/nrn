#include <../../nrnconf.h>
/* $Header$ */
/*
$Log$
Revision 1.1  2003/02/11 18:36:09  hines
Initial revision

Revision 1.1.1.1  2003/01/01 17:46:35  hines
NEURON --  Version 5.4 2002/12/23 from prospero as of 1-1-2003

Revision 1.1.1.1  2002/06/26 16:23:07  hines
NEURON 5.3 2002/06/04

 * Revision 1.1.1.1  2001/01/01  20:30:35  hines
 * preparation for general sparse matrix
 *
 * Revision 1.3  2000/03/27  14:18:12  hines
 * All sources now include nrnconf.h instead of config.h so that minor
 * option changes (eg in src/parallel/bbsconf.h) do not require recompilation
 * of everything.
 *
 * Revision 1.2  2000/03/15  19:02:16  hines
 * trying to merge nrn4.3.0 stuff that used imake into the nrn4.2.3 distribution.
 * Now called nrn4.3.1
 *
 * Revision 1.1.1.1  2000/03/09  13:55:34  hines
 * almost working
 *
 * Revision 1.2  1995/05/19  12:51:40  hines
 * change the oc version number and date
 *
 * Revision 1.1.1.1  1994/10/12  17:22:14  hines
 * NEURON 3.0 distribution
 *
 * Revision 2.116  1994/09/27  17:03:05  hines
 * when oc_jump_target failed then the next error could try to longjump to
 * a function that had already returned. fixed.
 *
 * Revision 2.115  1994/09/26  18:25:40  hines
 * fix warning on sgi
 *
 * Revision 2.114  1994/09/20  16:20:06  hines
 * port to dec alpha
 *
 * Revision 2.113  1994/09/20  14:50:42  hines
 * port to dec alpha (cxx thinks struct name inside struct is new def)
 *
 * Revision 2.112  1994/09/20  14:49:25  hines
 * execute1("cmd") will return even on execerror. Note: does
 * not surround with {}
 *
 * Revision 2.111  1994/09/12  13:37:14  hines
 * execute("~command") will eliminate tilde and execute command without
 * enclosing it in {}. This allows one to create func, proc dynamically.
 *
 * Revision 2.110  1994/08/18  12:19:15  hines
 * allow ob.sec.range[i](x)
 * this necessitated allowing parsing of ob[i](x) and then giving
 * an error dynamically.
 *
 * Revision 2.109  1994/08/17  19:48:49  hines
 * ocmain.o : get it from lib (nocable and ocnoiv too)
 *
 * Revision 2.108  1994/08/05  14:16:14  hines
 * show_errmess_always()
 * errno not defined from include file in hoc.c
 * NullArgument replaces null arguments in Imakefile
 *
 * Revision 2.107  1994/07/30  17:33:51  hines
 * checking whether bison or yacc is used
 *
 * Revision 2.106  1994/07/30  14:52:40  hines
 * pwman_place(left, top)
 *
 * Revision 2.105  1994/07/12  17:52:26  hines
 * more portable way of dealing with ocjump
 *
 * Revision 2.104  1994/07/12  13:47:54  hines
 * from last change there are 80 shift reduce conflicts
 *
 * Revision 2.103  1994/07/12  13:45:48  hines
 * can use constant objects of form
 * template[index].
 * now object names are printed in this format as well.
 *
 * Revision 2.102  1994/07/04  15:10:54  hines
 * only print Oc banner if not NEURON
 *
 * Revision 2.101  1994/06/29  12:37:28  hines
 * can switch between bison and yacc. so far I have never used the partial
 * parseing feature of bison anyway. This works around a problem on some
 * crays in which there is a bug in bison.
 *
 * Revision 2.100  1994/06/21  12:36:04  hines
 * allow recursive calls to interpreter even in case of hoc_execerror.
 *
 * Revision 2.99  1994/06/17  13:00:35  hines
 * a number of things fixed while looking at the test coverage of hoc.c
 * mostly just to allow coverage.
 * CBUFSIZE is larger than 256
 * array index computed with double arithmetic.
 * corrected handling of end of file while in comments
 *
 * Revision 2.98  1994/06/17  12:53:28  hines
 * when a hoc_exec_cmd fails another execerror is called since
 * otherwise the pc is invalid. at this time it is not possible to
 * recover while still interpreting since the interpreter is
 * initialized. This is not a problem with buttons, etc since when
 * they are running the interpreter at the top level and they don't get
 * control until the interpreter is done.
 *
 * Revision 2.97  1994/06/07  20:54:40  hines
 * anyname (eg local) can also be a mechanism name
 *
 * Revision 2.96  1994/06/07  20:46:12  hines
 * all_objectvars not setting objectdata when accessing arrayinfo
 * fixed
 *
 * Revision 2.95  1994/06/07  18:42:38  hines
 * allow redeclaration of names in templates that are built-in names
 *
 * Revision 2.94  1994/06/07  15:01:12  hines
 * no trailing blank after an object name
 *
 * Revision 2.93  1994/05/24  17:36:11  hines
 * allobjects() prints all object names with number of references
 *
 * Revision 2.92  1994/05/24  15:41:38  hines
 * allobjectrefs recurses on this. Fixed
 *
 * Revision 2.91  1994/05/20  19:13:58  hines
 * object_push(objref) and object_pop()
 * allow more easy saving of internal state of objects.
 * after an object_puhs(objref) subsequent commands are executed in
 * the context of the object til the next object_pop()
 * Warning: do not create new variables within one of these object contexts.
 *
 * Revision 2.90  1994/05/10  20:19:14  hines
 * global lineno no longer initialized when defined.
 * Note: there is a potential problem when calling oc_run() from
 * a descendent of an object since when there is an error, the highest
 * caller gets the execerror and not the lowest. it would be better ot
 * nicely unwrap all the calls instead of doing a longjmp
 *
 * Revision 2.89  1994/05/09  19:11:46  hines
 * call_constructor checks if object is a point process and if so
 * sets the Point_process->ob field so one can know the object given
 * the point process structure. This was done incorrectly a little while
 * ago.
 *
 * Revision 2.88  1994/05/09  13:54:21  hines
 * observers hook for templates.
 *
 * Revision 2.87  1994/05/05  18:19:32  hines
 * error when hoc starts out interpreting a template. fixed
 *
 * Revision 2.86  1994/05/03  15:15:30  hines
 * checkpoint returns int not void
 *
 * Revision 2.85  1994/05/03  15:11:02  hines
 * built-in classes can be checkpointed
 *
 * Revision 2.84  1994/05/02  12:53:54  hines
 * quit() function
 *
 * Revision 2.83  1994/04/27  12:43:47  hines
 * objref is keyword synonym for objectvar
 *
 * Revision 2.82  1994/04/27  12:17:56  hines
 * minor warnings from objectcenter fixed (and one serious problem from
 * last checkin)
 * PI, E, etc are now userdoubles and are available within objects.
 *
 * Revision 2.81  1994/04/27  11:21:31  hines
 * support for ivoc/SRC/checkpoint.c
 * usage is checkpoint("filename")
 * then ivoc filename ...
 * so far only for oc
 *
 * Revision 2.80  1994/04/06  17:14:24  hines
 * minor changes to structures to allow checkpointing in ivoc
 * basically no more codepfd
 *
 * Revision 2.79  1994/04/05  16:35:20  hines
 * hook for checkpoint
 * hoc_new_object_asgn(Object**, Symbol*, void*)
 * double hoc_call_objfunc(Symbol*, int narg, Object*)
 *
 * Revision 2.78  1994/03/15  17:09:14  hines
 * minor leak plugged for "this" in templates
 *
 * Revision 2.77  1994/03/10  15:26:07  hines
 * capture SIGPIPE print message and ignore
 *
 * Revision 2.76  1994/03/10  14:33:13  hines
 * error on last checkin
 *
 * Revision 2.75  1994/03/10  14:28:33  hines
 * invoke coredump_on_error() if want it on next segmentation violation or
 * bus error.
 *
 * Revision 2.74  1994/03/07  21:51:53  hines
 * make ob = new Obj(ob) act appropriately and don't make assignment til
 * after init was called.
 *
 * Revision 2.73  1994/03/04  21:37:01  hines
 * help for files invoked on command line
 *
 * Revision 2.72  1994/03/04  18:07:22  hines
 * *** empty log message ***
 *
 * Revision 2.71  1994/02/23  15:51:16  hines
 * RETREAT_SIGNAL (SIGHUP) handled and user written routine "linda_retreat()"
 * called on a doEvent(). Just exit if no such routine.
 *
 * Revision 2.70  1994/02/23  12:53:31  hines
 * help word word ... in ivoc tickles the help system
 *
 * Revision 2.69  1994/02/10  19:41:06  hines
 * set term=vt125 replaces vt100 in plot.c since that is a common term
 * even in a window system
 *
 * Revision 2.68  1994/01/21  18:14:31  hines
 * some NeXT portability fixes and turn off parse error messages when
 * hoc_execerror_message == 0
 *
 * Revision 2.67  1994/01/11  20:22:24  hines
 * set errno to 0 every input line so don't get spurious interrupted
 * system call messages from ivoc select system call
 *
 * Revision 2.66  1993/12/15  15:02:01  hines
 * SECTIONREF and allow ob connect ob()(0or1), expr
 *
 * Revision 2.65  1993/11/09  14:30:04  hines
 * partial port to djg dos extender go32
 *
 * Revision 2.64  1993/11/04  15:58:42  hines
 * forgot to checkin last time
 *
 * Revision 2.63  1993/11/04  15:55:48  hines
 * port to solaris2 (no more warnings)
 *
 * Revision 2.62  1993/10/18  13:27:38  hines
 * beginnings of an audit system
 *
 * Revision 2.61  1993/09/06  13:33:54  hines
 * hoc_scan reads one extra character after scanning the number and
 * discards it if it is a newline. Otherwise it ungetc's it.
 *
 * Revision 2.60  93/09/01  08:23:33  hines
 * some string assignment statements not popping stack so loops can eventually
 * give stack overflow
 * 
 * Revision 2.59  93/08/19  16:38:20  hines
 * if (!0) print "hello"
 * now works. before the ! was not transmitted from yylex as NOT
 * and print "hello" was given the wrong $$ code index
 * 
 * Revision 2.58  93/08/15  12:54:53  hines
 * fix warning of function declared external and later static
 * 
 * Revision 2.56  93/08/14  15:05:37  hines
 * when in multiline statement typed directly to interpreter from console
 * we shut off the event driven interface so that there can be no 
 * extraneous statements mixed with the current statement.
 * 
 * Revision 2.55  93/08/13  17:11:26  hines
 * fix bug where we did not allow multiple line statements entered
 * from console as direct command
 * 
 * Revision 2.54  93/08/11  11:04:49  hines
 * Zach Mainen's contribution of xstatebutton and xcheckbox
 * 
 * Revision 2.53  93/07/23  11:33:26  hines
 * in hoc_total_array_data didn't work for RANGEVAR
 * 
 * Revision 2.52  93/07/19  09:36:15  hines
 * freeing double arrays has different notify call
 * 
 * Revision 2.51  93/07/08  14:23:58  hines
 * portable to SGI
 * 
 * Revision 2.49  93/06/30  11:28:45  hines
 * expand_env_var needed a static buf for return.
 * 
 * Revision 2.48  93/05/25  14:02:39  hines
 * some functions that check on the arg type. eg hoc_is_object_arg(narg)
 * 
 * Revision 2.47  93/05/04  15:26:43  hines
 * another place where obdsave needs to remember that hoc_top_level_data
 * may change
 * 
 * Revision 2.46  93/04/29  15:30:13  hines
 * notify_pointer_freed called whenever an object is freed.
 * 
 * Revision 2.45  93/04/28  10:07:18  hines
 * when a new variable is created at the top level then hoc_top_level_data
 * is reallocated. (this can happen when xopen is called) however the
 * function call handlers save and restore a pointe to hoc_objectdata and
 * between save and restore the pointer may become invalid. This is
 * avoided with the idiom
 * Objectdata* obdsav = hoc_objectdata_save()
 * ...
 * hoc_objectdata = hoc_objectdata_restore(obdsav);
 * 
 * Revision 2.44  93/04/21  10:34:49  hines
 * datum element can be _pvoid for generic pointer storage
 * 
 * Revision 2.43  93/04/20  08:40:24  hines
 * lists can have void* elements (use insertvoid lappendvoid VOIDITM(q) )
 * 
 * Revision 2.42  93/04/17  07:02:58  hines
 * IRIX can use alloca
 * 
 * Revision 2.41  93/04/14  15:04:04  hines
 * some fixes to Imakefile
 * 
 * Revision 2.40  93/04/13  12:12:19  hines
 * SIG_RETURN already defined in HPUX : so defined as SIG_RETURN_TYEP
 * 
 * Revision 2.39  93/04/12  14:08:44  hines
 * first pass at merging Fisher's LINDA addition
 * 
 * Revision 2.38  93/03/18  07:53:34  hines
 * colors made static
 * 
 * Revision 2.37  93/03/15  10:04:44  hines
 * assert needs to be #undef before #define
 * 
 * Revision 2.36  93/03/11  09:32:32  hines
 * better error message when pointer to rangevar does not have arc spec
 * 
 * Revision 2.35  93/03/11  07:38:03  hines
 * Nigel Goddards fix to div() so it works on a cray.
 * 
 * Revision 2.34  93/03/09  09:46:14  hines
 * Object** hoc_temp_objvar(Symbol* template_symbol, void* cppobject)
 * not entirely safe when "function returning object" is arg to another
 * function. Safe when used in context
 * 	o = obj.function_returning_object()
 * 
 * Revision 2.33  93/03/09  07:45:20  hines
 * hoc_call_func accepts BLTIN
 * 
 * Revision 2.32  93/03/05  15:57:44  hines
 * slider and can call a user interpreted function or procedure via
 * hoc_call_func(Symbol*, int narg) with args on stack in first arg order
 * 
 * Revision 2.31  93/03/05  08:42:20  hines
 * some slight performance improvements by avoidin hoc_lookup for objects
 * and use stack macros in code.c
 * 
 * Revision 2.30  93/03/02  08:30:12  hines
 * parallization of shortfor loop as in
 * parallel for i=1,10 print i
 * 
 * Revision 2.29  93/02/23  14:44:05  hines
 * compiles under DOS
 * 
 * Revision 2.28  93/02/22  14:29:58  hines
 * nobody uses scanf("%F"..., everybody accepts scanf("%lf"...
 * 
 * Revision 2.27  93/02/22  13:12:30  hines
 * works with turboc on DOS (but no readline)
 * 
 * Revision 2.26  93/02/22  10:57:34  hines
 * checking of malloc out of order
 * 
 * Revision 2.25  93/02/20  08:55:45  hines
 * AIX
 * 
 * Revision 2.24  93/02/15  08:12:32  hines
 * for Linus
 * I mean linux
 * 
 * Revision 2.23  93/02/12  10:01:10  hines
 * public names which are top level functions generated a syntax error.
 * 
 * Revision 2.22  93/02/12  08:51:44  hines
 * beginning of port to PC-Dos
 * 
 * Revision 2.21  93/02/11  17:04:30  hines
 * some fixes for NeXT
 * 
 * Revision 2.20  93/02/03  11:28:01  hines
 * a bit more generic for portability. will possibly work on NeXT
 * 
 * Revision 2.19  93/02/02  10:34:38  hines
 * static functions declared before used
 * 
 * Revision 2.18  93/01/27  13:54:00  hines
 * get METHOD3 from options.h
 * 
 * Revision 2.17  93/01/23  13:40:47  hines
 * don't inlclude stdio.h twice in ytab.c
 * 
 * Revision 2.16  93/01/22  17:34:29  hines
 * ocmodl, ivmodl working both for shared and static libraries
 * 
 * Revision 2.15  93/01/21  16:21:30  hines
 * load_func("name"), load_proc("name1", ...), load_template(...)
 * searches *.oc and *.hoc in . HOCLIBRARYPATH, and $NEURONHOME/lib/hoc
 * for the appropriate declaration and xopen's the first file that contains
 * such a declaration.
 * 
 * Revision 2.14  93/01/21  13:19:34  hines
 * // commenst out the rest of the line
 * 
 * Revision 2.13  93/01/21  12:45:49  hines
 * memory leak on multiple strdef of same symbol
 * 
 * Revision 2.12  93/01/15  08:20:25  hines
 * can print nam of object with printf("%s",object)
 * 
 * Revision 2.11  93/01/14  13:04:06  hines
 * switch from Objectdata.pstr to Objectdata.ppstr so we can eventually have
 * arrays of strings and so ivoc's varlabel will work properly.
 * 
 * Revision 2.10  93/01/13  08:52:17  hines
 * stubs for xvarlabel, xradiobutton, and xfixedvalue
 * mechanism in place for notifying if strdef is freed.
 * 
 * Revision 2.9  93/01/12  16:55:39  hines
 * really 76 shift/reduce conflicts
 * 
 * Revision 2.8  93/01/12  16:53:17  hines
 * "str" = strdef was being allowed
 * 
 * Revision 2.7  93/01/12  08:58:49  hines
 * to assign to a hoc string use
 * hoc_assign_str(char** cpp, char* buf)
 * Some minor modifications to allow use of some functions by File class
 * 
 * Revision 2.6  93/01/08  17:08:39  hines
 * used by ivoc to make a hocusr with makeiv
 * 
 * Revision 2.5  93/01/06  09:26:23  hines
 * cray c90 needs include <errno.h>   remove some codecenter warnings
 * 
 * Revision 2.4  93/01/05  17:20:19  hines
 * print perror() instead of errno=
 * void sec_access_pop  Section* nrn_pop_sec
 * 
 * Revision 2.1  92/12/31  09:38:16  hines
 * ready for beta release
 * 
 * Revision 1.130  92/12/30  12:45:56  hines
 * don't require empty line after endtemplate
 * 
 * Revision 1.129  92/12/22  16:53:21  hines
 * problem with new lines after public and external fixed.
 * public and external now can be anywhere in template but they had better
 * be before declarations (at least for external) and usage.
 * 
 * Revision 1.128  92/12/22  15:29:31  hines
 * a template can have an external statement followin the public
 * statement which is a comma separated list of functions or procedures
 * defined at the top level which can be executed within the object.
 * (the function is executed in a top level context).
 * public and external statements are optional
 * 
 * Revision 1.127  92/12/18  15:12:03  hines
 * fix bug in which pointers to range variables in object not allowed.
 * 
 * Revision 1.126  92/12/11  09:41:33  hines
 * execute("command", [objvar]) executes command in context of object (top-level
 * if arg not present). int hoc_obj_execute(const char* cmd, Object*) used
 * in C.
 * 
 * Revision 1.125  92/11/27  08:19:33  hines
 * can turn off error messages temporarily
 * 
 * Revision 1.124  92/11/16  11:20:23  hines
 * error in x.c message when color doesn't exist
 * xlabel("string")
 * 
 * Revision 1.123  92/11/13  16:09:20  hines
 * parseexec gives execerror when incomplete parse
 * 
 * Revision 1.122  92/11/13  07:34:23  hines
 * hoc_object_pathname had problem with recursive objects
 * 
 * Revision 1.121  92/11/11  12:40:55  hines
 * hoc_val_pointer more robust.
 * boolean is_obj_type(Object*, typename)
 * 
 * Revision 1.120  92/11/10  07:38:37  hines
 * char* expand_env_var(char*) expands all the $(...) in the string.
 * moved from ivoc/main.c to here so oc can use it too.
 * xopen("...") now allows environment variables
 * 
 * Revision 1.119  92/11/09  17:14:28  hines
 * botched last imakefile checkin by checking in the wrong one
 * 
 * Revision 1.118  92/11/09  17:12:53  hines
 * trivial fix to Imakefile
 * 
 * Revision 1.117  92/10/30  16:05:01  hines
 * echoing proper number of shift/reduce conflicts
 * 
 * Revision 1.116  92/10/29  16:45:28  hines
 * char* hoc_object_pathname(Object*)
 * 
 * Revision 1.115  92/10/29  09:20:16  hines
 * some errors in freeing objects fixed and replace usage of getarg for
 * non numbers.
 * 
 * Revision 1.114  92/10/28  17:33:51  hines
 * *getarg() now checks to make sure it is returning a number
 * 
 * Revision 1.113  92/10/27  12:08:18  hines
 * list.c hoclist.h moved from nrnoc to oc
 * all templates maintain a list of their objects
 * 
 * Revision 1.112  92/10/27  09:17:48  hines
 * rangevar(x) = y is an allowed syntax and sets the value at the segment that
 * contains x
 * 
 * Revision 1.111  92/10/24  12:12:48  hines
 * botched last checkin. now its right.
 * 
 * Revision 1.110  92/10/24  12:02:22  hines
 * portability fixes found at Pittsburgh for CRAY and HP
 * 
 * Revision 1.109  92/10/24  10:27:21  hines
 * error in stub for xpvalue()
 * 
 * Revision 1.108  92/10/24  10:17:49  hines
 * botched last checkin for symbol.c. notifies interviews when hoc_free_val..
 * called
 * 
 * Revision 1.107  92/10/24  10:16:35  hines
 * noiv.c is place to put function stubs that only make sense with interviews
 * whenever any VAR is freed it should be done with hoc_free_val... which
 * will notify the interviews objects that hold pointers to variables.
 * 
 * Revision 1.106  92/10/23  14:07:35  hines
 * only complete statements can be parsed in hoc_oc for now
 * also resetting parser yystart on hoc-execerror
 * object_name(Object*) returns object name
 * 
 * Revision 1.105  92/10/23  08:21:30  hines
 * fix some memory leaks with purify having to do with failing to free
 * arayinfo
 * 
 * Revision 1.104  92/10/22  12:33:21  hines
 * for debugging there is a list of all objects (#define OBLIST 1)
 * that can be printed. This list is turned off in hoc_oop.c
 * 
 * Revision 1.103  92/10/22  09:49:17  hines
 * ob.sec syntax must always do a poptypestack at end.
 * 
 * Revision 1.102  92/10/21  15:50:03  hines
 * this pointer is not referenced during initialization nor is it
 * unreferenced during freeing the object
 * 
 * Revision 1.101  92/10/21  14:51:50  hines
 * fixed several errors having to do with stack and frame overflows
 * under object.section { stmt} and calling hoc procedures with
 * object.proc()
 * 
 * Revision 1.100  92/10/21  11:10:31  hines
 * didn't allow local variable names used as object components. fixed
 * objectvar this
 * inside template will automatically get set to itself.
 * fixed error in popping the object type stack when sections were a component
 * 
 * Revision 1.99  92/10/14  14:35:59  hines
 * numarg() returns the number of arguments in a hoc procedure or function.
 * hoc_typestack returns the type of the Datum on the stack
 * hoc_argtype(i) returns the type of the ith arg.
 * 
 * Revision 1.98  92/10/14  10:10:25  hines
 * move oc specific stuff out of axis.c and into code2.c
 * new argument function hoc_pgetarg checks for double pointer on stack
 * and returns it.
 * hoc_val_pointer(string) returns a pointer to the variable resulting
 * from parsing the string.
 * 
 * Revision 1.97  92/10/10  12:32:08  hines
 * old section list syntax discarded in favor of making them first class
 * objects.
 * style is
 * objectvar s
 * s = new SectionList()
 * sec s.append()
 * sec s.remove()
 * forsec s
 * ifsec s
 * 
 * Revision 1.96  92/10/10  10:24:06  hines
 * ob.sec.range, ob.sec.range(x), ob.sec.property now allowed.
 * ob.sec.range(x) = expr means just change the value of node closest to x
 * 
 * Revision 1.95  92/10/09  12:16:13  hines
 * remove old style point process syntax
 * make hoc_run_expr(sym) much more general
 * added hoc_run_stmt(sym) as well
 * create them with hoc_parse_expr(char*, Symlist**) and
 * hoc_parse_stmt(char*, Symlist**)
 * 
 * Revision 1.94  92/10/09  07:48:23  hines
 * method available for setting and retrieving values from built-in classes.
 * used with new style of point processes.
 * 
 * Revision 1.93  92/10/08  08:36:10  hines
 * some extra parse error messages about assignments and redeclaring
 * variables.
 * 
 * Revision 1.92  92/10/07  15:43:26  hines
 * arg syntax changed to $s1 and $o1 because O looked too much like 0
 * 
 * Revision 1.91  92/10/07  15:34:36  hines
 * error in stack frame on call to C++ constructors.
 * 
 * Revision 1.90  92/10/07  14:07:08  hines
 * get rid of irrelevant type clash on default action
 * 
 * Revision 1.89  92/10/07  13:51:15  hines
 * strings can be passed as args and read and set with $S1 = $S2
 * 
 * Revision 1.88  92/10/07  10:45:14  hines
 * a simpler connect statement is now available in addition to the old
 * baroque syntax. it is
 * connect sec1(x), sec2(0 or 1)
 * 
 * Revision 1.87  92/10/07  09:47:27  hines
 * keyword for setting pointers in models has been changed
 * from connect to setpointer
 * 
 * Revision 1.86  92/10/05  10:15:49  hines
 * when _CRAY math functions get l suffix
 * 
 * Revision 1.85  92/10/05  08:04:28  hines
 * bad ; in one line
 * 
 * Revision 1.84  92/10/02  17:28:40  hines
 * can pass pointer to anything with &
 * 
 * Revision 1.83  92/09/24  16:50:17  hines
 * METHOD3 for nrnoc. when _method3 != 0 then x = i/nnode from i=0 to nnode
 * 
 * Revision 1.82  92/09/18  15:13:45  hines
 * hoc_list now won't conflict with OS::list
 * 
 * Revision 1.81  92/09/18  13:39:01  hines
 * c++ class members interfaced to oc can return an Object**
 * used first for built in List class
 * 
 * Revision 1.80  92/08/25  12:30:48  hines
 * hoc_araystr(Symbol*, index, Objectdata*) returns a string expressing the
 * array part of a variable
 * hoc_total_array_data(Symbol*, Objectdata*) returns the total length of
 * the variable vector.
 * 
 * Revision 1.79  92/08/24  14:10:02  hines
 * define OOP 1 in hoc.h
 * 
 * Revision 1.78  92/08/18  10:24:43  hines
 * templates can not make new objects using templates declared at the top level.
 * first looks for local templates though.
 * 
 * Revision 1.77  92/08/18  07:31:45  hines
 * arrays in different objects can have different sizes.
 * Now one uses araypt(symbol, SYMBOL) or araypt(symbol, OBJECTVAR) to
 * return index of an array variable.
 * 
 * Revision 1.76  92/08/17  12:49:38  hines
 * arrays in different objects can have different sizes. Now arayinfo
 * gets carried along in objectdata. arayinfo is reference counted.
 * 
 * Revision 1.75  92/08/17  08:19:09  hines
 * strdef, double, and objectvar can appear inside functions (any compound
 * statement). They are still global (within a template). When executed
 * (and if they are arrays) the old arayinfo is thrown away and the
 * proper aray storage is allocated. Note that arrays should at least be
 * initialized outside of a statement or else the parser will complain when
 * it sees the array syntax. next step is to give each array in each object
 * it's own arayinfo so they can have different bounds. At this time
 * they could be inconsistent. One should not try to have the same name
 * with different dimensions though.
 * 
 * Revision 1.74  92/08/17  07:32:05  hines
 * change from new(template) to new template(args)
 * strange problem with tests/test5.hoc with segmentation violation that
 * isn't seen with objectcenter and goes away when xopens have a path to the
 * file.
 * 
 * Revision 1.73  92/08/15  12:41:32  hines
 * can pass object variable as arg to oc functions and procedures. Within
 * the procedures the syntax is
 * $$1.ob.ob.var etc.
 * see test7.hoc for usage with array of strings
 * 
 * Revision 1.72  92/08/12  16:16:16  hines
 * uninsert mechanism_name
 * 
 * Revision 1.71  92/08/12  11:56:34  hines
 * hoc_fake_ret() allows calls to functions that do a ret(). These functions
 * can have no arguments and the caller must pop the stack and deal with
 * hoc_returning.
 * This was done so init... could be called both from hoc_spinit and from
 * hoc.
 * last function called by hoc_spinit in hocusr.c is hoc_last_init()
 * 
 * Revision 1.70  92/08/12  10:45:42  hines
 * Changes of sejnowski lab. also gets sred from hoc. Major addition is
 * a new x.c which is modified for screen updating following an expose
 * event. This is in x_sejnowski.c and will compile to x.o when
 * Sejnowski is defined as 1 in Imakefile (don't forget imknrn -a when
 * changed and delete old x.o)
 * Does not contain get_default on this checkin
 * 
 * Revision 1.69  92/08/12  08:51:12  hines
 * FARADAY and R (molar gas constant) added as built-in constants.
 * 
 * Revision 1.68  92/08/10  16:00:05  hines
 * forgot to checkin parse.y last time
 * 
 * Revision 1.67  92/08/10  15:59:38  hines
 * lists of sections with seclistdef ,,,
 * append_seclist(seclist)
 * remove_seclist(seclist)
 * forsec seclist
 * ifsec seclist
 * 
 * 
 * Revision 1.66  92/08/10  10:30:28  hines
 * hoc_thisobject contains a pointer to the current object. It is 0 at the
 * top level.
 * 
 * Revision 1.65  92/08/08  12:57:43  hines
 * access object.section
 * 
 * Revision 1.64  92/08/08  12:42:22  hines
 * to get decent section names each object of a template has a unique index
 * a count of the objects is kept in the template and when the count is
 * 0 then the index restarts at 0.
 * print ob will show the style
 * 
 * Revision 1.63  92/08/08  11:56:50  hines
 * ob.section.var not implemented but hooks are there to do it later
 * 
 * Revision 1.62  92/08/08  10:03:01  hines
 * properly push section from object
 * 
 * Revision 1.61  92/08/07  16:13:27  hines
 * sections as objects. sections now live in nmodl style list
 * 
 * Revision 1.60  92/07/31  16:16:58  hines
 * float_epsilon used for logical comparisons, int(), indexing and short for
 * default value is 1e-11
 * Hopfully no one has to worry about roundoff for the loops to come out
 * right.
 * 
 * 
 * Revision 1.59  92/07/31  14:34:57  hines
 * avoid saber warning with hoc_xpop not declared double in nocable.c
 * 
 * Revision 1.58  92/07/31  14:17:04  hines
 * better handling of errno as in hoc
 * 
 * Revision 1.57  92/07/31  13:49:19  hines
 * fix up forsec and ifsec so it works in object context
 * 
 * Revision 1.56  92/07/31  12:12:38  hines
 * following merged from hoc
 * The regular expression has been augmented with
 * {istart-iend} where istart and iend are integers. The expression matches
 * any integer that falls in this range.
 * 
 * Revision 1.55  92/07/31  12:01:15  hines
 * forsec and ifsec merged from hoc/neuron
 * 
 * Revision 1.54  92/07/31  08:57:45  hines
 * no longer use strings.h
 * 
 * Revision 1.53  92/07/31  08:55:29  hines
 * following merged from hoc to oc
 * Stewart Jasloves contribution to axis labels. This can be invoked by
 * setting #define Jaslove 1. It is 0 by default. The 3rd and 6th arguments
 * of axis() may have a precision which specifies the number of digits
 * after the decimal point for axis labels. eg. 5.3 denotes 5 tic marks with
 * 3 digits after the decimal point for each tic label
 * 
 * 
 * Revision 1.52  92/07/31  08:41:19  hines
 * ecalloc returns 0 when requesting 0 space. (now matches hoc)
 * 
 * Revision 1.51  92/07/31  08:33:45  hines
 * failed to checkin code2.c when chkarg got moved there
 * 
 * Revision 1.50  92/07/31  08:32:16  hines
 * C interface objects see the top level symbol table
 * 
 * Revision 1.49  92/07/31  08:29:56  hines
 * remove the old newgraph stuff
 * 
 * Revision 1.48  92/07/07  09:50:05  hines
 * chkarg moved to code2.c so always available
 * 
 * Revision 1.47  92/07/06  16:13:38  hines
 * good start with interfacing c++ objects so they are callable from oc
 * 
 * Revision 1.46  92/05/13  13:41:22  hines
 * some names in hoc.h were reserved for c++.These are changed to
 * cName when __cplusplus is defined.
 * 
 * Revision 1.45  92/05/13  13:39:45  hines
 * plot(-3) merely seeked to beginning of file without erasing it.
 * now plt(-3) closes and reopens a 0 length file.
 * only for fig plots not done for codraw at this time.
 * 
 * Revision 1.44  92/04/15  11:22:58  hines
 * double hoc_run_expr(sym) returns value of expresssion in sym made by
 * hoc_parse_exper()
 * 
 * Revision 1.43  92/04/09  12:39:54  hines
 * ready to add idplot usage with newgraph(), addgraph(), initgraph(), xgraph()
 * flushgraph().
 * A facilitating function exists called
 * Symbol* hoc_parse_expr(char* str, Symlist** psymlist) which
 * return a procedure symbol which can be used as
 * hoc_execute(sym->u.u_proc->defn.in);
 * val = hoc_xpop();
 * 
 * Revision 1.42  92/03/27  14:06:22  hines
 * turn off readline event hook if hoc_interviews becomes 0
 * 
 * Revision 1.41  92/03/19  08:57:38  hines
 * axis labels close to origin set to 0 so label not strange looking.
 * 
 * Revision 1.40  92/03/17  08:09:52  hines
 * xmenufile() adds menu which can save or open menufiles.
 * 
 * Revision 1.39  92/03/11  19:07:38  hines
 * proper passing of strings as arguments to built-in functions.
 * gargstr(i) and hoc_pgargstr(i)
 * 
 * Revision 1.38  92/03/11  15:37:27  hines
 * doEvents() will execute InterViews events if any.
 * 
 * Revision 1.37  92/03/11  13:56:13  hines
 * hoc.c inadvertently checked in last time with sigsegvec removed
 * 
 * Revision 1.36  92/03/11  13:53:56  hines
 * all parse errors now hoc_execerror
 * 
 * Revision 1.35  92/03/06  16:01:18  hines
 * hoc_oc can be used while executing. uses rinitcode(). But a problem
 * exists when single tokens are parsed.
 * 
 * Revision 1.34  92/03/06  07:19:46  hines
 * hoc_ac_ global variable defined to allow evaluation of expressions
 * via hoc_oc("hoc_ac_ = expr\n");
 * Then the user can find the value in extern double hoc_ac_.
 * 
 * Revision 1.33  92/03/05  14:46:39  hines
 * number that terminates string demonstrated a bug in ungetc. now
 * we test to make sure we don't unget a '\0'
 * 
 * Revision 1.32  92/03/05  13:25:41  hines
 * fix problem where not initcode after a execution by hoc_oc()
 * xvalue added
 * 
 * Revision 1.31  92/02/26  10:27:54  hines
 * xview removed. nrnoc verified (needed fixes) for reversed section syntax.
 * 
 * Revision 1.30  92/02/21  17:06:54  hines
 * new signal handling. hoc_oc() gets a longjump and returns. on return
 * the normal longjump is to hoc_run1()
 * 
 * Revision 1.29  92/02/21  14:38:57  hines
 * Completely event driven with event loop from InterViews.
 * 
 * Revision 1.28  92/02/20  10:55:02  hines
 * after em call from hoc_oc, execute the buffer
 * 
 * Revision 1.27  92/02/19  15:02:02  hines
 * preparing for sections within objects. syntax is
 * ob.section stmt    section.var
 * this causes about 36 extra shift/reduce conflicts
 * 
 * Revision 1.26  92/02/17  08:56:51  hines
 * Imake defines BISON. Default parser is yacc (for hoc, nmodl, etc.)
 * 
 * Revision 1.25  92/02/06  12:12:42  hines
 * Can have comments on same line with no tokens in between.
 * error fixed in translation of range vector with explicit section as first
 * item in a statement.
 * 
 * Revision 1.24  92/02/05  08:42:18  hines
 * Can connect a NEURON {POINTER var,...} in models to external variables using
 * connect var_suffix, anyvar
 * 
 * Revision 1.23  92/02/03  11:22:59  hines
 * rudimentary event driven version of oc.
 * 
 * Revision 1.22  92/01/31  15:02:06  hines
 * fix a trivial type mismatch that was revealed on switch to bison.
 * Ready now to make oc event driven.
 * 
 * Revision 1.21  92/01/30  11:05:12  hines
 * leave out an xflush in x.c so doesn't hang. error in cable part of code.c
 * found by codecenter. "if (a=b) instead of if(a == b)"
 * Imakefile invokes ObjectOrientedHoc
 * 
 * Revision 1.20  92/01/30  08:43:33  hines
 * add -I.. to makedepend include path so it can find a touched stdarg.h
 * 
 * Revision 1.19  92/01/30  08:17:21  hines
 * bug fixes found in hoc incorporated. if()return, no else, objectcenter
 * warnings.
 * 
 * Revision 1.18  92/01/30  08:15:26  hines
 * y
 * under Imake control
 * 
 * Revision 1.17  91/11/18  10:54:10  hines
 * xview menus. ^C doesnt work yet.
 * 
 * Revision 1.16  91/11/13  07:57:09  hines
 * userdouble uses old style sym->u.pval
 * 
 * Revision 1.15  91/11/05  11:24:17  hines
 * all neuron/examples produce same results with nrnoc as with neuron.
 * Found quite a few bugs this way.
 * 
 * Revision 1.14  91/10/30  14:40:49  hines
 * stack.hoc works and can be reloaded
 * 
 * Revision 1.13  91/10/29  15:05:21  hines
 * emacs command line editor with history
 * 
 * Revision 1.12  91/10/25  12:03:52  hines
 * double arrays in objects work.  init (if it exists) in a template
 * is executed for each new object.
 * 
 * Revision 1.11  91/10/25  10:30:03  hines
 * stop works inside object procedures
 * fixed problem with single object statement in loop or if with no braces
 * 
 * Revision 1.10  91/10/25  09:32:55  hines
 * object arrays. stack.hoc works. need to work on stop command and
 * necessity for braces around single statements in for loops.
 * 
 * Revision 1.9  91/10/24  16:22:11  hines
 * can call object functions and procedures. Object arrays not working yet.
 * object_id(obj) is used for tests.
 * STKDEBUG turned on in code.c in which every element also carries a type.
 * 
 * Revision 1.8  91/10/22  15:47:17  hines
 * objects and components. can assign, evaluate, and print.
 * No object arrays or function components yet.
 * At level of test3.hoc
 * 
 * Revision 1.7  91/10/18  14:40:51  hines
 * symbol tables now are type Symlist containing pointers to first and last
 * symbols.  New symbols get added onto the end.
 * 
 * Revision 1.6  91/10/17  15:01:39  hines
 * VAR, STRING now handled with pointer to value located in object data space
 * to allow future multiple objects. Ie symbol for var, string, objectvar
 * has offset into a pointer data space.
 * 
 * Revision 1.5  91/10/16  13:14:14  hines
 * fix up mistakes in last checkin
 * 
 * Revision 1.4  91/10/16  12:48:00  hines
 * symbol tables for the templates. Prototype not even executed.
 * need to come to grips with how to find data in the objects.
 * 
 * Revision 1.3  91/10/16  07:33:03  hines
 * prototype of type checking
 * 
 * Revision 1.2  91/10/14  17:36:16  hines
 * scaffolding for oop in place. Syntax about right. No action yet.
 * 
 * Revision 1.1  91/10/11  11:12:22  hines
 * Initial revision
 * 
 * --------------------------------
 * Revision 4.44  91/10/01  11:37:55  hines
 * optional use of xview libraries
 * 
 * Revision 4.43  91/10/01  11:34:27  hines
 * gather console input at one place in preparation for adding
 * emacs like command line editing. Hoc input now reads entire line.
 * Revision 3.1  89/07/07  14:16:30  mlh
 * automattic version numbering
 * 
 * Revision 3.0  89/07/07  13:58:28  mlh
 * *** empty log message ***
 * 
*/

/* hoc */
char *RCS_hoc_version = "$Revision: 3 $";
char *RCS_hoc_date = "$Date: 2003-02-11 19:36:05 +0100 (Tue, 11 Feb 2003) $";

