#include <../../nrnconf.h>
#include "hoc.h"
#define Ret(a) \
    hoc_ret(); \
    hoc_pushx(a);

#include "gui-redirect.h"

int newstyle;
unsigned int text_style = 0, text_size = 1, text_orient = 0;

void hoc_settext(void) {
    TRY_GUI_REDIRECT_DOUBLE("settext", NULL);
    if (!ifarg(1)) {
        text_style = 0;
        text_size = 1;
        text_orient = 0;
    } else if (ifarg(3)) {
        text_size = *getarg(1);
        text_style = *getarg(2);
        text_orient = *getarg(3);
    } else if (ifarg(2)) {
        text_size = *getarg(1);
        text_style = *getarg(2);
    } else if (ifarg(1)) {
        text_size = *getarg(1);
    }
    if (text_style < 1)
        text_style = 1;
    if (text_style > 4)
        text_style = 0;
    if (text_orient > 1)
        text_orient = 0;
    newstyle = 1;
    Ret(1.);
}
