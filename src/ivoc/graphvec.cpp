#include <../../nrnconf.h>
#if HAVE_IV  // to end of file

#define USEGNU 1

#include "graph.h"
#include "ivoc.h"

#if USEGNU
#include "oc2iv.h"
#include "ivocvect.h"

Object** DataVec::new_vect(GLabel* gl) const {
    int i, cnt;
    Vect* vec;
    cnt = count();
    vec = new Vect(cnt);
    for (i = 0; i < cnt; ++i) {
        (*vec)[i] = get_val(i);
    }
    if (gl) {
        vec->label(gl->text());
    }
    Object** obp = vec->temp_objvar();
    hoc_obj_ref(*obp);
    return obp;
}


double gr_getline(void* v) {
    TRY_GUI_REDIRECT_ACTUAL_DOUBLE("Graph.getline", v);
    Graph* g = (Graph*) v;
    GlyphIndex i, cnt;
    cnt = g->count();
    i = (int) chkarg(1, -1, cnt);
    if (i < 0 || i > cnt - 1) {
        i = -1;
    }
    Vect* x = vector_arg(2);
    Vect* y = vector_arg(3);
    for (i += 1; i < cnt; ++i) {
        GraphItem* gi = (GraphItem*) g->component(i);
        if (gi->is_polyline()) {
            GPolyLine* gpl = (GPolyLine*) gi->body();
            long size = gpl->x_data()->count();
            x->resize(size);
            y->resize(size);
            for (long j = 0; j < size; ++j) {
                x->elem(j) = gpl->x(j);
                y->elem(j) = gpl->y(j);
            }
            if (gpl->label()) {
                y->label(gpl->label()->text());
            }
            return (double) i;
        }
    }
    return -1.;
}

#else
void DataVec::new_vect(Object**, DataVec*) {
    hoc_execerror("No Vector class", 0);
}
#endif

#endif
