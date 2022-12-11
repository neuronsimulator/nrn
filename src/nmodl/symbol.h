
extern List* symlist[];
#define SYMITER(arg1)            \
    for (i = 'A'; i <= 'z'; i++) \
        ITERATE(qs, symlist[i])  \
    if ((s = SYM(qs))->type == arg1)

#define SYMITER_STAT             \
    for (i = 'A'; i <= 'z'; i++) \
        ITERATE(qs, symlist[i])  \
    if ((s = SYM(qs))->subtype & STAT)
