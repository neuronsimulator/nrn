#pragma once

#include "hoc.h"


/* do not know why this is not in parse.hpp */
extern int yyparse(void);
extern int yylex(void);

extern void hoc_acterror(const char*, const char*);
extern void hoc_execute(Inst*);
extern int hoc_yyparse(void);
extern void hoc_define(Symbol*);
extern void hoc_iterator_object(Symbol*, int, Inst*, Inst*, Object*);
extern int hoc_zzdebug;
int hoc_moreinput();
extern Symlist* hoc_p_symlist;
extern void hoc_defnonly(const char*);
extern Symbol* hoc_decl(Symbol*);
extern Symbol* hoc_which_template(Symbol*);
extern void hoc_begintemplate(Symbol*);
extern void hoc_endtemplate(Symbol*);
extern void hoc_add_publiclist(Symbol*);
extern void hoc_external_var(Symbol*);
extern void hoc_insertcode(Inst*, Inst*, Pfrv);

extern int hoc_lineno;
extern int hoc_indef;
extern Inst* hoc_codei(int i);
extern void hoc_codein(Inst* f);
extern void hoc_codesym(Symbol* f);
extern Inst* hoc_codeptr(void* vp);
extern Inst* hoc_Code(Pfrv);
extern void hoc_ob_check(int);
extern void hoc_obvar_declare(Symbol* sym, int type, int pmes);
extern void hoc_help(void);
extern char* hoc_strgets(char*, int);
extern int hoc_strgets_need(void);
